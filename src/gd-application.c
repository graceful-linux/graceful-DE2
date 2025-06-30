//
// Created by dingjing on 25-6-27.
//

#include "gd-application.h"

#include <gtk/gtk.h>

#include "gd-wm.h"
#include "gd-ui-scaling.h"
#include "backends/gd-backend.h"
#include "gd-session-manager-gen.dbus.h"
#include "gd-confirm-display-change-dialog.h"


struct _GDApplication
{
    GObject                         parent;
    GDWm*                           wm;
    GDBackend*                      backend;
    GDUiScaling*                    uiScaling;
    guint                           busName;
    GtkStyleProvider*               provider;

    GtkWidget*                      displayChangeDialog;
};
G_DEFINE_TYPE(GDApplication, gd_application, G_TYPE_OBJECT)


static void gd_application_dispose (GObject* obj);
static void confirm_display_change_cb (GDMonitorManager* mm, GDApplication *app);
static void keep_changes_cb (GDConfirmDisplayChangeDialog *dialog, bool keep_changes, GDApplication* app);


static void gd_application_init (GDApplication* obj)
{
    GDMonitorManager* mm = NULL;

    obj->wm = gd_wm_new();
    obj->busName = g_bus_own_name(G_BUS_TYPE_SESSION, "org.graceful.DE2",
        G_BUS_NAME_OWNER_FLAGS_REPLACE | G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT,
        NULL, NULL, NULL, NULL, NULL);
    obj->backend = gd_backend_new (GD_BACKEND_TYPE_X11_CM);
    obj->uiScaling = gd_ui_scaling_new (obj->backend);

    mm = gd_backend_get_monitor_manager (obj->backend);
    g_signal_connect (mm, "confirm-display-change", G_CALLBACK (confirm_display_change_cb), obj);
}

static void gd_application_class_init (GDApplicationClass* klass)
{
    GObjectClass* objClass = G_OBJECT_CLASS(klass);
    objClass->dispose = gd_application_dispose;
}


GDApplication * gd_application_new(void)
{
    return g_object_new(GD_TYPE_APPLICATION, NULL);
}

static void gd_application_dispose (GObject* obj)
{
    GDApplication* app = GD_APPLICATION(obj);

    C_FREE_FUNC(app->busName, g_bus_unown_name);
    C_FREE_FUNC_NULL(app->wm, g_object_unref);
    C_FREE_FUNC_NULL(app->backend, g_object_unref);
    C_FREE_FUNC_NULL(app->uiScaling, g_object_unref);
    C_FREE_FUNC_NULL(app->displayChangeDialog, gtk_widget_destroy);

    G_OBJECT_CLASS(gd_application_parent_class)->dispose(obj);
}

static void confirm_display_change_cb (GDMonitorManager* mm, GDApplication *app)
{
    gint timeout;

    timeout = gd_monitor_manager_get_display_configuration_timeout ();

    g_clear_pointer (&app->displayChangeDialog, gtk_widget_destroy);
    app->displayChangeDialog = gd_confirm_display_change_dialog_new (timeout);

    g_signal_connect (app->displayChangeDialog, "keep-changes", G_CALLBACK (keep_changes_cb), app);

    gtk_window_present (GTK_WINDOW (app->displayChangeDialog));
}

static void keep_changes_cb (GDConfirmDisplayChangeDialog *dialog, bool keep_changes, GDApplication* app)
{
    GDMonitorManager* mm = gd_backend_get_monitor_manager (app->backend);

    gd_monitor_manager_confirm_configuration (mm, keep_changes);
    g_clear_pointer (&app->displayChangeDialog, gtk_widget_destroy);
}