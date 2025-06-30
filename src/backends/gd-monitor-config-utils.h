//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_MONITOR_CONFIG_UTILS_H
#define graceful_DE2_GD_MONITOR_CONFIG_UTILS_H
#include "macros/macros.h"

#include <glib.h>

#include "gd-monitor-manager.h"
#include "gd-monitor-manager-enums-private.h"


C_BEGIN_EXTERN_C

GList *
gd_clone_logical_monitor_config_list (GList *logical_monitor_configs);

bool
gd_verify_logical_monitor_config_list (GList                       *logical_monitor_configs,
                                       GDLogicalMonitorLayoutMode   layout_mode,
                                       GDMonitorManager            *monitor_manager,
                                       GError                     **error);


C_END_EXTERN_C

#endif // graceful_DE2_GD_MONITOR_CONFIG_UTILS_H
