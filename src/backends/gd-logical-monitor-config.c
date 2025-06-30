//
// Created by dingjing on 25-6-28.
//
#include <gio/gio.h>
#include <math.h>

#include "gd-monitor-spec-private.h"
#include "gd-monitor-config-private.h"
#include "gd-logical-monitor-config-private.h"


void
gd_logical_monitor_config_free(GDLogicalMonitorConfig* config)
{
    GList* monitor_configs;

    monitor_configs = config->monitor_configs;

    g_list_free_full(monitor_configs, (GDestroyNotify)gd_monitor_config_free);
    g_free(config);
}

bool
gd_logical_monitor_configs_have_monitor(GList* logical_monitor_configs, GDMonitorSpec* monitor_spec)
{
    GList* l;

    for (l = logical_monitor_configs; l; l = l->next) {
        GDLogicalMonitorConfig* logical_monitor_config = l->data;
        GList* k;

        for (k = logical_monitor_config->monitor_configs; k; k = k->next) {
            GDMonitorConfig* monitor_config = k->data;

            if (gd_monitor_spec_equals(monitor_spec, monitor_config->monitor_spec)) return TRUE;
        }
    }

    return FALSE;
}

bool
gd_verify_logical_monitor_config(GDLogicalMonitorConfig* config, GDLogicalMonitorLayoutMode layout_mode, GDMonitorManager* monitor_manager, GError** error)
{
    GList* l;
    int layout_width;
    int layout_height;
    GDMonitorConfig* first_monitor_config;
    int mode_width;
    int mode_height;
    int expected_mode_width;
    int expected_mode_height;
    float scale;

    if (config->layout.x < 0 || config->layout.y < 0) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Invalid logical monitor position (%d, %d)", config->layout.x, config->layout.y);

        return FALSE;
    }

    if (!config->monitor_configs) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Logical monitor is empty");

        return FALSE;
    }

    expected_mode_width = 0;
    expected_mode_height = 0;
    scale = config->scale;

    first_monitor_config = config->monitor_configs->data;
    mode_width = first_monitor_config->mode_spec->width;
    mode_height = first_monitor_config->mode_spec->height;

    for (l = config->monitor_configs; l; l = l->next) {
        GDMonitorConfig* monitor_config;

        monitor_config = l->data;

        if (monitor_config->mode_spec->width != mode_width || monitor_config->mode_spec->height != mode_height) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Monitors modes in logical monitor not equal");
            return FALSE;
        }
    }

    if (gd_monitor_transform_is_rotated(config->transform)) {
        layout_width = config->layout.height;
        layout_height = config->layout.width;
    }
    else {
        layout_width = config->layout.width;
        layout_height = config->layout.height;
    }

    switch (layout_mode) {
    case GD_LOGICAL_MONITOR_LAYOUT_MODE_LOGICAL: {
        float scaled_width;
        float scaled_height;

        scaled_width = mode_width / scale;
        scaled_height = mode_height / scale;

        if (floorf(scaled_width) != scaled_width || floorf(scaled_height) != scaled_height) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Scaled logical monitor size is fractional");

            return FALSE;
        }

        expected_mode_width = (int)roundf(layout_width * scale);
        expected_mode_height = (int)roundf(layout_height * scale);
    }
    break;

    case GD_LOGICAL_MONITOR_LAYOUT_MODE_PHYSICAL:
        if (!G_APPROX_VALUE(scale, roundf (scale), FLT_EPSILON)) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "A fractional scale with physical layout mode not allowed");

            return FALSE;
        }

        expected_mode_width = layout_width;
        expected_mode_height = layout_height;
        break;

    default:
        break;
    }

    if (mode_width != expected_mode_width || mode_height != expected_mode_height) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Monitor mode size doesn't match scaled monitor layout");

        return FALSE;
    }

    return TRUE;
}
