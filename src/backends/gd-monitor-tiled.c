//
// Created by dingjing on 25-6-28.
//
#include "gd-monitor-tiled-private.h"

#include "gd-rectangle.h"
#include "macros/macros.h"
#include "gd-crtc-private.h"
#include "gd-output-private.h"
#include "gd-monitor-private.h"
#include "gd-crtc-mode-private.h"
#include "gd-monitor-spec-private.h"
#include "gd-monitor-manager-types-private.h"


typedef struct
{
    GDMonitorMode parent;
    bool is_tiled;
} GDMonitorModeTiled;

struct _GDMonitorTiled
{
    GDMonitor parent;

    GDMonitorManager* monitor_manager;

    uint32_t tile_group_id;

    /* The tile (0, 0) output. */
    GDOutput* origin_output;

    /* The output enabled even when a non-tiled mode is used. */
    GDOutput* main_output;
};

G_DEFINE_TYPE(GDMonitorTiled, gd_monitor_tiled, GD_TYPE_MONITOR)

static bool is_crtc_mode_tiled(GDOutput* output, GDCrtcMode* crtc_mode)
{
    const GDOutputInfo* output_info;
    const GDCrtcModeInfo* crtc_mode_info;

    output_info = gd_output_get_info(output);
    crtc_mode_info = gd_crtc_mode_get_info(crtc_mode);

    return (crtc_mode_info->width == (int)output_info->tile_info.tile_w && crtc_mode_info->height == (int)output_info->tile_info.tile_h);
}

static GDCrtcMode*
find_tiled_crtc_mode(GDOutput* output, GDCrtcMode* reference_crtc_mode)
{
    const GDOutputInfo* output_info;
    const GDCrtcModeInfo* reference_crtc_mode_info;
    GDCrtcMode* crtc_mode;
    guint i;

    output_info = gd_output_get_info(output);
    reference_crtc_mode_info = gd_crtc_mode_get_info(reference_crtc_mode);
    crtc_mode = output_info->preferred_mode;

    if (is_crtc_mode_tiled(output, crtc_mode)) return crtc_mode;

    for (i = 0; i < output_info->n_modes; i++) {
        const GDCrtcModeInfo* crtc_mode_info;

        crtc_mode = output_info->modes[i];
        crtc_mode_info = gd_crtc_mode_get_info(crtc_mode);

        if (!is_crtc_mode_tiled(output, crtc_mode)) continue;

        if (crtc_mode_info->refresh_rate != reference_crtc_mode_info->refresh_rate) continue;

        if (crtc_mode_info->flags != reference_crtc_mode_info->flags) continue;

        return crtc_mode;
    }

    return NULL;
}

static void
calculate_tiled_size(GDMonitor* monitor, gint* out_width, gint* out_height)
{
    GList* outputs;
    GList* l;
    gint width;
    gint height;

    outputs = gd_monitor_get_outputs(monitor);

    width = 0;
    height = 0;

    for (l = outputs; l; l = l->next) {
        const GDOutputInfo* output_info;

        output_info = gd_output_get_info(l->data);

        if (output_info->tile_info.loc_v_tile == 0) width += output_info->tile_info.tile_w;

        if (output_info->tile_info.loc_h_tile == 0) height += output_info->tile_info.tile_h;
    }

    *out_width = width;
    *out_height = height;
}

static GDMonitorMode*
create_tiled_monitor_mode(GDMonitorTiled* tiled, GDCrtcMode* reference_crtc_mode, bool* out_is_preferred)
{
    GDMonitor* monitor;
    GList* outputs;
    bool is_preferred;
    GDMonitorModeTiled* mode;
    gint width, height;
    GList* l;
    guint i;

    monitor = GD_MONITOR(tiled);
    outputs = gd_monitor_get_outputs(monitor);

    is_preferred = TRUE;

    mode = g_new0(GDMonitorModeTiled, 1);
    mode->is_tiled = TRUE;

    calculate_tiled_size(monitor, &width, &height);

    mode->parent.monitor = monitor;
    mode->parent.spec = gd_monitor_create_spec(monitor, width, height, reference_crtc_mode);

    mode->parent.id = gd_monitor_mode_spec_generate_id(&mode->parent.spec);
    mode->parent.crtcModes = g_new0(GDMonitorCrtcMode, g_list_length(outputs));

    for (l = outputs, i = 0; l; l = l->next, i++) {
        GDOutput* output;
        const GDOutputInfo* output_info;
        GDCrtcMode* tiled_crtc_mode;

        output = l->data;
        output_info = gd_output_get_info(output);
        tiled_crtc_mode = find_tiled_crtc_mode(output, reference_crtc_mode);

        if (!tiled_crtc_mode) {
            g_warning("No tiled mode found on %s", gd_output_get_name(output));
            gd_monitor_mode_free((GDMonitorMode*)mode);
            return NULL;
        }

        mode->parent.crtcModes[i].output = output;
        mode->parent.crtcModes[i].crtcMode = tiled_crtc_mode;

        is_preferred = (is_preferred && tiled_crtc_mode == output_info->preferred_mode);
    }

    *out_is_preferred = is_preferred;

    return (GDMonitorMode*)mode;
}

