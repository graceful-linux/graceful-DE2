//
// Created by dingjing on 25-6-28.
//
#include "gd-crtc-private.h"

#include "gd-monitor-private.h"
#include "gd-output-private.h"
#include "gd-rectangle-private.h"
#include "gd-monitor-config-private.h"
#include "gd-logical-monitor-private.h"

typedef struct
{
    GDMonitorManager* monitor_manager;
    GDLogicalMonitor* logical_monitor;
} AddMonitorFromConfigData;

typedef struct
{
    GDLogicalMonitor* logical_monitor;
    GDLogicalMonitorCrtcFunc func;
    gpointer user_data;
} ForeachCrtcData;

G_DEFINE_TYPE(GDLogicalMonitor, gd_logical_monitor, G_TYPE_OBJECT)

static bool foreach_crtc(GDMonitor* monitor, GDMonitorMode* mode, GDMonitorCrtcMode* monitor_crtc_mode, gpointer user_data, GError** error)
{
    ForeachCrtcData* data = user_data;

    data->func(data->logical_monitor, monitor, monitor_crtc_mode->output, gd_output_get_assigned_crtc(monitor_crtc_mode->output), data->user_data);

    return TRUE;
}

static void
add_monitor_from_config(GDMonitorConfig* monitor_config, AddMonitorFromConfigData* data)
{
    GDMonitorSpec* monitor_spec;
    GDMonitor* monitor;

    monitor_spec = monitor_config->monitor_spec;
    monitor = gd_monitor_manager_get_monitor_from_spec(data->monitor_manager, monitor_spec);

    gd_logical_monitor_add_monitor(data->logical_monitor, monitor);
}

static GDMonitor*
get_first_monitor(GDMonitorManager* monitor_manager, GList* monitor_configs)
{
    GDMonitorConfig* first_monitor_config;
    GDMonitorSpec* first_monitor_spec;

    first_monitor_config = g_list_first(monitor_configs)->data;
    first_monitor_spec = first_monitor_config->monitor_spec;

    return gd_monitor_manager_get_monitor_from_spec(monitor_manager, first_monitor_spec);
}

static GDMonitorTransform
derive_monitor_transform(GDMonitor* monitor)
{
    GDOutput* main_output;
    GDCrtc* crtc;
    const GDCrtcConfig* crtc_config;
    GDMonitorTransform transform;

    main_output = gd_monitor_get_main_output(monitor);
    crtc = gd_output_get_assigned_crtc(main_output);
    crtc_config = gd_crtc_get_config(crtc);
    transform = crtc_config->transform;

    return gd_monitor_crtc_to_logical_transform(monitor, transform);
}

static void
gd_logical_monitor_dispose(GObject* object)
{
    GDLogicalMonitor* logical_monitor;

    logical_monitor = GD_LOGICAL_MONITOR(object);

    if (logical_monitor->monitors) {
        g_list_free_full(logical_monitor->monitors, g_object_unref);
        logical_monitor->monitors = NULL;
    }

    G_OBJECT_CLASS(gd_logical_monitor_parent_class)->dispose(object);
}

static void
gd_logical_monitor_class_init(GDLogicalMonitorClass* logical_monitor_class)
{
    GObjectClass* object_class;

    object_class = G_OBJECT_CLASS(logical_monitor_class);

    object_class->dispose = gd_logical_monitor_dispose;
}

static void
gd_logical_monitor_init(GDLogicalMonitor* logical_monitor)
{
}

GDLogicalMonitor*
gd_logical_monitor_new(GDMonitorManager* monitor_manager, GDLogicalMonitorConfig* logical_monitor_config, gint monitor_number)
{
    GDLogicalMonitor* logical_monitor;
    GList* monitor_configs;
    GDMonitor* first_monitor;
    GDOutput* main_output;
    AddMonitorFromConfigData data;

    logical_monitor = g_object_new(GD_TYPE_LOGICAL_MONITOR, NULL);

    monitor_configs = logical_monitor_config->monitor_configs;
    first_monitor = get_first_monitor(monitor_manager, monitor_configs);
    main_output = gd_monitor_get_main_output(first_monitor);

    logical_monitor->number = monitor_number;
    logical_monitor->winsys_id = gd_output_get_id(main_output);
    logical_monitor->scale = logical_monitor_config->scale;
    logical_monitor->transform = logical_monitor_config->transform;
    logical_monitor->in_fullscreen = -1;
    logical_monitor->rect = logical_monitor_config->layout;

    logical_monitor->is_presentation = TRUE;

    data.monitor_manager = monitor_manager;
    data.logical_monitor = logical_monitor;

    g_list_foreach(monitor_configs, (GFunc)add_monitor_from_config, &data);

    return logical_monitor;
}

