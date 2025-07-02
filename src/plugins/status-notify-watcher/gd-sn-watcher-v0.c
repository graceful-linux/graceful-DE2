//
// Created by dingjing on 25-7-2.
//
#include "gd-sn-watcher-v0.h"

struct _GDSnWatcherV0
{
    GDSnWatcherV0GenSkeleton    parent;

    guint                       busNameId;
    GSList*                     hosts;
    GSList*                     items;
};

typedef enum
{
    GD_WATCH_TYPE_HOST,
    GD_WATCH_TYPE_ITEM
} GDWatchType;

typedef struct
{
    GDSnWatcherV0*      v0;
    GDWatchType         type;

    gchar*              service;
    gchar*              busName;
    gchar*              objectPath;
    guint               watchId;
} GDWatch;

static void gd_sn_watcher_v0_gen_init(GDSnWatcherV0GenIface* iface);

G_DEFINE_TYPE_WITH_CODE(GDSnWatcherV0, gd_sn_watcher_v0, GD_TYPE_SN_WATCHER_V0_GEN_SKELETON, G_IMPLEMENT_INTERFACE(GD_TYPE_SN_WATCHER_V0_GEN, gd_sn_watcher_v0_gen_init))

static void update_registered_items(GDSnWatcherV0* v0)
{
    GVariantBuilder builder;

    g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));

    GSList* l = NULL;
    for (l = v0->items; l != NULL; l = g_slist_next(l)) {
        GDWatch* watch = (GDWatch*)l->data;
        gchar* item = g_strdup_printf("%s%s", watch->busName, watch->objectPath);
        g_variant_builder_add(&builder, "s", item);
        g_free(item);
    }

    GVariant* variant = g_variant_builder_end(&builder);
    const gchar** items = g_variant_get_strv(variant, NULL);

    gd_sn_watcher_v0_gen_set_registered_items(GD_SN_WATCHER_V0_GEN(v0), items);
    g_variant_unref(variant);
    g_free(items);
}

static void gd_watch_free(gpointer data)
{
    GDWatch* watch = (GDWatch*)data;

    if (watch->watchId > 0) {
        g_bus_unwatch_name(watch->watchId);
    }

    g_free(watch->service);
    g_free(watch->busName);
    g_free(watch->objectPath);

    g_free(watch);
}

static void name_vanished_cb(GDBusConnection* connection, const char* name, gpointer uData)
{
    GDWatch* watch = (GDWatch*) uData;
    GDSnWatcherV0* v0 = watch->v0;
    GDSnWatcherV0Gen* gen = GD_SN_WATCHER_V0_GEN(v0);

    if (watch->type == GD_WATCH_TYPE_HOST) {
        v0->hosts = g_slist_remove(v0->hosts, watch);
        if (v0->hosts == NULL) {
            gd_sn_watcher_v0_gen_set_is_host_registered(gen, FALSE);
            gd_sn_watcher_v0_gen_emit_host_registered(gen);
        }
    }
    else if (watch->type == GD_WATCH_TYPE_ITEM) {
        v0->items = g_slist_remove(v0->items, watch);
        update_registered_items(v0);
        gchar* tmp = g_strdup_printf("%s%s", watch->busName, watch->objectPath);
        gd_sn_watcher_v0_gen_emit_item_unregistered(gen, tmp);
        g_free(tmp);
    }
    else {
        g_assert_not_reached();
    }

    gd_watch_free(watch);
}

static GDWatch* gd_watch_new(GDSnWatcherV0* v0, GDWatchType type, const gchar* service, const gchar* busName, const gchar* objectPath)
{
    GDWatch* watch = g_new0(GDWatch, 1);

    watch->v0 = v0;
    watch->type = type;

    watch->service = g_strdup(service);
    watch->busName = g_strdup(busName);
    watch->objectPath = g_strdup(objectPath);
    watch->watchId = g_bus_watch_name(G_BUS_TYPE_SESSION, busName, G_BUS_NAME_WATCHER_FLAGS_NONE, NULL, name_vanished_cb, watch, NULL);

    return watch;
}

static GDWatch* gd_watch_find(GSList* list, const gchar* busName, const gchar* objectPath)
{
    GSList* l = NULL;
    for (l = list; l != NULL; l = g_slist_next(l)) {
        GDWatch* watch = (GDWatch*)l->data;
        if (g_strcmp0(watch->busName, busName) == 0 && g_strcmp0(watch->objectPath, objectPath) == 0) {
            return watch;
        }
    }

    return NULL;
}

