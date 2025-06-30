//
// Created by dingjing on 25-6-28.
//
#include "gd-gpu-private.h"

#include "gd-output-private.h"
#include "gd-backend-private.h"

typedef struct
{
    GDBackend* backend;

    GList* outputs;
    GList* crtcs;
    GList* modes;
} GDGpuPrivate;

enum { PROP_0, PROP_BACKEND, LAST_PROP };

static GParamSpec* gpu_properties[LAST_PROP] = {NULL};

G_DEFINE_TYPE_WITH_PRIVATE(GDGpu, gd_gpu, G_TYPE_OBJECT)


static void gd_gpu_finalize(GObject* object)
{
    GDGpu * gpu;
    GDGpuPrivate* priv;

    gpu = GD_GPU(object);
    priv = gd_gpu_get_instance_private(gpu);

    g_list_free_full(priv->outputs, g_object_unref);
    g_list_free_full(priv->modes, g_object_unref);
    g_list_free_full(priv->crtcs, g_object_unref);

    G_OBJECT_CLASS(gd_gpu_parent_class)->finalize(object);
}

static void gd_gpu_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec)
{
    GDGpu* gpu;
    GDGpuPrivate* priv;

    gpu = GD_GPU(object);
    priv = gd_gpu_get_instance_private(gpu);

    switch (property_id) {
    case PROP_BACKEND:
        g_value_set_object(value, priv->backend);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void gd_gpu_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec)
{
    GDGpu* gpu;
    GDGpuPrivate* priv;

    gpu = GD_GPU(object);
    priv = gd_gpu_get_instance_private(gpu);

    switch (property_id) {
    case PROP_BACKEND:
        priv->backend = g_value_get_object(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void gd_gpu_install_properties(GObjectClass* object_class)
{
    gpu_properties[PROP_BACKEND] = g_param_spec_object("backend", "GfBackend", "GfBackend", GD_TYPE_BACKEND, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, LAST_PROP, gpu_properties);
}

static void gd_gpu_class_init(GDGpuClass* gpu_class)
{
    GObjectClass* object_class;

    object_class = G_OBJECT_CLASS(gpu_class);

    object_class->finalize = gd_gpu_finalize;
    object_class->get_property = gd_gpu_get_property;
    object_class->set_property = gd_gpu_set_property;

    gd_gpu_install_properties(object_class);
}

static void gd_gpu_init(GDGpu* gpu)
{
}

bool gd_gpu_read_current(GDGpu* gpu, GError** error)
{
    GDGpuPrivate* priv;
    gboolean ret;
    GList* old_outputs;
    GList* old_crtcs;
    GList* old_modes;

    priv = gd_gpu_get_instance_private(gpu);

    /* TODO: Get rid of this when objects incref:s what they need instead. */
    old_outputs = priv->outputs;
    old_crtcs = priv->crtcs;
    old_modes = priv->modes;

    ret = GD_GPU_GET_CLASS(gpu)->read_current(gpu, error);

    g_list_free_full(old_outputs, g_object_unref);
    g_list_free_full(old_modes, g_object_unref);
    g_list_free_full(old_crtcs, g_object_unref);

    return ret;
}

bool gd_gpu_has_hotplug_mode_update(GDGpu* gpu)
{
    GDGpuPrivate* priv;
    GList* l;

    priv = gd_gpu_get_instance_private(gpu);

    for (l = priv->outputs; l; l = l->next) {
        GDOutput* output;
        const GDOutputInfo* output_info;

        output = l->data;
        output_info = gd_output_get_info(output);

        if (output_info->hotplug_mode_update) return TRUE;
    }

    return FALSE;
}

GDBackend* gd_gpu_get_backend(GDGpu* self)
{
    GDGpuPrivate* priv;

    priv = gd_gpu_get_instance_private(self);

    return priv->backend;
}

GList* gd_gpu_get_outputs(GDGpu* gpu)
{
    GDGpuPrivate* priv;

    priv = gd_gpu_get_instance_private(gpu);

    return priv->outputs;
}

GList*
gd_gpu_get_crtcs(GDGpu* gpu)
{
    GDGpuPrivate* priv;

    priv = gd_gpu_get_instance_private(gpu);

    return priv->crtcs;
}

GList*
gd_gpu_get_modes(GDGpu* gpu)
{
    GDGpuPrivate* priv;

    priv = gd_gpu_get_instance_private(gpu);

    return priv->modes;
}

void
gd_gpu_take_outputs(GDGpu* gpu, GList* outputs)
{
    GDGpuPrivate* priv;

    priv = gd_gpu_get_instance_private(gpu);

    priv->outputs = outputs;
}

void
gd_gpu_take_crtcs(GDGpu* gpu, GList* crtcs)
{
    GDGpuPrivate* priv;

    priv = gd_gpu_get_instance_private(gpu);

    priv->crtcs = crtcs;
}

void
gd_gpu_take_modes(GDGpu* gpu, GList* modes)
{
    GDGpuPrivate* priv;

    priv = gd_gpu_get_instance_private(gpu);

    priv->modes = modes;
}
