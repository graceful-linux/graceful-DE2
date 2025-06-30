//
// Created by dingjing on 25-6-28.
//
#include <gio/gio.h>
#include <math.h>

#include "gd-crtc-private.h"
#include "gd-output-private.h"
#include "gd-monitor-manager.h"
#include "gd-monitor-private.h"
#include "gd-backend-private.h"
#include "gd-crtc-mode-private.h"
#include "gd-rectangle-private.h"
#include "gd-monitor-config-utils.h"
#include "gd-monitor-spec-private.h"
#include "gd-monitor-config-private.h"
#include "gd-monitor-config-store-private.h"
#include "gd-monitor-config-manager-private.h"
#include "gd-logical-monitor-config-private.h"


#define CONFIG_HISTORY_MAX_SIZE 3

typedef struct
{
    GDMonitorManager* monitor_manager;
    GDMonitorsConfig* config;
    GDLogicalMonitorConfig* logical_monitor_config;
    GDMonitorConfig* monitor_config;
    GPtrArray* crtc_assignments;
    GPtrArray* output_assignments;
    GArray* reserved_crtcs;
} MonitorAssignmentData;

typedef enum
{
    MONITOR_MATCH_ALL = 0,
    MONITOR_MATCH_EXTERNAL = (1 << 0),
    MONITOR_MATCH_BUILTIN = (1 << 1),
    MONITOR_MATCH_VISIBLE = (1 << 2),
    MONITOR_MATCH_WITH_SUGGESTED_POSITION = (1 << 3),
    MONITOR_MATCH_PRIMARY = (1 << 4),
    MONITOR_MATCH_ALLOW_FALLBACK = (1 << 5)
} MonitorMatchRule;

typedef enum { MONITOR_POSITIONING_LINEAR, MONITOR_POSITIONING_SUGGESTED, } MonitorPositioningMode;

struct _GDMonitorConfigManager
{
    GObject parent;

    GDMonitorManager* monitor_manager;

    GDMonitorConfigStore* config_store;

    GDMonitorsConfig* current_config;
    GQueue config_history;
};

G_DEFINE_TYPE(GDMonitorConfigManager, gd_monitor_config_manager, G_TYPE_OBJECT)

static GDMonitorsConfig*
get_root_config(GDMonitorsConfig* config)
{
    if (config->parent_config == NULL) return config;

    return get_root_config(config->parent_config);
}

static gboolean
has_same_root_config(GDMonitorsConfig* config_a, GDMonitorsConfig* config_b)
{
    return get_root_config(config_a) == get_root_config(config_b);
}

static void
history_unref(gpointer data, gpointer user_data)
{
    g_object_unref(data);
}

static gboolean
is_lid_closed(GDMonitorManager* monitor_manager)
{
    GDBackend* backend;

    backend = gd_monitor_manager_get_backend(monitor_manager);

    return gd_backend_is_lid_closed(backend);
}

static gboolean
monitor_matches_rule(GDMonitor* monitor, GDMonitorManager* monitor_manager, MonitorMatchRule match_rule)
{
    if (monitor == NULL) return FALSE;

    if (match_rule & MONITOR_MATCH_BUILTIN) {
        if (!gd_monitor_is_laptop_panel(monitor)) return FALSE;
    }
    else if (match_rule & MONITOR_MATCH_EXTERNAL) {
        if (gd_monitor_is_laptop_panel(monitor)) return FALSE;
    }

    if (match_rule & MONITOR_MATCH_VISIBLE) {
        if (gd_monitor_is_laptop_panel(monitor) && is_lid_closed(monitor_manager)) return FALSE;
    }

    if (match_rule & MONITOR_MATCH_WITH_SUGGESTED_POSITION) {
        if (!gd_monitor_get_suggested_position(monitor, NULL, NULL)) return FALSE;
    }

    return TRUE;
}

static GList*
find_monitors(GDMonitorManager* monitor_manager, MonitorMatchRule match_rule, GDMonitor* not_this_one)
{
    GList* result;
    GList* monitors;
    GList* l;

    result = NULL;
    monitors = gd_monitor_manager_get_monitors(monitor_manager);

    for (l = g_list_last(monitors); l; l = l->prev) {
        GDMonitor* monitor = l->data;

        if (not_this_one != NULL && monitor == not_this_one) continue;

        if (monitor_matches_rule(monitor, monitor_manager, match_rule)) result = g_list_prepend(result, monitor);
    }

    return result;
}

static GDMonitor*
find_monitor_with_highest_preferred_resolution(GDMonitorManager* monitor_manager, MonitorMatchRule match_rule)
{
    GList* monitors;
    GList* l;
    int largest_area = 0;
    GDMonitor* largest_monitor = NULL;

    monitors = find_monitors(monitor_manager, match_rule, NULL);

    for (l = monitors; l; l = l->next) {
        GDMonitor* monitor = l->data;
        GDMonitorMode* mode;
        int width, height;
        int area;

        mode = gd_monitor_get_preferred_mode(monitor);
        gd_monitor_mode_get_resolution(mode, &width, &height);
        area = width * height;

        if (area > largest_area) {
            largest_area = area;
            largest_monitor = monitor;
        }
    }

    g_clear_pointer(&monitors, g_list_free);

    return largest_monitor;
}

/*
 * Try to find the primary monitor. The priority of classification is:
 *
 * 1. Find the primary monitor as reported by the underlying system,
 * 2. Find the laptop panel
 * 3. Find the external monitor with highest resolution
 *
 * If the laptop lid is closed, exclude the laptop panel from possible
 * alternatives, except if no other alternatives exist.
 */
