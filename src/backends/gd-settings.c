//
// Created by dingjing on 25-6-28.
//

#include "gd-settings.h"

#include <gio/gio.h>

#include "gd-backend.h"
#include "gd-settings-private.h"
#include "gd-monitor-manager-private.h"
#include "gd-logical-monitor-private.h"
#include "gd-monitor-manager-types-private.h"


struct _GDSettings
{
    GObject     parent;
    GDBackend*  backend;
    GSettings*  interface;
    gint        uiScalingFactor;
    gint        globalScalingFactor;
};

enum
{
    PROP_0,

    PROP_BACKEND,

    LAST_PROP
};

static GParamSpec* gsSettingsProperties[LAST_PROP] = { NULL };

enum
{
    UI_SCALING_FACTOR_CHANGED,
    GLOBAL_SCALING_FACTOR_CHANGED,
    LAST_SIGNAL
};

static guint gsSettingsSignals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GDSettings, gd_settings, G_TYPE_OBJECT)

static gint calculate_ui_scaling_factor (GDSettings* settings)
{
    GDMonitorManager *monitor_manager;
    GDLogicalMonitor *primary_logical_monitor;

    monitor_manager = gd_backend_get_monitor_manager (settings->backend);

    if (!monitor_manager)
        return 1;

    primary_logical_monitor = gd_monitor_manager_get_primary_logical_monitor (monitor_manager);

    if (!primary_logical_monitor)
        return 1;

    return (gint) gd_logical_monitor_get_scale (primary_logical_monitor);
}

static bool update_ui_scaling_factor (GDSettings *settings)
{
    const gint uf = calculate_ui_scaling_factor (settings);

    if (settings->uiScalingFactor != uf) {
        settings->uiScalingFactor = uf;
        return true;
    }

    return false;
}

static void monitors_changed_cb (GDMonitorManager* mm, GDSettings* settings)
{
    if (update_ui_scaling_factor (settings))
        g_signal_emit (settings, gsSettingsSignals[UI_SCALING_FACTOR_CHANGED], 0);
}

static void global_scaling_factor_changed_cb (GDSettings *settings, void* uData)
{
    if (update_ui_scaling_factor (settings)) {
        g_signal_emit (settings, gsSettingsSignals[UI_SCALING_FACTOR_CHANGED], 0);
    }
}

static bool update_global_scaling_factor (GDSettings *settings)
{
    gint scale = (gint) g_settings_get_uint (settings->interface, "scaling-factor");
    if (settings->globalScalingFactor != scale) {
        settings->globalScalingFactor = scale;
        return true;
    }

    return false;
}

static void interface_changed_cb (GSettings* interface, const gchar* key, GDSettings* settings)
{
    if (g_str_equal (key, "scaling-factor")) {
        if (update_global_scaling_factor (settings)) {
            g_signal_emit (settings, gsSettingsSignals[GLOBAL_SCALING_FACTOR_CHANGED], 0);
        }
    }
}

static void gd_settings_dispose (GObject *object)
{
    GDSettings *settings = GD_SETTINGS (object);

    g_clear_object (&settings->interface);
    settings->backend = NULL;

    G_OBJECT_CLASS (gd_settings_parent_class)->dispose (object);
}

static void gd_settings_get_property (GObject* object, guint propertyId, GValue* value, GParamSpec* pspec)
{
    GDSettings *settings = GD_SETTINGS (object);

    switch (propertyId) {
        case PROP_BACKEND: {
            g_value_set_object (value, settings->backend);
            break;
        }
        default: {
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, propertyId, pspec);
            break;
        }
    }
}

static void gd_settings_set_property (GObject* object, guint propertyId, const GValue* value, GParamSpec* pspec)
{
    GDSettings* settings = GD_SETTINGS (object);

    switch (propertyId) {
        case PROP_BACKEND: {
            settings->backend = g_value_get_object (value);
            break;
        }
        default: {
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, propertyId, pspec);
            break;
        }
    }
}

static void gd_settings_install_properties (GObjectClass* objectClass)
{
    gsSettingsProperties[PROP_BACKEND] = g_param_spec_object (
                        "backend",
                         "GDBackend",
                         "GDBackend", GD_TYPE_BACKEND,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (objectClass, LAST_PROP, gsSettingsProperties);
}

static void gd_settings_install_signals (GObjectClass* objectClass)
{
    gsSettingsSignals[UI_SCALING_FACTOR_CHANGED] =
        g_signal_new ("ui-scaling-factor-changed",
                  G_TYPE_FROM_CLASS (objectClass), G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE, 0);

    gsSettingsSignals[GLOBAL_SCALING_FACTOR_CHANGED] =
        g_signal_new ("global-scaling-factor-changed",
                  G_TYPE_FROM_CLASS (objectClass), G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void gd_settings_class_init (GDSettingsClass* settingsClass)
{
    GObjectClass *objectClass = G_OBJECT_CLASS (settingsClass);

    objectClass->dispose = gd_settings_dispose;
    objectClass->get_property = gd_settings_get_property;
    objectClass->set_property = gd_settings_set_property;

    gd_settings_install_properties (objectClass);
    gd_settings_install_signals (objectClass);
}

static void gd_settings_init (GDSettings *settings)
{
    settings->interface = g_settings_new ("org.gnome.desktop.interface");
    g_signal_connect (settings->interface, "changed", G_CALLBACK (interface_changed_cb), settings);

    /* Chain up inter-dependent settings. */
    g_signal_connect (settings, "global-scaling-factor-changed", G_CALLBACK (global_scaling_factor_changed_cb), NULL);

    update_global_scaling_factor (settings);
}

GDSettings* gd_settings_new (GDBackend *backend)
{
    GDSettings *settings = g_object_new (GD_TYPE_SETTINGS, "backend", backend, NULL);
    return settings;
}

void gd_settings_post_init (GDSettings* settings)
{
    GDMonitorManager* mm = gd_backend_get_monitor_manager (settings->backend);
    g_signal_connect_object (mm, "monitors-changed",
                           G_CALLBACK (monitors_changed_cb),
                           settings, G_CONNECT_AFTER);

    update_ui_scaling_factor (settings);
}

int gd_settings_get_ui_scaling_factor (GDSettings *settings)
{
    g_assert (settings->uiScalingFactor != 0);

    return settings->uiScalingFactor;
}

bool gd_settings_get_global_scaling_factor (GDSettings* settings, gint* globalScalingFactor)
{
    if (settings->globalScalingFactor == 0) {
        return false;
    }

    *globalScalingFactor = settings->globalScalingFactor;

    return true;
}
