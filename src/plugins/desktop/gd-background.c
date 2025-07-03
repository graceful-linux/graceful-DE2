//
// Created by dingjing on 25-6-30.
//
#include "gd-background.h"

#include "../common/gd-bg.h"
#include "gd-desktop-window.h"

typedef struct
{
    cairo_surface_t* start;
    cairo_surface_t* end;

    double start_time;
    double total_duration;
    bool is_first_frame;
    double percent_done;

    guint timeout_id;
} FadeData;

struct _GDBackground
{
    GObject parent;

    GtkWidget* window;

    GSettings* settings2;

    GDBG* bg;

    guint change_id;

    FadeData* fade_data;
    cairo_surface_t* surface;
};

enum { PROP_0, PROP_WINDOW, LAST_PROP };

static GParamSpec* background_properties[LAST_PROP] = {NULL};

enum { READY, CHANGED, LAST_SIGNAL };

static guint background_signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(GDBackground, gd_background, G_TYPE_OBJECT)

static void free_fade_data(FadeData* data)
{
    if (data->timeout_id != 0) {
        g_source_remove(data->timeout_id);
        data->timeout_id = 0;
    }

    g_clear_pointer(&data->start, cairo_surface_destroy);
    g_clear_pointer(&data->end, cairo_surface_destroy);
    g_free(data);
}

static bool fade_cb(gpointer user_data)
{
    GDBackground* self;
    FadeData* fade;
    double current_time;
    double percent_done;

    self = GD_BACKGROUND(user_data);
    fade = self->fade_data;

    current_time = g_get_real_time() / (double)G_USEC_PER_SEC;
    percent_done = (current_time - fade->start_time) / fade->total_duration;
    percent_done = CLAMP(percent_done, 0.0, 1.0);

    if (fade->is_first_frame && percent_done > 0.33) {
        fade->total_duration *= 1.5;
        fade->is_first_frame = FALSE;
        return fade_cb(self);
    }

    gtk_widget_queue_draw(self->window);

    fade->is_first_frame = FALSE;
    fade->percent_done = percent_done;

    if (percent_done < 0.99) return G_SOURCE_CONTINUE;

    self->fade_data->timeout_id = 0;

    g_clear_pointer(&self->surface, cairo_surface_destroy);
    self->surface = cairo_surface_reference(fade->end);

    g_clear_pointer(&self->fade_data, free_fade_data);

    gd_bg_set_surface_as_root(gtk_widget_get_display(self->window), self->surface);

    g_signal_emit(self, background_signals[CHANGED], 0);

    return G_SOURCE_REMOVE;
}

static void change(GDBackground* self, bool fade)
{
    GdkDisplay* display = gtk_widget_get_display(self->window);
    GdkScreen* screen = gtk_widget_get_screen(self->window);
    GdkWindow* root = gdk_screen_get_root_window(screen);

    int width = gd_desktop_window_get_width(GD_DESKTOP_WINDOW(self->window));
    int height = gd_desktop_window_get_height(GD_DESKTOP_WINDOW(self->window));

    g_clear_pointer(&self->fade_data, free_fade_data);

    if (fade) {
        FadeData* data;

        g_assert(self->fade_data == NULL);
        self->fade_data = data = g_new0(FadeData, 1);

        if (self->surface != NULL) data->start = cairo_surface_reference(self->surface);
        else data->start = gd_bg_get_surface_from_root(display, width, height);

        data->end = gd_bg_create_surface(self->bg, root, width, height, TRUE);

        data->start_time = g_get_real_time() / (double)G_USEC_PER_SEC;
        data->total_duration = .75;
        data->is_first_frame = TRUE;
        data->percent_done = .0;

        data->timeout_id = g_timeout_add(1000 / 60.0, (void*) fade_cb, self);
    }
    else {
        g_clear_pointer(&self->surface, cairo_surface_destroy);
        self->surface = gd_bg_create_surface(self->bg, root, width, height, TRUE);

        gd_bg_set_surface_as_root(display, self->surface);

        g_signal_emit(self, background_signals[CHANGED], 0);
    }

    g_signal_emit(self, background_signals[READY], 0);
    gtk_widget_queue_draw(self->window);
}

typedef struct
{
    GDBackground* background;
    bool fade;
} ChangeData;

static bool
change_cb(gpointer uData)
{
    ChangeData* data = (ChangeData*) uData;

    change(data->background, data->fade);
    data->background->change_id = 0;

    return G_SOURCE_REMOVE;
}

