//
// Created by dingjing on 25-6-28.
//
#include "gd-monitor-private.h"

#include <math.h>
#include <gio/gio.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "gd-backend.h"
#include "gd-output-private.h"
#include "gd-settings-private.h"
#include "gd-crtc-mode-private.h"
#include "gd-monitor-spec-private.h"


#define SCALE_FACTORS_PER_INTEGER 4
#define SCALE_FACTORS_STEPS (1.0f / (float) SCALE_FACTORS_PER_INTEGER)
#define MINIMUM_SCALE_FACTOR 1.0f
#define MAXIMUM_SCALE_FACTOR 4.0f
#define MINIMUM_LOGICAL_AREA (800 * 480)
#define MAXIMUM_REFRESH_RATE_DIFF 0.001f


#define HIDPI_LIMIT 192
#define HIDPI_MIN_HEIGHT 1200
#define SMALLEST_4K_WIDTH 3656


typedef struct
{
    GDBackend* backend;

    GList* outputs;
    GList* modes;
    GHashTable* mode_ids;

    GDMonitorMode* preferred_mode;
    GDMonitorMode* current_mode;
    GDMonitorSpec* spec;
    GDLogicalMonitor* logical_monitor;
    glong winsys_id;
    char* display_name;
} GDMonitorPrivate;

enum { PROP_0, PROP_BACKEND, LAST_PROP };

static GParamSpec* monitor_properties[LAST_PROP] = {NULL};

G_DEFINE_TYPE_WITH_PRIVATE(GDMonitor, gd_monitor, G_TYPE_OBJECT)

static const gdouble known_diagonals[] = {12.1, 13.3, 15.6};

static gchar* diagonal_to_str(gdouble d)
{
    guint i;

    for (i = 0; i < G_N_ELEMENTS(known_diagonals); i++) {
        gdouble delta;

        delta = C_ABS(known_diagonals[i] - d);
        if (delta < 0.1) return g_strdup_printf("%0.1lf\"", known_diagonals[i]);
    }

    return g_strdup_printf("%d\"", (int)(d + 0.5));
}

static gchar* make_display_name(GDMonitor* monitor, GDMonitorManager* manager)
{
    gchar* inches;
    gchar* vendor_name;
    const char* vendor;
    const char* product_name;
    int width_mm;
    int height_mm;

    if (gd_monitor_is_laptop_panel(monitor)) return g_strdup(_("Built-in display"));

    inches = NULL;
    vendor_name = NULL;
    vendor = gd_monitor_get_vendor(monitor);
    product_name = NULL;

    gd_monitor_get_physical_dimensions(monitor, &width_mm, &height_mm);

    if (width_mm > 0 && height_mm > 0) {
        if (!gd_monitor_has_aspect_as_size(monitor)) {
            double d;

            d = sqrt(width_mm * width_mm + height_mm * height_mm);
            inches = diagonal_to_str(d / 25.4);
        }
        else {
            product_name = gd_monitor_get_product(monitor);
        }
    }

    if (g_strcmp0(vendor, "unknown") != 0) {
        vendor_name = gd_monitor_manager_get_vendor_name(manager, vendor);

        if (!vendor_name) vendor_name = g_strdup(vendor);
    }
    else {
        if (inches != NULL) vendor_name = g_strdup(_("Unknown"));
        else vendor_name = g_strdup(_("Unknown Display"));
    }

    if (inches != NULL) {
        gchar* display_name;

        display_name = g_strdup_printf(C_("This is a monitor vendor name, followed by a " "size in inches, like 'Dell 15\"'", "%s %s"), vendor_name, inches);

        g_free(vendor_name);
        g_free(inches);

        return display_name;
    }
    else if (product_name != NULL) {
        gchar* display_name;

        display_name = g_strdup_printf(C_("This is a monitor vendor name followed by " "product/model name where size in inches " "could not be calculated, e.g. Dell U2414H", "%s %s"), vendor_name, product_name);

        g_free(vendor_name);

        return display_name;
    }

    return vendor_name;
}

static bool is_current_mode_known(GDMonitor* monitor)
{
    GDOutput* output;
    GDCrtc* crtc;

    output = gd_monitor_get_main_output(monitor);
    crtc = gd_output_get_assigned_crtc(output);

    return gd_monitor_is_active(monitor) == (crtc && gd_crtc_get_config(crtc));
}

