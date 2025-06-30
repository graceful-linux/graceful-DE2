//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_MONITOR_MANAGER_KMS_PRIVATE_H
#define graceful_DE2_GD_MONITOR_MANAGER_KMS_PRIVATE_H
#include "macros/macros.h"

#include <glib.h>

#include "gd-monitor-manager.h"
#include "gd-monitor-manager-private.h"


C_BEGIN_EXTERN_C

#define GD_TYPE_MONITOR_MANAGER_KMS (gd_monitor_manager_kms_get_type ())
G_DECLARE_FINAL_TYPE (GDMonitorManagerKms, gd_monitor_manager_kms, GD, MONITOR_MANAGER_KMS, GDMonitorManager)

C_END_EXTERN_C

#endif // graceful_DE2_GD_MONITOR_MANAGER_KMS_PRIVATE_H