static GDMonitorMode*
create_untiled_monitor_mode(GDMonitorTiled* tiled, GDOutput* main_output, GDCrtcMode* crtc_mode)
{
    GDMonitor* monitor;
    GList* outputs;
    GDMonitorModeTiled* mode;
    const GDCrtcModeInfo* crtc_mode_info;
    GList* l;
    guint i;

    monitor = GD_MONITOR(tiled);
    outputs = gd_monitor_get_outputs(monitor);

    if (is_crtc_mode_tiled(main_output, crtc_mode)) return NULL;

    mode = g_new0(GDMonitorModeTiled, 1);
    mode->is_tiled = FALSE;

    mode->parent.monitor = monitor;

    crtc_mode_info = gd_crtc_mode_get_info(crtc_mode);
    mode->parent.spec = gd_monitor_create_spec(monitor, crtc_mode_info->width, crtc_mode_info->height, crtc_mode);

    mode->parent.id = gd_monitor_mode_spec_generate_id(&mode->parent.spec);
    mode->parent.crtcModes = g_new0(GDMonitorCrtcMode, g_list_length(outputs));

    for (l = outputs, i = 0; l; l = l->next, i++) {
        GDOutput* output = l->data;

        if (output == main_output) {
            mode->parent.crtcModes[i].output = output;
            mode->parent.crtcModes[i].crtcMode = crtc_mode;
        }
        else {
            mode->parent.crtcModes[i].output = output;
            mode->parent.crtcModes[i].crtcMode = NULL;
        }
    }

    return &mode->parent;
}

static void
generate_tiled_monitor_modes(GDMonitorTiled* tiled)
{
    GDMonitor* monitor;
    GDOutput* main_output;
    const GDOutputInfo* main_output_info;
    GList* tiled_modes;
    GDMonitorMode* best_mode;
    guint i;
    GList* l;

    monitor = GD_MONITOR(tiled);
    main_output = gd_monitor_get_main_output(monitor);
    main_output_info = gd_output_get_info(main_output);

    tiled_modes = NULL;
    best_mode = NULL;

    for (i = 0; i < main_output_info->n_modes; i++) {
        GDCrtcMode* reference_crtc_mode;
        GDMonitorMode* mode;
        bool is_preferred;

        reference_crtc_mode = main_output_info->modes[i];

        if (!is_crtc_mode_tiled(main_output, reference_crtc_mode)) continue;

        mode = create_tiled_monitor_mode(tiled, reference_crtc_mode, &is_preferred);

        if (!mode) continue;

        tiled_modes = g_list_append(tiled_modes, mode);

        if (gd_monitor_is_mode_assigned(monitor, mode)) gd_monitor_set_current_mode(monitor, mode);

        if (is_preferred) gd_monitor_set_preferred_mode(monitor, mode);
    }

    while ((l = tiled_modes)) {
        GDMonitorMode* mode;
        GDMonitorMode* preferred_mode;

        mode = l->data;
        tiled_modes = g_list_remove_link(tiled_modes, l);

        if (!gd_monitor_add_mode(monitor, mode, FALSE)) {
            gd_monitor_mode_free(mode);
            continue;
        }

        preferred_mode = gd_monitor_get_preferred_mode(monitor);
        if (!preferred_mode) {
            if (!best_mode || mode->spec.refreshRate > best_mode->spec.refreshRate) best_mode = mode;
        }
    }

    if (best_mode) gd_monitor_set_preferred_mode(monitor, best_mode);
}

