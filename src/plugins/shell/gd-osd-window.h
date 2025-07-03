//
// Created by dingjing on 25-7-3.
//

#ifndef graceful_DE2_GD_OSD_WINDOW_H
#define graceful_DE2_GD_OSD_WINDOW_H
#include "macros/macros.h"
#include "../common/gd-popup-window.h"

C_BEGIN_EXTERN_C

#define GD_TYPE_OSD_WINDOW gd_osd_window_get_type ()
G_DECLARE_FINAL_TYPE (GDOsdWindow, gd_osd_window, GD, OSD_WINDOW, GDPopupWindow)

GDOsdWindow *gd_osd_window_new           (gint monitor);
void         gd_osd_window_set_icon      (GDOsdWindow* window, GIcon* icon);
void         gd_osd_window_set_label     (GDOsdWindow* window, const gchar* label);
void         gd_osd_window_set_level     (GDOsdWindow* window, gdouble level);
void         gd_osd_window_set_max_level (GDOsdWindow* window, gdouble maxLevel);
void         gd_osd_window_show          (GDOsdWindow* window);
void         gd_osd_window_hide          (GDOsdWindow* window);


C_END_EXTERN_C


#endif // graceful_DE2_GD_OSD_WINDOW_H
