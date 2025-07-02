//
// Created by dingjing on 25-7-2.
//

#ifndef graceful_DE2_GD_STATUS_NOTIFIER_WATCHER_H
#define graceful_DE2_GD_STATUS_NOTIFIER_WATCHER_H
#include "macros/macros.h"

#include <glib-object.h>

C_BEGIN_EXTERN_C

#define GD_TYPE_STATUS_NOTIFIER_WATCHER gd_status_notifier_watcher_get_type ()
G_DECLARE_FINAL_TYPE (GDStatusNotifierWatcher, gd_status_notifier_watcher, GD, STATUS_NOTIFIER_WATCHER, GObject)

GDStatusNotifierWatcher *gd_status_notifier_watcher_new (void);

C_END_EXTERN_C


#endif // graceful_DE2_GD_STATUS_NOTIFIER_WATCHER_H
