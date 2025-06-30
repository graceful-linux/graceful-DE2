//
// Created by dingjing on 25-6-28.
//
#include "macros/macros.h"
#include "gd-crtc-mode-private.h"

typedef struct
{
    uint64_t id;
    char* name;
    GDCrtcModeInfo* info;
} GDCrtcModePrivate;

enum
{
    PROP_0,
    PROP_ID,
    PROP_NAME,
    PROP_INFO,
    LAST_PROP
};

static GParamSpec* crtc_mode_properties[LAST_PROP] = {NULL};

G_DEFINE_TYPE_WITH_PRIVATE(GDCrtcMode, gd_crtc_mode, G_TYPE_OBJECT)

static void gd_crtc_mode_finalize(GObject* object)
{
    GDCrtcMode * self;
    GDCrtcModePrivate* priv;

    self = GD_CRTC_MODE(object);
    priv = gd_crtc_mode_get_instance_private(self);

    g_clear_pointer(&priv->name, g_free);
    g_clear_pointer(&priv->info, gd_crtc_mode_info_unref);

    G_OBJECT_CLASS(gd_crtc_mode_parent_class)->finalize(object);
}

static void gd_crtc_mode_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec)
{
    GDCrtcMode* self;
    GDCrtcModePrivate* priv;

    self = GD_CRTC_MODE(object);
    priv = gd_crtc_mode_get_instance_private(self);

    switch (property_id) {
    case PROP_ID:
        g_value_set_uint64(value, priv->id);
        break;

    case PROP_NAME:
        g_value_set_string(value, priv->name);
        break;

    case PROP_INFO:
        g_value_set_boxed(value, priv->info);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void gd_crtc_mode_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec)
{
    GDCrtcMode* self;
    GDCrtcModePrivate* priv;

    self = GD_CRTC_MODE(object);
    priv = gd_crtc_mode_get_instance_private(self);

    switch (property_id) {
    case PROP_ID:
        priv->id = g_value_get_uint64(value);
        break;

    case PROP_NAME:
        priv->name = g_value_dup_string(value);
        break;

    case PROP_INFO:
        priv->info = gd_crtc_mode_info_ref(g_value_get_boxed(value));
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void gd_crtc_mode_class_init(GDCrtcModeClass* self_class)
{
    GObjectClass* object_class;

    object_class = G_OBJECT_CLASS(self_class);

    object_class->finalize = gd_crtc_mode_finalize;
    object_class->get_property = gd_crtc_mode_get_property;
    object_class->set_property = gd_crtc_mode_set_property;

    crtc_mode_properties[PROP_ID] = g_param_spec_uint64("id", "id", "CRTC mode id", 0, UINT64_MAX, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
    crtc_mode_properties[PROP_NAME] = g_param_spec_string("name", "name", "Name of CRTC mode", NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
    crtc_mode_properties[PROP_INFO] = g_param_spec_boxed("info", "info", "GfCrtcModeInfo", GD_TYPE_CRTC_MODE_INFO, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, LAST_PROP, crtc_mode_properties);
}

static void gd_crtc_mode_init(GDCrtcMode* self)
{
}

uint64_t gd_crtc_mode_get_id(GDCrtcMode* self)
{
    GDCrtcModePrivate* priv;

    priv = gd_crtc_mode_get_instance_private(self);

    return priv->id;
}

const char* gd_crtc_mode_get_name(GDCrtcMode* self)
{
    GDCrtcModePrivate* priv;

    priv = gd_crtc_mode_get_instance_private(self);

    return priv->name;
}

const GDCrtcModeInfo* gd_crtc_mode_get_info(GDCrtcMode* self)
{
    GDCrtcModePrivate* priv;

    priv = gd_crtc_mode_get_instance_private(self);

    return priv->info;
}
