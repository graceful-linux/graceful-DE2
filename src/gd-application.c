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

#include "plugins/shell/shell.h"
#include "plugins/panel/main-panel.h"
#include "plugins/desktop/gd-desktop.h"
#include "plugins/notifications/gd-notifications.h"
#include "plugins/status-notify-watcher/gd-status-notifier-watcher.h"


struct _GDApplication
{
    GObject                         parent;

    GDWm*                           wm;
    GDBackend*                      backend;
    guint                           busName;
    GDDesktop*                      desktop;
    GtkStyleProvider*               provider;
    GSettings*                      settings;
    GDUiScaling*                    uiScaling;

    FlashbackShell*                 shell;
    GDPanel*                        panel;

    GDNotifications*                notifications;
    GDStatusNotifierWatcher*        statusNotifierWatcher;

    GtkWidget*                      displayChangeDialog;
};
G_DEFINE_TYPE(GDApplication, gd_application, G_TYPE_OBJECT)


static void update_theme (GDApplication* obj);
static void gd_application_dispose (GObject* obj);
static void confirm_display_change_cb (GDMonitorManager* mm, GDApplication *app);
static void settings_changed (GSettings* settings, const gchar* key, void* uData);
static void theme_changed (GtkSettings* settings, GParamSpec* pspec, GDApplication* obj);
static void keep_changes_cb (GDConfirmDisplayChangeDialog *dialog, bool keep_changes, GDApplication* app);


static void gd_application_init (GDApplication* obj)
{
    /**
     * 监听窗口管理器进程变化，以及处理指定快捷键相关功能(适用于GNOME 2.x, metacity)
     */
    obj->wm = gd_wm_new();
    obj->panel = gd_panel_new ("/org/graceful/DE2/panel");
    obj->backend = gd_backend_new (GD_BACKEND_TYPE_X11_CM);
    obj->uiScaling = gd_ui_scaling_new (obj->backend);

    GDMonitorManager* mm = gd_backend_get_monitor_manager (obj->backend);
    g_signal_connect (mm, "confirm-display-change", G_CALLBACK (confirm_display_change_cb), obj);

    // settings
    GtkSettings* settings = gtk_settings_get_default();
    g_signal_connect (settings, "notify::gtk-theme-name", G_CALLBACK (theme_changed), obj);

    obj->settings = g_settings_new ("org.gnome.gnome-flashback");
    g_signal_connect (obj->settings, "changed", G_CALLBACK (settings_changed), obj);
    settings_changed (obj->settings, NULL, obj);

    obj->busName = g_bus_own_name(G_BUS_TYPE_SESSION, "org.graceful.DE2",
        G_BUS_NAME_OWNER_FLAGS_REPLACE | G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT,
        NULL, NULL, NULL, NULL, NULL);
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
    C_FREE_FUNC_NULL(app->panel, g_object_unref);
    C_FREE_FUNC_NULL(app->shell, g_object_unref);
    C_FREE_FUNC_NULL(app->backend, g_object_unref);
    C_FREE_FUNC_NULL(app->desktop, g_object_unref);
    C_FREE_FUNC_NULL(app->uiScaling, g_object_unref);
    C_FREE_FUNC_NULL(app->notifications, g_object_unref);
    C_FREE_FUNC_NULL(app->statusNotifierWatcher, g_object_unref);
    C_FREE_FUNC_NULL(app->displayChangeDialog, gtk_widget_destroy);

    G_OBJECT_CLASS(gd_application_parent_class)->dispose(obj);
}

static void confirm_display_change_cb (GDMonitorManager* mm, GDApplication *app)
{
    const gint timeout = gd_monitor_manager_get_display_configuration_timeout ();

    g_clear_pointer (&app->displayChangeDialog, gtk_widget_destroy);
    app->displayChangeDialog = gd_confirm_display_change_dialog_new (timeout);

    g_signal_connect (app->displayChangeDialog, "keep-changes", G_CALLBACK (keep_changes_cb), app);

    gtk_window_present (GTK_WINDOW (app->displayChangeDialog));
}

static void keep_changes_cb (GDConfirmDisplayChangeDialog *dialog, bool keepChanges, GDApplication* app)
{
    GDMonitorManager* mm = gd_backend_get_monitor_manager (app->backend);

    gd_monitor_manager_confirm_configuration (mm, keepChanges);
    g_clear_pointer (&app->displayChangeDialog, gtk_widget_destroy);
}

