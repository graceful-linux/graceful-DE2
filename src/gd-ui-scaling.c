//
// Created by dingjing on 25-6-28.
//

#include "gd-ui-scaling.h"

#include "gd-dbus-x11.dbus.h"
#include "backends/gd-settings.h"
#include "backends/gd-monitor-manager.h"
#include "backends/gd-dbus-display-config.h"

struct _GDUiScaling
{
    GObject parent;

    GDBackend* backend;

    GDDBusX11* x11;
    guint bus_name_id;
};

enum { PROP_0, PROP_BACKEND, LAST_PROP };

static GParamSpec* ui_scaling_properties[LAST_PROP] = {NULL};

G_DEFINE_TYPE(GDUiScaling, gd_ui_scaling, G_TYPE_OBJECT)

static void
update_ui_scaling_factor(GDUiScaling* self)
{
    GDSettings* settings;
    int ui_scaling_factor;

    settings = gd_backend_get_settings(self->backend);
    ui_scaling_factor = gd_settings_get_ui_scaling_factor(settings);

    gd_dbus_x11_set_ui_scaling_factor(self->x11, ui_scaling_factor);
}

static void
ui_scaling_factor_changed_cb(GDSettings* settings, GDUiScaling* self)
{
    update_ui_scaling_factor(self);
}

static void
bus_acquired_cb(GDBusConnection* connection, const char* name, gpointer user_data)
{
    GDUiScaling* self;
    GDBusInterfaceSkeleton* skeleton;
    GError* error;
    gboolean exported;

    self = GD_UI_SCALING(user_data);
    skeleton = G_DBUS_INTERFACE_SKELETON(self->x11);

    error = NULL;
    exported = g_dbus_interface_skeleton_export(skeleton, connection, "/org/gnome/Mutter/X11", &error);

    if (!exported) {
        g_warning("%s", error->message);
        g_error_free(error);
        return;
    }
}

static void
name_acquired_cb(GDBusConnection* connection, const char* name, gpointer user_data)
{
}

static void
name_lost_cb(GDBusConnection* connection, const char* name, gpointer user_data)
{
}

static void
gd_ui_scaling_constructed(GObject* object)
{
    GDUiScaling* self;

    self = GD_UI_SCALING(object);

    G_OBJECT_CLASS(gd_ui_scaling_parent_class)->constructed(object);

    self->x11 = gd_dbus_x11_skeleton_new();

    self->bus_name_id = g_bus_own_name(G_BUS_TYPE_SESSION, "org.gnome.Mutter.X11", G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT | G_BUS_NAME_OWNER_FLAGS_REPLACE, bus_acquired_cb, name_acquired_cb, name_lost_cb, self, NULL);

    g_signal_connect(gd_backend_get_settings (self->backend), "ui-scaling-factor-changed", G_CALLBACK (ui_scaling_factor_changed_cb), self);

    update_ui_scaling_factor(self);
}

static void
gd_ui_scaling_dispose(GObject* object)
{
    GDUiScaling* self;

    self = GD_UI_SCALING(object);

    self->backend = NULL;

    if (self->bus_name_id != 0) {
        g_bus_unown_name(self->bus_name_id);
        self->bus_name_id = 0;
    }

    g_clear_object(&self->x11);

    G_OBJECT_CLASS(gd_ui_scaling_parent_class)->dispose(object);
}

static void
gd_ui_scaling_get_property(GObject* object, unsigned int property_id, GValue* value, GParamSpec* pspec)
{
    GDUiScaling* self;

    self = GD_UI_SCALING(object);

    switch (property_id) {
    case PROP_BACKEND:
        g_value_set_object(value, self->backend);
        break;

    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
gd_ui_scaling_set_property(GObject* object, unsigned int property_id, const GValue* value, GParamSpec* pspec)
{
    GDUiScaling* self;

    self = GD_UI_SCALING(object);

    switch (property_id) {
    case PROP_BACKEND:
        self->backend = g_value_get_object(value);
        break;

    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
install_properties(GObjectClass* object_class)
{
    ui_scaling_properties[PROP_BACKEND] = g_param_spec_object("backend", "GDBackend", "GDBackend", GD_TYPE_BACKEND, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, LAST_PROP, ui_scaling_properties);
}

static void
gd_ui_scaling_class_init(GDUiScalingClass* self_class)
{
    GObjectClass* object_class;

    object_class = G_OBJECT_CLASS(self_class);

    object_class->constructed = gd_ui_scaling_constructed;
    object_class->dispose = gd_ui_scaling_dispose;
    object_class->get_property = gd_ui_scaling_get_property;
    object_class->set_property = gd_ui_scaling_set_property;

    install_properties(object_class);
}

static void
gd_ui_scaling_init(GDUiScaling* self)
{
}

GDUiScaling*
gd_ui_scaling_new(GDBackend* backend)
{
    return g_object_new(GD_TYPE_UI_SCALING, "backend", backend, NULL);
}
