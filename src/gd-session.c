//
// Created by dingjing on 25-6-27.
//

#include "gd-session.h"
#include <gio/gio.h>
#include <glib/gi18n.h>

#include "gd-session-manager-gen.dbus.h"
#include "gd-sm-client-private-gen.dbus.h"


enum
{
    PROP_0,

    PROP_STARTUP_ID,

    PROP_NUM
};

enum
{
    NAME_LOST,
    SIGNAL_SESSION_READY,
    SIGNAL_SESSION_END,
    SIGNAL_NUM
};

struct _GDSession
{
    GObject                         parent;
    char*                           startupId;
    char*                           clientId;
    guint                           nameId;
    bool                            nameAcquired;
    GCancellable*                   cancellable;
    GDSmClientPrivateGen*           clientPrivate;
    GDSessionManagerGen*            sessionManager;
};
G_DEFINE_TYPE(GDSession, gd_session, G_TYPE_OBJECT)

static GParamSpec* gsSessionProperties[PROP_NUM] = { NULL };
static guint gsSessionSignals[SIGNAL_NUM] = { 0 };


static void respond_to_end_session (GDSession *self);
static void stop_cb (GDSmClientPrivateGen* object, GDSession* self);
static void setenv_cb (GObject* sObj, GAsyncResult* res, gpointer uData);
static void name_lost_cb (GDBusConnection* cnn, const char* name, void* uData);
static void register_client_cb (GObject* sObj, GAsyncResult* res, void* uData);
static void is_session_running_cb (GObject* sObj, GAsyncResult* res, void* uData);
static void name_acquired_cb (GDBusConnection* conn, const char* name, void* uData);
static void client_private_ready_cb (GObject* sObj, GAsyncResult* res, void* uData);
static void session_manager_ready_cb (GObject* sObj, GAsyncResult* res, void* uData);
static void end_session_cb (GDSmClientPrivateGen* object, guint flags, GDSession* self);
static void query_end_session_cb (GDSmClientPrivateGen* object, guint flags, GDSession* self);


static void name_acquired_cb (GDBusConnection* conn, const char* name, void* uData)
{
    GDSession *self = GD_SESSION (uData);
    self->nameAcquired = TRUE;
    GDBusProxyFlags flags = G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES | G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS;

    gd_session_manager_gen_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                              flags,
                                              "org.graceful.DE2.SessionManager",
                                              "/org/graceful/DE2/SessionManager",
                                              self->cancellable,
                                              session_manager_ready_cb,
                                              self);
}

static void name_lost_cb (GDBusConnection* cnn, const char* name, void* uData)
{
    GDSession *self = GD_SESSION (uData);
    g_signal_emit (self, gsSessionSignals[NAME_LOST], 0, self->nameAcquired);
}

static void gd_session_constructed (GObject* obj)
{
    GDSession* self = GD_SESSION(obj);

    const GBusNameOwnerFlags flags = G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT | G_BUS_NAME_OWNER_FLAGS_REPLACE;

    self->nameId = g_bus_own_name(G_BUS_TYPE_SESSION, "org.graceful.DE2", flags,
        NULL, name_acquired_cb, name_lost_cb, self, NULL);
}

static void gd_session_dispose (GObject* obj)
{
    GDSession* self = GD_SESSION(obj);

    C_FREE_FUNC(self->nameId, g_bus_unown_name);
    C_FREE_FUNC(self->cancellable, g_cancellable_cancel);
    C_FREE_FUNC_NULL(self->cancellable, g_object_unref);

    C_FREE_FUNC_NULL(self->sessionManager, g_object_unref);
    C_FREE_FUNC_NULL(self->clientPrivate, g_object_unref);

    G_OBJECT_CLASS(gd_session_parent_class)->dispose(obj);
}

static void gd_session_finalize (GObject* obj)
{
    GDSession* self = GD_SESSION(obj);
    C_FREE_FUNC_NULL(self->startupId, g_free);
    C_FREE_FUNC_NULL(self->clientId, g_free);

    G_OBJECT_CLASS(gd_session_parent_class)->finalize(obj);
}

