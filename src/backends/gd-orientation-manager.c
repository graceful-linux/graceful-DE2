//
// Created by dingjing on 25-6-28.
//
#include "gd-orientation-manager-private.h"

#include "macros/macros.h"


#include <gio/gio.h>

#define TOUCHSCREEN_SCHEMA      "org.gnome.settings-daemon.peripherals.touchscreen"
#define ORIENTATION_LOCK_KEY    "orientation-lock"

struct _GDOrientationManager
{
    GObject             parent;
    GCancellable*       cancellable;

    guint               iioWatchId;
    guint               syncIdleId;
    GDBusProxy*         iioProxy;
    GDOrientation       prevOrientation;
    GDOrientation       currOrientation;
    bool                hasAccel;
    GSettings*          settings;
};

enum
{
    PROP_0,
    PROP_HAS_ACCELEROMETER,
    LAST_PROP
};

static GParamSpec* gsManagerProperties[LAST_PROP] = {NULL};

enum
{
    ORIENTATION_CHANGED,
    LAST_SIGNAL
};

static guint gsManagerSignals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(GDOrientationManager, gd_orientation_manager, G_TYPE_OBJECT)

static GDOrientation orientation_from_string(const gchar* orientation)
{
    if (g_strcmp0(orientation, "normal") == 0) {
        return GD_ORIENTATION_NORMAL;
    }

    if (g_strcmp0(orientation, "bottom-up") == 0) {
        return GD_ORIENTATION_BOTTOM_UP;
    }

    if (g_strcmp0(orientation, "left-up") == 0) {
        return GD_ORIENTATION_LEFT_UP;
    }

    if (g_strcmp0(orientation, "right-up") == 0) {
        return GD_ORIENTATION_RIGHT_UP;
    }

    return GD_ORIENTATION_UNDEFINED;
}

static void read_iio_proxy(GDOrientationManager* manager)
{
    manager->currOrientation = GD_ORIENTATION_UNDEFINED;

    if (!manager->iioProxy) {
        manager->hasAccel = FALSE;
        return;
    }

    GVariant* variant = g_dbus_proxy_get_cached_property(manager->iioProxy, "HasAccelerometer");
    if (variant) {
        manager->hasAccel = g_variant_get_boolean(variant);
        g_variant_unref(variant);
    }
    C_RETURN_IF_OK(!manager->hasAccel);

    variant = g_dbus_proxy_get_cached_property(manager->iioProxy, "AccelerometerOrientation");
    if (variant) {
        const gchar* str = g_variant_get_string(variant, NULL);
        manager->currOrientation = orientation_from_string(str);
        g_variant_unref(variant);
    }
}

static void sync_state(GDOrientationManager* manager)
{
    const bool hadAccel = manager->hasAccel;

    read_iio_proxy(manager);

    if (hadAccel != manager->hasAccel) {
        g_object_notify_by_pspec(G_OBJECT(manager), gsManagerProperties[PROP_HAS_ACCELEROMETER]);
    }

    C_RETURN_IF_OK(g_settings_get_boolean(manager->settings, ORIENTATION_LOCK_KEY));
    C_RETURN_IF_OK(manager->prevOrientation == manager->currOrientation);

    manager->prevOrientation = manager->currOrientation;
    C_RETURN_IF_OK(manager->currOrientation == GD_ORIENTATION_UNDEFINED);

    g_signal_emit(manager, gsManagerSignals[ORIENTATION_CHANGED], 0);
}

static gboolean sync_state_cb(gpointer uData)
{
    GDOrientationManager* self = uData;
    self->syncIdleId = 0;

    sync_state(self);

    return G_SOURCE_REMOVE;
}

static void queue_sync_state(GDOrientationManager* self)
{
    if (self->syncIdleId != 0) return;

    self->syncIdleId = g_idle_add(sync_state_cb, self);
}

static void orientation_lock_changed_cb(GSettings* settings, const gchar* key, gpointer uData)
{
    GDOrientationManager* manager = GD_ORIENTATION_MANAGER(uData);

    queue_sync_state(manager);
}

static void iio_properties_changed_cb(GDBusProxy* proxy, GVariant* changedProp, GStrv invalidatedProp, gpointer uData)
{
    GDOrientationManager* manager = GD_ORIENTATION_MANAGER(uData);

    queue_sync_state(manager);
}

static void accelerometer_claimed_cb(GObject* source, GAsyncResult* res, gpointer uData)
{
    GError* error = NULL;

    GVariant* variant = g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &error);
    if (!variant) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
            g_warning("Failed to claim accelerometer: %s", error->message);
        }
        g_error_free(error);
        return;
    }

    GDOrientationManager* manager = GD_ORIENTATION_MANAGER(uData);

    g_variant_unref(variant);
    sync_state(manager);
}

