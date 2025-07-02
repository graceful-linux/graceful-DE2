//
// Created by dingjing on 25-7-2.
//

#ifndef graceful_DE2_GD_NOTIFICATIONS_H
#define graceful_DE2_GD_NOTIFICATIONS_H
#include "macros/macros.h"

#include <glib-object.h>

C_BEGIN_EXTERN_C

#define GD_TYPE_NOTIFICATIONS gd_notifications_get_type ()
G_DECLARE_FINAL_TYPE (GDNotifications, gd_notifications, GD, NOTIFICATIONS, GObject)

GDNotifications* gd_notifications_new (void);

C_END_EXTERN_C

#endif // graceful_DE2_GD_NOTIFICATIONS_H