static bool gd_monitor_mode_spec_equals(GDMonitorModeSpec* spec, GDMonitorModeSpec* other_spec)
{
    gfloat refresh_rate_diff;

    refresh_rate_diff = ABS(spec->refreshRate - other_spec->refreshRate);

    return (spec->width == other_spec->width && spec->height == other_spec->height && refresh_rate_diff < MAXIMUM_REFRESH_RATE_DIFF && spec->flags == other_spec->flags);
}

static float calculate_scale(GDMonitor* monitor, GDMonitorMode* monitor_mode, GDMonitorScalesConstraint constraints)
{
    gint resolution_width, resolution_height;
    gint width_mm, height_mm;
    gint scale;

    scale = 1.0;

    gd_monitor_mode_get_resolution(monitor_mode, &resolution_width, &resolution_height);

    if (resolution_height < HIDPI_MIN_HEIGHT) return scale;

    /* 4K TV */
    switch (gd_monitor_get_connector_type(monitor)) {
    case GD_CONNECTOR_TYPE_HDMIA:
    case GD_CONNECTOR_TYPE_HDMIB:
        if (resolution_width < SMALLEST_4K_WIDTH) return scale;
        break;

    case GD_CONNECTOR_TYPE_Unknown:
    case GD_CONNECTOR_TYPE_VGA:
    case GD_CONNECTOR_TYPE_DVII:
    case GD_CONNECTOR_TYPE_DVID:
    case GD_CONNECTOR_TYPE_DVIA:
    case GD_CONNECTOR_TYPE_Composite:
    case GD_CONNECTOR_TYPE_SVIDEO:
    case GD_CONNECTOR_TYPE_LVDS:
    case GD_CONNECTOR_TYPE_Component:
    case GD_CONNECTOR_TYPE_9PinDIN:
    case GD_CONNECTOR_TYPE_DisplayPort:
    case GD_CONNECTOR_TYPE_TV:
    case GD_CONNECTOR_TYPE_eDP:
    case GD_CONNECTOR_TYPE_VIRTUAL:
    case GD_CONNECTOR_TYPE_DSI:
    case GD_CONNECTOR_TYPE_DPI:
    case GD_CONNECTOR_TYPE_WRITEBACK:
    case GD_CONNECTOR_TYPE_SPI:
    case GD_CONNECTOR_TYPE_USB: default:
        break;
    }

    gd_monitor_get_physical_dimensions(monitor, &width_mm, &height_mm);
    if (gd_monitor_has_aspect_as_size(monitor)) return scale;

    if (width_mm > 0 && height_mm > 0) {
        gdouble dpi_x, dpi_y;

        dpi_x = (gdouble)resolution_width / (width_mm / 25.4);
        dpi_y = (gdouble)resolution_height / (height_mm / 25.4);

        /*
         * We don't completely trust these values so both must be high, and never
         * pick higher ratio than 2 automatically.
         */
        if (dpi_x > HIDPI_LIMIT && dpi_y > HIDPI_LIMIT) scale = 2.0;
    }

    return scale;
}

static bool is_logical_size_large_enough(gint width, gint height)
{
    return width * height >= MINIMUM_LOGICAL_AREA;
}

static bool is_scale_valid_for_size(float width, float height, float scale)
{
    if (scale < MINIMUM_SCALE_FACTOR || scale > MAXIMUM_SCALE_FACTOR) return FALSE;

    return is_logical_size_large_enough(floorf(width / scale), floorf(height / scale));
}

float gd_get_closest_monitor_scale_factor_for_resolution(float width, float height, float scale, float threshold)
{
    guint i, j;
    gfloat scaled_h;
    gfloat scaled_w;
    gfloat best_scale;
    gint base_scaled_w;
    gboolean found_one;

    best_scale = 0;

    if (fmodf(width, scale) == 0.0f && fmodf(height, scale) == 0.0f) return scale;

    i = 0;
    found_one = FALSE;
    base_scaled_w = floorf(width / scale);

    do {
        for (j = 0; j < 2; j++) {
            gfloat current_scale;
            gint offset = i * (j ? 1 : -1);

            scaled_w = base_scaled_w + offset;
            current_scale = width / scaled_w;
            scaled_h = height / current_scale;

            if (current_scale >= scale + threshold || current_scale <= scale - threshold || current_scale < MINIMUM_SCALE_FACTOR || current_scale > MAXIMUM_SCALE_FACTOR) {
                return best_scale;
            }

            if (floorf(scaled_h) == scaled_h) {
                found_one = TRUE;

                if (fabsf(current_scale - scale) < fabsf(best_scale - scale)) best_scale = current_scale;
            }
        }

        i++;
    } while (!found_one);

    return best_scale;
}

