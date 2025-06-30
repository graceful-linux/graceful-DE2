//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GF_MONITOR_CONFIG_MANAGER_PRIVATE_H
#define graceful_DE2_GF_MONITOR_CONFIG_MANAGER_PRIVATE_H
#include "macros/macros.h"

#include <glib.h>

#include "gd-monitor-manager.h"
#include "gd-monitor-config-store-private.h"
#include "gd-monitor-manager-types-private.h"


C_BEGIN_EXTERN_C

#define GD_TYPE_MONITOR_CONFIG_MANAGER (gd_monitor_config_manager_get_type ())

G_DECLARE_FINAL_TYPE (GDMonitorConfigManager, gd_monitor_config_manager, GD, MONITOR_CONFIG_MANAGER, GObject)

GDMonitorConfigManager *gd_monitor_config_manager_new                       (GDMonitorManager            *monitor_manager);

GDMonitorConfigStore   *gd_monitor_config_manager_get_store                 (GDMonitorConfigManager      *config_manager);

gboolean                gd_monitor_config_manager_assign                    (GDMonitorManager            *manager,
                                                                             GDMonitorsConfig            *config,
                                                                             GPtrArray                  **crtc_assignments,
                                                                             GPtrArray                  **output_assignments,
                                                                             GError                     **error);

GDMonitorsConfig       *gd_monitor_config_manager_get_stored                (GDMonitorConfigManager      *config_manager);

GDMonitorsConfig       *gd_monitor_config_manager_create_linear             (GDMonitorConfigManager      *config_manager);

GDMonitorsConfig       *gd_monitor_config_manager_create_fallback           (GDMonitorConfigManager      *config_manager);

GDMonitorsConfig       *gd_monitor_config_manager_create_suggested          (GDMonitorConfigManager      *config_manager);

GDMonitorsConfig       *gd_monitor_config_manager_create_for_orientation    (GDMonitorConfigManager      *config_manager,
                                                                             GDMonitorsConfig            *base_config,
                                                                             GDMonitorTransform           transform);

GDMonitorsConfig       *gd_monitor_config_manager_create_for_builtin_orientation (GDMonitorConfigManager *config_manager,
                                                                                  GDMonitorsConfig       *base_config);

GDMonitorsConfig       *gd_monitor_config_manager_create_for_rotate_monitor (GDMonitorConfigManager      *config_manager);

GDMonitorsConfig       *gd_monitor_config_manager_create_for_switch_config  (GDMonitorConfigManager      *config_manager,
                                                                             GDMonitorSwitchConfigType    config_type);

void                    gd_monitor_config_manager_set_current               (GDMonitorConfigManager      *config_manager,
                                                                             GDMonitorsConfig            *config);

GDMonitorsConfig       *gd_monitor_config_manager_get_current               (GDMonitorConfigManager      *config_manager);

GDMonitorsConfig       *gd_monitor_config_manager_pop_previous              (GDMonitorConfigManager      *config_manager);

GDMonitorsConfig       *gd_monitor_config_manager_get_previous              (GDMonitorConfigManager      *config_manager);

void                    gd_monitor_config_manager_clear_history             (GDMonitorConfigManager      *config_manager);

void                    gd_monitor_config_manager_save_current              (GDMonitorConfigManager      *config_manager);

GDMonitorsConfigKey    *gd_create_monitors_config_key_for_current_state     (GDMonitorManager            *monitor_manager);


C_END_EXTERN_C

#endif // graceful_DE2_GF_MONITOR_CONFIG_MANAGER_PRIVATE_H
