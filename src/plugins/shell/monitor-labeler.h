//
// Created by dingjing on 25-7-3.
//

#ifndef graceful_DE2_MONITOR_LABELER_H
#define graceful_DE2_MONITOR_LABELER_H
#include "macros/macros.h"

#include <glib-object.h>

#include "backends/gd-monitor-manager.h"


C_BEGIN_EXTERN_C

#define FLASHBACK_TYPE_MONITOR_LABELER flashback_monitor_labeler_get_type ()
G_DECLARE_FINAL_TYPE (FlashbackMonitorLabeler, flashback_monitor_labeler, FLASHBACK, MONITOR_LABELER, GObject)

FlashbackMonitorLabeler*    flashback_monitor_labeler_new  (void);
void                        flashback_monitor_labeler_show (FlashbackMonitorLabeler* labeler, GDMonitorManager* mm, const gchar* sender, GVariant* params);
void                        flashback_monitor_labeler_hide (FlashbackMonitorLabeler* labeler, const gchar* sender);

C_END_EXTERN_C

#endif // graceful_DE2_MONITOR_LABELER_H
