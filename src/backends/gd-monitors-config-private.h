//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_MONITORS_CONFIG_PRIVATE_H
#define graceful_DE2_GD_MONITORS_CONFIG_PRIVATE_H
#include "gd-monitor-manager-private.h"
#include "gd-monitor-manager-types-private.h"

C_BEGIN_EXTERN_C

struct _GDMonitorsConfigKey
{
    GList* monitor_specs;
    GDLogicalMonitorLayoutMode layout_mode;
};

enum _GDMonitorsConfigFlag
{
    GD_MONITORS_CONFIG_FLAG_NONE = 0,
    GD_MONITORS_CONFIG_FLAG_SYSTEM_CONFIG = (1 << 0)
};

struct _GDMonitorsConfig
{
    GObject parent;
    GDMonitorsConfig* parent_config;
    GDMonitorsConfigKey* key;
    GList* logical_monitor_configs;
    GList* disabled_monitor_specs;
    GDMonitorsConfigFlag flags;
    GDLogicalMonitorLayoutMode layout_mode;
    GDMonitorSwitchConfigType switch_config;
};

#define GD_TYPE_MONITORS_CONFIG (gd_monitors_config_get_type ())
G_DECLARE_FINAL_TYPE(GDMonitorsConfig, gd_monitors_config, GD, MONITORS_CONFIG, GObject)

GDMonitorsConfig* gd_monitors_config_new_full(GList* logical_monitor_configs, GList* disabled_monitor_specs, GDLogicalMonitorLayoutMode layout_mode, GDMonitorsConfigFlag flags);
GDMonitorsConfig* gd_monitors_config_new(GDMonitorManager* monitor_manager, GList* logical_monitor_configs, GDLogicalMonitorLayoutMode layout_mode, GDMonitorsConfigFlag flags);
void gd_monitors_config_set_parent_config(GDMonitorsConfig* config, GDMonitorsConfig* parent_config);
GDMonitorSwitchConfigType gd_monitors_config_get_switch_config(GDMonitorsConfig* config);
void gd_monitors_config_set_switch_config(GDMonitorsConfig* config, GDMonitorSwitchConfigType switch_config);
guint gd_monitors_config_key_hash(gconstpointer data);
gboolean gd_monitors_config_key_equal(gconstpointer data_a, gconstpointer data_b);
void gd_monitors_config_key_free(GDMonitorsConfigKey* config_key);
gboolean gd_verify_monitors_config(GDMonitorsConfig* config, GDMonitorManager* monitor_manager, GError** error);

C_END_EXTERN_C


#endif // graceful_DE2_GD_MONITORS_CONFIG_PRIVATE_H
