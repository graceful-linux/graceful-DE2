//
// Created by dingjing on 25-7-2.
//

#ifndef graceful_DE2_ND_NOTIFICATION_BOX_H
#define graceful_DE2_ND_NOTIFICATION_BOX_H
#include "macros/macros.h"

#include <gtk/gtk.h>
#include "nd-notification.h"

C_BEGIN_EXTERN_C

#define ND_TYPE_NOTIFICATION_BOX         (nd_notification_box_get_type ())
#define ND_NOTIFICATION_BOX(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ND_TYPE_NOTIFICATION_BOX, NdNotificationBox))
#define ND_NOTIFICATION_BOX_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), ND_TYPE_NOTIFICATION_BOX, NdNotificationBoxClass))
#define ND_IS_NOTIFICATION_BOX(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ND_TYPE_NOTIFICATION_BOX))
#define ND_IS_NOTIFICATION_BOX_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ND_TYPE_NOTIFICATION_BOX))
#define ND_NOTIFICATION_BOX_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ND_TYPE_NOTIFICATION_BOX, NdNotificationBoxClass))

typedef struct NdNotificationBoxPrivate NdNotificationBoxPrivate;

typedef struct
{
    GtkEventBox                 parent;
    NdNotificationBoxPrivate*   priv;
} NdNotificationBox;

typedef struct
{
    GtkEventBoxClass   parent_class;
} NdNotificationBoxClass;

GType               nd_notification_box_get_type             (void);
NdNotificationBox*  nd_notification_box_new_for_notification (NdNotification    *notification);
NdNotification*     nd_notification_box_get_notification     (NdNotificationBox *notification_box);

C_END_EXTERN_C

#endif // graceful_DE2_ND_NOTIFICATION_BOX_H