static GDMonitor*
find_primary_monitor(GDMonitorManager* monitor_manager, MonitorMatchRule match_rule)
{
    GDMonitor* monitor;

    monitor = gd_monitor_manager_get_primary_monitor(monitor_manager);

    if (monitor_matches_rule(monitor, monitor_manager, match_rule)) return monitor;

    monitor = gd_monitor_manager_get_laptop_panel(monitor_manager);

    if (monitor_matches_rule(monitor, monitor_manager, match_rule)) return monitor;

    monitor = find_monitor_with_highest_preferred_resolution(monitor_manager, match_rule);

    if (monitor != NULL) return monitor;

    if (match_rule & MONITOR_MATCH_ALLOW_FALLBACK)
        return find_monitor_with_highest_preferred_resolution(monitor_manager, MONITOR_MATCH_ALL);

    return NULL;
}

static GDMonitorTransform
get_monitor_transform(GDMonitorManager* monitor_manager, GDMonitor* monitor)
{
    GDBackend* backend;
    GDOrientationManager* orientation_manager;
    GDOrientation orientation;

    if (!gd_monitor_is_laptop_panel(monitor) || !gd_monitor_manager_get_panel_orientation_managed(monitor_manager)) {
        return GD_MONITOR_TRANSFORM_NORMAL;
    }

    backend = gd_monitor_manager_get_backend(monitor_manager);
    orientation_manager = gd_backend_get_orientation_manager(backend);
    orientation = gd_orientation_manager_get_orientation(orientation_manager);

    return gd_monitor_transform_from_orientation(orientation);
}

static void
scale_logical_monitor_width(GDLogicalMonitorLayoutMode layout_mode, float scale, int mode_width, int mode_height, int* width, int* height)
{
    switch (layout_mode) {
    case GD_LOGICAL_MONITOR_LAYOUT_MODE_LOGICAL:
        *width = (int)roundf(mode_width / scale);
        *height = (int)roundf(mode_height / scale);
        return;
    case GD_LOGICAL_MONITOR_LAYOUT_MODE_PHYSICAL:
        *width = mode_width;
        *height = mode_height;
        return;

    default:
        break;
    }

    g_assert_not_reached();
}

static GDLogicalMonitorConfig*
create_preferred_logical_monitor_config(GDMonitorManager* monitor_manager, GDMonitor* monitor, int x, int y, float scale, GDLogicalMonitorLayoutMode layout_mode)
{
    GDMonitorMode* mode;
    int width, height;
    GDMonitorTransform transform;
    GDMonitorConfig* monitor_config;
    GDLogicalMonitorConfig* logical_monitor_config;

    mode = gd_monitor_get_preferred_mode(monitor);
    gd_monitor_mode_get_resolution(mode, &width, &height);

    scale_logical_monitor_width(layout_mode, scale, width, height, &width, &height);

    monitor_config = gd_monitor_config_new(monitor, mode);
    transform = get_monitor_transform(monitor_manager, monitor);

    if (gd_monitor_transform_is_rotated(transform)) {
        int temp = width;

        width = height;
        height = temp;
    }

    logical_monitor_config = g_new0(GDLogicalMonitorConfig, 1);
    *logical_monitor_config = (GDLogicalMonitorConfig)
    {
        .
        layout = (GDRectangle)
        {
            .
            x = x,
            .
            y = y,
            .
            width = width,
            .
            height = height
        }
        ,
        .
        transform = transform,
        .
        scale = scale,
        .
        monitor_configs = g_list_append(NULL, monitor_config)
    };

    return logical_monitor_config;
}

static GDLogicalMonitorConfig*
find_monitor_config(GDMonitorsConfig* config, GDMonitor* monitor, GDMonitorMode* monitor_mode)
{
    int mode_width;
    int mode_height;
    GList* l;

    gd_monitor_mode_get_resolution(monitor_mode, &mode_width, &mode_height);

    for (l = config->logical_monitor_configs; l; l = l->next) {
        GDLogicalMonitorConfig* logical_monitor_config;
        GList* l_monitor;

        logical_monitor_config = l->data;

        for (l_monitor = logical_monitor_config->monitor_configs; l_monitor; l_monitor = l_monitor->next) {
            GDMonitorConfig* monitor_config;
            GDMonitorModeSpec* mode_spec;

            monitor_config = l_monitor->data;
            mode_spec = gd_monitor_mode_get_spec(monitor_mode);

            if (gd_monitor_spec_equals(gd_monitor_get_spec(monitor), monitor_config->monitor_spec) && gd_monitor_mode_spec_has_similar_size(mode_spec, monitor_config->mode_spec)) return logical_monitor_config;
        }
    }

    return NULL;
}

static gboolean
get_last_scale_for_monitor(GDMonitorConfigManager* config_manager, GDMonitor* monitor, GDMonitorMode* monitor_mode, float* out_scale)
{
    GList* configs;
    GList* l;

    configs = NULL;

    if (config_manager->current_config != NULL) configs = g_list_append(configs, config_manager->current_config);

    configs = g_list_concat(configs, g_list_copy(config_manager->config_history.head));

    for (l = configs; l; l = l->next) {
        GDMonitorsConfig* config;
        GDLogicalMonitorConfig* logical_monitor_config;

        config = l->data;
        logical_monitor_config = find_monitor_config(config, monitor, monitor_mode);

        if (logical_monitor_config != NULL) {
            *out_scale = logical_monitor_config->scale;
            g_list_free(configs);

            return TRUE;
        }
    }

    g_list_free(configs);

    return FALSE;
}


static float
compute_scale_for_monitor(GDMonitorConfigManager* config_manager, GDMonitor* monitor, GDMonitor* primary_monitor)

