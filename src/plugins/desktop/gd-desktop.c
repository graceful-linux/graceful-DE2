//
// Created by dingjing on 25-6-30.
//
#include "gd-desktop.h"

#include <gio/gio.h>

#include "gd-desktop-window.h"

struct _GDDesktop
{
    GObject parent;

    GSettings*      settings;
    GtkWidget*      window;
};

G_DEFINE_TYPE(GDDesktop, gd_desktop, G_TYPE_OBJECT)

static void ready_cb(GDDesktopWindow* window, GDDesktop* self)
{
    gtk_widget_show(self->window);
}

static void gd_desktop_dispose(GObject* object)
{
    GDDesktop* self = GD_DESKTOP(object);

    g_clear_object(&self->settings);
    g_clear_pointer(&self->window, gtk_widget_destroy);

    G_OBJECT_CLASS(gd_desktop_parent_class)->dispose(object);
}

static void gd_desktop_class_init(GDDesktopClass* self)
{
    GObjectClass* obj = G_OBJECT_CLASS(self);

    obj->dispose = gd_desktop_dispose;
}

static void gd_desktop_init(GDDesktop* self)
{
    self->settings = g_settings_new("org.gnome.gnome-flashback.desktop");

    bool drawBackground = g_settings_get_boolean(self->settings, "draw-background");
    bool showIcons = g_settings_get_boolean(self->settings, "show-icons");

    GError* error = NULL;
    self->window = gd_desktop_window_new(drawBackground, showIcons, &error);
    if (error != NULL) {
        g_warning("%s", error->message);
        g_error_free(error);
        return;
    }

    g_settings_bind(self->settings, "draw-background", self->window, "draw-background", G_SETTINGS_BIND_GET);
    g_settings_bind(self->settings, "show-icons", self->window, "show-icons", G_SETTINGS_BIND_GET);
    if (!gd_desktop_window_is_ready(GD_DESKTOP_WINDOW(self->window))) {
        g_signal_connect(self->window, "ready", G_CALLBACK (ready_cb), self);
    }
    else {
        gtk_widget_show(self->window);
    }
}

GDDesktop* gd_desktop_new(void)
{
    return g_object_new(GD_TYPE_DESKTOP, NULL);
}

void gd_desktop_set_monitor_manager(GDDesktop* self, GDMonitorManager* monitor_manager)
{
    if (self->window == NULL) return;

    gd_desktop_window_set_monitor_manager(GD_DESKTOP_WINDOW(self->window), monitor_manager);
}