static void settings_changed (GSettings* settings, const gchar* key, void* uData)
{
    GDApplication* application = GD_APPLICATION (uData);
    GDMonitorManager* mm = gd_backend_get_monitor_manager (application->backend);

    g_info("[main] settings changed for %s", key ? key : "(null)");

#define SETTING_CHANGED(objName, settingName, funcName) \
    if (key == NULL || g_strcmp0 (key, settingName) == 0) { \
        if (g_settings_get_boolean (settings, settingName)) { \
            if (application->objName == NULL) { \
                application->objName = funcName (); \
            } \
        } \
        else { \
            g_clear_object (&application->objName); \
        } \
    }

  // SETTING_CHANGED (automount, "automount-manager", gsd_automount_manager_new)
  SETTING_CHANGED (shell, "shell", flashback_shell_new)
  // SETTING_CHANGED (a11y_keyboard, "a11y-keyboard", gf_a11y_keyboard_new)
  // SETTING_CHANGED (audio_device_selection, "audio-device-selection", gf_audio_device_selection_new)
  SETTING_CHANGED (desktop, "desktop", gd_desktop_new)
  // SETTING_CHANGED (dialog, "end-session-dialog", gf_end_session_dialog_new)
  // SETTING_CHANGED (input_settings, "input-settings", gf_input_settings_new)
  // SETTING_CHANGED (input_sources, "input-sources", gf_input_sources_new)
  SETTING_CHANGED (notifications, "notifications", gd_notifications_new)
  // SETTING_CHANGED (root_background, "root-background", gf_root_background_new)
  // SETTING_CHANGED (screencast, "screencast", gf_screencast_new)
  // SETTING_CHANGED (screensaver, "screensaver", gf_screensaver_new)
  // SETTING_CHANGED (screenshot, "screenshot", gf_screenshot_new)
  SETTING_CHANGED (statusNotifierWatcher, "status-notifier-watcher", gd_status_notifier_watcher_new)

#undef SETTING_CHANGED

  if (application->desktop) {
      gd_desktop_set_monitor_manager (application->desktop, mm);
  }

  // if (application->input_settings)
    // gf_input_settings_set_monitor_manager (application->input_settings,
                                           // monitor_manager);

  // if (application->screensaver)
    // {
      // gf_screensaver_set_monitor_manager (application->screensaver,
                                          // monitor_manager);

      // gf_screensaver_set_input_sources (application->screensaver,
                                        // application->input_sources);
    // }

    if (application->shell) {
        flashback_shell_set_monitor_manager (application->shell, mm);
    }
}

static void theme_changed (GtkSettings* settings, GParamSpec* pspec, GDApplication* obj)
{
    update_theme (obj);
}

static void update_theme (GDApplication* obj)
{
    // GDSupportedTheme *theme;
    guint priority;
    gboolean preferDark;
    gchar* resource = NULL;
    gchar* themeName = NULL;

    GdkScreen *screen = gdk_screen_get_default ();
    GtkSettings *settings = gtk_settings_get_default ();

    if (obj->provider != NULL) {
        gtk_style_context_remove_provider_for_screen (screen, obj->provider);
        g_clear_object (&obj->provider);
    }

    g_object_get (settings, "gtk-theme-name", &themeName, "gtk-application-prefer-dark-theme", &preferDark, NULL);

    // if (is_theme_supported (theme_name, &theme)) {
        // resource = get_theme_resource (theme, prefer_dark);
        // priority = GTK_STYLE_PROVIDER_PRIORITY_APPLICATION;
    // }
    // else {
    {
        resource = g_strdup ("/org/gnome/gnome-flashback/theme/fallback.css");
        priority = GTK_STYLE_PROVIDER_PRIORITY_FALLBACK;
    }

    GtkCssProvider *css = gtk_css_provider_new ();
    obj->provider =  GTK_STYLE_PROVIDER (css);

    gtk_css_provider_load_from_resource (css, resource);
    gtk_style_context_add_provider_for_screen (screen, obj->provider, priority);

    C_FREE_FUNC_NULL(themeName, g_free);
    C_FREE_FUNC_NULL(resource, g_free);
}
