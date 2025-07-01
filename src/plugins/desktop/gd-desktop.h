//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_DESKTOP_H
#define graceful_DE2_GD_DESKTOP_H

#include "backends/gd-monitor-manager.h"

C_BEGIN_EXTERN_C

#define GD_TYPE_DESKTOP (gd_desktop_get_type ())
G_DECLARE_FINAL_TYPE (GDDesktop, gd_desktop, GD, DESKTOP, GObject)

GDDesktop *gd_desktop_new                 (void);

void       gd_desktop_set_monitor_manager (GDDesktop* self, GDMonitorManager* mm);

C_END_EXTERN_C

#endif // graceful_DE2_GD_DESKTOP_H
