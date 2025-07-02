//
// Created by dingjing on 25-7-2.
//

#ifndef graceful_DE2_GD_SN_WATCHER_V0_H
#define graceful_DE2_GD_SN_WATCHER_V0_H
#include "macros/macros.h"
#include "gd-sn-watcher-vo-gen.dbus.h"

C_BEGIN_EXTERN_C

#define GD_TYPE_SN_WATCHER_V0 gd_sn_watcher_v0_get_type ()
G_DECLARE_FINAL_TYPE (GDSnWatcherV0, gd_sn_watcher_v0, GD, SN_WATCHER_V0, GDSnWatcherV0GenSkeleton)

GDSnWatcherV0*  gd_sn_watcher_v0_new (void);

C_END_EXTERN_C

#endif // graceful_DE2_GD_SN_WATCHER_V0_H