static void
generate_untiled_monitor_modes(GDMonitorTiled* tiled)
{
    GDMonitor* monitor;
    GDOutput* main_output;
    const GDOutputInfo* main_output_info;
    guint i;

    monitor = GD_MONITOR(tiled);
    main_output = gd_monitor_get_main_output(monitor);
    main_output_info = gd_output_get_info(main_output);

    for (i = 0; i < main_output_info->n_modes; i++) {
        GDCrtcMode* crtc_mode;
        GDMonitorMode* mode;
        GDMonitorMode* preferred_mode;

        crtc_mode = main_output_info->modes[i];
        mode = create_untiled_monitor_mode(tiled, main_output, crtc_mode);

        if (!mode) continue;

        if (!gd_monitor_add_mode(monitor, mode, FALSE)) {
            gd_monitor_mode_free(mode);
            continue;
        }

        if (gd_monitor_is_mode_assigned(monitor, mode)) {
            GDMonitorMode* current_mode;

            current_mode = gd_monitor_get_current_mode(monitor);
            g_assert(!current_mode);

            gd_monitor_set_current_mode(monitor, mode);
        }

        preferred_mode = gd_monitor_get_preferred_mode(monitor);
        if (!preferred_mode && crtc_mode == main_output_info->preferred_mode) gd_monitor_set_preferred_mode(monitor, mode);
    }
}

static GDMonitorMode*
find_best_mode(GDMonitor* monitor)
{
    GList* modes;
    GDMonitorMode* best_mode;
    GList* l;

    modes = gd_monitor_get_modes(monitor);
    best_mode = NULL;

    for (l = modes; l; l = l->next) {
        GDMonitorMode* mode;
        gint area, best_area;

        mode = l->data;

        if (!best_mode) {
            best_mode = mode;
            continue;
        }

        area = mode->spec.width * mode->spec.height;
        best_area = best_mode->spec.width * best_mode->spec.height;
        if (area > best_area) {
            best_mode = mode;
            continue;
        }

        if (mode->spec.refreshRate > best_mode->spec.refreshRate) {
            best_mode = mode;
            continue;
        }
    }

    return best_mode;
}

static void
generate_modes(GDMonitorTiled* tiled)
{
    GDMonitor* monitor;

    monitor = GD_MONITOR(tiled);

    generate_tiled_monitor_modes(tiled);

    if (!gd_monitor_get_preferred_mode(monitor)) {
        GDMonitorSpec* spec;
        spec = gd_monitor_get_spec(monitor);
        g_warning("Tiled monitor on %s didn't have any tiled modes", spec->connector);
    }

    generate_untiled_monitor_modes(tiled);

    if (!gd_monitor_get_preferred_mode(monitor)) {
        GDMonitorSpec* spec;

        spec = gd_monitor_get_spec(monitor);

        g_warning("Tiled monitor on %s didn't have a valid preferred mode", spec->connector);

        gd_monitor_set_preferred_mode(monitor, find_best_mode(monitor));
    }
}

static int
count_untiled_crtc_modes(GDOutput* output)
{
    const GDOutputInfo* output_info;
    gint count;
    guint i;

    output_info = gd_output_get_info(output);

    count = 0;
    for (i = 0; i < output_info->n_modes; i++) {
        GDCrtcMode* crtc_mode;

        crtc_mode = output_info->modes[i];
        if (!is_crtc_mode_tiled(output, crtc_mode)) count++;
    }

    return count;
}

static GDOutput*
find_untiled_output(GDMonitorTiled* tiled)
{
    GDMonitor* monitor;
    GList* outputs;
    GDOutput* best_output;
    gint best_mode_count;
    GList* l;

    monitor = GD_MONITOR(tiled);
    outputs = gd_monitor_get_outputs(monitor);

    best_output = tiled->origin_output;
    best_mode_count = count_untiled_crtc_modes(tiled->origin_output);

    for (l = outputs; l; l = l->next) {
        GDOutput* output;
        gint mode_count;

        output = l->data;
        if (output == tiled->origin_output) continue;

        mode_count = count_untiled_crtc_modes(output);
        if (mode_count > best_mode_count) {
            best_mode_count = mode_count;
            best_output = output;
        }
    }

    return best_output;
}

