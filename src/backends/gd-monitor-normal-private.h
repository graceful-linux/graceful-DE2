//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_MONITOR_NORMAL_PRIVATE_H
#define graceful_DE2_GD_MONITOR_NORMAL_PRIVATE_H
#include "macros/macros.h"

#include <glib.h>

#include "gd-monitor-manager.h"
#include "gd-monitor-private.h"
#include "gd-monitor-manager-types-private.h"

C_BEGIN_EXTERN_C


#define GD_TYPE_MONITOR_NORMAL (gd_monitor_normal_get_type ())
G_DECLARE_FINAL_TYPE (GDMonitorNormal, gd_monitor_normal, GD, MONITOR_NORMAL, GDMonitor);

GDMonitorNormal* gd_monitor_normal_new (GDMonitorManager *monitor_manager, GDOutput* output);

C_END_EXTERN_C

#endif // graceful_DE2_GD_MONITOR_NORMAL_PRIVATE_H
