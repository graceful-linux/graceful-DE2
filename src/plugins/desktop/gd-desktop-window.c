//
// Created by dingjing on 25-6-30.
//
#include "gd-desktop-window.h"

#include <gdk/gdkx.h>
#include <glib/gi18n.h>
#include <X11/Xatom.h>

#include "gd-background.h"
#include "gd-icon-view.h"
#include "../common/gd-bg.h"

struct _GDDesktopWindow
{
    GtkWindow parent;

    GDMonitorManager* monitor_manager;

    bool draw_background;
    GDBackground* background;
    bool event_filter_added;
    cairo_surface_t* surface;

    bool show_icons;
    GtkWidget* icon_view;

    int width;
    int height;

    guint move_resize_id;

    bool ready;

    GdkRGBA* representative_color;
};

enum
{
    PROP_0,
    PROP_DRAW_BACKGROUND,
    PROP_SHOW_ICONS,
    LAST_PROP
};

static GParamSpec* window_properties[LAST_PROP] = {NULL};

enum { READY, SIZE_CHANGED, LAST_SIGNAL };

static guint window_signals[LAST_SIGNAL] = {0};

static void initable_iface_init(GInitableIface* iface);

G_DEFINE_TYPE_WITH_CODE(GDDesktopWindow, gd_desktop_window, GTK_TYPE_WINDOW, G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init))


static bool get_representative_color(GDDesktopWindow* self, GdkRGBA* color)
{
    GdkDisplay* display;
    Display* xdisplay;
    Atom atom;
    int status;
    Atom actual_type;
    int actual_format;
    unsigned long n_items;
    unsigned long bytes_after;
    unsigned char* prop;

    display = gtk_widget_get_display(GTK_WIDGET(self));
    xdisplay = gdk_x11_display_get_xdisplay(display);

    atom = XInternAtom(xdisplay, "_GNOME_BACKGROUND_REPRESENTATIVE_COLORS", True);
    if (atom == None) return FALSE;

    gdk_x11_display_error_trap_push(display);

    prop = NULL;
    status = XGetWindowProperty(xdisplay, XDefaultRootWindow(xdisplay), atom, 0, G_MAXLONG, False, XA_STRING, &actual_type, &actual_format, &n_items, &bytes_after, &prop);

    gdk_x11_display_error_trap_pop_ignored(display);

    if (status != Success || actual_type != XA_STRING || n_items == 0) {
        XFree(prop);
        return FALSE;
    }

    gdk_rgba_parse(color, (const char*)prop);
    XFree(prop);

    return TRUE;
}

static bool color_is_dark(GdkRGBA* color)
{
    double relative_luminance = 0.299 * color->red + 0.587 * color->green + 0.114 * color->blue;

    return relative_luminance < 0.5;
}

static void update_css_class(GDDesktopWindow* self)
{
    GtkStyleContext* context = gtk_widget_get_style_context(GTK_WIDGET(self));

    if (self->representative_color != NULL) {
        if (color_is_dark(self->representative_color)) {
            gtk_style_context_remove_class(context, "light");
            gtk_style_context_add_class(context, "dark");
        }
        else {
            gtk_style_context_remove_class(context, "dark");
            gtk_style_context_add_class(context, "light");
        }
    }
    else {
        gtk_style_context_remove_class(context, "dark");
        gtk_style_context_remove_class(context, "light");
    }
}

static void update_representative_color(GDDesktopWindow* self)
{
    g_clear_pointer(&self->representative_color, gdk_rgba_free);

    if (self->background != NULL) {
        GdkRGBA* color = gd_background_get_average_color(self->background);
        self->representative_color = gdk_rgba_copy(color);
    }
    else {
        GdkRGBA color;
        if (get_representative_color(self, &color)) {
            self->representative_color = gdk_rgba_copy(&color);
        }
    }

    if (self->icon_view != NULL) {
        gd_icon_view_set_representative_color(GD_ICON_VIEW(self->icon_view), self->representative_color);
    }

    update_css_class(self);
}