{
    GDMonitorManager* monitor_manager;
    GDMonitor* target_monitor;
    GDMonitorManagerCapability capabilities;
    GDLogicalMonitorLayoutMode layout_mode;
    GDMonitorMode* monitor_mode;
    float scale;

    monitor_manager = config_manager->monitor_manager;
    target_monitor = monitor;
    capabilities = gd_monitor_manager_get_capabilities(monitor_manager);

    if ((capabilities & GD_MONITOR_MANAGER_CAPABILITY_GLOBAL_SCALE_REQUIRED) && primary_monitor != NULL) {
        target_monitor = primary_monitor;
    }

    layout_mode = gd_monitor_manager_get_default_layout_mode(monitor_manager);
    monitor_mode = gd_monitor_get_preferred_mode(target_monitor);

    if (get_last_scale_for_monitor(config_manager, target_monitor, monitor_mode, &scale)) return scale;

    return gd_monitor_manager_calculate_monitor_mode_scale(monitor_manager, layout_mode, target_monitor, monitor_mode);
}

static GDMonitorsConfig*
create_for_switch_config_all_mirror(GDMonitorConfigManager* config_manager)
{
    GDMonitorManager* monitor_manager = config_manager->monitor_manager;
    GDMonitor* primary_monitor;
    GDLogicalMonitorLayoutMode layout_mode;
    GDLogicalMonitorConfig* logical_monitor_config = NULL;
    GList* logical_monitor_configs;
    GList* monitor_configs = NULL;
    gint common_mode_w = 0, common_mode_h = 0;
    float best_scale = 1.0;
    GDMonitor* monitor;
    GList* modes;
    GList* monitors;
    GList* l;
    GDMonitorsConfig* monitors_config;
    int width;
    int height;

    primary_monitor = find_primary_monitor(monitor_manager, MONITOR_MATCH_ALLOW_FALLBACK);

    if (primary_monitor == NULL) return NULL;

    layout_mode = gd_monitor_manager_get_default_layout_mode(monitor_manager);
    monitors = gd_monitor_manager_get_monitors(monitor_manager);
    monitor = monitors->data;
    modes = gd_monitor_get_modes(monitor);
    for (l = modes; l; l = l->next) {
        GDMonitorMode* mode = l->data;
        gboolean common_mode_size = TRUE;
        gint mode_w, mode_h;
        GList* ll;

        gd_monitor_mode_get_resolution(mode, &mode_w, &mode_h);

        for (ll = monitors->next; ll; ll = ll->next) {
            GDMonitor* monitor_b = ll->data;
            gboolean have_same_mode_size = FALSE;
            GList* mm;

            for (mm = gd_monitor_get_modes(monitor_b); mm; mm = mm->next) {
                GDMonitorMode* mode_b = mm->data;
                gint mode_b_w, mode_b_h;

                gd_monitor_mode_get_resolution(mode_b, &mode_b_w, &mode_b_h);

                if (mode_w == mode_b_w && mode_h == mode_b_h) {
                    have_same_mode_size = TRUE;
                    break;
                }
            }

            if (!have_same_mode_size) {
                common_mode_size = FALSE;
                break;
            }
        }

        if (common_mode_size && common_mode_w * common_mode_h < mode_w * mode_h) {
            common_mode_w = mode_w;
            common_mode_h = mode_h;
        }
    }

    if (common_mode_w == 0 || common_mode_h == 0) return NULL;

    for (l = monitors; l; l = l->next) {
        GDMonitor* l_monitor = l->data;
        GDMonitorMode* mode = NULL;
        GList* ll;
        float scale;

        for (ll = gd_monitor_get_modes(l_monitor); ll; ll = ll->next) {
            gint mode_w, mode_h;

            mode = ll->data;
            gd_monitor_mode_get_resolution(mode, &mode_w, &mode_h);

            if (mode_w == common_mode_w && mode_h == common_mode_h) break;
        }

        if (!mode) continue;

        scale = compute_scale_for_monitor(config_manager, l_monitor, primary_monitor);

        best_scale = MAX(best_scale, scale);
        monitor_configs = g_list_prepend(monitor_configs, gd_monitor_config_new(l_monitor, mode));
    }

    scale_logical_monitor_width(layout_mode, best_scale, common_mode_w, common_mode_h, &width, &height);

    logical_monitor_config = g_new0(GDLogicalMonitorConfig, 1);
    *logical_monitor_config = (GDLogicalMonitorConfig)
    {
        .
        layout = (GDRectangle)
        {
            .
            x = 0,
            .
            y = 0,
            .
            width = width,
            .
            height = height
        }
        ,
        .
        scale = best_scale,
        .
        monitor_configs = monitor_configs,
        .
        is_primary = TRUE
    };

    logical_monitor_configs = g_list_append(NULL, logical_monitor_config);

    monitors_config = gd_monitors_config_new(monitor_manager, logical_monitor_configs, layout_mode, GD_MONITORS_CONFIG_FLAG_NONE);

    if (monitors_config)
        gd_monitors_config_set_switch_config(monitors_config, GD_MONITOR_SWITCH_CONFIG_ALL_MIRROR);

    return monitors_config;
}

static gboolean
verify_suggested_monitors_config(GList* logical_monitor_configs)
{
    GList* region;
    GList* l;

    region = NULL;

    for (l = logical_monitor_configs; l; l = l->next) {
        GDLogicalMonitorConfig* logical_monitor_config = l->data;
        GDRectangle* rect = &logical_monitor_config->layout;

        if (gd_rectangle_overlaps_with_region(region, rect)) {
            g_warning("Suggested monitor config has overlapping region, " "rejecting");

            g_list_free(region);

            return FALSE;
        }

        region = g_list_prepend(region, rect);
    }

    for (l = region; region->next && l; l = l->next) {
        GDRectangle* rect = l->data;

        if (!gd_rectangle_is_adjacent_to_any_in_region(region, rect)) {
            g_warning("Suggested monitor config has monitors with no " "neighbors, rejecting");

            g_list_free(region);

            return FALSE;
        }
    }

    g_list_free(region);

    return TRUE;
}

