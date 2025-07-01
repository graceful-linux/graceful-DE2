//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_MONITOR_VIEW_H
#define graceful_DE2_GD_MONITOR_VIEW_H

#include "gd-desktop-enums.h"
#include "gd-dummy-icon.h"

C_BEGIN_EXTERN_C

#define GD_TYPE_MONITOR_VIEW (gd_monitor_view_get_type ())
G_DECLARE_FINAL_TYPE (GDMonitorView, gd_monitor_view, GD, MONITOR_VIEW, GtkFixed)

GtkWidget  *gd_monitor_view_new            (GdkMonitor       *monitor,
                                            GDIconView       *icon_view,
                                            GDDummyIcon      *dummy_icon,
                                            guint             column_spacing,
                                            guint             row_spacing);

void        gd_monitor_view_set_placement  (GDMonitorView    *self,
                                            GDPlacement       placement);

void        gd_monitor_view_set_size       (GDMonitorView    *self,
                                            int               width,
                                            int               height);

GdkMonitor *gd_monitor_view_get_monitor    (GDMonitorView    *self);

bool    gd_monitor_view_is_primary     (GDMonitorView    *self);

bool    gd_monitor_view_add_icon       (GDMonitorView    *self,
                                            GtkWidget        *icon);

void        gd_monitor_view_remove_icon    (GDMonitorView    *self,
                                            GtkWidget        *icon);

GList      *gd_monitor_view_get_icons      (GDMonitorView    *self,
                                            GdkRectangle     *rect);

GDIcon     *gd_monitor_view_find_next_icon (GDMonitorView    *self,
                                            GDIcon           *next_to,
                                            GtkDirectionType  direction);

void        gd_monitor_view_select_icons   (GDMonitorView    *self,
                                            GdkRectangle     *rect);

C_END_EXTERN_C

#endif // graceful_DE2_GD_MONITOR_VIEW_H
