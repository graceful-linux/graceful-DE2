//
// Created by dingjing on 25-7-2.
//

#ifndef graceful_DE2_ND_NOTIFICATION_H
#define graceful_DE2_ND_NOTIFICATION_H
#include "macros/macros.h"

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

C_BEGIN_EXTERN_C

#define ND_TYPE_NOTIFICATION        (nd_notification_get_type ())
#define ND_NOTIFICATION(object)     (G_TYPE_CHECK_INSTANCE_CAST ((object), ND_TYPE_NOTIFICATION, NdNotification))
#define ND_IS_NOTIFICATION(object)  (G_TYPE_CHECK_INSTANCE_TYPE ((object), ND_TYPE_NOTIFICATION))

typedef struct _NdNotification NdNotification;

typedef struct _NdNotificationClass
{
    GObjectClass parent_class;
} NdNotificationClass;

typedef enum
{
    ND_NOTIFICATION_CLOSED_EXPIRED = 1,
    ND_NOTIFICATION_CLOSED_USER = 2,
    ND_NOTIFICATION_CLOSED_API = 3,
    ND_NOTIFICATION_CLOSED_RESERVED = 4
} NdNotificationClosedReason;

GType               nd_notification_get_type            (void) G_GNUC_CONST;
NdNotification*     nd_notification_new                 (const char* sender);
bool                nd_notification_update              (NdNotification* notification, const gchar* appName, const gchar* appIcon,
                                                         const gchar* summary, const gchar* body, const gchar* const* actions,
                                                         GVariant* hints, gint timeout);
void                nd_notification_set_is_queued       (NdNotification* notification, bool isQueued);
bool                nd_notification_get_is_queued       (NdNotification* notification);
gint64              nd_notification_get_update_time     (NdNotification* notification);
guint               nd_notification_get_id              (NdNotification* notification);
int                 nd_notification_get_timeout         (NdNotification* notification);
const char*         nd_notification_get_sender          (NdNotification* notification);
const char*         nd_notification_get_app_name        (NdNotification* notification);
const char*         nd_notification_get_summary         (NdNotification* notification);
const char*         nd_notification_get_body            (NdNotification* notification);
char**              nd_notification_get_actions         (NdNotification* notification);
GIcon*              nd_notification_get_icon            (NdNotification* notification);

bool                nd_notification_get_is_resident     (NdNotification *notification);
bool                nd_notification_get_is_transient    (NdNotification *notification);
bool                nd_notification_get_action_icons    (NdNotification *notification);

void                nd_notification_close               (NdNotification *notification, NdNotificationClosedReason reason);
void                nd_notification_action_invoked      (NdNotification *notification, const char* action);
void                nd_notification_url_clicked         (NdNotification *notification, const char* url);
bool                validate_markup                     (const gchar    *markup);

C_END_EXTERN_C

#endif // graceful_DE2_ND_NOTIFICATION_H
