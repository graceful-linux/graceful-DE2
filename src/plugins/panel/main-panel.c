//
// Created by dingjing on 25-7-3.
//
#include "main-panel.h"

#include <gio/gio.h>


struct _GDPanel
{
    GObject             parent;

    char*               startupId;
    guint               nameId;
    bool                nameAcquired;
    GCancellable*       cancellable;
    GDBusProxy*         proxy;

    char*               clientId;
    GDBusProxy*         clientPrivate;
};

enum
{
    PROP_0,

    PROP_STARTUP_ID,

    LAST_PROP
};

enum
{
    NAME_LOST,
    PANEL_READY,
    PANEL_END,

    LAST_SIGNAL
};

static GParamSpec*  gsSessionProperties[LAST_PROP] = {NULL};
static guint        gsSessionSignals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GDPanel, gd_panel, G_TYPE_OBJECT)

static void gd_panel_constructed (GObject* obj);
static void name_lost_cb (GDBusConnection* conn, const char* name, void* uData);
static void name_acquired_cb (GDBusConnection* conn, const char* name, void* uData);
static void session_manager_ready_cb (GObject* srcObj, GAsyncResult *res, void* uData);
static void gd_panel_set_property (GObject* obj, guint propId, const GValue* value, GParamSpec* pspec);

static void gd_panel_register (GDPanel* self);
static void respond_to_end_session (GDPanel* self);
static void client_private_ready_cb (GObject* srcObj, GAsyncResult *res, void* uData);
static void register_client_cb (GObject* source_object, GAsyncResult *res, void* uData);
static void g_signal_cb (GDBusProxy *proxy, char* senderName, char* signalName, GVariant* parameters, GDPanel* self);



GDPanel* gd_panel_new (const char* startupId)
{
    return g_object_new(GD_TYPE_PANEL, "startup-id", startupId, NULL);
}


static void gd_panel_constructed (GObject* obj)
{
    GDPanel* self = GD_PANEL(obj);
    const GBusNameOwnerFlags flags = G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT | G_BUS_NAME_OWNER_FLAGS_REPLACE;

    G_OBJECT_CLASS(gd_panel_parent_class)->constructed (obj);

    self->nameId = g_bus_own_name(G_BUS_TYPE_SESSION,
        "org.graceful.DE2.panel", flags,
        NULL,
        name_acquired_cb,
        name_lost_cb, self,
        NULL);
}

static void gd_panel_dispose (GObject* obj)
{
    GDPanel* self = GD_PANEL(obj);

    if (0 != self->nameId) {
        g_bus_unown_name(self->nameId);
        self->nameId = 0;
    }

    g_cancellable_cancel(self->cancellable);
    C_FREE_FUNC_NULL(self->cancellable, g_object_unref);
    C_FREE_FUNC_NULL(self->proxy, g_object_unref);
    C_FREE_FUNC_NULL(self->clientPrivate, g_object_unref);

    G_OBJECT_CLASS(gd_panel_parent_class)->dispose (obj);
}

static void gd_panel_finalize (GObject* obj)
{
    GDPanel* self = GD_PANEL(obj);

    C_FREE_FUNC_NULL(self->startupId, g_free);
    C_FREE_FUNC_NULL(self->clientId, g_free);

    G_OBJECT_CLASS(gd_panel_parent_class)->finalize (obj);
}

static void gd_panel_class_init (GDPanelClass* klass)
{
    GObjectClass* oc = G_OBJECT_CLASS(klass);

    oc->constructed = gd_panel_constructed;
    oc->dispose = gd_panel_dispose;
    oc->finalize = gd_panel_finalize;
    oc->set_property = gd_panel_set_property;

    gsSessionProperties[PROP_STARTUP_ID] =
        g_param_spec_string ("startup-id", "startup-id", "startup-id", "",
            G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);
    g_object_class_install_properties(oc, LAST_PROP, gsSessionProperties);

    gsSessionSignals[NAME_LOST] =
        g_signal_new ("name-lost",
            GD_TYPE_PANEL,
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            NULL,
            G_TYPE_NONE,
            1, G_TYPE_BOOLEAN);

    gsSessionSignals[PANEL_READY] =
        g_signal_new ("session-ready",
            GD_TYPE_PANEL,
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            NULL,
            G_TYPE_NONE,
            0);

    gsSessionSignals[PANEL_END] =
        g_signal_new ("end-session",
            GD_TYPE_PANEL,
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            NULL,
            G_TYPE_NONE,
            0);
}

