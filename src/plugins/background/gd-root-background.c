//
// Created by dingjing on 25-7-3.
//
#include "gd-root-background.h"

#include "../common/gd-bg.h"

#include <gtk/gtk.h>


struct _GDRootBackground
{
    GObject     parent;

    guint       changeIdleId;
    gulong      sizeChangedId;
    gulong      monitorsChangedId;

    GDBG*       bg;
};

G_DEFINE_TYPE(GDRootBackground, gd_root_background, G_TYPE_OBJECT)

static void get_display_size(GdkDisplay* display, int* width, int* height)
{
    int i = 0;
    GdkRectangle rect;

    rect = (GdkRectangle){};
    display = gdk_display_get_default();
    int nMonitors = gdk_display_get_n_monitors(display);

    for (i = 0; i < nMonitors; i++) {
        GdkMonitor* monitor = gdk_display_get_monitor(display, i);

        GdkRectangle geometry;
        gdk_monitor_get_geometry(monitor, &geometry);
        gdk_rectangle_union(&rect, &geometry, &rect);
    }

    *width = rect.width;
    *height = rect.height;
}

static void set_background(GDRootBackground* self)
{
    gint width;
    gint height;

    GdkDisplay* display = gdk_display_get_default();
    GdkScreen* screen = gdk_display_get_default_screen(display);
    GdkWindow* root = gdk_screen_get_root_window(screen);

    get_display_size(display, &width, &height);

    cairo_surface_t* surface = gd_bg_create_surface(self->bg, root, width, height, TRUE);

    gd_bg_set_surface_as_root(display, surface);
    cairo_surface_destroy(surface);
}

static void changed_cb(GDBG* bg, GDRootBackground* self)
{
    set_background(self);
}

static void transitioned_cb(GDBG* bg, GDRootBackground* self)
{
    set_background(self);
}

static gboolean change_cb(gpointer user_data)
{
    GDRootBackground* self = GD_ROOT_BACKGROUND(user_data);

    set_background(self);
    self->changeIdleId = 0;

    return G_SOURCE_REMOVE;
}

static void queue_change(GDRootBackground* self)
{
    if (self->changeIdleId != 0) return;

    self->changeIdleId = g_idle_add(change_cb, self);
}

static void monitors_changed_cb(GdkScreen* screen, GDRootBackground* self)
{
    queue_change(self);
}

static void size_changed_cb(GdkScreen* screen, GDRootBackground* self)
{
    queue_change(self);
}

static void gd_root_background_dispose(GObject* object)
{
    GDRootBackground* self = GD_ROOT_BACKGROUND(object);
    GdkScreen* screen = gdk_screen_get_default();

    if (self->monitorsChangedId != 0) {
        g_signal_handler_disconnect(screen, self->monitorsChangedId);
        self->monitorsChangedId = 0;
    }

    if (self->sizeChangedId != 0) {
        g_signal_handler_disconnect(screen, self->sizeChangedId);
        self->sizeChangedId = 0;
    }

    if (self->changeIdleId != 0) {
        g_source_remove(self->changeIdleId);
        self->changeIdleId = 0;
    }

    g_clear_object(&self->bg);

    G_OBJECT_CLASS(gd_root_background_parent_class)->dispose(object);
}

static void gd_root_background_class_init(GDRootBackgroundClass* self_class)
{
    GObjectClass* oc = G_OBJECT_CLASS(self_class);

    oc->dispose = gd_root_background_dispose;
}

static void gd_root_background_init(GDRootBackground* self)
{
    GdkScreen* screen = gdk_screen_get_default();

    self->monitorsChangedId = g_signal_connect(screen, "monitors-changed", G_CALLBACK (monitors_changed_cb), self);
    self->sizeChangedId = g_signal_connect(screen, "size-changed", G_CALLBACK (size_changed_cb), self);

    self->bg = gd_bg_new("org.gnome.desktop.background");

    g_signal_connect(self->bg, "changed", G_CALLBACK (changed_cb), self);
    g_signal_connect(self->bg, "transitioned", G_CALLBACK (transitioned_cb), self);

    gd_bg_load_from_preferences(self->bg);
}

GDRootBackground* gd_root_background_new(void)
{
    return g_object_new(GD_TYPE_ROOT_BACKGROUND, NULL);
}