static void ensure_surface(GDDesktopWindow* self)
{
    GtkWidget* widget;
    GdkDisplay* display;

    widget = GTK_WIDGET(self);

    display = gtk_widget_get_display(widget);

    self->surface = gd_bg_get_surface_from_root(display, self->width, self->height);

    gtk_widget_queue_draw(widget);
}

static GdkFilterReturn filter_func(GdkXEvent* xevent, GdkEvent* event, gpointer user_data)
{
    XEvent* x;
    GDDesktopWindow* self;
    GdkDisplay* display;
    Display* xdisplay;
    Atom pixmap_atom;
    Atom color_atom;

    x = (XEvent*)xevent;

    if (x->type != PropertyNotify) return GDK_FILTER_CONTINUE;

    self = GD_DESKTOP_WINDOW(user_data);

    display = gtk_widget_get_display(GTK_WIDGET(self));
    xdisplay = gdk_x11_display_get_xdisplay(display);

    pixmap_atom = XInternAtom(xdisplay, "_XROOTPMAP_ID", False);
    color_atom = XInternAtom(xdisplay, "_GNOME_BACKGROUND_REPRESENTATIVE_COLORS", False);

    if (x->xproperty.atom == pixmap_atom) {
        GdkScreen* screen;

        screen = gtk_widget_get_screen(GTK_WIDGET(self));

        if (!gdk_screen_is_composited(screen)) {
            g_clear_pointer(&self->surface, cairo_surface_destroy);
            ensure_surface(self);
        }
    }
    else if (x->xproperty.atom == color_atom) {
        update_representative_color(self);
    }

    return GDK_FILTER_CONTINUE;
}

static void remove_event_filter(GDDesktopWindow* self)
{
    C_RETURN_IF_OK(!self->event_filter_added);

    GdkScreen* screen = gtk_widget_get_screen(GTK_WIDGET(self));
    GdkWindow* root = gdk_screen_get_root_window(screen);

    gdk_window_remove_filter(root, filter_func, self);
    self->event_filter_added = FALSE;
}

static void add_event_filter(GDDesktopWindow* self)
{
    C_RETURN_IF_OK(self->event_filter_added);

    GdkScreen* screen = gtk_widget_get_screen(GTK_WIDGET(self));
    GdkWindow* root = gdk_screen_get_root_window(screen);

    gdk_window_add_filter(root, filter_func, self);
    self->event_filter_added = TRUE;
}

static void composited_changed_cb(GdkScreen* screen, GDDesktopWindow* self)
{
    C_RETURN_IF_OK(self->draw_background);

    if (gdk_screen_is_composited(screen)) {
        g_clear_pointer(&self->surface, cairo_surface_destroy);
    }
    else {
        ensure_surface(self);
    }
}

static void emit_ready(GDDesktopWindow* self)
{
    C_RETURN_IF_OK(self->ready);

    g_signal_emit(self, window_signals[READY], 0);
    self->ready = TRUE;
}

static void ready_cb(GDBackground* background, GDDesktopWindow* self)
{
    emit_ready(self);
}

static void changed_cb(GDBackground* background, GDDesktopWindow* self)
{
    update_representative_color(self);
}

static void draw_background_changed(GDDesktopWindow* self)
{
    if (self->draw_background) {
        remove_event_filter(self);
        g_clear_pointer(&self->surface, cairo_surface_destroy);

        g_assert(self->background == NULL);
        self->background = gd_background_new(GTK_WIDGET(self));

        g_signal_connect(self->background, "ready", G_CALLBACK (ready_cb), self);
        g_signal_connect(self->background, "changed", G_CALLBACK (changed_cb), self);
    }
    else {
        g_clear_object(&self->background);

        GdkScreen* screen = gtk_widget_get_screen(GTK_WIDGET(self));

        add_event_filter(self);
        update_representative_color(self);

        if (!gdk_screen_is_composited(screen)) {
            ensure_surface(self);
        }

        emit_ready(self);
    }
}

