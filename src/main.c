//
// Created by dingjing on 25-6-27.
//
#include <gtk/gtk.h>
#include <glib-unix.h>
#include <glib/gi18n.h>

#include "gd-application.h"
#include "gd-session.h"


typedef struct
{
    GMainLoop*                  loop;
    GDApplication*              application;
    int                         exitStatus;
} GDMainData;


static bool     on_int_signal       (void* uData);
static bool     on_term_signal      (void* uData);
static void     main_loop_quit      (GDMainData* data);
static bool     parse_arguments     (int* argc, char**argv[]);
static void     name_lost_cb        (GDSession* session, bool wasAcquired, GDMainData* data);
static void     session_end_cb      (GDSession* session, GDMainData* data);
static void     session_ready_cb    (GDSession* session, bool isRunning, GDMainData* data);

static bool gsVersion = false;

static GOptionEntry gsEntries[] = {
    {"version", 'v', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &gsVersion, N_("Print version and exit"), NULL},
    {NULL}
};

int main (int argc, char* argv[])
{
    static GDMainData  mainData;

    textdomain(PACKAGE_NAME);
    bind_textdomain_codeset(PACKAGE_NAME, "UTF-8");

    gtk_init(&argc, &argv);

    if (!parse_arguments(&argc, &argv)) {
        return EXIT_FAILURE;
    }

    if (gsVersion) {
        g_print("%s Version: %s\n", PACKAGE_NAME, PACKAGE_VERSION);
        return EXIT_SUCCESS;
    }

    mainData.exitStatus = EXIT_SUCCESS;
    mainData.loop = g_main_loop_new(NULL, FALSE);
    const char* autostartId = g_getenv("DESKTOP_AUTOSTART_ID");
    GDSession* session = gd_session_new(autostartId ? autostartId : "");
    g_unsetenv("DESKTOP_AUTOSTART_ID");

    g_signal_connect(session, "name-lost", G_CALLBACK(name_lost_cb), &mainData);
    g_signal_connect(session, "session-ready", G_CALLBACK(session_ready_cb), &mainData);
    g_signal_connect(session, "session-end", G_CALLBACK(session_end_cb), &mainData);

    g_main_loop_run(mainData.loop);

    C_FREE_FUNC_NULL(mainData.loop, g_main_loop_unref);
    C_FREE_FUNC_NULL(session, g_object_unref);

    return mainData.exitStatus;
}

static void name_lost_cb (GDSession* session, bool wasAcquired, GDMainData* data)
{
    main_loop_quit(data);
    C_RETURN_IF_OK(wasAcquired);
    data->exitStatus = EXIT_FAILURE;
    g_warning(_("[main] Failed to acquire bus name!"));
}

static void session_ready_cb (GDSession* session, bool isRunning, GDMainData* data)
{
    g_unix_signal_add(SIGTERM, (GSourceFunc) on_term_signal, data);
    g_unix_signal_add(SIGINT, (GSourceFunc) on_int_signal, data);

    if (!isRunning) {
        gd_session_set_environment(session, "XDG_MENU_PREFIX", "graceful-DE2-");
    }

    data->application = gd_application_new();
    gd_session_register(session);
}

static void session_end_cb (GDSession* session, GDMainData* data)
{
    main_loop_quit(data);
    (void) session;
}

static bool on_int_signal (void* uData)
{
    main_loop_quit(uData);

    return G_SOURCE_REMOVE;
}

static bool on_term_signal (void* uData)
{
    main_loop_quit(uData);

    return G_SOURCE_REMOVE;
}

static void main_loop_quit (GDMainData* data)
{
    C_FREE_FUNC_NULL(data->application, g_object_unref);
    g_main_loop_quit(data->loop);
}

static bool parse_arguments (int* argc, char**argv[])
{
    GError* error = NULL;
    GOptionContext* ctx = g_option_context_new(NULL);
    GOptionGroup* group = gtk_get_option_group(false);

    g_option_context_add_main_entries(ctx, gsEntries, NULL);
    g_option_context_add_group(ctx, group);

    if (!g_option_context_parse(ctx, argc, argv, &error)) {
        g_warning(_("[main] Failed to parse command line argument: %s"), error->message);
        C_FREE_FUNC_NULL(error, g_error_free);
        C_FREE_FUNC_NULL(ctx, g_option_context_free);
        return false;
    }

    C_FREE_FUNC_NULL(error, g_error_free);
    C_FREE_FUNC_NULL(ctx, g_option_context_free);

    return true;
}
