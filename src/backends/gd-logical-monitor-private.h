//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_LOGICAL_MONITOR_PRIVATE_H
#define graceful_DE2_GD_LOGICAL_MONITOR_PRIVATE_H
#include "macros/macros.h"

#include <gobject/gobject.h>

#include "gd-rectangle.h"
#include "gd-direction.h"
#include "gd-logical-monitor-private.h"
#include "gd-logical-monitor-config-private.h"

C_BEGIN_EXTERN_C
#define GD_TYPE_LOGICAL_MONITOR (gd_logical_monitor_get_type ())
G_DECLARE_FINAL_TYPE(GDLogicalMonitor, gd_logical_monitor, GD, LOGICAL_MONITOR, GObject)

struct _GDLogicalMonitor
{
    GObject parent;

    gint number;
    GDRectangle rect;
    bool is_primary;
    bool is_presentation;
    bool in_fullscreen;
    gfloat scale;
    GDMonitorTransform transform;

    glong winsys_id;
    GList* monitors;
};

typedef void (*GDLogicalMonitorCrtcFunc)(GDLogicalMonitor* logical_monitor, GDMonitor* monitor, GDOutput* output, GDCrtc* crtc, gpointer user_data);


GDLogicalMonitor* gd_logical_monitor_new(GDMonitorManager* monitor_manager, GDLogicalMonitorConfig* logical_monitor_config, gint monitor_number);

GDLogicalMonitor* gd_logical_monitor_new_derived(GDMonitorManager* monitor_manager, GDMonitor* monitor, GDRectangle* layout, gfloat scale, gint monitor_number);

void gd_logical_monitor_add_monitor(GDLogicalMonitor* logical_monitor, GDMonitor* monitor);

gboolean gd_logical_monitor_is_primary(GDLogicalMonitor* logical_monitor);

void gd_logical_monitor_make_primary(GDLogicalMonitor* logical_monitor);

gfloat gd_logical_monitor_get_scale(GDLogicalMonitor* logical_monitor);

GDMonitorTransform gd_logical_monitor_get_transform(GDLogicalMonitor* logical_monitor);

GDRectangle gd_logical_monitor_get_layout(GDLogicalMonitor* logical_monitor);

GList* gd_logical_monitor_get_monitors(GDLogicalMonitor* logical_monitor);

gboolean gd_logical_monitor_has_neighbor(GDLogicalMonitor* monitor, GDLogicalMonitor* neighbor, GDDirection direction);

void gd_logical_monitor_foreach_crtc(GDLogicalMonitor* logical_monitor, GDLogicalMonitorCrtcFunc func, gpointer user_data);

C_END_EXTERN_C

#endif // graceful_DE2_GD_LOGICAL_MONITOR_PRIVATE_H
