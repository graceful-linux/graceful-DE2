//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_MONITOR_MANAGER_ENUMS_PRIVATE_H
#define graceful_DE2_GD_MONITOR_MANAGER_ENUMS_PRIVATE_H
#include "macros/macros.h"

C_BEGIN_EXTERN_C

typedef enum
{
    GD_MONITOR_MANAGER_CAPABILITY_NONE = 0,
    GD_MONITOR_MANAGER_CAPABILITY_LAYOUT_MODE = (1 << 0),
    GD_MONITOR_MANAGER_CAPABILITY_GLOBAL_SCALE_REQUIRED = (1 << 1)
} GDMonitorManagerCapability;

/* Equivalent to the 'method' enum in org.gnome.Mutter.DisplayConfig */
typedef enum
{
    GD_MONITORS_CONFIG_METHOD_VERIFY = 0,
    GD_MONITORS_CONFIG_METHOD_TEMPORARY = 1,
    GD_MONITORS_CONFIG_METHOD_PERSISTENT = 2
} GDMonitorsConfigMethod;

/* Equivalent to the 'layout-mode' enum in org.gnome.Mutter.DisplayConfig */
typedef enum
{
    GD_LOGICAL_MONITOR_LAYOUT_MODE_LOGICAL = 1,
    GD_LOGICAL_MONITOR_LAYOUT_MODE_PHYSICAL = 2
} GDLogicalMonitorLayoutMode;

/* This matches the values in drm_mode.h */
typedef enum
{
    GD_CONNECTOR_TYPE_Unknown = 0,
    GD_CONNECTOR_TYPE_VGA = 1,
    GD_CONNECTOR_TYPE_DVII = 2,
    GD_CONNECTOR_TYPE_DVID = 3,
    GD_CONNECTOR_TYPE_DVIA = 4,
    GD_CONNECTOR_TYPE_Composite = 5,
    GD_CONNECTOR_TYPE_SVIDEO = 6,
    GD_CONNECTOR_TYPE_LVDS = 7,
    GD_CONNECTOR_TYPE_Component = 8,
    GD_CONNECTOR_TYPE_9PinDIN = 9,
    GD_CONNECTOR_TYPE_DisplayPort = 10,
    GD_CONNECTOR_TYPE_HDMIA = 11,
    GD_CONNECTOR_TYPE_HDMIB = 12,
    GD_CONNECTOR_TYPE_TV = 13,
    GD_CONNECTOR_TYPE_eDP = 14,
    GD_CONNECTOR_TYPE_VIRTUAL = 15,
    GD_CONNECTOR_TYPE_DSI = 16,
    GD_CONNECTOR_TYPE_DPI = 17,
    GD_CONNECTOR_TYPE_WRITEBACK = 18,
    GD_CONNECTOR_TYPE_SPI = 19,
    GD_CONNECTOR_TYPE_USB = 20
} GDConnectorType;


C_END_EXTERN_C


#endif // graceful_DE2_GD_MONITOR_MANAGER_ENUMS_PRIVATE_H
