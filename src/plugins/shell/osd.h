//
// Created by dingjing on 25-7-3.
//

#ifndef graceful_DE2_OSD_H
#define graceful_DE2_OSD_H
#include "macros/macros.h"

#include "backends/gd-monitor-manager.h"

C_BEGIN_EXTERN_C

#define FLASHBACK_TYPE_OSD flashback_osd_get_type ()
G_DECLARE_FINAL_TYPE (FlashbackOsd, flashback_osd, FLASHBACK, OSD, GObject)

FlashbackOsd* flashback_osd_new  (void);
void          flashback_osd_show (FlashbackOsd* osd, GDMonitorManager* mm, GVariant* params);


C_END_EXTERN_C

#endif // graceful_DE2_OSD_H
