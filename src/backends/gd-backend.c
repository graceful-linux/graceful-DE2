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
    GDSettings*             settings;
    GDOrientationManager*   orientationManager;
    GDMonitorManager*       mm;
    guint                   upowerWatchId;
    GDBusProxy*             upowerProxy;
    gboolean                lidIsClosed;
    GList*                  gpus;
} GDBackendPrivate;

enum
{
    LID_IS_CLOSED_CHANGED,
    GPU_ADDED,
    LAST_SIGNAL
};

static guint backend_signals[LAST_SIGNAL] = {0};

static void initable_iface_init(GInitableIface* initable_iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(GDBackend, gd_backend, G_TYPE_OBJECT,
    G_ADD_PRIVATE (GDBackend) G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init))

static GDMonitorManager* create_monitor_manager(GDBackend* backend, GError** error)
{
    return GD_BACKEND_GET_CLASS(backend)->create_monitor_manager(backend, error);
}

static void upower_properties_changed_cb(GDBusProxy* proxy, GVariant* changed_properties, GStrv invalidatedProperties, GDBackend* self)
{
    GDBackendPrivate* priv;
    GVariant* v;
    gboolean lid_is_closed;

    priv = gd_backend_get_instance_private(self);

    v = g_variant_lookup_value(changed_properties, "LidIsClosed", G_VARIANT_TYPE_BOOLEAN);

    if (v == NULL) return;

    lid_is_closed = g_variant_get_boolean(v);
    g_variant_unref(v);

    if (priv->lidIsClosed == lid_is_closed) return;

    priv->lidIsClosed = lid_is_closed;

    g_signal_emit(self, backend_signals[LID_IS_CLOSED_CHANGED], 0, priv->lidIsClosed);
}

static void upower_ready_cb(GObject* srcObj, GAsyncResult* res, gpointer uData)
{
    GError* error = NULL;

    GDBusProxy* proxy = g_dbus_proxy_new_finish(res, &error);
    if (proxy == NULL) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
            g_warning("Failed to create UPower proxy: %s", error->message);
        }

        g_error_free(error);
        return;
    }

    GDBackend* self = GD_BACKEND(uData);
    GDBackendPrivate* priv = gd_backend_get_instance_private(self);
    priv->upowerProxy = proxy;

    g_signal_connect(proxy, "g-properties-changed", G_CALLBACK (upower_properties_changed_cb), self);

    GVariant* v = g_dbus_proxy_get_cached_property(proxy, "LidIsClosed");
    C_RETURN_IF_FAIL(v);

    priv->lidIsClosed = g_variant_get_boolean(v);
    g_variant_unref(v);

    if (priv->lidIsClosed) {
        g_signal_emit(self, backend_signals[LID_IS_CLOSED_CHANGED], 0, priv->lidIsClosed);
    }
}

static void upower_appeared_cb(GDBusConnection* connection, const char* name, const char* nameOwner, gpointer uData)
{
    GDBackend* self = GD_BACKEND(uData);

    g_dbus_proxy_new(
        connection,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        "org.freedesktop.UPower",
        "/org/freedesktop/UPower",
        "org.freedesktop.UPower",
        NULL, upower_ready_cb, self);
}

static void upower_vanished_cb(GDBusConnection* connection, const char* name, gpointer uData)
{
    GDBackend* self = GD_BACKEND(uData);
    GDBackendPrivate* priv = gd_backend_get_instance_private(self);

    g_clear_object(&priv->upowerProxy);
}

static gboolean gd_backend_initable_init(GInitable* initable, GCancellable* cancellable, GError** error)
{
    GDBackend* backend = GD_BACKEND(initable);
    GDBackendPrivate* priv = gd_backend_get_instance_private(backend);

    priv->settings = gd_settings_new(backend);
    priv->orientationManager = gd_orientation_manager_new();

    priv->mm = create_monitor_manager(backend, error);
    if (!priv->mm) return FALSE;

    return TRUE;
}

static void initable_iface_init(GInitableIface* initable_iface)
{
    initable_iface->init = gd_backend_initable_init;
}

static bool gd_backend_real_is_lid_closed(GDBackend* self)
{
    GDBackendPrivate* priv = gd_backend_get_instance_private(self);

    return priv->lidIsClosed;
}

