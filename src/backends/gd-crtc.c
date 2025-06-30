//
// Created by dingjing on 25-6-28.
//
#include "gd-crtc-private.h"

#include "gd-gpu-private.h"


typedef struct
{
    uint64_t id;
    GDGpu* gpu;
    GDMonitorTransform all_transforms;
    GList* outputs;
    GDCrtcConfig* config;
} GDCrtcPrivate;

enum
{
    PROP_0,
    PROP_ID,
    PROP_GPU,
    PROP_ALL_TRANSFORMS,
    LAST_PROP
};

static GParamSpec* crtc_properties[LAST_PROP] = {NULL};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(GDCrtc, gd_crtc, G_TYPE_OBJECT)

static void gd_crtc_finalize(GObject* object)
{
    GDCrtc * crtc;
    GDCrtcPrivate* priv;

    crtc = GD_CRTC(object);
    priv = gd_crtc_get_instance_private(crtc);

    g_clear_pointer(&priv->config, g_free);
    g_clear_pointer(&priv->outputs, g_list_free);

    G_OBJECT_CLASS(gd_crtc_parent_class)->finalize(object);
}

static void gd_crtc_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec)
{
    GDCrtc* self;
    GDCrtcPrivate* priv;

    self = GD_CRTC(object);
    priv = gd_crtc_get_instance_private(self);

    switch (property_id) {
    case PROP_ID:
        g_value_set_uint64(value, priv->id);
        break;

    case PROP_GPU:
        g_value_set_object(value, priv->gpu);
        break;

    case PROP_ALL_TRANSFORMS:
        g_value_set_uint(value, priv->all_transforms);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void gd_crtc_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec)
{
    GDCrtc* self;
    GDCrtcPrivate* priv;

    self = GD_CRTC(object);
    priv = gd_crtc_get_instance_private(self);

    switch (property_id) {
    case PROP_ID:
        priv->id = g_value_get_uint64(value);
        break;

    case PROP_GPU:
        priv->gpu = g_value_get_object(value);
        break;

    case PROP_ALL_TRANSFORMS:
        priv->all_transforms = g_value_get_uint(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void gd_crtc_class_init(GDCrtcClass* crtc_class)
{
    GObjectClass* object_class;

    object_class = G_OBJECT_CLASS(crtc_class);

    object_class->finalize = gd_crtc_finalize;
    object_class->get_property = gd_crtc_get_property;
    object_class->set_property = gd_crtc_set_property;

    crtc_properties[PROP_ID] = g_param_spec_uint64("id", "id", "id", 0, UINT64_MAX, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

    crtc_properties[PROP_GPU] = g_param_spec_object("gpu", "GfGpu", "GfGpu", GD_TYPE_GPU, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

    crtc_properties[PROP_ALL_TRANSFORMS] = g_param_spec_uint("all-transforms", "all-transforms", "All transforms", 0, GD_MONITOR_ALL_TRANSFORMS, GD_MONITOR_ALL_TRANSFORMS,
                                                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, LAST_PROP, crtc_properties);
}

static void gd_crtc_init(GDCrtc* self)
{
    GDCrtcPrivate* priv;

    priv = gd_crtc_get_instance_private(self);

    priv->all_transforms = GD_MONITOR_ALL_TRANSFORMS;
}

uint64_t gd_crtc_get_id(GDCrtc* self)
{
    GDCrtcPrivate* priv;

    priv = gd_crtc_get_instance_private(self);

    return priv->id;
}

GDGpu* gd_crtc_get_gpu(GDCrtc* crtc)
{
    GDCrtcPrivate* priv;

    priv = gd_crtc_get_instance_private(crtc);

    return priv->gpu;
}

GDMonitorTransform gd_crtc_get_all_transforms(GDCrtc* self)
{
    GDCrtcPrivate* priv;

    priv = gd_crtc_get_instance_private(self);

    return priv->all_transforms;
}

const GList* gd_crtc_get_outputs(GDCrtc* self)
{
    GDCrtcPrivate* priv;

    priv = gd_crtc_get_instance_private(self);

    return priv->outputs;
}

void gd_crtc_assign_output(GDCrtc* self, GDOutput* output)
{
    GDCrtcPrivate* priv;

    priv = gd_crtc_get_instance_private(self);

    priv->outputs = g_list_append(priv->outputs, output);
}

void gd_crtc_unassign_output(GDCrtc* self, GDOutput* output)
{
    GDCrtcPrivate* priv;

    priv = gd_crtc_get_instance_private(self);

    g_return_if_fail(g_list_find(priv->outputs, output));

    priv->outputs = g_list_remove(priv->outputs, output);
}

void gd_crtc_set_config(GDCrtc* self, GDRectangle* layout, GDCrtcMode* mode, GDMonitorTransform transform)
{
    GDCrtcPrivate* priv;
    GDCrtcConfig* config;

    priv = gd_crtc_get_instance_private(self);

    gd_crtc_unset_config(self);

    config = g_new0(GDCrtcConfig, 1);
    config->layout = *layout;
    config->mode = mode;
    config->transform = transform;

    priv->config = config;
}

void gd_crtc_unset_config(GDCrtc* self)
{
    GDCrtcPrivate* priv;

    priv = gd_crtc_get_instance_private(self);

    g_clear_pointer(&priv->config, g_free);
}

const GDCrtcConfig* gd_crtc_get_config(GDCrtc* self)
{
    GDCrtcPrivate* priv;

    priv = gd_crtc_get_instance_private(self);

    return priv->config;
}