static void show_icons_changed(GDDesktopWindow* self)
{
    if (self->show_icons) {
        g_assert(self->icon_view == NULL);
        self->icon_view = gd_icon_view_new();

        if (self->representative_color != NULL) {
            gd_icon_view_set_representative_color(GD_ICON_VIEW(self->icon_view), self->representative_color);
        }

        gtk_container_add(GTK_CONTAINER(self), self->icon_view);
        gtk_widget_show(self->icon_view);
    }
    else if (self->icon_view != NULL) {
        gtk_container_remove(GTK_CONTAINER(self), self->icon_view);
        self->icon_view = NULL;
    }
}

static void set_draw_background(GDDesktopWindow* self, bool drawBackground)
{
    C_RETURN_IF_OK(drawBackground == self->draw_background);

    self->draw_background = drawBackground;
    draw_background_changed(self);
}

static void set_show_icons(GDDesktopWindow* self, bool showIcons)
{
    C_RETURN_IF_OK(showIcons == self->show_icons);

    self->show_icons = showIcons;
    show_icons_changed(self);
}

static void move_resize(GDDesktopWindow* self)
{
    int i = 0;
    GtkWidget* widget = GTK_WIDGET(self);
    GdkRectangle rect = (GdkRectangle){};
    GdkDisplay* display = gdk_display_get_default();
    int n_monitors = gdk_display_get_n_monitors(display);

    for (i = 0; i < n_monitors; i++) {
        GdkRectangle geometry;
        GdkMonitor* monitor = gdk_display_get_monitor(display, i);
        gdk_monitor_get_geometry(monitor, &geometry);
        gdk_rectangle_union(&rect, &geometry, &rect);
    }

    if (rect.width < 640) rect.width = 640;

    if (rect.height < 480) rect.height = 480;

    if (rect.width == self->width && rect.height == self->height) return;

    self->width = rect.width;
    self->height = rect.height;

    if (gtk_widget_get_realized(widget)) {
        GdkWindow* window = gtk_widget_get_window(widget);
        gdk_window_move_resize(window, 0, 0, rect.width, rect.height);
    }

    g_signal_emit(self, window_signals[SIZE_CHANGED], 0);

    GdkScreen* screen = gtk_widget_get_screen(widget);

    if (!self->draw_background && !gdk_screen_is_composited(screen)) {
        g_clear_pointer(&self->surface, cairo_surface_destroy);
        ensure_surface(self);
    }
}

static bool move_resize_cb(gpointer user_data)
{
    GDDesktopWindow* self = GD_DESKTOP_WINDOW(user_data);
    self->move_resize_id = 0;

    move_resize(self);

    return G_SOURCE_REMOVE;
}

static void queue_move_resize(GDDesktopWindow* self)
{
    C_RETURN_IF_OK(0 != self->move_resize_id);

    self->move_resize_id = g_idle_add((void*) move_resize_cb, self);
    g_source_set_name_by_id(self->move_resize_id, "[graceful-DE2] move_resize_cb");
}

static void notify_geometry_cb(GdkMonitor* monitor, GParamSpec* pspec, GDDesktopWindow* self)
{
    g_info("geometry resize!");
    queue_move_resize(self);
}

static void monitor_added_cb(GdkDisplay* display, GdkMonitor* monitor, GDDesktopWindow* self)
{
    g_info("monitor added!");
    g_signal_connect_object(monitor, "notify::geometry", G_CALLBACK(notify_geometry_cb), self, 0);

    queue_move_resize(self);
}

static void
monitor_removed_cb(GdkDisplay* display, GdkMonitor* monitor, GDDesktopWindow* self)
{
    queue_move_resize(self);
}

