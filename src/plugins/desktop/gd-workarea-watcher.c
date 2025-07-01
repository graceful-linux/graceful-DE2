//
// Created by dingjing on 25-7-1.
//
#include "gd-workarea-watcher.h"

#include <gdk/gdkx.h>
#include <X11/Xatom.h>

struct _GDWorkareaWatcher
{
    GObject parent;

    Atom net_supported;
    Atom net_current_desktop;
    Atom net_workarea;
    Atom gtk_workareas;

    bool gtk_workareas_supported;
    Atom gtk_workareas_dn;

    guint changed_id;
};

enum { CHANGED, LAST_SIGNAL };

static guint workarea_watcher_signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(GDWorkareaWatcher, gd_workarea_watcher, G_TYPE_OBJECT)

static bool
emit_changed_cb(gpointer user_data)
{
    GDWorkareaWatcher* self;

    self = GD_WORKAREA_WATCHER(user_data);

    g_signal_emit(self, workarea_watcher_signals[CHANGED], 0);
    self->changed_id = 0;

    return G_SOURCE_REMOVE;
}

static void
emit_changed_idle(GDWorkareaWatcher* self)
{
    if (self->changed_id != 0) return;

    self->changed_id = g_idle_add((void*)emit_changed_cb, self);

    g_source_set_name_by_id(self->changed_id, "[gnome-flashback] emit_changed_cb");
}

static void
update_gtk_workareas_supported(GDWorkareaWatcher* self, bool emit)
{
    bool changed;
    bool gtk_workareas_supported;
    Atom gtk_workareas_dn;
    GdkDisplay* display;
    Display* xdisplay;
    int result;
    Atom actual_type;
    int actual_format;
    unsigned long n_items;
    unsigned long bytes_after;
    unsigned char* prop;

    changed = FALSE;
    gtk_workareas_supported = FALSE;
    gtk_workareas_dn = None;

    display = gdk_display_get_default();
    xdisplay = gdk_x11_display_get_xdisplay(display);
    prop = NULL;

    gdk_x11_display_error_trap_push(display);

    result = XGetWindowProperty(xdisplay, XDefaultRootWindow(xdisplay), self->net_supported, 0, G_MAXLONG, False, XA_ATOM, &actual_type, &actual_format, &n_items, &bytes_after, &prop);

    gdk_x11_display_error_trap_pop_ignored(display);

    if (result == Success && actual_type == XA_ATOM) {
        Atom* atoms;
        unsigned int i;

        atoms = (Atom*)prop;

        for (i = 0; i < n_items; i++) {
            if (atoms[i] == self->gtk_workareas) {
                gtk_workareas_supported = TRUE;
                break;
            }
        }
    }

    XFree(prop);
    prop = NULL;

    if (gtk_workareas_supported) {
        int current_desktop;

        current_desktop = -1;

        gdk_x11_display_error_trap_push(display);

        result = XGetWindowProperty(xdisplay, XDefaultRootWindow(xdisplay), self->net_current_desktop, 0, G_MAXLONG, False, XA_CARDINAL, &actual_type, &actual_format, &n_items, &bytes_after, &prop);

        gdk_x11_display_error_trap_pop_ignored(display);

        if (result == Success && actual_type == XA_CARDINAL && actual_format == 32 && n_items != 0) {
            current_desktop = ((long*)prop)[0];
        }

        XFree(prop);
        prop = NULL;

        if (current_desktop != -1) {
            char* atom_name;

            atom_name = g_strdup_printf("_GTK_WORKAREAS_D%d", current_desktop);
            gtk_workareas_dn = XInternAtom(xdisplay, atom_name, False);
            g_free(atom_name);
        }
    }

    if (self->gtk_workareas_supported != gtk_workareas_supported) {
        self->gtk_workareas_supported = gtk_workareas_supported;
        changed = TRUE;
    }

    if (self->gtk_workareas_dn != gtk_workareas_dn) {
        self->gtk_workareas_dn = gtk_workareas_dn;
        changed = TRUE;
    }

    if (changed && emit) emit_changed_idle(self);
}

static GdkFilterReturn
filter_func(GdkXEvent* xevent, GdkEvent* event, gpointer user_data)
{
    XEvent* x;
    GDWorkareaWatcher* self;

    x = (XEvent*)xevent;

    if (x->type != PropertyNotify) return GDK_FILTER_CONTINUE;

    self = GD_WORKAREA_WATCHER(user_data);

    if (self->gtk_workareas_supported && x->xproperty.atom == self->gtk_workareas_dn) {
        emit_changed_idle(self);
    }
    else if (!self->gtk_workareas_supported && x->xproperty.atom == self->net_workarea) {
        emit_changed_idle(self);
    }
    else if (x->xproperty.atom == self->net_current_desktop || x->xproperty.atom == self->net_supported) {
        update_gtk_workareas_supported(self, TRUE);
    }

    return GDK_FILTER_CONTINUE;
}

static void
gd_workarea_watcher_finalize(GObject* object)
{
    GDWorkareaWatcher* self;
    GdkDisplay* display;
    GdkScreen* screen;
    GdkWindow* root;

    self = GD_WORKAREA_WATCHER(object);

    display = gdk_display_get_default();
    screen = gdk_display_get_default_screen(display);
    root = gdk_screen_get_root_window(screen);

    gdk_window_remove_filter(root, filter_func, self);

    if (self->changed_id != 0) {
        g_source_remove(self->changed_id);
        self->changed_id = 0;
    }

    G_OBJECT_CLASS(gd_workarea_watcher_parent_class)->finalize(object);
}

static void
install_signals(void)
{
    workarea_watcher_signals[CHANGED] = g_signal_new("changed", GD_TYPE_WORKAREA_WATCHER, G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void
gd_workarea_watcher_class_init(GDWorkareaWatcherClass* self_class)
{
    GObjectClass* object_class;

    object_class = G_OBJECT_CLASS(self_class);

    object_class->finalize = gd_workarea_watcher_finalize;

    install_signals();
}

static void
gd_workarea_watcher_init(GDWorkareaWatcher* self)
{
    GdkDisplay* display;
    Display* xdisplay;
    GdkScreen* screen;
    GdkWindow* root;
    GdkEventMask event_mask;

    display = gdk_display_get_default();
    xdisplay = gdk_x11_display_get_xdisplay(display);

    self->net_supported = XInternAtom(xdisplay, "_NET_SUPPORTED", False);
    self->net_current_desktop = XInternAtom(xdisplay, "_NET_CURRENT_DESKTOP", False);
    self->net_workarea = XInternAtom(xdisplay, "_NET_WORKAREA", False);
    self->gtk_workareas = XInternAtom(xdisplay, "_GTK_WORKAREAS", False);

    self->gtk_workareas_supported = FALSE;
    self->gtk_workareas_dn = None;

    screen = gdk_display_get_default_screen(display);
    root = gdk_screen_get_root_window(screen);

    event_mask = gdk_window_get_events(root);
    event_mask |= GDK_PROPERTY_NOTIFY;

    gdk_window_add_filter(root, filter_func, self);
    gdk_window_set_events(root, event_mask);

    update_gtk_workareas_supported(self, FALSE);
}

GDWorkareaWatcher*
gd_workarea_watcher_new(void)
{
    return g_object_new(GD_TYPE_WORKAREA_WATCHER, NULL);
}