static void
add_tiled_monitor_outputs(GDGpu* gpu, GDMonitorTiled* tiled)
{
    GDMonitor* monitor;
    GList* outputs;
    GList* l;

    monitor = GD_MONITOR(tiled);

    outputs = gd_gpu_get_outputs(gpu);
    for (l = outputs; l; l = l->next) {
        GDOutput* output;
        const GDOutputInfo* output_info;

        output = l->data;
        output_info = gd_output_get_info(output);

        if (output_info->tile_info.group_id != tiled->tile_group_id) continue;

        gd_monitor_append_output(monitor, output);
        gd_output_set_monitor(output, GD_MONITOR(tiled));
    }
}

static void
calculate_tile_coordinate(GDMonitor* monitor, GDOutput* output, GDMonitorTransform crtc_transform, gint* out_x, gint* out_y)
{
    const GDOutputInfo* output_info;
    GList* outputs;
    GList* l;
    gint x;
    gint y;

    output_info = gd_output_get_info(output);
    outputs = gd_monitor_get_outputs(monitor);
    x = y = 0;

    for (l = outputs; l; l = l->next) {
        const GDOutputInfo* other_output_info;

        other_output_info = gd_output_get_info(l->data);

        switch (crtc_transform) {
        case GD_MONITOR_TRANSFORM_NORMAL:
        case GD_MONITOR_TRANSFORM_FLIPPED:
            if (other_output_info->tile_info.loc_v_tile == output_info->tile_info.loc_v_tile && other_output_info->tile_info.loc_h_tile < output_info->tile_info.loc_h_tile) x += other_output_info->tile_info.tile_w;
            if (other_output_info->tile_info.loc_h_tile == output_info->tile_info.loc_h_tile && other_output_info->tile_info.loc_v_tile < output_info->tile_info.loc_v_tile) y += other_output_info->tile_info.tile_h;
            break;

        case GD_MONITOR_TRANSFORM_180:
        case GD_MONITOR_TRANSFORM_FLIPPED_180:
            if (other_output_info->tile_info.loc_v_tile == output_info->tile_info.loc_v_tile && other_output_info->tile_info.loc_h_tile > output_info->tile_info.loc_h_tile) x += other_output_info->tile_info.tile_w;
            if (other_output_info->tile_info.loc_h_tile == output_info->tile_info.loc_h_tile && other_output_info->tile_info.loc_v_tile > output_info->tile_info.loc_v_tile) y += other_output_info->tile_info.tile_h;
            break;

        case GD_MONITOR_TRANSFORM_270:
        case GD_MONITOR_TRANSFORM_FLIPPED_270:
            if (other_output_info->tile_info.loc_v_tile == output_info->tile_info.loc_v_tile && other_output_info->tile_info.loc_h_tile > output_info->tile_info.loc_h_tile) y += other_output_info->tile_info.tile_w;
            if (other_output_info->tile_info.loc_h_tile == output_info->tile_info.loc_h_tile && other_output_info->tile_info.loc_v_tile > output_info->tile_info.loc_v_tile) x += other_output_info->tile_info.tile_h;
            break;

        case GD_MONITOR_TRANSFORM_90:
        case GD_MONITOR_TRANSFORM_FLIPPED_90:
            if (other_output_info->tile_info.loc_v_tile == output_info->tile_info.loc_v_tile && other_output_info->tile_info.loc_h_tile < output_info->tile_info.loc_h_tile) y += other_output_info->tile_info.tile_w;
            if (other_output_info->tile_info.loc_h_tile == output_info->tile_info.loc_h_tile && other_output_info->tile_info.loc_v_tile < output_info->tile_info.loc_v_tile) x += other_output_info->tile_info.tile_h;
            break;

        default:
            break;
        }
    }

    *out_x = x;
    *out_y = y;
}

static void
gd_monitor_tiled_finalize(GObject* object)
{
    GDMonitorTiled* self;

    self = GD_MONITOR_TILED(object);

    gd_monitor_manager_tiled_monitor_removed(self->monitor_manager, GD_MONITOR(self));

    G_OBJECT_CLASS(gd_monitor_tiled_parent_class)->finalize(object);
}