static void unset_monitor(gpointer data, gpointer uData)
{
    GDOutput* output;

    output = GD_OUTPUT(data);

    gd_output_unset_monitor(output);
}

static void gd_monitor_dispose(GObject* object)
{
    GDMonitor* monitor;
    GDMonitorPrivate* priv;

    monitor = GD_MONITOR(object);
    priv = gd_monitor_get_instance_private(monitor);

    if (priv->outputs) {
        g_list_foreach(priv->outputs, unset_monitor, NULL);
        g_list_free_full(priv->outputs, g_object_unref);
        priv->outputs = NULL;
    }

    G_OBJECT_CLASS(gd_monitor_parent_class)->dispose(object);
}

static void gd_monitor_finalize(GObject* object)
{
    GDMonitor* monitor;
    GDMonitorPrivate* priv;

    monitor = GD_MONITOR(object);
    priv = gd_monitor_get_instance_private(monitor);

    g_hash_table_destroy(priv->mode_ids);
    g_list_free_full(priv->modes, (GDestroyNotify)gd_monitor_mode_free);
    gd_monitor_spec_free(priv->spec);
    g_free(priv->display_name);

    G_OBJECT_CLASS(gd_monitor_parent_class)->finalize(object);
}

static void gd_monitor_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec)
{
    GDMonitor* monitor;
    GDMonitorPrivate* priv;

    monitor = GD_MONITOR(object);
    priv = gd_monitor_get_instance_private(monitor);

    switch (property_id) {
    case PROP_BACKEND:
        g_value_set_object(value, priv->backend);
        break;

    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void gd_monitor_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec)
{
    GDMonitor* monitor;
    GDMonitorPrivate* priv;

    monitor = GD_MONITOR(object);
    priv = gd_monitor_get_instance_private(monitor);

    switch (property_id) {
    case PROP_BACKEND:
        priv->backend = g_value_get_object(value);
        break;

    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void gd_monitor_install_properties(GObjectClass* object_class)
{
    monitor_properties[PROP_BACKEND] = g_param_spec_object("backend", "GDBackend", "GDBackend", GD_TYPE_BACKEND, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties(object_class, LAST_PROP, monitor_properties);
}

static void gd_monitor_class_init(GDMonitorClass* monitor_class)
{
    GObjectClass* object_class;

    object_class = G_OBJECT_CLASS(monitor_class);

    object_class->dispose = gd_monitor_dispose;
    object_class->finalize = gd_monitor_finalize;
    object_class->get_property = gd_monitor_get_property;
    object_class->set_property = gd_monitor_set_property;

    gd_monitor_install_properties(object_class);
}

static void gd_monitor_init(GDMonitor* monitor)
{
    GDMonitorPrivate* priv;

    priv = gd_monitor_get_instance_private(monitor);

    priv->mode_ids = g_hash_table_new(g_str_hash, g_str_equal);
}

GDBackend* gd_monitor_get_backend(GDMonitor* monitor)
{
    GDMonitorPrivate* priv;

    priv = gd_monitor_get_instance_private(monitor);

    return priv->backend;
}

void gd_monitor_make_display_name(GDMonitor* monitor)
{
    GDMonitorPrivate* priv;
    GDMonitorManager* manager;

    priv = gd_monitor_get_instance_private(monitor);
    manager = gd_backend_get_monitor_manager(priv->backend);

    g_free(priv->display_name);
    priv->display_name = make_display_name(monitor, manager);
}

const char* gd_monitor_get_display_name(GDMonitor* monitor)
{
    GDMonitorPrivate* priv;

    priv = gd_monitor_get_instance_private(monitor);

    return priv->display_name;
}

bool gd_monitor_is_mode_assigned(GDMonitor* monitor, GDMonitorMode* mode)
{
    GDMonitorPrivate* priv;
    GList* l;
    gint i;

    priv = gd_monitor_get_instance_private(monitor);

    for (l = priv->outputs, i = 0; l; l = l->next, i++) {
        GDOutput* output;
        GDMonitorCrtcMode* monitor_crtc_mode;
        GDCrtc* crtc;
        const GDCrtcConfig* crtc_config;

        output = l->data;
        monitor_crtc_mode = &mode->crtcModes[i];
        crtc = gd_output_get_assigned_crtc(output);
        crtc_config = crtc ? gd_crtc_get_config(crtc) : NULL;

        if (monitor_crtc_mode->crtcMode && (!crtc || !crtc_config || crtc_config->mode != monitor_crtc_mode->crtcMode)) return FALSE;
        else if (!monitor_crtc_mode->crtcMode && crtc) return FALSE;
    }

    return TRUE;
}

void gd_monitor_append_output(GDMonitor* monitor, GDOutput* output)
{
    GDMonitorPrivate* priv;

    priv = gd_monitor_get_instance_private(monitor);

    priv->outputs = g_list_append(priv->outputs, g_object_ref(output));
}

void gd_monitor_set_winsys_id(GDMonitor* monitor, glong winsys_id)
{
    GDMonitorPrivate* priv;

    priv = gd_monitor_get_instance_private(monitor);

    priv->winsys_id = winsys_id;
}

void gd_monitor_set_preferred_mode(GDMonitor* monitor, GDMonitorMode* mode)
{
    GDMonitorPrivate* priv;

    priv = gd_monitor_get_instance_private(monitor);

    priv->preferred_mode = mode;
}

GDMonitorModeSpec gd_monitor_create_spec(GDMonitor* monitor, int width, int height, GDCrtcMode* crtc_mode)
{
    const GDOutputInfo* output_info;
    const GDCrtcModeInfo* crtc_mode_info;
    GDMonitorModeSpec spec;

    output_info = gd_monitor_get_main_output_info(monitor);
    crtc_mode_info = gd_crtc_mode_get_info(crtc_mode);

    if (gd_monitor_transform_is_rotated(output_info->panel_orientation_transform)) {
        int temp;

        temp = width;
        width = height;
        height = temp;
    }

    spec.width = width;
    spec.height = height;
    spec.refreshRate = crtc_mode_info->refresh_rate;
    spec.flags = crtc_mode_info->flags & HANDLED_CRTC_MODE_FLAGS;

    return spec;
}

const GDOutputInfo* gd_monitor_get_main_output_info(GDMonitor* self)
{
    GDOutput* output;

    output = gd_monitor_get_main_output(self);

    return gd_output_get_info(output);
}

void gd_monitor_generate_spec(GDMonitor* monitor)
{
    GDMonitorPrivate* priv;
    const GDOutputInfo* output_info;
    GDMonitorSpec* monitor_spec;

    priv = gd_monitor_get_instance_private(monitor);
    output_info = gd_monitor_get_main_output_info(monitor);

    monitor_spec = g_new0(GDMonitorSpec, 1);

    monitor_spec->connector = g_strdup(output_info->name);
    monitor_spec->vendor = g_strdup(output_info->vendor);
    monitor_spec->product = g_strdup(output_info->product);
    monitor_spec->serial = g_strdup(output_info->serial);

    priv->spec = monitor_spec;
}

bool gd_monitor_add_mode(GDMonitor* monitor, GDMonitorMode* monitor_mode, bool replace)
{
    GDMonitorPrivate* priv;
    GDMonitorMode* existing_mode;

    priv = gd_monitor_get_instance_private(monitor);

    existing_mode = g_hash_table_lookup(priv->mode_ids, gd_monitor_mode_get_id(monitor_mode));

    if (existing_mode && !replace) return FALSE;

    if (existing_mode) priv->modes = g_list_remove(priv->modes, existing_mode);

    priv->modes = g_list_append(priv->modes, monitor_mode);
    g_hash_table_replace(priv->mode_ids, monitor_mode->id, monitor_mode);

    return TRUE;
}

void gd_monitor_mode_free(GDMonitorMode* monitor_mode)
{
    g_free(monitor_mode->id);
    g_free(monitor_mode->crtcModes);
    g_free(monitor_mode);
}

gchar* gd_monitor_mode_spec_generate_id(GDMonitorModeSpec* spec)
{
    gboolean is_interlaced;

    is_interlaced = !!(spec->flags & GD_CRTC_MODE_FLAG_INTERLACE);

    return g_strdup_printf("%dx%d%s@%.3f", spec->width, spec->height, is_interlaced ? "i" : "", (double)spec->refreshRate);
}

GDMonitorSpec* gd_monitor_get_spec(GDMonitor* monitor)
{
    GDMonitorPrivate* priv;

    priv = gd_monitor_get_instance_private(monitor);

    return priv->spec;
}

bool gd_monitor_is_active(GDMonitor* monitor)
{
    GDMonitorPrivate* priv;

    priv = gd_monitor_get_instance_private(monitor);

    return !!priv->current_mode;
}

GDOutput* gd_monitor_get_main_output(GDMonitor* monitor)
{
    return GD_MONITOR_GET_CLASS(monitor)->get_main_output(monitor);
}

bool gd_monitor_is_primary(GDMonitor* monitor)
{
    GDOutput* output;

    output = gd_monitor_get_main_output(monitor);

    return gd_output_is_primary(output);
}

bool gd_monitor_supports_underscanning(GDMonitor* monitor)
{
    const GDOutputInfo* output_info;

    output_info = gd_monitor_get_main_output_info(monitor);

    return output_info->supports_underscanning;
}

bool gd_monitor_is_underscanning(GDMonitor* monitor)
{
    GDOutput* output;

    output = gd_monitor_get_main_output(monitor);

    return gd_output_is_underscanning(output);
}

bool gd_monitor_get_max_bpc(GDMonitor* self, unsigned int* max_bpc)
{
    GDOutput* output;

    output = gd_monitor_get_main_output(self);

    return gd_output_get_max_bpc(output, max_bpc);
}

bool gd_monitor_is_laptop_panel(GDMonitor* monitor)
{
    const GDOutputInfo* output_info;

    output_info = gd_monitor_get_main_output_info(monitor);

    switch (output_info->connector_type) {
    case GD_CONNECTOR_TYPE_eDP:
    case GD_CONNECTOR_TYPE_LVDS:
    case GD_CONNECTOR_TYPE_DSI:
        return TRUE;

    case GD_CONNECTOR_TYPE_HDMIA:
    case GD_CONNECTOR_TYPE_HDMIB:
    case GD_CONNECTOR_TYPE_Unknown:
    case GD_CONNECTOR_TYPE_VGA:
    case GD_CONNECTOR_TYPE_DVII:
    case GD_CONNECTOR_TYPE_DVID:
    case GD_CONNECTOR_TYPE_DVIA:
    case GD_CONNECTOR_TYPE_Composite:
    case GD_CONNECTOR_TYPE_SVIDEO:
    case GD_CONNECTOR_TYPE_Component:
    case GD_CONNECTOR_TYPE_9PinDIN:
    case GD_CONNECTOR_TYPE_DisplayPort:
    case GD_CONNECTOR_TYPE_TV:
    case GD_CONNECTOR_TYPE_VIRTUAL:
    case GD_CONNECTOR_TYPE_DPI:
    case GD_CONNECTOR_TYPE_WRITEBACK:
    case GD_CONNECTOR_TYPE_SPI:
    case GD_CONNECTOR_TYPE_USB: default:
        break;
    }

    return FALSE;
}

bool gd_monitor_is_same_as(GDMonitor* monitor, GDMonitor* other_monitor)
{
    GDMonitorPrivate* priv;
    GDMonitorPrivate* other_priv;

    priv = gd_monitor_get_instance_private(monitor);
    other_priv = gd_monitor_get_instance_private(other_monitor);

    return priv->winsys_id == other_priv->winsys_id;
}

GList* gd_monitor_get_outputs(GDMonitor* monitor)
{
    GDMonitorPrivate* priv;

    priv = gd_monitor_get_instance_private(monitor);

    return priv->outputs;
}

void gd_monitor_get_current_resolution(GDMonitor* monitor, gint* width, gint* height)
{
    GDMonitorMode* mode = gd_monitor_get_current_mode(monitor);

    *width = mode->spec.width;
    *height = mode->spec.height;
}

void gd_monitor_derive_layout(GDMonitor* monitor, GDRectangle* layout)
{
    GD_MONITOR_GET_CLASS(monitor)->derive_layout(monitor, layout);
}

void gd_monitor_get_physical_dimensions(GDMonitor* monitor, gint* width_mm, gint* height_mm)
{
    const GDOutputInfo* output_info;

    output_info = gd_monitor_get_main_output_info(monitor);

    *width_mm = output_info->width_mm;
    *height_mm = output_info->height_mm;
}

const gchar* gd_monitor_get_connector(GDMonitor* monitor)
{
    const GDOutputInfo* output_info;

    output_info = gd_monitor_get_main_output_info(monitor);

    return output_info->name;
}

const gchar* gd_monitor_get_vendor(GDMonitor* monitor)
{
    const GDOutputInfo* output_info;

    output_info = gd_monitor_get_main_output_info(monitor);

    return output_info->vendor;
}

const gchar* gd_monitor_get_product(GDMonitor* monitor)
{
    const GDOutputInfo* output_info;

    output_info = gd_monitor_get_main_output_info(monitor);

    return output_info->product;
}

const gchar* gd_monitor_get_serial(GDMonitor* monitor)
{
    const GDOutputInfo* output_info;

    output_info = gd_monitor_get_main_output_info(monitor);

    return output_info->serial;
}

GDConnectorType gd_monitor_get_connector_type(GDMonitor* monitor)
{
    const GDOutputInfo* output_info;

    output_info = gd_monitor_get_main_output_info(monitor);

    return output_info->connector_type;
}

GDMonitorTransform gd_monitor_logical_to_crtc_transform(GDMonitor* monitor, GDMonitorTransform transform)
{
    GDOutput* output;

    output = gd_monitor_get_main_output(monitor);

    return gd_output_logical_to_crtc_transform(output, transform);
}

GDMonitorTransform gd_monitor_crtc_to_logical_transform(GDMonitor* monitor, GDMonitorTransform transform)
{
    GDOutput* output;

    output = gd_monitor_get_main_output(monitor);

    return gd_output_crtc_to_logical_transform(output, transform);
}

bool gd_monitor_get_suggested_position(GDMonitor* monitor, gint* x, gint* y)
{
    return GD_MONITOR_GET_CLASS(monitor)->get_suggested_position(monitor, x, y);
}

GDLogicalMonitor* gd_monitor_get_logical_monitor(GDMonitor* monitor)
{
    GDMonitorPrivate* priv;

    priv = gd_monitor_get_instance_private(monitor);

    return priv->logical_monitor;
}

GDMonitorMode* gd_monitor_get_mode_from_id(GDMonitor* monitor, const gchar* monitor_mode_id)
{
    GDMonitorPrivate* priv;

    priv = gd_monitor_get_instance_private(monitor);

    return g_hash_table_lookup(priv->mode_ids, monitor_mode_id);
}

bool gd_monitor_mode_spec_has_similar_size(GDMonitorModeSpec* monitor_mode_spec, GDMonitorModeSpec* other_monitor_mode_spec)
{
    const float target_ratio = 1.0;
    const float epsilon = 0.15;

    return G_APPROX_VALUE(((float) monitor_mode_spec->width / other_monitor_mode_spec->width) * ((float) monitor_mode_spec->height / other_monitor_mode_spec->height), target_ratio, epsilon);
}

GDMonitorMode* gd_monitor_get_mode_from_spec(GDMonitor* monitor, GDMonitorModeSpec* monitor_mode_spec)
{
    GDMonitorPrivate* priv;
    GList* l;

    priv = gd_monitor_get_instance_private(monitor);

    for (l = priv->modes; l; l = l->next) {
        GDMonitorMode* monitor_mode = l->data;
        if (gd_monitor_mode_spec_equals(monitor_mode_spec, &monitor_mode->spec)) return monitor_mode;
    }

    return NULL;
}

GDMonitorMode* gd_monitor_get_preferred_mode(GDMonitor* monitor)
{
    GDMonitorPrivate* priv;

    priv = gd_monitor_get_instance_private(monitor);

    return priv->preferred_mode;
}

GDMonitorMode* gd_monitor_get_current_mode(GDMonitor* monitor)
{
    GDMonitorPrivate* priv;

    priv = gd_monitor_get_instance_private(monitor);

    return priv->current_mode;
}

void gd_monitor_derive_current_mode(GDMonitor* monitor)
{
    GDMonitorPrivate* priv;
    GDMonitorMode* current_mode;
    GList* l;

    priv = gd_monitor_get_instance_private(monitor);
    current_mode = NULL;

    for (l = priv->modes; l; l = l->next) {
        GDMonitorMode* mode = l->data;

        if (gd_monitor_is_mode_assigned(monitor, mode)) {
            current_mode = mode;
            break;
        }
    }

    priv->current_mode = current_mode;

    g_warn_if_fail(is_current_mode_known (monitor));
}

void gd_monitor_set_current_mode(GDMonitor* monitor, GDMonitorMode* mode)
{
    GDMonitorPrivate* priv;

    priv = gd_monitor_get_instance_private(monitor);

    priv->current_mode = mode;
}

GList* gd_monitor_get_modes(GDMonitor* monitor)
{
    GDMonitorPrivate* priv;

    priv = gd_monitor_get_instance_private(monitor);

    return priv->modes;
}

void gd_monitor_calculate_crtc_pos(GDMonitor* monitor, GDMonitorMode* monitor_mode, GDOutput* output, GDMonitorTransform crtc_transform, int* out_x, int* out_y)
{
    GD_MONITOR_GET_CLASS(monitor)->calculate_crtc_pos(monitor, monitor_mode, output, crtc_transform, out_x, out_y);
}

gfloat gd_monitor_calculate_mode_scale(GDMonitor* monitor, GDMonitorMode* monitor_mode, GDMonitorScalesConstraint constraints)
{
    GDMonitorPrivate* priv;
    GDSettings* settings;
    gint global_scaling_factor;

    priv = gd_monitor_get_instance_private(monitor);
    settings = gd_backend_get_settings(priv->backend);

    if (gd_settings_get_global_scaling_factor(settings, &global_scaling_factor)) return global_scaling_factor;

    return calculate_scale(monitor, monitor_mode, constraints);
}

gfloat* gd_monitor_calculate_supported_scales(GDMonitor* monitor, GDMonitorMode* monitor_mode, GDMonitorScalesConstraint constraints, int* n_supported_scales)
{
    guint i, j;
    gint width, height;
    GArray* supported_scales;

    supported_scales = g_array_new(FALSE, FALSE, sizeof(gfloat));

    gd_monitor_mode_get_resolution(monitor_mode, &width, &height);

    for (i = floor(MINIMUM_SCALE_FACTOR); i <= ceilf(MAXIMUM_SCALE_FACTOR); i++) {
        if (constraints & GD_MONITOR_SCALES_CONSTRAINT_NO_FRAC) {
            if (is_scale_valid_for_size(width, height, i)) {
                float scale = i;
                g_array_append_val(supported_scales, scale);
            }
        }
        else {
            float max_bound;

            if (i == floorf(MINIMUM_SCALE_FACTOR) || i == ceilf(MAXIMUM_SCALE_FACTOR)) max_bound = SCALE_FACTORS_STEPS;
            else max_bound = SCALE_FACTORS_STEPS / 2.0f;

            for (j = 0; j < SCALE_FACTORS_PER_INTEGER; j++) {
                gfloat scale;
                gfloat scale_value = i + j * SCALE_FACTORS_STEPS;

                if (!is_scale_valid_for_size(width, height, scale_value)) continue;

                scale = gd_get_closest_monitor_scale_factor_for_resolution(width, height, scale_value, max_bound);

                if (scale > 0.0f)
                    g_array_append_val(supported_scales, scale);
            }
        }
    }

    if (supported_scales->len == 0) {
        gfloat fallback_scale;

        fallback_scale = 1.0;

        g_array_append_val(supported_scales, fallback_scale);
    }

    *n_supported_scales = supported_scales->len;
    return (gfloat*)g_array_free(supported_scales, FALSE);
}

const gchar* gd_monitor_mode_get_id(GDMonitorMode* monitor_mode)
{
    return monitor_mode->id;
}

GDMonitorModeSpec* gd_monitor_mode_get_spec(GDMonitorMode* monitor_mode)
{
    return &monitor_mode->spec;
}

void gd_monitor_mode_get_resolution(GDMonitorMode* monitor_mode, gint* width, gint* height)
{
    *width = monitor_mode->spec.width;
    *height = monitor_mode->spec.height;
}

gfloat gd_monitor_mode_get_refresh_rate(GDMonitorMode* monitor_mode)
{
    return monitor_mode->spec.refreshRate;
}

GDCrtcModeFlag gd_monitor_mode_get_flags(GDMonitorMode* monitor_mode)
{
    return monitor_mode->spec.flags;
}

bool gd_monitor_mode_foreach_crtc(GDMonitor* monitor, GDMonitorMode* mode, GDMonitorModeFunc func, gpointer user_data, GError** error)
{
    GDMonitorPrivate* priv;
    GList* l;
    gint i;

    priv = gd_monitor_get_instance_private(monitor);

    for (l = priv->outputs, i = 0; l; l = l->next, i++) {
        GDMonitorCrtcMode* monitor_crtc_mode = &mode->crtcModes[i];
        if (!monitor_crtc_mode->crtcMode) continue;
        if (!func(monitor, mode, monitor_crtc_mode, user_data, error)) return FALSE;
    }

    return TRUE;
}

bool gd_monitor_mode_foreach_output(GDMonitor* monitor, GDMonitorMode* mode, GDMonitorModeFunc func, gpointer user_data, GError** error)
{
    GDMonitorPrivate* priv;
    GList* l;
    gint i;

    priv = gd_monitor_get_instance_private(monitor);

    for (l = priv->outputs, i = 0; l; l = l->next, i++) {
        GDMonitorCrtcMode* monitor_crtc_mode = &mode->crtcModes[i];

        if (!func(monitor, mode, monitor_crtc_mode, user_data, error)) return FALSE;
    }

    return TRUE;
}

bool gd_monitor_mode_should_be_advertised(GDMonitorMode* monitor_mode)
{
    GDMonitorMode* preferred_mode;

    g_return_val_if_fail(monitor_mode != NULL, FALSE);

    preferred_mode = gd_monitor_get_preferred_mode(monitor_mode->monitor);
    if (monitor_mode->spec.width == preferred_mode->spec.width && monitor_mode->spec.height == preferred_mode->spec.height) return TRUE;

    return is_logical_size_large_enough(monitor_mode->spec.width, monitor_mode->spec.height);
}

bool gd_verify_monitor_mode_spec(GDMonitorModeSpec* mode_spec, GError** error)
{
    if (mode_spec->width > 0 && mode_spec->height > 0 && mode_spec->refreshRate > 0.0f) {
        return TRUE;
    }
    else {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Monitor mode invalid");
        return FALSE;
    }
}

bool gd_monitor_has_aspect_as_size(GDMonitor* monitor)
{
    int width_mm;
    int height_mm;

    gd_monitor_get_physical_dimensions(monitor, &width_mm, &height_mm);

    return (width_mm == 1600 && height_mm == 900) || (width_mm == 1600 && height_mm == 1000) || (width_mm == 160 && height_mm == 90) || (width_mm == 160 && height_mm == 100) || (width_mm == 16 && height_mm == 9) || (width_mm == 16 &&
        height_mm == 10);
}

void gd_monitor_set_logical_monitor(GDMonitor* monitor, GDLogicalMonitor* logical_monitor)
{
    GDMonitorPrivate* priv;

    priv = gd_monitor_get_instance_private(monitor);

    priv->logical_monitor = logical_monitor;
}

bool gd_monitor_get_backlight_info(GDMonitor* self, int* backlight_min, int* backlight_max)
{
    GDOutput* main_output;
    int value;

    main_output = gd_monitor_get_main_output(self);
    value = gd_output_get_backlight(main_output);

    if (value >= 0) {
        const GDOutputInfo* output_info;

        output_info = gd_output_get_info(main_output);

        if (backlight_min) *backlight_min = output_info->backlight_min;

        if (backlight_max) *backlight_max = output_info->backlight_max;

        return TRUE;
    }

    return FALSE;
}

void gd_monitor_set_backlight(GDMonitor* self, int value)
{
    GDMonitorPrivate* priv;
    GList* l;

    priv = gd_monitor_get_instance_private(self);

    for (l = priv->outputs; l; l = l->next) {
        GDOutput* output = l->data;
        gd_output_set_backlight(output, value);
    }
}

bool gd_monitor_get_backlight(GDMonitor* self, int* value)
{
    if (gd_monitor_get_backlight_info(self, NULL, NULL)) {
        GDOutput* main_output;

        main_output = gd_monitor_get_main_output(self);

        *value = gd_output_get_backlight(main_output);

        return TRUE;
    }

    return FALSE;
}