static bool gd_desktop_window_initable_init(GInitable* initable, GCancellable* cancellable, GError** error)
{
    GtkWidget* widget = GTK_WIDGET(initable);
    GdkDisplay* display = gtk_widget_get_display(widget);
    Display* xdisplay = gdk_x11_display_get_xdisplay(display);

    char* atomName = g_strdup_printf("_NET_DESKTOP_MANAGER_S%d", XDefaultScreen(xdisplay));

    Atom atom = XInternAtom(xdisplay, atomName, False);
    C_FREE_FUNC_NULL(atomName, g_free);

    if (XGetSelectionOwner(xdisplay, atom) != None) {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Another desktop manager is running.");
        return FALSE;
    }

    gtk_widget_realize(widget);

    GdkWindow* window = gtk_widget_get_window(widget);
    Window xWindow = gdk_x11_window_get_xid(window);

    XSetSelectionOwner(xdisplay, atom, xWindow, CurrentTime);

    return TRUE;
}

static void
initable_iface_init(GInitableIface* iface)
{
    iface->init = (void*) gd_desktop_window_initable_init;
}

static void
gd_desktop_window_constructed(GObject* object)
{
    GDDesktopWindow* self;
    GParamSpecBoolean* spec;

    self = GD_DESKTOP_WINDOW(object);

    G_OBJECT_CLASS(gd_desktop_window_parent_class)->constructed(object);

    spec = (GParamSpecBoolean*)window_properties[PROP_DRAW_BACKGROUND];
    if (self->draw_background == spec->default_value) draw_background_changed(self);

    spec = (GParamSpecBoolean*)window_properties[PROP_SHOW_ICONS];
    if (self->show_icons == spec->default_value) show_icons_changed(self);
}

static void
gd_desktop_window_dispose(GObject* object)
{
    GDDesktopWindow* self = GD_DESKTOP_WINDOW(object);

    g_clear_object(&self->background);

    G_OBJECT_CLASS(gd_desktop_window_parent_class)->dispose(object);
}

static void
gd_desktop_window_finalize(GObject* object)
{
    GDDesktopWindow* self = GD_DESKTOP_WINDOW(object);

    remove_event_filter(self);
    g_clear_pointer(&self->surface, cairo_surface_destroy);

    g_clear_pointer(&self->representative_color, gdk_rgba_free);

    if (self->move_resize_id != 0) {
        g_source_remove(self->move_resize_id);
        self->move_resize_id = 0;
    }

    G_OBJECT_CLASS(gd_desktop_window_parent_class)->finalize(object);
}

