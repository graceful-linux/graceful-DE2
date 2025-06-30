//
// Created by dingjing on 25-6-30.
//

#include <gio/gio.h>

#include "gd-rectangle-private.h"
#include "gd-monitor-config-utils.h"
#include "gd-monitor-spec-private.h"
#include "gd-monitor-config-private.h"
#include "gd-monitors-config-private.h"
#include "gd-logical-monitor-config-private.h"
#include "gd-monitor-manager-types-private.h"


G_DEFINE_TYPE(GDMonitorsConfig, gd_monitors_config, G_TYPE_OBJECT)

static bool
gd_monitors_config_is_monitor_enabled(GDMonitorsConfig* config, GDMonitorSpec* monitor_spec)
{
    return gd_logical_monitor_configs_have_monitor(config->logical_monitor_configs, monitor_spec);
}

static GDMonitorsConfigKey*
gd_monitors_config_key_new(GList* logical_monitor_configs, GList* disabled_monitor_specs, GDLogicalMonitorLayoutMode layout_mode)
{
    GDMonitorsConfigKey* config_key;
    GList* monitor_specs;
    GList* l;

    monitor_specs = NULL;
    for (l = logical_monitor_configs; l; l = l->next) {
        GDLogicalMonitorConfig* logical_monitor_config = l->data;
        GList* k;

        for (k = logical_monitor_config->monitor_configs; k; k = k->next) {
            GDMonitorConfig* monitor_config = k->data;
            GDMonitorSpec* monitor_spec;

            monitor_spec = gd_monitor_spec_clone(monitor_config->monitor_spec);
            monitor_specs = g_list_prepend(monitor_specs, monitor_spec);
        }
    }

    for (l = disabled_monitor_specs; l; l = l->next) {
        GDMonitorSpec* monitor_spec = l->data;

        monitor_spec = gd_monitor_spec_clone(monitor_spec);
        monitor_specs = g_list_prepend(monitor_specs, monitor_spec);
    }

    monitor_specs = g_list_sort(monitor_specs, (GCompareFunc)gd_monitor_spec_compare);

    config_key = g_new0(GDMonitorsConfigKey, 1);
    config_key->monitor_specs = monitor_specs;
    config_key->layout_mode = layout_mode;

    return config_key;
}

static void
gd_monitors_config_finalize(GObject* object)
{
    GDMonitorsConfig* config;

    config = GD_MONITORS_CONFIG(object);

    g_clear_object(&config->parent_config);

    gd_monitors_config_key_free(config->key);
    g_list_free_full(config->logical_monitor_configs, (GDestroyNotify)gd_logical_monitor_config_free);
    g_list_free_full(config->disabled_monitor_specs, (GDestroyNotify)gd_monitor_spec_free);

    G_OBJECT_CLASS(gd_monitors_config_parent_class)->finalize(object);
}

static void
gd_monitors_config_class_init(GDMonitorsConfigClass* config_class)
{
    GObjectClass* object_class;

    object_class = G_OBJECT_CLASS(config_class);

    object_class->finalize = gd_monitors_config_finalize;
}

static void
gd_monitors_config_init(GDMonitorsConfig* config)
{
}

GDMonitorsConfig*
gd_monitors_config_new_full(GList* logical_monitor_configs, GList* disabled_monitor_specs, GDLogicalMonitorLayoutMode layout_mode, GDMonitorsConfigFlag flags)
{
    GDMonitorsConfig* config;

    config = g_object_new(GD_TYPE_MONITORS_CONFIG, NULL);
    config->logical_monitor_configs = logical_monitor_configs;
    config->disabled_monitor_specs = disabled_monitor_specs;
    config->layout_mode = layout_mode;
    config->key = gd_monitors_config_key_new(logical_monitor_configs, disabled_monitor_specs, layout_mode);
    config->flags = flags;
    config->switch_config = GD_MONITOR_SWITCH_CONFIG_UNKNOWN;

    return config;
}

