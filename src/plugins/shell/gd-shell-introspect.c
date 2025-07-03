//
// Created by dingjing on 25-7-3.
//
#include "gd-shell-introspect.h"

#include "gd-shell-introspect-gen.dbus.h"

#define SHELL_INTROSPECT_DBUS_NAME "org.graceful.DE2.Introspect"
#define SHELL_INTROSPECT_DBUS_PATH "/org/graceful/DE2/Introspect"

#define INTROSPECT_DBUS_API_VERSION 2

struct _GDShellIntrospect
{
    GObject                 parent;

    GDShellIntrospectGen*   introspect;
    gint                    busNameId;
    GSettings*              interfaceSettings;
};

G_DEFINE_TYPE(GDShellIntrospect, gd_shell_introspect, G_TYPE_OBJECT)

static void enable_animations_changed_cb(GSettings* settings, const char* key, GDShellIntrospect* self)
{
    const gboolean ae = g_settings_get_boolean(settings, key);
    gd_shell_introspect_gen_set_animations_enabled(self->introspect, ae);
}

static gboolean handle_get_running_applications(GDShellIntrospectGen* object, GDBusMethodInvocation* invocation, GDShellIntrospect* self)
{
    g_dbus_method_invocation_return_error_literal(invocation,
        G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
        "GetRunningApplications method is not implemented!");

    return TRUE;
}

static gboolean handle_get_windows(GDShellIntrospectGen* object, GDBusMethodInvocation* invocation, GDShellIntrospect* self)
{
    g_dbus_method_invocation_return_error_literal(invocation,
        G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
        "GetWindows method is not implemented!");

    return TRUE;
}

static void bus_acquired_handler(GDBusConnection* connection, const char* name, gpointer user_data)
{
    GDShellIntrospect* self = GD_SHELL_INTROSPECT(user_data);

    GDBusInterfaceSkeleton* skeleton = G_DBUS_INTERFACE_SKELETON(self->introspect);

    GError* error = NULL;
    if (!g_dbus_interface_skeleton_export(skeleton, connection, SHELL_INTROSPECT_DBUS_PATH, &error)) {
        g_warning("Failed to export interface: %s", error->message);
        g_error_free(error);
        return;
    }

    g_signal_connect(self->introspect, "handle-get-running-applications", G_CALLBACK(handle_get_running_applications), self);
    g_signal_connect(self->introspect, "handle-get-windows", G_CALLBACK(handle_get_windows), self);

    gd_shell_introspect_gen_set_version(self->introspect, INTROSPECT_DBUS_API_VERSION);
}

static void name_acquired_handler(GDBusConnection* connection, const char* name, gpointer uData)
{
}

static void name_lost_handler(GDBusConnection* connection, const char* name, gpointer uData)
{
}

static void gd_shell_introspect_dispose(GObject* object)
{
    GDShellIntrospect* self;

    self = GD_SHELL_INTROSPECT(object);

    if (self->busNameId != 0) {
        g_bus_unown_name(self->busNameId);
        self->busNameId = 0;
    }

    if (self->introspect != NULL) {
        GDBusInterfaceSkeleton* skeleton = G_DBUS_INTERFACE_SKELETON(self->introspect);
        g_dbus_interface_skeleton_unexport(skeleton);

        g_object_unref(self->introspect);
        self->introspect = NULL;
    }

    g_clear_object(&self->interfaceSettings);
    G_OBJECT_CLASS(gd_shell_introspect_parent_class)->dispose(object);
}

static void gd_shell_introspect_class_init(GDShellIntrospectClass* self)
{
    GObjectClass* oc = G_OBJECT_CLASS(self);

    oc->dispose = gd_shell_introspect_dispose;
}

static void gd_shell_introspect_init(GDShellIntrospect* self)
{
    self->introspect = gd_shell_introspect_gen_skeleton_new();

    self->interfaceSettings = g_settings_new("org.gnome.desktop.interface");

    g_signal_connect(self->interfaceSettings, "changed::enable-animations", G_CALLBACK(enable_animations_changed_cb), self);

    enable_animations_changed_cb(self->interfaceSettings, "enable-animations", self);

    self->busNameId = g_bus_own_name(G_BUS_TYPE_SESSION,
        SHELL_INTROSPECT_DBUS_NAME,
        G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT | G_BUS_NAME_OWNER_FLAGS_REPLACE,
        bus_acquired_handler, name_acquired_handler, name_lost_handler, self, NULL);
}

GDShellIntrospect* gd_shell_introspect_new(void)
{
    return g_object_new(GD_TYPE_SHELL_INTROSPECT, NULL);
}