GDLogicalMonitor*
gd_logical_monitor_new_derived(GDMonitorManager* monitor_manager, GDMonitor* monitor, GDRectangle* layout, gfloat scale, gint monitor_number)
{
    GDLogicalMonitor* logical_monitor;
    GDMonitorTransform transform;
    GDOutput* main_output;

    logical_monitor = g_object_new(GD_TYPE_LOGICAL_MONITOR, NULL);

    transform = derive_monitor_transform(monitor);
    main_output = gd_monitor_get_main_output(monitor);

    logical_monitor->number = monitor_number;
    logical_monitor->winsys_id = gd_output_get_id(main_output);
    logical_monitor->scale = scale;
    logical_monitor->transform = transform;
    logical_monitor->in_fullscreen = -1;
    logical_monitor->rect = *layout;

    logical_monitor->is_presentation = TRUE;

    gd_logical_monitor_add_monitor(logical_monitor, monitor);

    return logical_monitor;
}

void
gd_logical_monitor_add_monitor(GDLogicalMonitor* logical_monitor, GDMonitor* monitor)
{
    gboolean is_presentation;
    GList* l;

    is_presentation = logical_monitor->is_presentation;
    logical_monitor->monitors = g_list_append(logical_monitor->monitors, g_object_ref(monitor));

    for (l = logical_monitor->monitors; l; l = l->next) {
        GDMonitor* l_monitor;
        GList* outputs;
        GList* l_output;

        l_monitor = l->data;
        outputs = gd_monitor_get_outputs(l_monitor);

        for (l_output = outputs; l_output; l_output = l_output->next) {
            GDOutput* output;

            output = l_output->data;
            is_presentation = (is_presentation && gd_output_is_presentation(output));
        }
    }

    logical_monitor->is_presentation = is_presentation;

    gd_monitor_set_logical_monitor(monitor, logical_monitor);
}

gboolean
gd_logical_monitor_is_primary(GDLogicalMonitor* logical_monitor)
{
    return logical_monitor->is_primary;
}

void
gd_logical_monitor_make_primary(GDLogicalMonitor* logical_monitor)
{
    logical_monitor->is_primary = TRUE;
}

gfloat
gd_logical_monitor_get_scale(GDLogicalMonitor* logical_monitor)
{
    return logical_monitor->scale;
}

GDMonitorTransform
gd_logical_monitor_get_transform(GDLogicalMonitor* logical_monitor)
{
    return logical_monitor->transform;
}

GDRectangle
gd_logical_monitor_get_layout(GDLogicalMonitor* logical_monitor)
{
    return logical_monitor->rect;
}

GList*
gd_logical_monitor_get_monitors(GDLogicalMonitor* logical_monitor)
{
    return logical_monitor->monitors;
}

gboolean
gd_logical_monitor_has_neighbor(GDLogicalMonitor* monitor, GDLogicalMonitor* neighbor, GDDirection direction)
{
    switch (direction) {
    case GD_DIRECTION_RIGHT:
        if (neighbor->rect.x == (monitor->rect.x + monitor->rect.width) && gd_rectangle_vert_overlap(&neighbor->rect, &monitor->rect)) return TRUE;
        break;

    case GD_DIRECTION_LEFT:
        if (monitor->rect.x == (neighbor->rect.x + neighbor->rect.width) && gd_rectangle_vert_overlap(&neighbor->rect, &monitor->rect)) return TRUE;
        break;

    case GD_DIRECTION_UP:
        if (monitor->rect.y == (neighbor->rect.y + neighbor->rect.height) && gd_rectangle_horiz_overlap(&neighbor->rect, &monitor->rect)) return TRUE;
        break;

    case GD_DIRECTION_DOWN:
        if (neighbor->rect.y == (monitor->rect.y + monitor->rect.height) && gd_rectangle_horiz_overlap(&neighbor->rect, &monitor->rect)) return TRUE;
        break;

    default:
        break;
    }

    return FALSE;
}

void
gd_logical_monitor_foreach_crtc(GDLogicalMonitor* logical_monitor, GDLogicalMonitorCrtcFunc func, gpointer user_data)
{
    GList* l;

    for (l = logical_monitor->monitors; l; l = l->next) {
        GDMonitor* monitor;
        GDMonitorMode* mode;
        ForeachCrtcData data;

        monitor = l->data;
        mode = gd_monitor_get_current_mode(monitor);

        data.logical_monitor = logical_monitor;
        data.func = func;
        data.user_data = user_data;

        gd_monitor_mode_foreach_crtc(monitor, mode, foreach_crtc, &data, NULL);
    }
}
