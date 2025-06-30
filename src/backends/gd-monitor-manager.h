//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_MONITOR_MANAGER_H
#define graceful_DE2_GD_MONITOR_MANAGER_H
#include <glib-object.h>

#include "macros/macros.h"


C_BEGIN_EXTERN_C

typedef enum
{
    GD_MONITOR_SWITCH_CONFIG_ALL_MIRROR,
    GD_MONITOR_SWITCH_CONFIG_ALL_LINEAR,
    GD_MONITOR_SWITCH_CONFIG_EXTERNAL,
    GD_MONITOR_SWITCH_CONFIG_BUILTIN,
    GD_MONITOR_SWITCH_CONFIG_UNKNOWN,
} GDMonitorSwitchConfigType;

typedef struct _GDMonitorManager GDMonitorManager;

gint                      gd_monitor_manager_get_monitor_for_connector         (GDMonitorManager* manager, const gchar* connector);
bool                      gd_monitor_manager_get_is_builtin_display_on         (GDMonitorManager* manager);
GDMonitorSwitchConfigType gd_monitor_manager_get_switch_config                 (GDMonitorManager* manager);
bool                      gd_monitor_manager_can_switch_config                 (GDMonitorManager* manager);
void                      gd_monitor_manager_switch_config                     (GDMonitorManager* manager, GDMonitorSwitchConfigType configType);
gint                      gd_monitor_manager_get_display_configuration_timeout (void);
bool                      gd_monitor_manager_get_panel_orientation_managed     (GDMonitorManager* self);
void                      gd_monitor_manager_confirm_configuration             (GDMonitorManager* manager, bool ok);


C_END_EXTERN_C

#endif // graceful_DE2_GD_MONITOR_MANAGER_H