static void gd_backend_constructed(GObject* object)
{
    GDBackend* self = GD_BACKEND(object);
    GDBackendClass* sc = GD_BACKEND_GET_CLASS(self);
    GDBackendPrivate* priv = gd_backend_get_instance_private(self);

    G_OBJECT_CLASS(gd_backend_parent_class)->constructed(object);
    C_RETURN_IF_OK(gd_backend_real_is_lid_closed != sc->is_lid_closed);

    priv->upowerWatchId = g_bus_watch_name(
        G_BUS_TYPE_SYSTEM,
        "org.freedesktop.UPower",
        G_BUS_NAME_WATCHER_FLAGS_NONE,
        upower_appeared_cb,
        upower_vanished_cb,
        self, NULL);
}

static void gd_backend_dispose(GObject* object)
{
    GDBackend* backend = GD_BACKEND(object);
    GDBackendPrivate* priv = gd_backend_get_instance_private(backend);

    g_clear_object(&priv->mm);
    g_clear_object(&priv->orientationManager);
    g_clear_object(&priv->settings);

    G_OBJECT_CLASS(gd_backend_parent_class)->dispose(object);
}

static void gd_backend_finalize(GObject* object)
{
    GDBackend* self = GD_BACKEND(object);
    GDBackendPrivate* priv = gd_backend_get_instance_private(self);

    if (priv->upowerWatchId != 0) {
        g_bus_unwatch_name(priv->upowerWatchId);
        priv->upowerWatchId = 0;
    }

    g_clear_object(&priv->upowerProxy);

    g_list_free_full(priv->gpus, g_object_unref);

    G_OBJECT_CLASS(gd_backend_parent_class)->finalize(object);
}

static void gd_backend_real_post_init(GDBackend* backend)
{
    GDBackendPrivate* priv = gd_backend_get_instance_private(backend);

    gd_monitor_manager_setup(priv->mm);
}

static void gd_backend_class_init(GDBackendClass* backend_class)
{
    GObjectClass* oc = G_OBJECT_CLASS(backend_class);

    oc->constructed = gd_backend_constructed;
    oc->dispose = gd_backend_dispose;
    oc->finalize = gd_backend_finalize;

    backend_class->post_init = gd_backend_real_post_init;
    backend_class->is_lid_closed = gd_backend_real_is_lid_closed;

    backend_signals[LID_IS_CLOSED_CHANGED] = g_signal_new(
        "lid-is-closed-changed",
        G_TYPE_FROM_CLASS(backend_class),
        G_SIGNAL_RUN_LAST,
        0,
        NULL,
        NULL,
        NULL,
        G_TYPE_NONE,
        1, G_TYPE_BOOLEAN);

    backend_signals[GPU_ADDED] = g_signal_new(
        "gpu-added",
        G_TYPE_FROM_CLASS(backend_class),
        G_SIGNAL_RUN_LAST,
        0,
        NULL,
        NULL,
        NULL,
        G_TYPE_NONE,
        1, GD_TYPE_GPU);
}

static void gd_backend_init(GDBackend* backend)
{
}

GDBackend* gd_backend_new(GDBackendType type)
{
    GType gtype;

    switch (type) {
    case GD_BACKEND_TYPE_X11_CM:
        gtype = GD_TYPE_BACKEND_X11_CM;
        break;

    default:
        g_assert_not_reached();
        break;
    }

    GDBackend* backend = g_object_new(gtype, NULL);
    GDBackendPrivate* priv = gd_backend_get_instance_private(backend);

    GError* error = NULL;
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
    GDBackendPrivate* priv = gd_backend_get_instance_private(backend);

    return priv->mm;
}

GDOrientationManager* gd_backend_get_orientation_manager(GDBackend* backend)
{
    GDBackendPrivate* priv = gd_backend_get_instance_private(backend);

    return priv->orientationManager;
}

GDSettings* gd_backend_get_settings(GDBackend* backend)
{
    GDBackendPrivate* priv = gd_backend_get_instance_private(backend);

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
    GDBackendPrivate* priv = gd_backend_get_instance_private(self);

    priv->gpus = g_list_append(priv->gpus, gpu);

    g_signal_emit(self, backend_signals[GPU_ADDED], 0, gpu);
}

GList* gd_backend_get_gpus(GDBackend* self)
{
    GDBackendPrivate* priv = gd_backend_get_instance_private(self);

    return priv->gpus;
}
