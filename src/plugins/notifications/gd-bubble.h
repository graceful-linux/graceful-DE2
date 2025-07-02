//
// Created by dingjing on 25-7-2.
//

#ifndef graceful_DE2_GD_BUBBLE_H
#define graceful_DE2_GD_BUBBLE_H
#include "macros/macros.h"

#include <gtk/gtk.h>

#include "nd-notification.h"
#include "../common/gd-popup-window.h"

C_BEGIN_EXTERN_C

#define GD_TYPE_BUBBLE gd_bubble_get_type ()
G_DECLARE_DERIVABLE_TYPE (GDBubble, gd_bubble, GD, BUBBLE, GDPopupWindow)

struct _GDBubbleClass
{
    GDPopupWindowClass parentClass;

    void (* changed) (GDBubble *bubble);
};

GDBubble       *gd_bubble_new_for_notification (NdNotification *notification);
NdNotification *gd_bubble_get_notification     (GDBubble       *bubble);

C_END_EXTERN_C

#endif // graceful_DE2_GD_BUBBLE_H