static GDMonitorsConfig*
create_monitors_config(GDMonitorConfigManager* config_manager, MonitorMatchRule match_rule, MonitorPositioningMode positioning, GDMonitorsConfigFlag config_flags)
{
    GDMonitorManager* monitor_manager = config_manager->monitor_manager;
    GDMonitor* primary_monitor;
    GDLogicalMonitorLayoutMode layout_mode;
    GList* logical_monitor_configs;
    float scale;
    int x, y;
    GList* monitors;
    GList* l;

    primary_monitor = find_primary_monitor(monitor_manager, match_rule | MONITOR_MATCH_VISIBLE);

    if (!primary_monitor) return NULL;

    x = y = 0;
    layout_mode = gd_monitor_manager_get_default_layout_mode(monitor_manager);
    logical_monitor_configs = NULL;

    monitors = NULL;
    if (!(match_rule & MONITOR_MATCH_PRIMARY)) monitors = find_monitors(monitor_manager, match_rule, primary_monitor);

    /*
     * The primary monitor needs to be at the head of the list for the
     * linear positioning to be correct.
     */
    monitors = g_list_prepend(monitors, primary_monitor);

    for (l = monitors; l; l = l->next) {
        GDMonitor* monitor = l->data;
        GDLogicalMonitorConfig* logical_monitor_config;
        gboolean has_suggested_position;

        switch (positioning) {
        case MONITOR_POSITIONING_SUGGESTED:
            has_suggested_position = gd_monitor_get_suggested_position(monitor, &x, &y);
            g_assert(has_suggested_position);
            break;

        case MONITOR_POSITIONING_LINEAR: default:
            break;
        }

        scale = compute_scale_for_monitor(config_manager, monitor, primary_monitor);

        logical_monitor_config = create_preferred_logical_monitor_config(monitor_manager, monitor, x, y, scale, layout_mode);

        logical_monitor_config->is_primary = (monitor == primary_monitor);
        logical_monitor_configs = g_list_append(logical_monitor_configs, logical_monitor_config);

        x += logical_monitor_config->layout.width;
    }

    g_clear_pointer(&monitors, g_list_free);

    if (positioning == MONITOR_POSITIONING_SUGGESTED) {
        if (!verify_suggested_monitors_config(logical_monitor_configs)) {
            g_list_free_full(logical_monitor_configs, (GDestroyNotify)gd_logical_monitor_config_free);

            return NULL;
        }
    }

    return gd_monitors_config_new(monitor_manager, logical_monitor_configs, layout_mode, config_flags);
}

static GDMonitorsConfig*
create_monitors_switch_config(GDMonitorConfigManager* config_manager, MonitorMatchRule match_rule, MonitorPositioningMode positioning, GDMonitorsConfigFlag config_flags, GDMonitorSwitchConfigType switch_config)
{
    GDMonitorsConfig* monitors_config;

    monitors_config = create_monitors_config(config_manager, match_rule, positioning, config_flags);

    if (monitors_config == NULL) return NULL;

    gd_monitors_config_set_switch_config(monitors_config, switch_config);

    return monitors_config;
}

static GDMonitorsConfig*
create_for_switch_config_external(GDMonitorConfigManager* config_manager)
{
    return create_monitors_switch_config(config_manager, MONITOR_MATCH_EXTERNAL, MONITOR_POSITIONING_LINEAR, GD_MONITORS_CONFIG_FLAG_NONE, GD_MONITOR_SWITCH_CONFIG_EXTERNAL);
}

static GDMonitorsConfig*
create_for_switch_config_builtin(GDMonitorConfigManager* config_manager)
{
    return create_monitors_switch_config(config_manager, MONITOR_MATCH_BUILTIN, MONITOR_POSITIONING_LINEAR, GD_MONITORS_CONFIG_FLAG_NONE, GD_MONITOR_SWITCH_CONFIG_BUILTIN);
}

static GDLogicalMonitorConfig*
find_logical_config_for_builtin_monitor(GDMonitorConfigManager* config_manager, GList* logical_monitor_configs)
{
    GDMonitor* panel;
    GList* l;

    panel = gd_monitor_manager_get_laptop_panel(config_manager->monitor_manager);

    if (panel == NULL) return NULL;

    for (l = logical_monitor_configs; l; l = l->next) {
        GDLogicalMonitorConfig* logical_monitor_config;
        GDMonitorConfig* monitor_config;

        logical_monitor_config = l->data;

        /*
         * We only want to return the config for the panel if it is
         * configured on its own, so we skip configs which contain clones.
         */
        if (g_list_length(logical_monitor_config->monitor_configs) != 1) continue;

        monitor_config = logical_monitor_config->monitor_configs->data;
        if (gd_monitor_spec_equals(gd_monitor_get_spec(panel), monitor_config->monitor_spec)) {
            GDMonitorMode* mode;

            mode = gd_monitor_get_mode_from_spec(panel, monitor_config->mode_spec);

            if (mode != NULL) return logical_monitor_config;
        }
    }

    return NULL;
}


