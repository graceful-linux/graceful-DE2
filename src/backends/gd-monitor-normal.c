//
// Created by dingjing on 25-6-28.
//
#include "gd-monitor-normal-private.h"

#include "gd-backend.h"
#include "gd-output-private.h"
#include "gd-monitor-private.h"
#include "gd-crtc-mode-private.h"
#include "gd-crtc-mode-info-private.h"
#include "gd-monitor-manager-private.h"


struct _GDMonitorNormal
{
    GDMonitor parent;
};

G_DEFINE_TYPE(GDMonitorNormal, gd_monitor_normal, GD_TYPE_MONITOR)

static void
generate_modes(GDMonitorNormal* normal)
{
    GDMonitor* monitor;
    GDOutput* output;
    const GDOutputInfo* output_info;
    GDCrtcMode* preferred_mode;
    GDCrtcModeFlag preferred_mode_flags;
    guint i;

    monitor = GD_MONITOR(normal);
    output = gd_monitor_get_main_output(monitor);
    output_info = gd_output_get_info(output);
    preferred_mode = output_info->preferred_mode;
    preferred_mode_flags = gd_crtc_mode_get_info(preferred_mode)->flags;

    for (i = 0; i < output_info->n_modes; i++) {
        GDCrtcMode* crtc_mode;
        const GDCrtcModeInfo* crtc_mode_info;
        GDMonitorMode* mode;
        gboolean replace;
        GDCrtc* crtc;

        crtc_mode = output_info->modes[i];
        crtc_mode_info = gd_crtc_mode_get_info(crtc_mode);

        mode = g_new0(GDMonitorMode, 1);
        mode->monitor = monitor;
        mode->spec = gd_monitor_create_spec(monitor, crtc_mode_info->width, crtc_mode_info->height, crtc_mode);

        mode->id = gd_monitor_mode_spec_generate_id(&mode->spec);

        mode->crtcModes = g_new(GDMonitorCrtcMode, 1);
        mode->crtcModes[0].output = output;
        mode->crtcModes[0].crtcMode = crtc_mode;

        /*
         * We don't distinguish between all available mode flags, just the ones
         * that are configurable. We still need to pick some mode though, so
         * prefer ones that has the same set of flags as the preferred mode;
         * otherwise take the first one in the list. This guarantees that the
         * preferred mode is always added.
         */
        replace = crtc_mode_info->flags == preferred_mode_flags;

        if (!gd_monitor_add_mode(monitor, mode, replace)) {
            g_assert(crtc_mode != output_info->preferred_mode);
            gd_monitor_mode_free(mode);
            continue;
        }

        if (crtc_mode == output_info->preferred_mode) gd_monitor_set_preferred_mode(monitor, mode);

        crtc = gd_output_get_assigned_crtc(output);

        if (crtc != NULL) {
            const GDCrtcConfig* crtc_config;

            crtc_config = gd_crtc_get_config(crtc);
            if (crtc_config && crtc_mode == crtc_config->mode) gd_monitor_set_current_mode(monitor, mode);
        }
    }
}

static GDOutput*
gd_monitor_normal_get_main_output(GDMonitor* monitor)
{
    GList* outputs;

    outputs = gd_monitor_get_outputs(monitor);

    return outputs->data;
}

static void
gd_monitor_normal_derive_layout(GDMonitor* monitor, GDRectangle* layout)
{
    GDOutput* output;
    GDCrtc* crtc;
    const GDCrtcConfig* crtc_config;

    output = gd_monitor_get_main_output(monitor);
    crtc = gd_output_get_assigned_crtc(output);
    crtc_config = gd_crtc_get_config(crtc);

    g_return_if_fail(crtc_config);

    *layout = crtc_config->layout;
}

static void
gd_monitor_normal_calculate_crtc_pos(GDMonitor* monitor, GDMonitorMode* monitor_mode, GDOutput* output, GDMonitorTransform crtc_transform, gint* out_x, gint* out_y)
{
    *out_x = 0;
    *out_y = 0;
}

static bool gd_monitor_normal_get_suggested_position(GDMonitor* monitor, gint* x, gint* y)
{
    const GDOutputInfo* output_info;

    output_info = gd_monitor_get_main_output_info(monitor);

    if (!output_info->hotplug_mode_update) return FALSE;

    if (output_info->suggested_x < 0 && output_info->suggested_y < 0) return FALSE;

    if (x != NULL) *x = output_info->suggested_x;

    if (y != NULL) *y = output_info->suggested_y;

    return TRUE;
}

static void
gd_monitor_normal_class_init(GDMonitorNormalClass* normal_class)
{
    GDMonitorClass* monitor_class;

    monitor_class = GD_MONITOR_CLASS(normal_class);

    monitor_class->get_main_output = gd_monitor_normal_get_main_output;
    monitor_class->derive_layout = gd_monitor_normal_derive_layout;
    monitor_class->calculate_crtc_pos = gd_monitor_normal_calculate_crtc_pos;
    monitor_class->get_suggested_position = gd_monitor_normal_get_suggested_position;
}

static void
gd_monitor_normal_init(GDMonitorNormal* normal)
{
}

GDMonitorNormal*
gd_monitor_normal_new(GDMonitorManager* monitor_manager, GDOutput* output)
{
    GDBackend* backend;
    GDMonitorNormal* normal;
    GDMonitor* monitor;

    backend = gd_monitor_manager_get_backend(monitor_manager);
    normal = g_object_new(GD_TYPE_MONITOR_NORMAL, "backend", backend, NULL);

    monitor = GD_MONITOR(normal);

    gd_monitor_append_output(monitor, output);
    gd_output_set_monitor(output, monitor);

    gd_monitor_set_winsys_id(monitor, gd_output_get_id(output));
    gd_monitor_generate_spec(monitor);
    generate_modes(normal);

    gd_monitor_make_display_name(monitor);

    return normal;
}