static void
gd_desktop_window_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec)
{
    GDDesktopWindow* self = GD_DESKTOP_WINDOW(object);
    switch (property_id) {
    case PROP_DRAW_BACKGROUND:
        set_draw_background(self, g_value_get_boolean(value));
        break;

    case PROP_SHOW_ICONS:
        set_show_icons(self, g_value_get_boolean(value));
        break;

    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static bool
gd_desktop_window_delete_event(GtkWidget* widget, GdkEventAny* event)
{
    return GDK_EVENT_STOP;
}

static bool
gd_desktop_window_draw(GtkWidget* widget, cairo_t* cr)
{
    GDDesktopWindow* self = GD_DESKTOP_WINDOW(widget);

    if (self->surface != NULL) {
        cairo_set_source_surface(cr, self->surface, 0, 0);
        cairo_paint(cr);
    }

    return GTK_WIDGET_CLASS(gd_desktop_window_parent_class)->draw(widget, cr);
}

static void
gd_desktop_window_realize(GtkWidget* widget)
{
    GDDesktopWindow* self;
    GdkScreen* screen;
    GdkVisual* visual;
    GdkWindow* window;

    self = GD_DESKTOP_WINDOW(widget);

    screen = gtk_widget_get_screen(widget);
    visual = gdk_screen_get_rgba_visual(screen);

    if (visual != NULL) gtk_widget_set_visual(widget, visual);

    GTK_WIDGET_CLASS(gd_desktop_window_parent_class)->realize(widget);

    window = gtk_widget_get_window(widget);
    gdk_window_move_resize(window, 0, 0, self->width, self->height);
}

static void
install_properties(GObjectClass* object_class)
{
    window_properties[PROP_DRAW_BACKGROUND] = g_param_spec_boolean("draw-background", "draw-background", "draw-background", TRUE, G_PARAM_CONSTRUCT | G_PARAM_WRITABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

    window_properties[PROP_SHOW_ICONS] = g_param_spec_boolean("show-icons", "show-icons", "show-icons", TRUE, G_PARAM_CONSTRUCT | G_PARAM_WRITABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, LAST_PROP, window_properties);
}

static void
install_signals(void)
{
    window_signals[READY] = g_signal_new("ready", GD_TYPE_DESKTOP_WINDOW, G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);

    window_signals[SIZE_CHANGED] = g_signal_new("size-changed", GD_TYPE_DESKTOP_WINDOW, G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void
gd_desktop_window_class_init(GDDesktopWindowClass* self_class)
{
    GObjectClass* object_class;
    GtkWidgetClass* widget_class;

    object_class = G_OBJECT_CLASS(self_class);
    widget_class = GTK_WIDGET_CLASS(self_class);

    object_class->constructed = gd_desktop_window_constructed;
    object_class->dispose = gd_desktop_window_dispose;
    object_class->finalize = gd_desktop_window_finalize;
    object_class->set_property = gd_desktop_window_set_property;

    widget_class->delete_event = (void*) gd_desktop_window_delete_event;
    widget_class->draw = (void*) gd_desktop_window_draw;
    widget_class->realize = (void*) gd_desktop_window_realize;

    install_properties(object_class);
    install_signals();

    gtk_widget_class_set_css_name(widget_class, "gf-desktop-window");
}

static void gd_desktop_window_init(GDDesktopWindow* self)
{
    int i = 0;

    GParamSpecBoolean* spec = (GParamSpecBoolean*)window_properties[PROP_DRAW_BACKGROUND];
    self->draw_background = spec->default_value;

    spec = (GParamSpecBoolean*)window_properties[PROP_SHOW_ICONS];
    self->show_icons = spec->default_value;

    GdkDisplay* display = gdk_display_get_default();
    int n_monitors = gdk_display_get_n_monitors(display);

    g_signal_connect_object(display, "monitor-added", G_CALLBACK(monitor_added_cb), self, 0);
    g_signal_connect_object(display, "monitor-removed", G_CALLBACK(monitor_removed_cb), self, 0);

    for (i = 0; i < n_monitors; i++) {
        GdkMonitor* monitor = gdk_display_get_monitor(display, i);
        g_signal_connect_object(monitor, "notify::geometry", G_CALLBACK(notify_geometry_cb), self, 0);
    }

    GdkScreen* screen = gtk_widget_get_screen(GTK_WIDGET(self));
    GdkWindow* root = gdk_screen_get_root_window(screen);
    gint events = gdk_window_get_events(root);

    gdk_window_set_events(root, events | GDK_PROPERTY_CHANGE_MASK);

    g_signal_connect_object(screen, "composited-changed", G_CALLBACK(composited_changed_cb), self, 0);

    move_resize(self);
}

GtkWidget* gd_desktop_window_new(bool draw_background, bool show_icons, GError** error)
{
    GtkWidget* window = g_object_new(GD_TYPE_DESKTOP_WINDOW,
        "app-paintable", TRUE,
        "type", GTK_WINDOW_TOPLEVEL,
        "type-hint", GDK_WINDOW_TYPE_HINT_DESKTOP,
        "draw-background", draw_background,
        "show-icons", show_icons,
        "title", _("Desktop"), NULL);

    if (!g_initable_init(G_INITABLE(window), NULL, error)) {
        gtk_widget_destroy(window);
        return NULL;
    }

    return window;
}

void
gd_desktop_window_set_monitor_manager(GDDesktopWindow* self, GDMonitorManager* monitor_manager)
{
    self->monitor_manager = monitor_manager;
}

bool
gd_desktop_window_is_ready(GDDesktopWindow* self)
{
    return self->ready;
}

int
gd_desktop_window_get_width(GDDesktopWindow* self)
{
    return self->width;
}

int
gd_desktop_window_get_height(GDDesktopWindow* self)
{
    return self->height;
}