static GDMonitorsConfig*
create_for_builtin_display_rotation(GDMonitorConfigManager* config_manager, GDMonitorsConfig* base_config, gboolean rotate, GDMonitorTransform transform)
{
    GDMonitorManager* monitor_manager = config_manager->monitor_manager;
    GDLogicalMonitorConfig* logical_monitor_config;
    GDLogicalMonitorConfig* current_logical_monitor_config;
    GDMonitorsConfig* config;
    GList* logical_monitor_configs;
    GList* current_configs;
    GDLogicalMonitorLayoutMode layout_mode;
    GList* current_monitor_configs;

    g_return_val_if_fail(base_config, NULL);

    current_configs = base_config->logical_monitor_configs;
    current_logical_monitor_config = find_logical_config_for_builtin_monitor(config_manager, current_configs);

    if (!current_logical_monitor_config) return NULL;

    if (rotate) transform = (current_logical_monitor_config->transform + 1) % GD_MONITOR_TRANSFORM_FLIPPED;
    else {
        GDMonitor* panel;

        /*
         * The transform coming from the accelerometer should be applied to
         * the crtc as is, without taking panel-orientation into account, this
         * is done so that non panel-orientation aware desktop environments do the
         * right thing. Mutter corrects for panel-orientation when applying the
         * transform from a logical-monitor-config, so we must convert here.
         */
        panel = gd_monitor_manager_get_laptop_panel(config_manager->monitor_manager);
        transform = gd_monitor_crtc_to_logical_transform(panel, transform);
    }

    if (current_logical_monitor_config->transform == transform) return NULL;

    current_monitor_configs = base_config->logical_monitor_configs;
    logical_monitor_configs = gd_clone_logical_monitor_config_list(current_monitor_configs);

    logical_monitor_config = find_logical_config_for_builtin_monitor(config_manager, logical_monitor_configs);
    logical_monitor_config->transform = transform;

    if (gd_monitor_transform_is_rotated(current_logical_monitor_config->transform) != gd_monitor_transform_is_rotated(logical_monitor_config->transform)) {
        gint temp = logical_monitor_config->layout.width;

        logical_monitor_config->layout.width = logical_monitor_config->layout.height;
        logical_monitor_config->layout.height = temp;
    }

    layout_mode = base_config->layout_mode;

    config = gd_monitors_config_new(monitor_manager, logical_monitor_configs, layout_mode, GD_MONITORS_CONFIG_FLAG_NONE);

    gd_monitors_config_set_parent_config(config, base_config);

    return config;
}

static bool
is_crtc_reserved(GDCrtc* crtc, GArray* reserved_crtcs)
{
    unsigned int i;

    for (i = 0; i < reserved_crtcs->len; i++) {
        uint64_t id;

        id = g_array_index(reserved_crtcs, uint64_t, i);

        if (id == gd_crtc_get_id(crtc)) return TRUE;
    }

    return FALSE;
}

static gboolean
is_crtc_assigned(GDCrtc* crtc, GPtrArray* crtc_assignments)
{
    unsigned int i;

    for (i = 0; i < crtc_assignments->len; i++) {
        GDCrtcAssignment* assigned_crtc_assignment;

        assigned_crtc_assignment = g_ptr_array_index(crtc_assignments, i);

        if (assigned_crtc_assignment->crtc == crtc) return TRUE;
    }

    return FALSE;
}

static GDCrtc*
find_unassigned_crtc(GDOutput* output, GPtrArray* crtc_assignments, GArray* reserved_crtcs)
{
    GDCrtc* crtc;
    const GDOutputInfo* output_info;
    unsigned int i;

    crtc = gd_output_get_assigned_crtc(output);
    if (crtc && !is_crtc_assigned(crtc, crtc_assignments)) return crtc;

    output_info = gd_output_get_info(output);

    /* then try to assign a CRTC that wasn't used */
    for (i = 0; i < output_info->n_possible_crtcs; i++) {
        crtc = output_info->possible_crtcs[i];

        if (is_crtc_assigned(crtc, crtc_assignments)) continue;

        if (is_crtc_reserved(crtc, reserved_crtcs)) continue;

        return crtc;
    }

    /* finally just give a CRTC that we haven't assigned */
    for (i = 0; i < output_info->n_possible_crtcs; i++) {
        crtc = output_info->possible_crtcs[i];

        if (is_crtc_assigned(crtc, crtc_assignments)) continue;

        return crtc;
    }

    return NULL;
}

