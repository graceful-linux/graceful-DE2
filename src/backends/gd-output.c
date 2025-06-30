//
// Created by dingjing on 25-6-30.
//
#include "gd-output-private.h"

#include "gd-crtc-private.h"

typedef struct
{
    uint64_t id;

    GDGpu* gpu;

    GDOutputInfo* info;

    GDMonitor* monitor;

    /* The CRTC driving this output, NULL if the output is not enabled */
    GDCrtc* crtc;

    gboolean is_primary;
    gboolean is_presentation;

    gboolean is_underscanning;

    gboolean has_max_bpc;
    unsigned int max_bpc;

    int backlight;
} GDOutputPrivate;

enum
{
    PROP_0,
    PROP_ID,
    PROP_GPU,
    PROP_INFO,
    LAST_PROP
};

static GParamSpec* output_properties[LAST_PROP] = {NULL};

enum { BACKLIGHT_CHANGED, LAST_SIGNAL };

static unsigned int output_signals[LAST_SIGNAL] = {0};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(GDOutput, gd_output, G_TYPE_OBJECT)

static void
gd_output_dispose(GObject* object)
{
    GDOutput* output;
    GDOutputPrivate* priv;

    output = GD_OUTPUT(object);
    priv = gd_output_get_instance_private(output);

    g_clear_object(&priv->crtc);

    G_OBJECT_CLASS(gd_output_parent_class)->dispose(object);
}

static void
gd_output_finalize(GObject* object)
{
    GDOutput* output;
    GDOutputPrivate* priv;

    output = GD_OUTPUT(object);
    priv = gd_output_get_instance_private(output);

    g_clear_pointer(&priv->info, gd_output_info_unref);

    G_OBJECT_CLASS(gd_output_parent_class)->finalize(object);
}