static void queue_change(GDBackground* self, bool fade)
{
    ChangeData* data;

    if (self->change_id != 0) return;

    data = g_new(ChangeData, 1);

    data->background = self;
    data->fade = fade;

    self->change_id = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, (void*) change_cb, data, g_free);

    g_source_set_name_by_id(self->change_id, "[graceful-DE2] change_cb");
}

static bool
draw_cb(GtkWidget* widget, cairo_t* cr, GDBackground* self)
{
    if (self->fade_data != NULL) {
        cairo_set_source_surface(cr, self->fade_data->start, 0, 0);
        cairo_paint(cr);

        cairo_set_source_surface(cr, self->fade_data->end, 0, 0);
        cairo_paint_with_alpha(cr, self->fade_data->percent_done);
    }
    else if (self->surface != NULL) {
        cairo_set_source_surface(cr, self->surface, 0, 0);
        cairo_paint(cr);
    }

    return FALSE;
}

static void
size_changed_cb(GDDesktopWindow* window, GDBackground* self)
{
    queue_change(self, FALSE);
}

static void
changed_cb(GDBG* bg, GDBackground* self)
{
    bool fade;

    fade = g_settings_get_boolean(self->settings2, "fade");
    queue_change(self, fade);
}

static void
transitioned_cb(GDBG* bg, GDBackground* self)
{
    queue_change(self, FALSE);
}

static void
gd_background_constructed(GObject* object)
{
    GDBackground* self;

    self = GD_BACKGROUND(object);

    G_OBJECT_CLASS(gd_background_parent_class)->constructed(object);

    self->settings2 = g_settings_new("org.gnome.gnome-flashback.desktop.background");

    self->bg = gd_bg_new("org.gnome.desktop.background");

    g_signal_connect_object(self->window, "draw", G_CALLBACK(draw_cb), self, 0);

    g_signal_connect_object(self->window, "size-changed", G_CALLBACK(size_changed_cb), self, 0);

    g_signal_connect(self->bg, "changed", G_CALLBACK(changed_cb), self);

    g_signal_connect(self->bg, "transitioned", G_CALLBACK(transitioned_cb), self);

    gd_bg_load_from_preferences(self->bg);
}

static void
gd_background_dispose(GObject* object)
{
    GDBackground* self;

    self = GD_BACKGROUND(object);

    g_clear_object(&self->settings2);
    g_clear_object(&self->bg);

    G_OBJECT_CLASS(gd_background_parent_class)->dispose(object);
}

static void
gd_background_finalize(GObject* object)
{
    GDBackground* self;

    self = GD_BACKGROUND(object);

    if (self->change_id != 0) {
        g_source_remove(self->change_id);
        self->change_id = 0;
    }

    g_clear_pointer(&self->fade_data, free_fade_data);
    g_clear_pointer(&self->surface, cairo_surface_destroy);

    G_OBJECT_CLASS(gd_background_parent_class)->finalize(object);
}

static void
gd_background_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec)
{
    GDBackground* self;

    self = GD_BACKGROUND(object);

    switch (property_id) {
    case PROP_WINDOW:
        g_assert(self->window == NULL);
        self->window = g_value_get_object(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
install_properties(GObjectClass* object_class)
{
    background_properties[PROP_WINDOW] = g_param_spec_object("window",
        "window", "window", GTK_TYPE_WIDGET, G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, LAST_PROP, background_properties);
}

static void
install_signals(void)
{
    background_signals[READY] = g_signal_new("ready",
        GD_TYPE_BACKGROUND, G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);

    background_signals[CHANGED] = g_signal_new("changed",
        GD_TYPE_BACKGROUND, G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void gd_background_class_init(GDBackgroundClass* self_class)
{
    GObjectClass* oc = G_OBJECT_CLASS(self_class);

    oc->constructed = gd_background_constructed;
    oc->dispose = gd_background_dispose;
    oc->finalize = gd_background_finalize;
    oc->set_property = gd_background_set_property;

    install_properties(oc);
    install_signals();
}

static void gd_background_init(GDBackground* self)
{
}

GDBackground* gd_background_new(GtkWidget* window)
{
    return g_object_new(GD_TYPE_BACKGROUND, "window", window, NULL);
}

GdkRGBA* gd_background_get_average_color(GDBackground* self)
{
    return gd_bg_get_average_color_from_surface(self->surface);
}