static gboolean
assign_monitor_crtc(GDMonitor* monitor, GDMonitorMode* mode, GDMonitorCrtcMode* monitor_crtc_mode, gpointer user_data, GError** error)
{
    MonitorAssignmentData* data = user_data;
    GDOutput* output;
    GDCrtc* crtc;
    GDMonitorTransform transform;
    GDMonitorTransform crtc_transform;
    GDMonitorTransform crtc_hw_transform;
    int crtc_x, crtc_y;
    float x_offset, y_offset;
    float scale;
    float width, height;
    GDCrtcMode* crtc_mode;
    const GDCrtcModeInfo* crtc_mode_info;
    GDRectangle crtc_layout;
    GDCrtcAssignment* crtc_assignment;
    GDOutputAssignment* output_assignment;
    GDMonitorConfig* first_monitor_config;
    gboolean assign_output_as_primary;
    gboolean assign_output_as_presentation;

    output = monitor_crtc_mode->output;
    crtc = find_unassigned_crtc(output, data->crtc_assignments, data->reserved_crtcs);

    if (!crtc) {
        GDMonitorSpec* monitor_spec = gd_monitor_get_spec(monitor);

        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "No available CRTC for monitor '%s %s' not found", monitor_spec->vendor, monitor_spec->product);

        return FALSE;
    }

    transform = data->logical_monitor_config->transform;
    crtc_transform = gd_monitor_logical_to_crtc_transform(monitor, transform);
    if (gd_monitor_manager_is_transform_handled(data->monitor_manager, crtc, crtc_transform)) crtc_hw_transform = crtc_transform;
    else crtc_hw_transform = GD_MONITOR_TRANSFORM_NORMAL;

    gd_monitor_calculate_crtc_pos(monitor, mode, output, crtc_transform, &crtc_x, &crtc_y);

    x_offset = data->logical_monitor_config->layout.x;
    y_offset = data->logical_monitor_config->layout.y;

    switch (data->config->layout_mode) {
    case GD_LOGICAL_MONITOR_LAYOUT_MODE_LOGICAL:
        scale = data->logical_monitor_config->scale;
        break;

    case GD_LOGICAL_MONITOR_LAYOUT_MODE_PHYSICAL: default:
        scale = 1.0;
        break;
    }

    crtc_mode = monitor_crtc_mode->crtcMode;
    crtc_mode_info = gd_crtc_mode_get_info(crtc_mode);

    if (gd_monitor_transform_is_rotated(crtc_transform)) {
        width = crtc_mode_info->height / scale;
        height = crtc_mode_info->width / scale;
    }
    else {
        width = crtc_mode_info->width / scale;
        height = crtc_mode_info->height / scale;
    }

    crtc_layout.x = (int)roundf(x_offset + (crtc_x / scale));
    crtc_layout.y = (int)roundf(y_offset + (crtc_y / scale));
    crtc_layout.width = (int)roundf(width);
    crtc_layout.height = (int)roundf(height);

    crtc_assignment = g_new0(GDCrtcAssignment, 1);
    *crtc_assignment = (GDCrtcAssignment)
    {
        .
        crtc = crtc,
        .
        mode = crtc_mode,
        .
        layout = crtc_layout,
        .
        transform = crtc_hw_transform,
        .
        outputs = g_ptr_array_new()
    };
    g_ptr_array_add(crtc_assignment->outputs, output);

    /*
     * Only one output can be marked as primary (due to Xrandr limitation),
     * so only mark the main output of the first monitor in the logical monitor
     * as such.
     */
    first_monitor_config = data->logical_monitor_config->monitor_configs->data;
    if (data->logical_monitor_config->is_primary && data->monitor_config == first_monitor_config && gd_monitor_get_main_output(monitor) == output) assign_output_as_primary = TRUE;
    else assign_output_as_primary = FALSE;

    if (data->logical_monitor_config->is_presentation) assign_output_as_presentation = TRUE;
    else assign_output_as_presentation = FALSE;

    output_assignment = g_new0(GDOutputAssignment, 1);
    *output_assignment = (GDOutputAssignment)
    {
        .
        output = output,
        .
        is_primary = assign_output_as_primary,
        .
        is_presentation = assign_output_as_presentation,
        .
        is_underscanning = data->monitor_config->enable_underscanning,
        .
        has_max_bpc = data->monitor_config->has_max_bpc,
        .
        max_bpc = data->monitor_config->max_bpc
    };

    g_ptr_array_add(data->crtc_assignments, crtc_assignment);
    g_ptr_array_add(data->output_assignments, output_assignment);

    return TRUE;
}

static gboolean
assign_monitor_crtcs(GDMonitorManager* manager, GDMonitorsConfig* config, GDLogicalMonitorConfig* logical_monitor_config, GDMonitorConfig* monitor_config, GPtrArray* crtc_assignments, GPtrArray* output_assignments, GArray* reserved_crtcs,
                     GError** error)
{
    GDMonitorSpec* monitor_spec = monitor_config->monitor_spec;
    GDMonitorModeSpec* monitor_mode_spec = monitor_config->mode_spec;
    GDMonitor* monitor;
    GDMonitorMode* monitor_mode;
    MonitorAssignmentData data;

    monitor = gd_monitor_manager_get_monitor_from_spec(manager, monitor_spec);
    if (!monitor) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Configured monitor '%s %s' not found", monitor_spec->vendor, monitor_spec->product);

        return FALSE;
    }

    monitor_mode = gd_monitor_get_mode_from_spec(monitor, monitor_mode_spec);
    if (!monitor_mode) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Invalid mode %dx%d (%.3f) for monitor '%s %s'",
            monitor_mode_spec->width, monitor_mode_spec->height, (gdouble)monitor_mode_spec->refreshRate, monitor_spec->vendor,
            monitor_spec->product);

        return FALSE;
    }

    data = (MonitorAssignmentData){
        .monitor_manager = manager, .config = config, .logical_monitor_config = logical_monitor_config, .monitor_config = monitor_config, .crtc_assignments = crtc_assignments, .output_assignments = output_assignments,
        .reserved_crtcs = reserved_crtcs
    };

    if (!gd_monitor_mode_foreach_crtc(monitor, monitor_mode, (GDMonitorModeFunc)assign_monitor_crtc, &data, error)) return FALSE;

    return TRUE;
}

static gboolean
assign_logical_monitor_crtcs(GDMonitorManager* manager, GDMonitorsConfig* config, GDLogicalMonitorConfig* logical_monitor_config, GPtrArray* crtc_assignments, GPtrArray* output_assignments, GArray* reserved_crtcs, GError** error)
{
    GList* l;

    for (l = logical_monitor_config->monitor_configs; l; l = l->next) {
        GDMonitorConfig* monitor_config = l->data;

        if (!assign_monitor_crtcs(manager, config, logical_monitor_config, monitor_config, crtc_assignments, output_assignments, reserved_crtcs, error)) return FALSE;
    }

    return TRUE;
}

