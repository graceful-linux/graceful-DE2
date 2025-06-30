//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_MONITOR_CONFIG_STORE_PRIVATE_H
#define graceful_DE2_GD_MONITOR_CONFIG_STORE_PRIVATE_H
#include "macros/macros.h"

#include "gd-monitor-manager.h"
#include "gd-output-info-private.h"
#include "gd-monitor-config-private.h"
#include "gd-monitors-config-private.h"

C_BEGIN_EXTERN_C

typedef struct
{
    bool enable_dbus;
} GDMonitorConfigPolicy;

#define GD_TYPE_MONITOR_CONFIG_STORE (gd_monitor_config_store_get_type ())
G_DECLARE_FINAL_TYPE(GDMonitorConfigStore, gd_monitor_config_store, GD, MONITOR_CONFIG_STORE, GObject)

GDMonitorConfigStore* gd_monitor_config_store_new(GDMonitorManager* monitor_manager);

GDMonitorsConfig* gd_monitor_config_store_lookup(GDMonitorConfigStore* config_store, GDMonitorsConfigKey* key);

void gd_monitor_config_store_add(GDMonitorConfigStore* config_store, GDMonitorsConfig* config);

void gd_monitor_config_store_remove(GDMonitorConfigStore* config_store, GDMonitorsConfig* config);

bool gd_monitor_config_store_set_custom(GDMonitorConfigStore* config_store, const gchar* read_path, const gchar* write_path, GDMonitorsConfigFlag flags, GError** error);

gint gd_monitor_config_store_get_config_count(GDMonitorConfigStore* config_store);

GDMonitorManager* gd_monitor_config_store_get_monitor_manager(GDMonitorConfigStore* config_store);

void gd_monitor_config_store_reset(GDMonitorConfigStore* config_store);

const GDMonitorConfigPolicy* gd_monitor_config_store_get_policy(GDMonitorConfigStore* config_store);


C_END_EXTERN_C

#endif // graceful_DE2_GD_MONITOR_CONFIG_STORE_PRIVATE_H
