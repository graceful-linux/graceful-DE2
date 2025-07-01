//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_WORKAREA_WATCHER_H
#define graceful_DE2_GD_WORKAREA_WATCHER_H
#include <glib-object.h>

#include "macros/macros.h"

C_BEGIN_EXTERN_C

#define GD_TYPE_WORKAREA_WATCHER (gd_workarea_watcher_get_type ())
G_DECLARE_FINAL_TYPE (GDWorkareaWatcher, gd_workarea_watcher, GD, WORKAREA_WATCHER, GObject)

GDWorkareaWatcher *gd_workarea_watcher_new (void);

C_END_EXTERN_C

#endif // graceful_DE2_GD_WORKAREA_WATCHER_H