static void
gd_output_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec)
{
    GDOutput* self;
    GDOutputPrivate* priv;

    self = GD_OUTPUT(object);
    priv = gd_output_get_instance_private(self);

    switch (property_id) {
    case PROP_ID:
        g_value_set_uint64(value, priv->id);
        break;

    case PROP_GPU:
        g_value_set_object(value, priv->gpu);
        break;

    case PROP_INFO:
        g_value_set_boxed(value, priv->info);
        break;

    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
gd_output_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec)
{
    GDOutput* self;
    GDOutputPrivate* priv;

    self = GD_OUTPUT(object);
    priv = gd_output_get_instance_private(self);

    switch (property_id) {
    case PROP_ID:
        priv->id = g_value_get_uint64(value);
        break;

    case PROP_GPU:
        priv->gpu = g_value_get_object(value);
        break;

    case PROP_INFO:
        priv->info = gd_output_info_ref(g_value_get_boxed(value));
        break;

    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
gd_output_class_init(GDOutputClass* output_class)
{
    GObjectClass* object_class;

    object_class = G_OBJECT_CLASS(output_class);

    object_class->dispose = gd_output_dispose;
    object_class->finalize = gd_output_finalize;
    object_class->get_property = gd_output_get_property;
    object_class->set_property = gd_output_set_property;

    output_properties[PROP_ID] = g_param_spec_uint64("id", "id", "CRTC id", 0, UINT64_MAX, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

    output_properties[PROP_GPU] = g_param_spec_object("gpu", "GDGpu", "GDGpu", GD_TYPE_GPU, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

    output_properties[PROP_INFO] = g_param_spec_boxed("info", "info", "GDOutputInfo", GD_TYPE_OUTPUT_INFO, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, LAST_PROP, output_properties);

    output_signals[BACKLIGHT_CHANGED] = g_signal_new("backlight-changed", G_TYPE_FROM_CLASS(output_class), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void
gd_output_init(GDOutput* self)
{
    GDOutputPrivate* priv;

    priv = gd_output_get_instance_private(self);

    priv->backlight = -1;
}

uint64_t
gd_output_get_id(GDOutput* self)
{
    GDOutputPrivate* priv;

    priv = gd_output_get_instance_private(self);

    return priv->id;
}

GDGpu*
gd_output_get_gpu(GDOutput* output)
{
    GDOutputPrivate* priv;

    priv = gd_output_get_instance_private(output);

    return priv->gpu;
}

const GDOutputInfo*
gd_output_get_info(GDOutput* self)
{
    GDOutputPrivate* priv;

    priv = gd_output_get_instance_private(self);

    return priv->info;
}

GDMonitor*
gd_output_get_monitor(GDOutput* self)
{
    GDOutputPrivate* priv;

    priv = gd_output_get_instance_private(self);

    g_warn_if_fail(priv->monitor);

    return priv->monitor;
}

void
gd_output_set_monitor(GDOutput* self, GDMonitor* monitor)
{
    GDOutputPrivate* priv;

    priv = gd_output_get_instance_private(self);

    g_warn_if_fail(!priv->monitor);

    priv->monitor = monitor;
}

void
gd_output_unset_monitor(GDOutput* self)
{
    GDOutputPrivate* priv;

    priv = gd_output_get_instance_private(self);

    g_warn_if_fail(priv->monitor);

    priv->monitor = NULL;
}

const char*
gd_output_get_name(GDOutput* self)
{
    GDOutputPrivate* priv;

    priv = gd_output_get_instance_private(self);

    return priv->info->name;
}

void
gd_output_assign_crtc(GDOutput* self, GDCrtc* crtc, const GDOutputAssignment* output_assignment)
{
    GDOutputPrivate* priv;

    priv = gd_output_get_instance_private(self);

    g_assert(crtc);

    gd_output_unassign_crtc(self);

    g_set_object(&priv->crtc, crtc);

    gd_crtc_assign_output(crtc, self);

    priv->is_primary = output_assignment->is_primary;
    priv->is_presentation = output_assignment->is_presentation;
    priv->is_underscanning = output_assignment->is_underscanning;

    priv->has_max_bpc = output_assignment->has_max_bpc;

    if (priv->has_max_bpc) priv->max_bpc = output_assignment->max_bpc;
}

void
gd_output_unassign_crtc(GDOutput* output)
{
    GDOutputPrivate* priv;

    priv = gd_output_get_instance_private(output);

    if (priv->crtc != NULL) {
        gd_crtc_unassign_output(priv->crtc, output);
        g_clear_object(&priv->crtc);
    }

    priv->is_primary = FALSE;
    priv->is_presentation = FALSE;
}

GDCrtc*
gd_output_get_assigned_crtc(GDOutput* output)
{
    GDOutputPrivate* priv;

    priv = gd_output_get_instance_private(output);

    return priv->crtc;
}

gboolean
gd_output_is_laptop(GDOutput* output)
{
    const GDOutputInfo* output_info;

    output_info = gd_output_get_info(output);

    switch (output_info->connector_type) {
    case GD_CONNECTOR_TYPE_LVDS:
    case GD_CONNECTOR_TYPE_eDP:
    case GD_CONNECTOR_TYPE_DSI:
        return TRUE;

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
    case GD_CONNECTOR_TYPE_HDMIA:
    case GD_CONNECTOR_TYPE_HDMIB:
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

GDMonitorTransform
gd_output_logical_to_crtc_transform(GDOutput* output, GDMonitorTransform transform)
{
    GDOutputPrivate* priv;
    GDMonitorTransform panel_orientation_transform;

    priv = gd_output_get_instance_private(output);

    panel_orientation_transform = priv->info->panel_orientation_transform;

    return gd_monitor_transform_transform(transform, panel_orientation_transform);
}

GDMonitorTransform
gd_output_crtc_to_logical_transform(GDOutput* output, GDMonitorTransform transform)
{
    GDOutputPrivate* priv;
    GDMonitorTransform panel_orientation_transform;
    GDMonitorTransform inverted_transform;

    priv = gd_output_get_instance_private(output);

    panel_orientation_transform = priv->info->panel_orientation_transform;
    inverted_transform = gd_monitor_transform_invert(panel_orientation_transform);

    return gd_monitor_transform_transform(transform, inverted_transform);
}

gboolean
gd_output_is_primary(GDOutput* self)
{
    GDOutputPrivate* priv;

    priv = gd_output_get_instance_private(self);

    return priv->is_primary;
}

gboolean
gd_output_is_presentation(GDOutput* self)
{
    GDOutputPrivate* priv;

    priv = gd_output_get_instance_private(self);

    return priv->is_presentation;
}

gboolean
gd_output_is_underscanning(GDOutput* self)
{
    GDOutputPrivate* priv;

    priv = gd_output_get_instance_private(self);

    return priv->is_underscanning;
}

gboolean
gd_output_get_max_bpc(GDOutput* self, unsigned int* max_bpc)
{
    GDOutputPrivate* priv;

    priv = gd_output_get_instance_private(self);

    if (priv->has_max_bpc && max_bpc != NULL) *max_bpc = priv->max_bpc;

    return priv->has_max_bpc;
}

void
gd_output_set_backlight(GDOutput* self, int backlight)
{
    GDOutputPrivate* priv;

    priv = gd_output_get_instance_private(self);

    g_return_if_fail(backlight >= priv->info->backlight_min);
    g_return_if_fail(backlight <= priv->info->backlight_max);

    priv->backlight = backlight;

    g_signal_emit(self, output_signals[BACKLIGHT_CHANGED], 0);
}

int
gd_output_get_backlight(GDOutput* self)
{
    GDOutputPrivate* priv;

    priv = gd_output_get_instance_private(self);

    return priv->backlight;
}

void
gd_output_add_possible_clone(GDOutput* self, GDOutput* possible_clone)
{
    GDOutputPrivate* priv;
    GDOutputInfo* output_info;

    priv = gd_output_get_instance_private(self);

    output_info = priv->info;

    output_info->n_possible_clones++;
    output_info->possible_clones = g_renew(GDOutput *, output_info->possible_clones, output_info->n_possible_clones);
    output_info->possible_clones[output_info->n_possible_clones - 1] = possible_clone;
}
