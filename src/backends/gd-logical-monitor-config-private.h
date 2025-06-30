//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_LOGICAL_MONITOR_CONFIG_PRIVATE_H
#define graceful_DE2_GD_LOGICAL_MONITOR_CONFIG_PRIVATE_H
#include "gd-monitor-manager.h"
#include "gd-monitor-manager-private.h"

#include "gd-rectangle.h"

G_BEGIN_DECLS typedef struct
{
    GDRectangle layout;
    GList* monitor_configs;
    GDMonitorTransform transform;
    gfloat scale;
    bool is_primary;
    bool is_presentation;
} GDLogicalMonitorConfig;

void gd_logical_monitor_config_free(GDLogicalMonitorConfig* config);
bool gd_logical_monitor_configs_have_monitor(GList* logical_monitor_configs, GDMonitorSpec* monitor_spec);
bool gd_verify_logical_monitor_config(GDLogicalMonitorConfig* config, GDLogicalMonitorLayoutMode layout_mode, GDMonitorManager* monitor_manager, GError** error);

#endif // graceful_DE2_GD_LOGICAL_MONITOR_CONFIG_PRIVATE_H
