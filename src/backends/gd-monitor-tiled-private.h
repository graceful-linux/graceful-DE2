//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_MONITOR_TILED_PRIVATE_H
#define graceful_DE2_GD_MONITOR_TILED_PRIVATE_H
#include "macros/macros.h"

#include <glib.h>

#include "gd-monitor-manager.h"
#include "gd-monitor-manager-private.h"
#include "gd-monitor-manager-types-private.h"


C_BEGIN_EXTERN_C


#define GD_TYPE_MONITOR_TILED (gd_monitor_tiled_get_type ())
G_DECLARE_FINAL_TYPE (GDMonitorTiled, gd_monitor_tiled, GD, MONITOR_TILED, GDMonitor)

GDMonitorTiled* gd_monitor_tiled_new               (GDMonitorManager *monitor_manager, GDOutput* output);
uint32_t        gd_monitor_tiled_get_tile_group_id (GDMonitorTiled   *monitor_tiled);


C_END_EXTERN_C

#endif // graceful_DE2_GD_MONITOR_TILED_PRIVATE_H