static void gd_panel_set_property (GObject* obj, guint propId, const GValue* value, GParamSpec* pspec)
{
    GDPanel* self = GD_PANEL(obj);
    switch (propId) {
        case PROP_STARTUP_ID: {
            self->startupId = g_value_dup_string(value);
            break;
        }
        default: {
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, propId, pspec);
            break;
        }
    }
}

static void gd_panel_init (GDPanel* self)
{
    self->cancellable = g_cancellable_new();
}

static void name_acquired_cb (GDBusConnection* conn, const char* name, void* uData)
{
    g_info("[panel] DBus '%s' acquired", name ? name : "(null)");

    GDPanel* self = GD_PANEL(uData);
    self->nameAcquired = true;

    GDBusProxyFlags flags = G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES | G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS;

    g_dbus_proxy_new_for_bus(G_BUS_TYPE_SESSION,
        flags, NULL,
        "org.graceful.DE2.SessionManager",
        "/org/graceful/DE2/SessionManager",
        "org.graceful.DE2.SessionManager",
        self->cancellable,
        session_manager_ready_cb, self);
}

static void name_lost_cb (GDBusConnection* conn, const char* name, void* uData)
{
    g_info("[panel] DBus '%s' lost", name ? name : "(null)");

}

static void session_manager_ready_cb (GObject* srcObj, GAsyncResult *res, void* uData)
{
    GError *error = NULL;

    GDBusProxy *proxy = g_dbus_proxy_new_for_bus_finish (res, &error);
    if (error != NULL) {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
            g_warning ("Failed to get session manager proxy: %s", error->message);
        }
        g_error_free (error);
        return;
    }

    GDPanel* self = GD_PANEL(uData);
    self->proxy = proxy;

    g_signal_emit (self, gsSessionSignals[PANEL_READY], 0);
}

static void gd_panel_register (GDPanel* self)
{
    g_dbus_proxy_call(self->proxy,
        "RegisterClient",
        g_variant_new("(ss)", "graceful-DE2", self->startupId),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        self->cancellable,
        register_client_cb, self);
}

static void register_client_cb (GObject* source_object, GAsyncResult *res, void* uData)
{
    GError *error = NULL;

    GVariant *variant = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, &error);
    if (error != NULL) {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
            g_warning ("Failed to register client: %s", error->message);
        }
        g_error_free (error);
        return;
    }

    GDPanel* self = GD_PANEL(uData);

    g_variant_get (variant, "(o)", &self->clientId);
    g_variant_unref (variant);
    g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                              G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
                              NULL,
                              "org.graceful.DE2.SessionManager",
                              self->clientId,
                              "org.graceful.DE2.SessionManager.ClientPrivate",
                              self->cancellable,
                              client_private_ready_cb,
                              self);
}

static void client_private_ready_cb (GObject* srcObj, GAsyncResult *res, void* uData)
{
    GError *error;

    error = NULL;
    GDBusProxy* proxy = g_dbus_proxy_new_for_bus_finish (res, &error);

    if (error != NULL) {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            g_warning ("Failed to get a client proxy: %s", error->message);

        g_error_free (error);
        return;
    }

    GDPanel* self = GD_PANEL(uData);
    self->clientPrivate = proxy;

    g_signal_connect (self->clientPrivate, "g-signal", G_CALLBACK (g_signal_cb), self);
}

static void g_signal_cb (GDBusProxy *proxy, char* senderName, char* signalName, GVariant* parameters, GDPanel* self)
{
    if (g_strcmp0 (signalName, "QueryEndSession") == 0) {
        respond_to_end_session (self);
    }
    else if (g_strcmp0 (signalName, "EndSession") == 0) {
        respond_to_end_session (self);
    }
    else if (g_strcmp0 (signalName, "Stop") == 0) {
        g_signal_emit (self, gsSessionSignals[PANEL_END], 0);
    }
}

static void respond_to_end_session (GDPanel* self)
{
    g_dbus_proxy_call (self->clientPrivate,
                       "EndSessionResponse",
                       g_variant_new ("(bs)", TRUE, ""),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       self->cancellable,
                       NULL,
                       NULL);
}