GDMonitorsConfigKey*
gd_create_monitors_config_key_for_current_state(GDMonitorManager* monitor_manager)
{
    GDMonitorsConfigKey* config_key;
    GDMonitorSpec* laptop_monitor_spec;
    GList* l;
    GList* monitor_specs;

    laptop_monitor_spec = NULL;
    monitor_specs = NULL;
    for (l = monitor_manager->monitors; l; l = l->next) {
        GDMonitor* monitor = l->data;
        GDMonitorSpec* monitor_spec;

        if (gd_monitor_is_laptop_panel(monitor)) {
            laptop_monitor_spec = gd_monitor_get_spec(monitor);

            if (is_lid_closed(monitor_manager)) continue;
        }

        monitor_spec = gd_monitor_spec_clone(gd_monitor_get_spec(monitor));
        monitor_specs = g_list_prepend(monitor_specs, monitor_spec);
    }

    if (!monitor_specs && laptop_monitor_spec) {
        monitor_specs = g_list_prepend(NULL, gd_monitor_spec_clone(laptop_monitor_spec));
    }

    if (!monitor_specs) return NULL;

    monitor_specs = g_list_sort(monitor_specs, (GCompareFunc)gd_monitor_spec_compare);

    config_key = g_new0(GDMonitorsConfigKey, 1);
    config_key->monitor_specs = monitor_specs;
    config_key->layout_mode = gd_monitor_manager_get_default_layout_mode(monitor_manager);

    return config_key;
}

static void
gd_crtc_assignment_free(GDCrtcAssignment* assignment)
{
    g_ptr_array_free(assignment->outputs, TRUE);
    g_free(assignment);
}

static void
gd_output_assignment_free(GDOutputAssignment* assignment)
{
    g_free(assignment);
}

static void
gd_monitor_config_manager_dispose(GObject* object)
{
    GDMonitorConfigManager* config_manager;

    config_manager = GD_MONITOR_CONFIG_MANAGER(object);

    g_clear_object(&config_manager->current_config);
    gd_monitor_config_manager_clear_history(config_manager);

    G_OBJECT_CLASS(gd_monitor_config_manager_parent_class)->dispose(object);
}

static void
gd_monitor_config_manager_class_init(GDMonitorConfigManagerClass* config_manager_class)
{
    GObjectClass* object_class;

    object_class = G_OBJECT_CLASS(config_manager_class);

    object_class->dispose = gd_monitor_config_manager_dispose;
}

static void
gd_monitor_config_manager_init(GDMonitorConfigManager* config_manager)
{
    g_queue_init(&config_manager->config_history);
}

GDMonitorConfigManager*
gd_monitor_config_manager_new(GDMonitorManager* monitor_manager)
{
    GDMonitorConfigManager* config_manager;

    config_manager = g_object_new(GD_TYPE_MONITOR_CONFIG_MANAGER, NULL);
    config_manager->monitor_manager = monitor_manager;
    config_manager->config_store = gd_monitor_config_store_new(monitor_manager);

    return config_manager;
}

GDMonitorConfigStore*
gd_monitor_config_manager_get_store(GDMonitorConfigManager* config_manager)
{
    return config_manager->config_store;
}

gboolean
gd_monitor_config_manager_assign(GDMonitorManager* manager, GDMonitorsConfig* config, GPtrArray** out_crtc_assignments, GPtrArray** out_output_assignments, GError** error)
{
    GPtrArray* crtc_assignments;
    GPtrArray* output_assignments;
    GArray* reserved_crtcs;
    GList* l;

    crtc_assignments = g_ptr_array_new_with_free_func((GDestroyNotify)gd_crtc_assignment_free);
    output_assignments = g_ptr_array_new_with_free_func((GDestroyNotify)gd_output_assignment_free);
    reserved_crtcs = g_array_new(FALSE, FALSE, sizeof (uint64_t));

    for (l = config->logical_monitor_configs; l; l = l->next) {
        GDLogicalMonitorConfig* logical_monitor_config = l->data;
        GList* k;

        for (k = logical_monitor_config->monitor_configs; k; k = k->next) {
            GDMonitorConfig* monitor_config = k->data;
            GDMonitorSpec* monitor_spec = monitor_config->monitor_spec;
            GDMonitor* monitor;
            GList* o;

            monitor = gd_monitor_manager_get_monitor_from_spec(manager, monitor_spec);

            for (o = gd_monitor_get_outputs(monitor); o; o = o->next) {
                GDOutput* output = o->data;
                GDCrtc* crtc;

                crtc = gd_output_get_assigned_crtc(output);
                if (crtc) {
                    uint64_t crtc_id;

                    crtc_id = gd_crtc_get_id(crtc);

                    g_array_append_val(reserved_crtcs, crtc_id);
                }
            }
        }
    }

    for (l = config->logical_monitor_configs; l; l = l->next) {
        GDLogicalMonitorConfig* logical_monitor_config = l->data;

        if (!assign_logical_monitor_crtcs(manager, config, logical_monitor_config, crtc_assignments, output_assignments, reserved_crtcs, error)) {
            g_ptr_array_free(crtc_assignments, TRUE);
            g_ptr_array_free(output_assignments, TRUE);
            g_array_free(reserved_crtcs, TRUE);
            return FALSE;
        }
    }

    *out_crtc_assignments = crtc_assignments;
    *out_output_assignments = output_assignments;

    g_array_free(reserved_crtcs, TRUE);

    return TRUE;
}

GDMonitorsConfig*
gd_monitor_config_manager_get_stored(GDMonitorConfigManager* config_manager)
{
    GDMonitorManager* monitor_manager = config_manager->monitor_manager;
    GDMonitorsConfigKey* config_key;
    GDMonitorsConfig* config;

    config_key = gd_create_monitors_config_key_for_current_state(monitor_manager);
    if (!config_key) return NULL;

    config = gd_monitor_config_store_lookup(config_manager->config_store, config_key);
    gd_monitors_config_key_free(config_key);

    return config;
}

