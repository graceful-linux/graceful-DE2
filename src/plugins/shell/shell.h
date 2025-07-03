//
// Created by dingjing on 25-7-3.
//

#ifndef graceful_DE2_SHELL_H
#define graceful_DE2_SHELL_H
#include "macros/macros.h"
#include <glib-object.h>
#include "backends/gd-monitor-manager.h"

C_BEGIN_EXTERN_C

#define FLASHBACK_TYPE_SHELL flashback_shell_get_type ()
G_DECLARE_FINAL_TYPE (FlashbackShell, flashback_shell, FLASHBACK, SHELL, GObject)

FlashbackShell* flashback_shell_new                 (void);
void            flashback_shell_set_monitor_manager (FlashbackShell* shell, GDMonitorManager* mm);


C_END_EXTERN_C

#endif // graceful_DE2_SHELL_H