static void gd_session_set_property (GObject* obj, guint propId, const GValue* value, GParamSpec* pspec)
{
    GDSession* self = GD_SESSION(obj);
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

static void gd_session_class_init (GDSessionClass* klass)
{
    GObjectClass* objClass = G_OBJECT_CLASS(klass);

    objClass->constructed = gd_session_constructed;
    objClass->dispose = gd_session_dispose;
    objClass->finalize = gd_session_finalize;
    objClass->set_property = gd_session_set_property;

    // install properties
    gsSessionProperties[PROP_STARTUP_ID] = g_param_spec_string(
        "startup-id",
        "startup-id",
        "startup-id",
        "",
        G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);
    g_object_class_install_properties(objClass, PROP_NUM, gsSessionProperties);

    // install signal
    gsSessionSignals[NAME_LOST] = g_signal_new(
        "name-lost",
        GD_TYPE_SESSION,
        G_SIGNAL_RUN_LAST,
        0,
        NULL,
        NULL,
        NULL,
        G_TYPE_NONE,
        1,
        G_TYPE_BOOLEAN);
    gsSessionSignals[SIGNAL_SESSION_READY] = g_signal_new(
        "session-ready",
        GD_TYPE_SESSION,
        G_SIGNAL_RUN_LAST,
        0,
        NULL,
        NULL,
        NULL,
        G_TYPE_NONE,
        1,
        G_TYPE_BOOLEAN);
    gsSessionSignals[SIGNAL_SESSION_END] = g_signal_new(
        "session-end",
        GD_TYPE_SESSION,
        G_SIGNAL_RUN_LAST,
        0,
        NULL,
        NULL,
        NULL,
        G_TYPE_NONE,
        0);
}

static void gd_session_init (GDSession* self)
{
    self->cancellable = g_cancellable_new();
}


GDSession * gd_session_new(const char * startupId)
{
    return g_object_new(GD_TYPE_SESSION, "startup-id", startupId, NULL);
}

void gd_session_set_environment(GDSession * self, const char * name, const char * value)
{
    gd_session_manager_gen_call_setenv(self->sessionManager, name, value, self->cancellable, setenv_cb, self);
}

void gd_session_register(GDSession * self)
{
    gd_session_manager_gen_call_register_client(self->sessionManager,
        "graceful-DE2", self->startupId, self->cancellable, register_client_cb, self);
}

static void register_client_cb (GObject* sObj, GAsyncResult* res, void* uData)
{
    GError* error = NULL;
    char* clientId = NULL;
    GDSession* self = NULL;

    gd_session_manager_gen_call_register_client_finish (GD_SESSION_MANAGER_GEN (sObj), &clientId, res, &error);
    if (error != NULL) {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
            g_warning (_("Failed to register client: %s\n"), error->message);
        }
        C_FREE_FUNC_NULL(error, g_error_free);
        return;
    }

    self = GD_SESSION (uData);
    self->clientId = clientId;

    gd_sm_client_private_gen_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                                G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
                                                "org.graceful.DE2.SessionManager",
                                                self->clientId,
                                                self->cancellable,
                                                client_private_ready_cb,
                                                self);
}

static void setenv_cb (GObject* sObj, GAsyncResult* res, gpointer uData)
{
    GError *error = NULL;

    gd_session_manager_gen_call_setenv_finish (GD_SESSION_MANAGER_GEN (sObj), res, &error);
    if (error != NULL) {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
            g_warning (_("Failed to set the environment: %s"), error->message);
        }
        C_FREE_FUNC_NULL(error, g_error_free);
    }
}

static void end_session_cb (GDSmClientPrivateGen* object, guint flags, GDSession* self)
{
    respond_to_end_session (self);
}

static void client_private_ready_cb (GObject* sObj, GAsyncResult* res, void* uData)
{
    GError *error = NULL;

    GDSmClientPrivateGen* clientPriv = gd_sm_client_private_gen_proxy_new_for_bus_finish (res, &error);
    if (error != NULL) {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
            g_warning (_("Failed to get a client private proxy: %s"), error->message);
        }
        C_FREE_FUNC_NULL(error, g_error_free);
        return;
    }

    GDSession* self = GD_SESSION (uData);
    self->clientPrivate = clientPriv;

    g_signal_connect (self->clientPrivate, "end-session", G_CALLBACK (end_session_cb), self);
    g_signal_connect (self->clientPrivate, "query-end-session", G_CALLBACK (query_end_session_cb), self);
    g_signal_connect (self->clientPrivate, "stop", G_CALLBACK (stop_cb), self);
}

static void respond_to_end_session (GDSession *self)
{
    gd_sm_client_private_gen_call_end_session_response (self->clientPrivate, TRUE, "", self->cancellable, NULL, NULL);
}

static void query_end_session_cb (GDSmClientPrivateGen* object, guint flags, GDSession* self)
{
    respond_to_end_session (self);
}

static void stop_cb (GDSmClientPrivateGen* object, GDSession* self)
{
    g_signal_emit (self, gsSessionSignals[SIGNAL_SESSION_END], 0);
}

static void session_manager_ready_cb (GObject* sObj, GAsyncResult* res, void* uData)
{
    GError* error = NULL;

    GDSessionManagerGen* sessionManager = gd_session_manager_gen_proxy_new_for_bus_finish (res, &error);

    if (error != NULL) {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
            g_warning (_("Failed to get session manager proxy: %s"), error->message);
        }
        C_FREE_FUNC_NULL(error, g_error_free);
        return;
    }

    GDSession* self = GD_SESSION (uData);
    self->sessionManager = sessionManager;
    gd_session_manager_gen_call_is_session_running (self->sessionManager, self->cancellable, is_session_running_cb, self);
}

static void is_session_running_cb (GObject* sObj, GAsyncResult* res, void* uData)
{
    gboolean isSessionRunning;

    GError *error = NULL;
    gd_session_manager_gen_call_is_session_running_finish (GD_SESSION_MANAGER_GEN (sObj), &isSessionRunning, res, &error);
    if (error != NULL) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
            C_FREE_FUNC_NULL(error, g_error_free);
            return;
        }
        else {
            isSessionRunning = TRUE;
            g_warning (_("Failed to check if session has entered the Running phase: %s"), error->message);
            C_FREE_FUNC_NULL(error, g_error_free);
        }
    }

    GDSession* self = GD_SESSION (uData);
    g_signal_emit (self, gsSessionSignals[SIGNAL_SESSION_READY], 0, isSessionRunning);
}