GDMonitorsConfig*
gd_monitor_config_manager_create_linear(GDMonitorConfigManager* config_manager)
{
    return create_monitors_config(config_manager, MONITOR_MATCH_VISIBLE | MONITOR_MATCH_ALLOW_FALLBACK, MONITOR_POSITIONING_LINEAR, GD_MONITORS_CONFIG_FLAG_NONE);
}

GDMonitorsConfig*
gd_monitor_config_manager_create_fallback(GDMonitorConfigManager* config_manager)
{
    return create_monitors_config(config_manager, MONITOR_MATCH_PRIMARY | MONITOR_MATCH_ALLOW_FALLBACK, MONITOR_POSITIONING_LINEAR, GD_MONITORS_CONFIG_FLAG_NONE);
}

GDMonitorsConfig*
gd_monitor_config_manager_create_suggested(GDMonitorConfigManager* config_manager)
{
    return create_monitors_config(config_manager, MONITOR_MATCH_WITH_SUGGESTED_POSITION, MONITOR_POSITIONING_SUGGESTED, GD_MONITORS_CONFIG_FLAG_NONE);
}

GDMonitorsConfig*
gd_monitor_config_manager_create_for_orientation(GDMonitorConfigManager* config_manager, GDMonitorsConfig* base_config, GDMonitorTransform transform)
{
    return create_for_builtin_display_rotation(config_manager, base_config, FALSE, transform);
}

GDMonitorsConfig*
gd_monitor_config_manager_create_for_builtin_orientation(GDMonitorConfigManager* config_manager, GDMonitorsConfig* base_config)
{
    GDMonitorManager* monitor_manager;
    gboolean panel_orientation_managed;
    GDMonitorTransform current_transform;
    GDMonitor* laptop_panel;

    monitor_manager = config_manager->monitor_manager;
    panel_orientation_managed = gd_monitor_manager_get_panel_orientation_managed(monitor_manager);

    g_return_val_if_fail(panel_orientation_managed, NULL);

    laptop_panel = gd_monitor_manager_get_laptop_panel(monitor_manager);
    current_transform = get_monitor_transform(monitor_manager, laptop_panel);

    return create_for_builtin_display_rotation(config_manager, base_config, FALSE, current_transform);
}

GDMonitorsConfig*
gd_monitor_config_manager_create_for_rotate_monitor(GDMonitorConfigManager* config_manager)
{
    if (config_manager->current_config == NULL) return NULL;

    return create_for_builtin_display_rotation(config_manager, config_manager->current_config, TRUE, GD_MONITOR_TRANSFORM_NORMAL);
}

GDMonitorsConfig*
gd_monitor_config_manager_create_for_switch_config(GDMonitorConfigManager* config_manager, GDMonitorSwitchConfigType config_type)
{
    GDMonitorManager* monitor_manager;
    GDMonitorsConfig* config;

    monitor_manager = config_manager->monitor_manager;
    config = NULL;

    if (!gd_monitor_manager_can_switch_config(monitor_manager)) return NULL;

    switch (config_type) {
    case GD_MONITOR_SWITCH_CONFIG_ALL_MIRROR:
        config = create_for_switch_config_all_mirror(config_manager);
        break;

    case GD_MONITOR_SWITCH_CONFIG_ALL_LINEAR:
        config = gd_monitor_config_manager_create_linear(config_manager);
        break;

    case GD_MONITOR_SWITCH_CONFIG_EXTERNAL:
        config = create_for_switch_config_external(config_manager);
        break;

    case GD_MONITOR_SWITCH_CONFIG_BUILTIN:
        config = create_for_switch_config_builtin(config_manager);
        break;

    case GD_MONITOR_SWITCH_CONFIG_UNKNOWN: default: g_warn_if_reached();
        break;
    }

    return config;
}

void
gd_monitor_config_manager_set_current(GDMonitorConfigManager* config_manager, GDMonitorsConfig* config)
{
    GDMonitorsConfig* current_config;
    gboolean overrides_current;

    current_config = config_manager->current_config;
    overrides_current = FALSE;

    if (config != NULL && current_config != NULL && has_same_root_config(config, current_config)) {
        overrides_current = gd_monitors_config_key_equal(config->key, current_config->key);
    }

    if (current_config != NULL && !overrides_current) {
        g_queue_push_head(&config_manager->config_history, g_object_ref(config_manager->current_config));
        if (g_queue_get_length(&config_manager->config_history) > CONFIG_HISTORY_MAX_SIZE) g_object_unref(g_queue_pop_tail(&config_manager->config_history));
    }

    g_set_object(&config_manager->current_config, config);
}

GDMonitorsConfig*
gd_monitor_config_manager_get_current(GDMonitorConfigManager* config_manager)
{
    return config_manager->current_config;
}

GDMonitorsConfig*
gd_monitor_config_manager_pop_previous(GDMonitorConfigManager* config_manager)
{
    return g_queue_pop_head(&config_manager->config_history);
}

GDMonitorsConfig*
gd_monitor_config_manager_get_previous(GDMonitorConfigManager* config_manager)
{
    return g_queue_peek_head(&config_manager->config_history);
}

void
gd_monitor_config_manager_clear_history(GDMonitorConfigManager* config_manager)
{
    g_queue_foreach(&config_manager->config_history, history_unref, NULL);
    g_queue_clear(&config_manager->config_history);
}

void
gd_monitor_config_manager_save_current(GDMonitorConfigManager* config_manager)
{
    g_return_if_fail(config_manager->current_config);

    gd_monitor_config_store_add(config_manager->config_store, config_manager->current_config);
}

bool
gd_monitor_manager_is_monitor_visible(GDMonitorManager* monitor_manager, GDMonitor* monitor)
{
    return monitor_matches_rule(monitor, monitor_manager, MONITOR_MATCH_VISIBLE);
}
