//
// Created by dingjing on 25-7-2.
//
#include "gd-sn-watcher-v0.h"
#include "gd-status-notifier-watcher.h"

struct _GDStatusNotifierWatcher
{
    GObject             parent;
    GDSnWatcherV0*      v0;
};

G_DEFINE_TYPE(GDStatusNotifierWatcher, gd_status_notifier_watcher, G_TYPE_OBJECT)

static void gd_status_notifier_watcher_dispose(GObject* object)
{
    GDStatusNotifierWatcher * watcher = GD_STATUS_NOTIFIER_WATCHER(object);

    g_clear_object(&watcher->v0);

    G_OBJECT_CLASS(gd_status_notifier_watcher_parent_class)->dispose(object);
}

static void gd_status_notifier_watcher_class_init(GDStatusNotifierWatcherClass* watcherClass)
{
    GObjectClass* oc = G_OBJECT_CLASS(watcherClass);
    oc->dispose = gd_status_notifier_watcher_dispose;
}

static void gd_status_notifier_watcher_init(GDStatusNotifierWatcher* watcher)
{
    watcher->v0 = gd_sn_watcher_v0_new();
}

GDStatusNotifierWatcher* gd_status_notifier_watcher_new(void)
{
    return g_object_new(GD_TYPE_STATUS_NOTIFIER_WATCHER, NULL);
}