static GDOutput*
gd_monitor_tiled_get_main_output(GDMonitor* monitor)
{
    GDMonitorTiled* tiled;

    tiled = GD_MONITOR_TILED(monitor);

    return tiled->main_output;
}

static void
gd_monitor_tiled_derive_layout(GDMonitor* monitor, GDRectangle* layout)
{
    GList* outputs;
    GList* l;
    gint min_x;
    gint min_y;
    gint max_x;
    gint max_y;

    outputs = gd_monitor_get_outputs(monitor);

    min_x = INT_MAX;
    min_y = INT_MAX;
    max_x = 0;
    max_y = 0;

    for (l = outputs; l; l = l->next) {
        GDOutput* output;
        GDCrtc* crtc;
        const GDCrtcConfig* crtc_config;

        output = l->data;
        crtc = gd_output_get_assigned_crtc(output);

        if (!crtc) continue;

        crtc_config = gd_crtc_get_config(crtc);
        g_return_if_fail(crtc_config);

        min_x = MIN(crtc_config->layout.x, min_x);
        min_y = MIN(crtc_config->layout.y, min_y);
        max_x = MAX(crtc_config->layout.x + crtc_config->layout.width, max_x);
        max_y = MAX(crtc_config->layout.y + crtc_config->layout.height, max_y);
    }

    *layout = (GDRectangle)
    {
        .
        x = min_x,
        .
        y = min_y,
        .
        width = max_x - min_x,
        .
        height = max_y - min_y
    };
}

static void
gd_monitor_tiled_calculate_crtc_pos(GDMonitor* monitor, GDMonitorMode* monitor_mode, GDOutput* output, GDMonitorTransform crtc_transform, gint* out_x, gint* out_y)
{
    GDMonitorModeTiled* mode_tiled;

    mode_tiled = (GDMonitorModeTiled*)monitor_mode;

    if (mode_tiled->is_tiled) {
        calculate_tile_coordinate(monitor, output, crtc_transform, out_x, out_y);
    }
    else {
        *out_x = 0;
        *out_y = 0;
    }
}

static bool
gd_monitor_tiled_get_suggested_position(GDMonitor* monitor, gint* x, gint* y)
{
    return FALSE;
}

static void
gd_monitor_tiled_class_init(GDMonitorTiledClass* tiled_class)
{
    GObjectClass* object_class;
    GDMonitorClass* monitor_class;

    object_class = G_OBJECT_CLASS(tiled_class);
    monitor_class = GD_MONITOR_CLASS(tiled_class);

    object_class->finalize = gd_monitor_tiled_finalize;

    monitor_class->get_main_output = gd_monitor_tiled_get_main_output;
    monitor_class->derive_layout = gd_monitor_tiled_derive_layout;
    monitor_class->calculate_crtc_pos = gd_monitor_tiled_calculate_crtc_pos;
    monitor_class->get_suggested_position = gd_monitor_tiled_get_suggested_position;
}

static void
gd_monitor_tiled_init(GDMonitorTiled* tiled)
{
}

GDMonitorTiled*
gd_monitor_tiled_new(GDMonitorManager* monitor_manager, GDOutput* output)
{
    const GDOutputInfo* output_info;
    GDBackend* backend;
    GDMonitorTiled* tiled;
    GDMonitor* monitor;

    output_info = gd_output_get_info(output);

    backend = gd_monitor_manager_get_backend(monitor_manager);
    tiled = g_object_new(GD_TYPE_MONITOR_TILED, "backend", backend, NULL);

    monitor = GD_MONITOR(tiled);

    tiled->monitor_manager = monitor_manager;

    tiled->tile_group_id = output_info->tile_info.group_id;
    gd_monitor_set_winsys_id(monitor, gd_output_get_id(output));

    tiled->origin_output = output;
    add_tiled_monitor_outputs(gd_output_get_gpu(output), tiled);

    tiled->main_output = find_untiled_output(tiled);

    gd_monitor_generate_spec(monitor);

    gd_monitor_manager_tiled_monitor_added(monitor_manager, monitor);
    generate_modes(tiled);

    gd_monitor_make_display_name(monitor);

    return tiled;
}

uint32_t
gd_monitor_tiled_get_tile_group_id(GDMonitorTiled* tiled)
{
    return tiled->tile_group_id;
}