GDMonitorsConfig*
gd_monitors_config_new(GDMonitorManager* monitor_manager, GList* logical_monitor_configs, GDLogicalMonitorLayoutMode layout_mode, GDMonitorsConfigFlag flags)
{
    GList* disabled_monitor_specs = NULL;
    GList* monitors;
    GList* l;

    monitors = gd_monitor_manager_get_monitors(monitor_manager);
    for (l = monitors; l; l = l->next) {
        GDMonitor* monitor = l->data;
        GDMonitorSpec* monitor_spec;

        if (!gd_monitor_manager_is_monitor_visible(monitor_manager, monitor)) continue;

        monitor_spec = gd_monitor_get_spec(monitor);
        if (gd_logical_monitor_configs_have_monitor(logical_monitor_configs, monitor_spec)) continue;

        disabled_monitor_specs = g_list_prepend(disabled_monitor_specs, gd_monitor_spec_clone(monitor_spec));
    }

    return gd_monitors_config_new_full(logical_monitor_configs, disabled_monitor_specs, layout_mode, flags);
}

void
gd_monitors_config_set_parent_config(GDMonitorsConfig* config, GDMonitorsConfig* parent_config)
{
    g_assert(config != parent_config);
    g_assert(parent_config == NULL || parent_config->parent_config != config);

    g_set_object(&config->parent_config, parent_config);
}

GDMonitorSwitchConfigType
gd_monitors_config_get_switch_config(GDMonitorsConfig* config)
{
    return config->switch_config;
}

void
gd_monitors_config_set_switch_config(GDMonitorsConfig* config, GDMonitorSwitchConfigType switch_config)
{
    config->switch_config = switch_config;
}

guint
gd_monitors_config_key_hash(gconstpointer data)
{
    const GDMonitorsConfigKey* config_key;
    glong hash;
    GList* l;

    config_key = data;
    hash = config_key->layout_mode;

    for (l = config_key->monitor_specs; l; l = l->next) {
        GDMonitorSpec* monitor_spec = l->data;

        hash ^= (g_str_hash(monitor_spec->connector) ^ g_str_hash(monitor_spec->vendor) ^ g_str_hash(monitor_spec->product) ^ g_str_hash(monitor_spec->serial));
    }

    return hash;
}

gboolean
gd_monitors_config_key_equal(gconstpointer data_a, gconstpointer data_b)
{
    const GDMonitorsConfigKey* config_key_a;
    const GDMonitorsConfigKey* config_key_b;
    GList * l_a, * l_b;

    config_key_a = data_a;
    config_key_b = data_b;

    if (config_key_a->layout_mode != config_key_b->layout_mode) return FALSE;

    for (l_a = config_key_a->monitor_specs, l_b = config_key_b->monitor_specs; l_a && l_b; l_a = l_a->next, l_b = l_b->next) {
        GDMonitorSpec* monitor_spec_a = l_a->data;
        GDMonitorSpec* monitor_spec_b = l_b->data;

        if (!gd_monitor_spec_equals(monitor_spec_a, monitor_spec_b)) return FALSE;
    }

    if (l_a || l_b) return FALSE;

    return TRUE;
}

void
gd_monitors_config_key_free(GDMonitorsConfigKey* config_key)
{
    g_list_free_full(config_key->monitor_specs, (GDestroyNotify)gd_monitor_spec_free);
    g_free(config_key);
}

gboolean
gd_verify_monitors_config(GDMonitorsConfig* config, GDMonitorManager* monitor_manager, GError** error)
{
    GList* l;

    if (!gd_verify_logical_monitor_config_list(config->logical_monitor_configs, config->layout_mode, monitor_manager, error)) return FALSE;

    for (l = config->disabled_monitor_specs; l; l = l->next) {
        GDMonitorSpec* monitor_spec = l->data;

        if (gd_monitors_config_is_monitor_enabled(config, monitor_spec)) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Assigned monitor explicitly disabled");
            return FALSE;
        }
    }

    return TRUE;
}
