//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_DESKTOP_WINDOW_H
#define graceful_DE2_GD_DESKTOP_WINDOW_H

#include "backends/gd-monitor-manager.h"
#include <gtk/gtk.h>

C_BEGIN_EXTERN_C

#define GD_TYPE_DESKTOP_WINDOW (gd_desktop_window_get_type ())
G_DECLARE_FINAL_TYPE (GDDesktopWindow, gd_desktop_window, GD, DESKTOP_WINDOW, GtkWindow)

GtkWidget *gd_desktop_window_new                 (bool draw_background, bool show_icons, GError** error);
void       gd_desktop_window_set_monitor_manager (GDDesktopWindow   *self, GDMonitorManager* monitor_manager);
bool       gd_desktop_window_is_ready            (GDDesktopWindow   *self);
int        gd_desktop_window_get_width           (GDDesktopWindow   *self);
int        gd_desktop_window_get_height          (GDDesktopWindow   *self);

C_END_EXTERN_C

#endif // graceful_DE2_GD_DESKTOP_WINDOW_H
