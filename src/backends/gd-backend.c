//
// Created by dingjing on 25-6-28.
//

#include "gd-backend.h"

#include <gio/gio.h>

#include "gd-gpu-private.h"
#include "gd-settings-private.h"
#include "gd-backend-x11-cm-private.h"
#include "gd-orientation-manager-private.h"

typedef struct
{
    GDSettings* settings;
    GDOrientationManager* orientation_manager;

    GDMonitorManager* monitor_manager;

    guint upower_watch_id;
    GDBusProxy* upower_proxy;
    gboolean lid_is_closed;

    GList* gpus;
} GDBackendPrivate;

enum
{
    LID_IS_CLOSED_CHANGED,
    GPU_ADDED,
    LAST_SIGNAL
};

static guint backend_signals[LAST_SIGNAL] = {0};

static void initable_iface_init(GInitableIface* initable_iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(GDBackend, gd_backend, G_TYPE_OBJECT, G_ADD_PRIVATE (GDBackend) G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init))

static GDMonitorManager* create_monitor_manager(GDBackend* backend, GError** error)
{
    return GD_BACKEND_GET_CLASS(backend)->create_monitor_manager(backend, error);
}

static void upower_properties_changed_cb(GDBusProxy* proxy, GVariant* changed_properties, GStrv invalidated_properties, GDBackend* self)
{
    GDBackendPrivate* priv;
    GVariant* v;
    gboolean lid_is_closed;

    priv = gd_backend_get_instance_private(self);

    v = g_variant_lookup_value(changed_properties, "LidIsClosed", G_VARIANT_TYPE_BOOLEAN);

    if (v == NULL) return;

    lid_is_closed = g_variant_get_boolean(v);
    g_variant_unref(v);

    if (priv->lid_is_closed == lid_is_closed) return;

    priv->lid_is_closed = lid_is_closed;

    g_signal_emit(self, backend_signals[LID_IS_CLOSED_CHANGED], 0, priv->lid_is_closed);
}

static void upower_ready_cb(GObject* source_object, GAsyncResult* res, gpointer user_data)
{
    GError* error;
    GDBusProxy* proxy;
    GDBackend* self;
    GDBackendPrivate* priv;
    GVariant* v;

    error = NULL;
    proxy = g_dbus_proxy_new_finish(res, &error);

    if (proxy == NULL) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
            g_warning("Failed to create UPower proxy: %s", error->message);
        }

        g_error_free(error);
        return;
    }

    self = GD_BACKEND(user_data);
    priv = gd_backend_get_instance_private(self);

    priv->upower_proxy = proxy;

    g_signal_connect(proxy, "g-properties-changed", G_CALLBACK (upower_properties_changed_cb), self);

    v = g_dbus_proxy_get_cached_property(proxy, "LidIsClosed");

    if (v == NULL) return;

    priv->lid_is_closed = g_variant_get_boolean(v);
    g_variant_unref(v);

    if (priv->lid_is_closed) {
        g_signal_emit(self, backend_signals[LID_IS_CLOSED_CHANGED], 0, priv->lid_is_closed);
    }
}

static void upower_appeared_cb(GDBusConnection* connection, const char* name, const char* name_owner, gpointer user_data)
{
    GDBackend* self;

    self = GD_BACKEND(user_data);

    g_dbus_proxy_new(connection, G_DBUS_PROXY_FLAGS_NONE, NULL, "org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", NULL, upower_ready_cb, self);
}

static void upower_vanished_cb(GDBusConnection* connection, const char* name, gpointer user_data)
{
    GDBackend* self;
    GDBackendPrivate* priv;

    self = GD_BACKEND(user_data);
    priv = gd_backend_get_instance_private(self);

    g_clear_object(&priv->upower_proxy);
}

static gboolean gd_backend_initable_init(GInitable* initable, GCancellable* cancellable, GError** error)
{
    GDBackend* backend;
    GDBackendPrivate* priv;

    backend = GD_BACKEND(initable);
    priv = gd_backend_get_instance_private(backend);

    priv->settings = gd_settings_new(backend);
    priv->orientation_manager = gd_orientation_manager_new();

    priv->monitor_manager = create_monitor_manager(backend, error);
    if (!priv->monitor_manager) return FALSE;

    return TRUE;
}

static void initable_iface_init(GInitableIface* initable_iface)
{
    initable_iface->init = gd_backend_initable_init;
}

static bool gd_backend_real_is_lid_closed(GDBackend* self)
{
    GDBackendPrivate* priv;

    priv = gd_backend_get_instance_private(self);

    return priv->lid_is_closed;
}

