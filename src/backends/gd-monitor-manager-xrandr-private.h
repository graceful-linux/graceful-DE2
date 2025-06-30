//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_MONITOR_MANAGER_XRANDR_PRIVATE_H
#define graceful_DE2_GD_MONITOR_MANAGER_XRANDR_PRIVATE_H

#include "macros/macros.h"

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include "gd-monitor-manager-private.h"

C_BEGIN_EXTERN_C


#define GD_TYPE_MONITOR_MANAGER_XRANDR (gd_monitor_manager_xrandr_get_type ())

G_DECLARE_FINAL_TYPE (GDMonitorManagerXrandr, gd_monitor_manager_xrandr, GD, MONITOR_MANAGER_XRANDR, GDMonitorManager)

Display  *gd_monitor_manager_xrandr_get_xdisplay  (GDMonitorManagerXrandr *xrandr);

bool  gd_monitor_manager_xrandr_has_randr15   (GDMonitorManagerXrandr *xrandr);

bool  gd_monitor_manager_xrandr_handle_xevent (GDMonitorManagerXrandr *xrandr, XEvent* event);

C_END_EXTERN_C

#endif // graceful_DE2_GD_MONITOR_MANAGER_XRANDR_PRIVATE_H