static bool gd_sn_watcher_v0_handle_register_host(GDSnWatcherV0Gen* object, GDBusMethodInvocation* invocation, const gchar* service)
{
    const gchar* busName;
    const gchar* objectPath;

    GDSnWatcherV0* v0 = GD_SN_WATCHER_V0(object);

    if (*service == '/') {
        busName = g_dbus_method_invocation_get_sender(invocation);
        objectPath = service;
    }
    else {
        busName = service;
        objectPath = "/StatusNotifierHost";
    }

    if (g_dbus_is_name(busName) == FALSE) {
        g_dbus_method_invocation_return_error(
            invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
            "D-Bus bus name '%s' is not valid", busName);
        return TRUE;
    }

    GDWatch* watch = gd_watch_find(v0->hosts, busName, objectPath);
    if (watch != NULL) {
#if 0
      g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                             G_DBUS_ERROR_INVALID_ARGS,
                                             "Status Notifier Host with bus name '%s' and object path '%s' is already registered",
                                             bus_name, object_path);
#endif

        gd_sn_watcher_v0_gen_complete_register_host(object, invocation);

        return TRUE;
    }

    watch = gd_watch_new(v0, GD_WATCH_TYPE_HOST, service, busName, objectPath);
    v0->hosts = g_slist_prepend(v0->hosts, watch);

    if (!gd_sn_watcher_v0_gen_get_is_host_registered(object)) {
        gd_sn_watcher_v0_gen_set_is_host_registered(object, TRUE);
        gd_sn_watcher_v0_gen_emit_host_registered(object);
    }

    gd_sn_watcher_v0_gen_complete_register_host(object, invocation);

    return TRUE;
}

static bool gd_sn_watcher_v0_handle_register_item(GDSnWatcherV0Gen* object, GDBusMethodInvocation* invocation, const gchar* service)
{
    const gchar* busName;
    const gchar* objectPath;

    GDSnWatcherV0* v0 = GD_SN_WATCHER_V0(object);

    if (*service == '/') {
        busName = g_dbus_method_invocation_get_sender(invocation);
        objectPath = service;
    }
    else {
        busName = service;
        objectPath = "/StatusNotifierItem";
    }

    if (g_dbus_is_name(busName) == FALSE) {
        g_dbus_method_invocation_return_error(invocation,
            G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS, "D-Bus bus name '%s' is not valid", busName);
        return TRUE;
    }
    GDWatch* watch = gd_watch_find(v0->items, busName, objectPath);
    if (watch != NULL) {
#if 0
      g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                             G_DBUS_ERROR_INVALID_ARGS,
                                             "Status Notifier Item with bus name '%s' and object path '%s' is already registered",
                                             bus_name, object_path);
#endif

        gd_sn_watcher_v0_gen_complete_register_item(object, invocation);

        return TRUE;
    }

    watch = gd_watch_new(v0, GD_WATCH_TYPE_ITEM, service, busName, objectPath);
    v0->items = g_slist_prepend(v0->items, watch);

    update_registered_items(v0);

    gchar* tmp = g_strdup_printf("%s%s", busName, objectPath);
    gd_sn_watcher_v0_gen_emit_item_registered(object, tmp);
    g_free(tmp);

    gd_sn_watcher_v0_gen_complete_register_item(object, invocation);

    return TRUE;
}

static void gd_sn_watcher_v0_gen_init(GDSnWatcherV0GenIface* iface)
{
    iface->handle_register_host = (void*) gd_sn_watcher_v0_handle_register_host;
    iface->handle_register_item = (void*) gd_sn_watcher_v0_handle_register_item;
}

static void bus_acquired_cb(GDBusConnection* connection, const gchar* name, gpointer userData)
{
    GDSnWatcherV0* v0 = GD_SN_WATCHER_V0(userData);
    GDBusInterfaceSkeleton* skeleton = G_DBUS_INTERFACE_SKELETON(v0);

    GError* error = NULL;
    const bool exported = g_dbus_interface_skeleton_export(skeleton, connection, "/StatusNotifierWatcher", &error);
    if (!exported) {
        g_warning("%s", error->message);
        g_error_free(error);
        return;
    }
}

static void gd_sn_watcher_v0_dispose(GObject* object)
{
    GDSnWatcherV0* v0 = GD_SN_WATCHER_V0(object);
    if (v0->busNameId > 0) {
        g_bus_unown_name(v0->busNameId);
        v0->busNameId = 0;
    }

    if (v0->hosts != NULL) {
        g_slist_free_full(v0->hosts, gd_watch_free);
        v0->hosts = NULL;
    }

    if (v0->items != NULL) {
        g_slist_free_full(v0->items, gd_watch_free);
        v0->items = NULL;
    }

    G_OBJECT_CLASS(gd_sn_watcher_v0_parent_class)->dispose(object);
}

static void gd_sn_watcher_v0_class_init(GDSnWatcherV0Class* v0Class)
{
    GObjectClass* oc = G_OBJECT_CLASS(v0Class);

    oc->dispose = gd_sn_watcher_v0_dispose;
}

static void gd_sn_watcher_v0_init(GDSnWatcherV0* v0)
{
    const GBusNameOwnerFlags flags = G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT | G_BUS_NAME_OWNER_FLAGS_REPLACE;

    v0->busNameId = g_bus_own_name(G_BUS_TYPE_SESSION,
        "org.kde.StatusNotifierWatcher", flags,
        bus_acquired_cb, NULL,
        NULL, v0, NULL);
}

GDSnWatcherV0* gd_sn_watcher_v0_new(void)
{
    return g_object_new(GD_TYPE_SN_WATCHER_V0, NULL);
}