static void gd_backend_constructed(GObject* object)
{
    GDBackend* self;
    GDBackendClass* self_class;
    GDBackendPrivate* priv;

    self = GD_BACKEND(object);
    self_class = GD_BACKEND_GET_CLASS(self);
    priv = gd_backend_get_instance_private(self);

    G_OBJECT_CLASS(gd_backend_parent_class)->constructed(object);

    if (self_class->is_lid_closed != gd_backend_real_is_lid_closed) return;

    priv->upower_watch_id = g_bus_watch_name(G_BUS_TYPE_SYSTEM, "org.freedesktop.UPower", G_BUS_NAME_WATCHER_FLAGS_NONE, upower_appeared_cb, upower_vanished_cb, self, NULL);
}

static void gd_backend_dispose(GObject* object)
{
    GDBackend* backend;
    GDBackendPrivate* priv;

    backend = GD_BACKEND(object);
    priv = gd_backend_get_instance_private(backend);

    g_clear_object(&priv->monitor_manager);
    g_clear_object(&priv->orientation_manager);
    g_clear_object(&priv->settings);

    G_OBJECT_CLASS(gd_backend_parent_class)->dispose(object);
}

static void gd_backend_finalize(GObject* object)
{
    GDBackend* self;
    GDBackendPrivate* priv;

    self = GD_BACKEND(object);
    priv = gd_backend_get_instance_private(self);

    if (priv->upower_watch_id != 0) {
        g_bus_unwatch_name(priv->upower_watch_id);
        priv->upower_watch_id = 0;
    }

    g_clear_object(&priv->upower_proxy);

    g_list_free_full(priv->gpus, g_object_unref);

    G_OBJECT_CLASS(gd_backend_parent_class)->finalize(object);
}

static void gd_backend_real_post_init(GDBackend* backend)
{
    GDBackendPrivate* priv;

    priv = gd_backend_get_instance_private(backend);

    gd_monitor_manager_setup(priv->monitor_manager);
}

static void gd_backend_class_init(GDBackendClass* backend_class)
{
    GObjectClass* object_class;

    object_class = G_OBJECT_CLASS(backend_class);

    object_class->constructed = gd_backend_constructed;
    object_class->dispose = gd_backend_dispose;
    object_class->finalize = gd_backend_finalize;

    backend_class->post_init = gd_backend_real_post_init;
    backend_class->is_lid_closed = gd_backend_real_is_lid_closed;

    backend_signals[LID_IS_CLOSED_CHANGED] = g_signal_new("lid-is-closed-changed", G_TYPE_FROM_CLASS(backend_class), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

    backend_signals[GPU_ADDED] = g_signal_new("gpu-added", G_TYPE_FROM_CLASS(backend_class), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, GD_TYPE_GPU);
}

static void gd_backend_init(GDBackend* backend)
{
}

GDBackend* gd_backend_new(GDBackendType type)
{
    GType gtype;
    GDBackend* backend;
    GDBackendPrivate* priv;
    GError* error;

    switch (type) {
    case GD_BACKEND_TYPE_X11_CM:
        gtype = GD_TYPE_BACKEND_X11_CM;
        break;

    default:
        g_assert_not_reached();
        break;
    }

    backend = g_object_new(gtype, NULL);
    priv = gd_backend_get_instance_private(backend);

    error = NULL;
    if (!g_initable_init(G_INITABLE(backend), NULL, &error)) {
        g_warning("Failed to create backend: %s", error->message);

        g_object_unref(backend);
        g_error_free(error);

        return NULL;
    }

    GD_BACKEND_GET_CLASS(backend)->post_init(backend);
    gd_settings_post_init(priv->settings);

    return backend;
}

GDMonitorManager* gd_backend_get_monitor_manager(GDBackend* backend)
{
    GDBackendPrivate* priv;

    priv = gd_backend_get_instance_private(backend);

    return priv->monitor_manager;
}

GDOrientationManager* gd_backend_get_orientation_manager(GDBackend* backend)
{
    GDBackendPrivate* priv;

    priv = gd_backend_get_instance_private(backend);

    return priv->orientation_manager;
}

GDSettings* gd_backend_get_settings(GDBackend* backend)
{
    GDBackendPrivate* priv;

    priv = gd_backend_get_instance_private(backend);

    return priv->settings;
}

void gd_backend_monitors_changed(GDBackend* backend)
{
}

bool gd_backend_is_lid_closed(GDBackend* self)
{
    return GD_BACKEND_GET_CLASS(self)->is_lid_closed(self);
}

void gd_backend_add_gpu(GDBackend* self, GDGpu* gpu)
{
    GDBackendPrivate* priv;

    priv = gd_backend_get_instance_private(self);

    priv->gpus = g_list_append(priv->gpus, gpu);

    g_signal_emit(self, backend_signals[GPU_ADDED], 0, gpu);
}

GList* gd_backend_get_gpus(GDBackend* self)
{
    GDBackendPrivate* priv;

    priv = gd_backend_get_instance_private(self);

    return priv->gpus;
}
