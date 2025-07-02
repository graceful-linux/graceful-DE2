//
// Created by dingjing on 25-7-2.
//
#include "gd-notifications.h"

#include "nd-daemon.h"

struct _GDNotifications
{
    GObject parent;

    NdDaemon* daemon;
};

G_DEFINE_TYPE(GDNotifications, gd_notifications, G_TYPE_OBJECT)

static void gd_notifications_dispose(GObject* object)
{
    GDNotifications* notifications = GD_NOTIFICATIONS(object);

    g_clear_object(&notifications->daemon);

    G_OBJECT_CLASS(gd_notifications_parent_class)->dispose(object);
}

static void gd_notifications_class_init(GDNotificationsClass* klass)
{
    GObjectClass* oc = G_OBJECT_CLASS(klass);
    oc->dispose = gd_notifications_dispose;
}

static void gd_notifications_init(GDNotifications* obj)
{
    obj->daemon = nd_daemon_new();
}

GDNotifications* gd_notifications_new(void)
{
    g_info("notification");
    return g_object_new(GD_TYPE_NOTIFICATIONS, NULL);
}