static void iio_proxy_ready_cb(GObject* source, GAsyncResult* res, gpointer uData)
{
    GError* error = NULL;

    GDBusProxy* proxy = g_dbus_proxy_new_finish(res, &error);

    if (!proxy) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
            g_warning("Failed to obtain IIO DBus proxy: %s", error->message);
        }
        g_error_free(error);
        return;
    }

    GDOrientationManager* manager = GD_ORIENTATION_MANAGER(uData);
    manager->iioProxy = proxy;

    g_signal_connect(manager->iioProxy, "g-properties-changed", G_CALLBACK (iio_properties_changed_cb), manager);
    g_dbus_proxy_call(manager->iioProxy,
        "ClaimAccelerometer",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        manager->cancellable,
        accelerometer_claimed_cb, manager);
}

static void iio_sensor_appeared_cb(GDBusConnection* connection, const gchar* name, const gchar* nameOwner, gpointer uData)
{
    GDOrientationManager* manager = GD_ORIENTATION_MANAGER(uData);

    manager->cancellable = g_cancellable_new();
    g_dbus_proxy_new(connection,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        "net.hadess.SensorProxy",
        "/net/hadess/SensorProxy",
        "net.hadess.SensorProxy",
        manager->cancellable,
        iio_proxy_ready_cb, manager);
}

static void iio_sensor_vanished_cb(GDBusConnection* connection, const gchar* name, gpointer uData)
{
    GDOrientationManager* manager = GD_ORIENTATION_MANAGER(uData);

    g_cancellable_cancel(manager->cancellable);
    g_clear_object(&manager->cancellable);
    g_clear_object(&manager->iioProxy);

    sync_state(manager);
}

static void gd_orientation_manager_dispose(GObject* object)
{
    GDOrientationManager* manager = GD_ORIENTATION_MANAGER(object);
    if (manager->cancellable != NULL) {
        g_cancellable_cancel(manager->cancellable);
        g_clear_object(&manager->cancellable);
    }

    if (manager->iioWatchId != 0) {
        g_bus_unwatch_name(manager->iioWatchId);
        manager->iioWatchId = 0;
    }

    if (manager->syncIdleId != 0) {
        g_source_remove(manager->syncIdleId);
        manager->syncIdleId = 0;
    }

    g_clear_object(&manager->iioProxy);
    g_clear_object(&manager->settings);

    G_OBJECT_CLASS(gd_orientation_manager_parent_class)->dispose(object);
}

static void gd_orientation_manager_get_property(GObject* object, guint propertyId, GValue* value, GParamSpec* pspec)
{
    GDOrientationManager* self = GD_ORIENTATION_MANAGER(object);
    switch (propertyId) {
        case PROP_HAS_ACCELEROMETER: {
            g_value_set_boolean(value, self->hasAccel);
            break;
        }
        default: {
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyId, pspec);
            break;
        }
    }
}

static void gd_orientation_manager_install_properties(GObjectClass* oc)
{
    gsManagerProperties[PROP_HAS_ACCELEROMETER] =
        g_param_spec_boolean(
            "has-accelerometer",
            "Has accelerometer",
            "Has accelerometer",
            FALSE,
            G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(oc, LAST_PROP, gsManagerProperties);
}

static void gd_orientation_manager_install_signals(GObjectClass* oc)
{
    gsManagerSignals[ORIENTATION_CHANGED]
        = g_signal_new(
            "orientation-changed",
            G_TYPE_FROM_CLASS(oc),
            G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            NULL,
            G_TYPE_NONE,
            0);
}

static void gd_orientation_manager_class_init(GDOrientationManagerClass* mc)
{
    GObjectClass* oc = G_OBJECT_CLASS(mc);

    oc->dispose = gd_orientation_manager_dispose;
    oc->get_property = gd_orientation_manager_get_property;

    gd_orientation_manager_install_properties(oc);
    gd_orientation_manager_install_signals(oc);
}

static void gd_orientation_manager_init(GDOrientationManager* manager)
{
    manager->iioWatchId = g_bus_watch_name(
        G_BUS_TYPE_SYSTEM,
        "net.hadess.SensorProxy",
        G_BUS_NAME_WATCHER_FLAGS_NONE,
        iio_sensor_appeared_cb,
        iio_sensor_vanished_cb,
        manager, NULL);

    manager->settings = g_settings_new(TOUCHSCREEN_SCHEMA);

    g_signal_connect(manager->settings, "changed::" ORIENTATION_LOCK_KEY, G_CALLBACK (orientation_lock_changed_cb), manager);

    sync_state(manager);
}

GDOrientationManager* gd_orientation_manager_new(void)
{
    return g_object_new(GD_TYPE_ORIENTATION_MANAGER, NULL);
}

GDOrientation gd_orientation_manager_get_orientation(GDOrientationManager* manager)
{
    return manager->currOrientation;
}

bool gd_orientation_manager_has_accelerometer(GDOrientationManager* self)
{
    return self->hasAccel;
}
