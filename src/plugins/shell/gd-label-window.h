//
// Created by dingjing on 25-7-3.
//

#ifndef graceful_DE2_GD_LABEL_WINDOW_H
#define graceful_DE2_GD_LABEL_WINDOW_H
#include "macros/macros.h"

#include "../common/gd-popup-window.h"


C_BEGIN_EXTERN_C

#define GD_TYPE_LABEL_WINDOW gd_label_window_get_type ()
G_DECLARE_FINAL_TYPE (GDLabelWindow, gd_label_window, GD, LABEL_WINDOW, GDPopupWindow)

GDLabelWindow *gd_label_window_new  (gint monitor, const gchar* label);
void           gd_label_window_show (GDLabelWindow *window);
void           gd_label_window_hide (GDLabelWindow *window);

C_END_EXTERN_C

#endif // graceful_DE2_GD_LABEL_WINDOW_H
