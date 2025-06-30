//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_POPUP_WINDOW_H
#define graceful_DE2_GD_POPUP_WINDOW_H
#include "macros/macros.h"

#include <gtk/gtk.h>

C_BEGIN_EXTERN_C

#define GD_TYPE_POPUP_WINDOW gd_popup_window_get_type ()
G_DECLARE_DERIVABLE_TYPE (GDPopupWindow, gd_popup_window, GD, POPUP_WINDOW, GtkWindow)

struct _GDPopupWindowClass
{
    GtkWindowClass parent_class;
};

void gd_popup_window_fade_start  (GDPopupWindow *window);
void gd_popup_window_fade_cancel (GDPopupWindow *window);

C_END_EXTERN_C

#endif // graceful_DE2_GD_POPUP_WINDOW_H
