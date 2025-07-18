//
// Created by dingjing on 25-6-30.
//

#include "gd-dbus-display-config.h"

#include <string.h>
#ifdef G_OS_UNIX
#  include <gio/gunixfdlist.h>
#endif

#ifdef G_ENABLE_DEBUG
#define g_marshal_value_peek_boolean(v)  g_value_get_boolean (v)
#define g_marshal_value_peek_char(v)     g_value_get_schar (v)
#define g_marshal_value_peek_uchar(v)    g_value_get_uchar (v)
#define g_marshal_value_peek_int(v)      g_value_get_int (v)
#define g_marshal_value_peek_uint(v)     g_value_get_uint (v)
#define g_marshal_value_peek_long(v)     g_value_get_long (v)
#define g_marshal_value_peek_ulong(v)    g_value_get_ulong (v)
#define g_marshal_value_peek_int64(v)    g_value_get_int64 (v)
#define g_marshal_value_peek_uint64(v)   g_value_get_uint64 (v)
#define g_marshal_value_peek_enum(v)     g_value_get_enum (v)
#define g_marshal_value_peek_flags(v)    g_value_get_flags (v)
#define g_marshal_value_peek_float(v)    g_value_get_float (v)
#define g_marshal_value_peek_double(v)   g_value_get_double (v)
#define g_marshal_value_peek_string(v)   (char*) g_value_get_string (v)
#define g_marshal_value_peek_param(v)    g_value_get_param (v)
#define g_marshal_value_peek_boxed(v)    g_value_get_boxed (v)
#define g_marshal_value_peek_pointer(v)  g_value_get_pointer (v)
#define g_marshal_value_peek_object(v)   g_value_get_object (v)
#define g_marshal_value_peek_variant(v)  g_value_get_variant (v)
#else /* !G_ENABLE_DEBUG */
/* WARNING: This code accesses GValues directly, which is UNSUPPORTED API.
 *          Do not access GValues directly in your code. Instead, use the
 *          g_value_get_*() functions
 */
#define g_marshal_value_peek_boolean(v)  (v)->data[0].v_int
#define g_marshal_value_peek_char(v)     (v)->data[0].v_int
#define g_marshal_value_peek_uchar(v)    (v)->data[0].v_uint
#define g_marshal_value_peek_int(v)      (v)->data[0].v_int
#define g_marshal_value_peek_uint(v)     (v)->data[0].v_uint
#define g_marshal_value_peek_long(v)     (v)->data[0].v_long
#define g_marshal_value_peek_ulong(v)    (v)->data[0].v_ulong
#define g_marshal_value_peek_int64(v)    (v)->data[0].v_int64
#define g_marshal_value_peek_uint64(v)   (v)->data[0].v_uint64
#define g_marshal_value_peek_enum(v)     (v)->data[0].v_long
#define g_marshal_value_peek_flags(v)    (v)->data[0].v_ulong
#define g_marshal_value_peek_float(v)    (v)->data[0].v_float
#define g_marshal_value_peek_double(v)   (v)->data[0].v_double
#define g_marshal_value_peek_string(v)   (v)->data[0].v_pointer
#define g_marshal_value_peek_param(v)    (v)->data[0].v_pointer
#define g_marshal_value_peek_boxed(v)    (v)->data[0].v_pointer
#define g_marshal_value_peek_pointer(v)  (v)->data[0].v_pointer
#define g_marshal_value_peek_object(v)   (v)->data[0].v_pointer
#define g_marshal_value_peek_variant(v)  (v)->data[0].v_pointer
#endif /* !G_ENABLE_DEBUG */

typedef struct
{
    GDBusArgInfo parent_struct;
    gboolean use_gvariant;
} _ExtendedGDBusArgInfo;

typedef struct
{
    GDBusMethodInfo parent_struct;
    const gchar* signal_name;
    gboolean pass_fdlist;
} _ExtendedGDBusMethodInfo;

typedef struct
{
    GDBusSignalInfo parent_struct;
    const gchar* signal_name;
} _ExtendedGDBusSignalInfo;

typedef struct
{
    GDBusPropertyInfo parent_struct;
    const gchar* hyphen_name;
    guint use_gvariant : 1;
    guint emits_changed_signal : 1;
} _ExtendedGDBusPropertyInfo;

typedef struct
{
    GDBusInterfaceInfo parent_struct;
    const gchar* hyphen_name;
} _ExtendedGDBusInterfaceInfo;

typedef struct
{
    const _ExtendedGDBusPropertyInfo* info;
    guint prop_id;
    GValue orig_value; /* the value before the change */
} ChangedProperty;

static void
_changed_property_free(ChangedProperty* data)
{
    g_value_unset(&data->orig_value);
    g_free(data);
}

static gboolean
_g_strv_equal0(gchar** a, gchar** b)
{
    gboolean ret = FALSE;
    guint n;
    if (a == NULL && b == NULL) {
        ret = TRUE;
        goto out;
    }
    if (a == NULL || b == NULL) goto out;
    if (g_strv_length(a) != g_strv_length(b)) goto out;
    for (n = 0; a[n] != NULL; n++)
        if (g_strcmp0(a[n], b[n]) != 0) goto out;
    ret = TRUE;
out:
    return ret;
}

static gboolean
_g_variant_equal0(GVariant* a, GVariant* b)
{
    gboolean ret = FALSE;
    if (a == NULL && b == NULL) {
        ret = TRUE;
        goto out;
    }
    if (a == NULL || b == NULL) goto out;
    ret = g_variant_equal(a, b);
out:
    return ret;
}

G_GNUC_UNUSED static gboolean
_g_value_equal(const GValue* a, const GValue* b)
{
    gboolean ret = FALSE;
    g_assert(G_VALUE_TYPE (a) == G_VALUE_TYPE (b));
    switch (G_VALUE_TYPE(a)) {
    case G_TYPE_BOOLEAN:
        ret = (g_value_get_boolean(a) == g_value_get_boolean(b));
        break;
    case G_TYPE_UCHAR:
        ret = (g_value_get_uchar(a) == g_value_get_uchar(b));
        break;
    case G_TYPE_INT:
        ret = (g_value_get_int(a) == g_value_get_int(b));
        break;
    case G_TYPE_UINT:
        ret = (g_value_get_uint(a) == g_value_get_uint(b));
        break;
    case G_TYPE_INT64:
        ret = (g_value_get_int64(a) == g_value_get_int64(b));
        break;
    case G_TYPE_UINT64:
        ret = (g_value_get_uint64(a) == g_value_get_uint64(b));
        break;
    case G_TYPE_DOUBLE: {
        /* Avoid -Wfloat-equal warnings by doing a direct bit compare */
        gdouble da = g_value_get_double(a);
        gdouble db = g_value_get_double(b);
        ret = memcmp(&da, &db, sizeof(gdouble)) == 0;
    }
    break;
    case G_TYPE_STRING:
        ret = (g_strcmp0(g_value_get_string(a), g_value_get_string(b)) == 0);
        break;
    case G_TYPE_VARIANT:
        ret = _g_variant_equal0(g_value_get_variant(a), g_value_get_variant(b));
        break;
    default:
        if (G_VALUE_TYPE(a) == G_TYPE_STRV) ret = _g_strv_equal0(g_value_get_boxed(a), g_value_get_boxed(b));
        else
            g_critical("_g_value_equal() does not handle type %s", g_type_name (G_VALUE_TYPE (a)));
        break;
    }
    return ret;
}

static void
_g_dbus_codegen_marshal_BOOLEAN__OBJECT(GClosure* closure, GValue* return_value, unsigned int n_param_values, const GValue* param_values, void* invocation_hint G_GNUC_UNUSED, void* marshal_data)
{
    typedef gboolean (*_GDbusCodegenMarshalBoolean_ObjectFunc)(void* data1, GDBusMethodInvocation* arg_method_invocation, void* data2);
    _GDbusCodegenMarshalBoolean_ObjectFunc callback;
    GCClosure* cc = (GCClosure*)closure;
    void * data1, * data2;
    gboolean v_return;

    g_return_if_fail(return_value != NULL);
    g_return_if_fail(n_param_values == 2);

    if (G_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = g_value_peek_pointer(param_values + 0);
    }
    else {
        data1 = g_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (_GDbusCodegenMarshalBoolean_ObjectFunc)(marshal_data ? marshal_data : cc->callback);

    v_return = callback(data1, g_marshal_value_peek_object(param_values + 1), data2);

    g_value_set_boolean(return_value, v_return);
}

static void
_g_dbus_codegen_marshal_BOOLEAN__OBJECT_UINT_UINT_INT(GClosure* closure, GValue* return_value, unsigned int n_param_values, const GValue* param_values, void* invocation_hint G_GNUC_UNUSED, void* marshal_data)
{
    typedef gboolean (*_GDbusCodegenMarshalBoolean_ObjectUintUintIntFunc)(void* data1, GDBusMethodInvocation* arg_method_invocation, guint arg_serial, guint arg_output, gint arg_value, void* data2);
    _GDbusCodegenMarshalBoolean_ObjectUintUintIntFunc callback;
    GCClosure* cc = (GCClosure*)closure;
    void * data1, * data2;
    gboolean v_return;

    g_return_if_fail(return_value != NULL);
    g_return_if_fail(n_param_values == 5);

    if (G_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = g_value_peek_pointer(param_values + 0);
    }
    else {
        data1 = g_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (_GDbusCodegenMarshalBoolean_ObjectUintUintIntFunc)(marshal_data ? marshal_data : cc->callback);

    v_return = callback(data1, g_marshal_value_peek_object(param_values + 1), g_marshal_value_peek_uint(param_values + 2), g_marshal_value_peek_uint(param_values + 3), g_marshal_value_peek_int(param_values + 4), data2);

    g_value_set_boolean(return_value, v_return);
}

static void
_g_dbus_codegen_marshal_BOOLEAN__OBJECT_UINT_STRING_INT(GClosure* closure, GValue* return_value, unsigned int n_param_values, const GValue* param_values, void* invocation_hint G_GNUC_UNUSED, void* marshal_data)
{
    typedef gboolean (*_GDbusCodegenMarshalBoolean_ObjectUintStringIntFunc)(void* data1, GDBusMethodInvocation* arg_method_invocation, guint arg_serial, const gchar* arg_connector, gint arg_value, void* data2);
    _GDbusCodegenMarshalBoolean_ObjectUintStringIntFunc callback;
    GCClosure* cc = (GCClosure*)closure;
    void * data1, * data2;
    gboolean v_return;

    g_return_if_fail(return_value != NULL);
    g_return_if_fail(n_param_values == 5);

    if (G_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = g_value_peek_pointer(param_values + 0);
    }
    else {
        data1 = g_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (_GDbusCodegenMarshalBoolean_ObjectUintStringIntFunc)(marshal_data ? marshal_data : cc->callback);

    v_return = callback(data1, g_marshal_value_peek_object(param_values + 1), g_marshal_value_peek_uint(param_values + 2), g_marshal_value_peek_string(param_values + 3), g_marshal_value_peek_int(param_values + 4), data2);

    g_value_set_boolean(return_value, v_return);
}

static void
_g_dbus_codegen_marshal_BOOLEAN__OBJECT_UINT_UINT(GClosure* closure, GValue* return_value, unsigned int n_param_values, const GValue* param_values, void* invocation_hint G_GNUC_UNUSED, void* marshal_data)
{
    typedef gboolean (*_GDbusCodegenMarshalBoolean_ObjectUintUintFunc)(void* data1, GDBusMethodInvocation* arg_method_invocation, guint arg_serial, guint arg_crtc, void* data2);
    _GDbusCodegenMarshalBoolean_ObjectUintUintFunc callback;
    GCClosure* cc = (GCClosure*)closure;
    void * data1, * data2;
    gboolean v_return;

    g_return_if_fail(return_value != NULL);
    g_return_if_fail(n_param_values == 4);

    if (G_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = g_value_peek_pointer(param_values + 0);
    }
    else {
        data1 = g_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (_GDbusCodegenMarshalBoolean_ObjectUintUintFunc)(marshal_data ? marshal_data : cc->callback);

    v_return = callback(data1, g_marshal_value_peek_object(param_values + 1), g_marshal_value_peek_uint(param_values + 2), g_marshal_value_peek_uint(param_values + 3), data2);

    g_value_set_boolean(return_value, v_return);
}

static void
_g_dbus_codegen_marshal_BOOLEAN__OBJECT_UINT_UINT_VARIANT_VARIANT_VARIANT(GClosure* closure, GValue* return_value, unsigned int n_param_values, const GValue* param_values, void* invocation_hint G_GNUC_UNUSED, void* marshal_data)
{
    typedef gboolean (*_GDbusCodegenMarshalBoolean_ObjectUintUintVariantVariantVariantFunc)(void* data1, GDBusMethodInvocation* arg_method_invocation, guint arg_serial, guint arg_crtc, GVariant* arg_red, GVariant* arg_green,
                                                                                            GVariant* arg_blue, void* data2);
    _GDbusCodegenMarshalBoolean_ObjectUintUintVariantVariantVariantFunc callback;
    GCClosure* cc = (GCClosure*)closure;
    void * data1, * data2;
    gboolean v_return;

    g_return_if_fail(return_value != NULL);
    g_return_if_fail(n_param_values == 7);

    if (G_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = g_value_peek_pointer(param_values + 0);
    }
    else {
        data1 = g_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (_GDbusCodegenMarshalBoolean_ObjectUintUintVariantVariantVariantFunc)(marshal_data ? marshal_data : cc->callback);

    v_return = callback(data1, g_marshal_value_peek_object(param_values + 1), g_marshal_value_peek_uint(param_values + 2), g_marshal_value_peek_uint(param_values + 3), g_marshal_value_peek_variant(param_values + 4),
                        g_marshal_value_peek_variant(param_values + 5), g_marshal_value_peek_variant(param_values + 6), data2);

    g_value_set_boolean(return_value, v_return);
}

static void
_g_dbus_codegen_marshal_BOOLEAN__OBJECT_UINT_UINT_VARIANT_VARIANT(GClosure* closure, GValue* return_value, unsigned int n_param_values, const GValue* param_values, void* invocation_hint G_GNUC_UNUSED, void* marshal_data)
{
    typedef gboolean (*_GDbusCodegenMarshalBoolean_ObjectUintUintVariantVariantFunc)(void* data1, GDBusMethodInvocation* arg_method_invocation, guint arg_serial, guint arg_method, GVariant* arg_logical_monitors, GVariant* arg_properties,
                                                                                     void* data2);
    _GDbusCodegenMarshalBoolean_ObjectUintUintVariantVariantFunc callback;
    GCClosure* cc = (GCClosure*)closure;
    void * data1, * data2;
    gboolean v_return;

    g_return_if_fail(return_value != NULL);
    g_return_if_fail(n_param_values == 6);

    if (G_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = g_value_peek_pointer(param_values + 0);
    }
    else {
        data1 = g_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (_GDbusCodegenMarshalBoolean_ObjectUintUintVariantVariantFunc)(marshal_data ? marshal_data : cc->callback);

    v_return = callback(data1, g_marshal_value_peek_object(param_values + 1), g_marshal_value_peek_uint(param_values + 2), g_marshal_value_peek_uint(param_values + 3), g_marshal_value_peek_variant(param_values + 4),
                        g_marshal_value_peek_variant(param_values + 5), data2);

    g_value_set_boolean(return_value, v_return);
}

static void
_g_dbus_codegen_marshal_BOOLEAN__OBJECT_UINT_UINT_VARIANT(GClosure* closure, GValue* return_value, unsigned int n_param_values, const GValue* param_values, void* invocation_hint G_GNUC_UNUSED, void* marshal_data)
{
    typedef gboolean (*_GDbusCodegenMarshalBoolean_ObjectUintUintVariantFunc)(void* data1, GDBusMethodInvocation* arg_method_invocation, guint arg_serial, guint arg_output, GVariant* arg_ctm, void* data2);
    _GDbusCodegenMarshalBoolean_ObjectUintUintVariantFunc callback;
    GCClosure* cc = (GCClosure*)closure;
    void * data1, * data2;
    gboolean v_return;

    g_return_if_fail(return_value != NULL);
    g_return_if_fail(n_param_values == 5);

    if (G_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = g_value_peek_pointer(param_values + 0);
    }
    else {
        data1 = g_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (_GDbusCodegenMarshalBoolean_ObjectUintUintVariantFunc)(marshal_data ? marshal_data : cc->callback);

    v_return = callback(data1, g_marshal_value_peek_object(param_values + 1), g_marshal_value_peek_uint(param_values + 2), g_marshal_value_peek_uint(param_values + 3), g_marshal_value_peek_variant(param_values + 4), data2);

    g_value_set_boolean(return_value, v_return);
}

/* ------------------------------------------------------------------------
 * Code for interface org.gnome.Mutter.DisplayConfig
 * ------------------------------------------------------------------------
 */

/**
 * SECTION:GDDBusDisplayConfig
 * @title: GDDBusDisplayConfig
 * @short_description: Generated C code for the org.gnome.Mutter.DisplayConfig D-Bus interface
 *
 * This section contains code for working with the <link linkend="gdbus-interface-org-gnome-Mutter-DisplayConfig.top_of_page">org.gnome.Mutter.DisplayConfig</link> D-Bus interface in C.
 */

enum { GD_DBUS__DISPLAY_CONFIG_MONITORS_CHANGED, };

static unsigned GD_DBUS__DISPLAY_CONFIG_SIGNALS[1] = {0};

/* ---- Introspection data for org.gnome.Mutter.DisplayConfig ---- */

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_get_resources_OUT_ARG_serial = {{-1, (gchar*)"serial", (gchar*)"u", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_get_resources_OUT_ARG_crtcs = {{-1, (gchar*)"crtcs", (gchar*)"a(uxiiiiiuaua{sv})", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_get_resources_OUT_ARG_outputs = {{-1, (gchar*)"outputs", (gchar*)"a(uxiausauaua{sv})", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_get_resources_OUT_ARG_modes = {{-1, (gchar*)"modes", (gchar*)"a(uxuudu)", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_get_resources_OUT_ARG_max_screen_width = {{-1, (gchar*)"max_screen_width", (gchar*)"i", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_get_resources_OUT_ARG_max_screen_height = {{-1, (gchar*)"max_screen_height", (gchar*)"i", NULL}, FALSE};

static const GDBusArgInfo* const _gd_dbus_display_config_method_info_get_resources_OUT_ARG_pointers[] = {
    &_gd_dbus_display_config_method_info_get_resources_OUT_ARG_serial.parent_struct, &_gd_dbus_display_config_method_info_get_resources_OUT_ARG_crtcs.parent_struct,
    &_gd_dbus_display_config_method_info_get_resources_OUT_ARG_outputs.parent_struct, &_gd_dbus_display_config_method_info_get_resources_OUT_ARG_modes.parent_struct,
    &_gd_dbus_display_config_method_info_get_resources_OUT_ARG_max_screen_width.parent_struct, &_gd_dbus_display_config_method_info_get_resources_OUT_ARG_max_screen_height.parent_struct, NULL
};

static const _ExtendedGDBusMethodInfo _gd_dbus_display_config_method_info_get_resources = {
    {-1, (gchar*)"GetResources", NULL, (GDBusArgInfo**)&_gd_dbus_display_config_method_info_get_resources_OUT_ARG_pointers, NULL}, "handle-get-resources", FALSE
};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_change_backlight_IN_ARG_serial = {{-1, (gchar*)"serial", (gchar*)"u", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_change_backlight_IN_ARG_output = {{-1, (gchar*)"output", (gchar*)"u", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_change_backlight_IN_ARG_value = {{-1, (gchar*)"value", (gchar*)"i", NULL}, FALSE};

static const GDBusArgInfo* const _gd_dbus_display_config_method_info_change_backlight_IN_ARG_pointers[] = {
    &_gd_dbus_display_config_method_info_change_backlight_IN_ARG_serial.parent_struct, &_gd_dbus_display_config_method_info_change_backlight_IN_ARG_output.parent_struct,
    &_gd_dbus_display_config_method_info_change_backlight_IN_ARG_value.parent_struct, NULL
};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_change_backlight_OUT_ARG_new_value = {{-1, (gchar*)"new_value", (gchar*)"i", NULL}, FALSE};

static const GDBusArgInfo* const _gd_dbus_display_config_method_info_change_backlight_OUT_ARG_pointers[] = {&_gd_dbus_display_config_method_info_change_backlight_OUT_ARG_new_value.parent_struct, NULL};

static const GDBusAnnotationInfo _gd_dbus_display_config_method_change_backlight_annotation_info_0 = {-1, (gchar*)"org.freedesktop.DBus.Deprecated", (gchar*)"true", NULL};

static const GDBusAnnotationInfo* const _gd_dbus_display_config_method_change_backlight_annotation_info_pointers[] = {&_gd_dbus_display_config_method_change_backlight_annotation_info_0, NULL};

static const _ExtendedGDBusMethodInfo _gd_dbus_display_config_method_info_change_backlight = {
    {
        -1, (gchar*)"ChangeBacklight", (GDBusArgInfo**)&_gd_dbus_display_config_method_info_change_backlight_IN_ARG_pointers, (GDBusArgInfo**)&_gd_dbus_display_config_method_info_change_backlight_OUT_ARG_pointers,
        (GDBusAnnotationInfo**)&_gd_dbus_display_config_method_change_backlight_annotation_info_pointers
    },
    "handle-change-backlight", FALSE
};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_set_backlight_IN_ARG_serial = {{-1, (gchar*)"serial", (gchar*)"u", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_set_backlight_IN_ARG_connector = {{-1, (gchar*)"connector", (gchar*)"s", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_set_backlight_IN_ARG_value = {{-1, (gchar*)"value", (gchar*)"i", NULL}, FALSE};

static const GDBusArgInfo* const _gd_dbus_display_config_method_info_set_backlight_IN_ARG_pointers[] = {
    &_gd_dbus_display_config_method_info_set_backlight_IN_ARG_serial.parent_struct, &_gd_dbus_display_config_method_info_set_backlight_IN_ARG_connector.parent_struct,
    &_gd_dbus_display_config_method_info_set_backlight_IN_ARG_value.parent_struct, NULL
};

static const _ExtendedGDBusMethodInfo _gd_dbus_display_config_method_info_set_backlight = {
    {-1, (gchar*)"SetBacklight", (GDBusArgInfo**)&_gd_dbus_display_config_method_info_set_backlight_IN_ARG_pointers, NULL, NULL}, "handle-set-backlight", FALSE
};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_get_crtc_gamma_IN_ARG_serial = {{-1, (gchar*)"serial", (gchar*)"u", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_get_crtc_gamma_IN_ARG_crtc = {{-1, (gchar*)"crtc", (gchar*)"u", NULL}, FALSE};

static const GDBusArgInfo* const _gd_dbus_display_config_method_info_get_crtc_gamma_IN_ARG_pointers[] = {
    &_gd_dbus_display_config_method_info_get_crtc_gamma_IN_ARG_serial.parent_struct, &_gd_dbus_display_config_method_info_get_crtc_gamma_IN_ARG_crtc.parent_struct, NULL
};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_get_crtc_gamma_OUT_ARG_red = {{-1, (gchar*)"red", (gchar*)"aq", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_get_crtc_gamma_OUT_ARG_green = {{-1, (gchar*)"green", (gchar*)"aq", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_get_crtc_gamma_OUT_ARG_blue = {{-1, (gchar*)"blue", (gchar*)"aq", NULL}, FALSE};

static const GDBusArgInfo* const _gd_dbus_display_config_method_info_get_crtc_gamma_OUT_ARG_pointers[] = {
    &_gd_dbus_display_config_method_info_get_crtc_gamma_OUT_ARG_red.parent_struct, &_gd_dbus_display_config_method_info_get_crtc_gamma_OUT_ARG_green.parent_struct,
    &_gd_dbus_display_config_method_info_get_crtc_gamma_OUT_ARG_blue.parent_struct, NULL
};

static const _ExtendedGDBusMethodInfo _gd_dbus_display_config_method_info_get_crtc_gamma = {
    {-1, (gchar*)"GetCrtcGamma", (GDBusArgInfo**)&_gd_dbus_display_config_method_info_get_crtc_gamma_IN_ARG_pointers, (GDBusArgInfo**)&_gd_dbus_display_config_method_info_get_crtc_gamma_OUT_ARG_pointers, NULL}, "handle-get-crtc-gamma",
    FALSE
};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_set_crtc_gamma_IN_ARG_serial = {{-1, (gchar*)"serial", (gchar*)"u", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_set_crtc_gamma_IN_ARG_crtc = {{-1, (gchar*)"crtc", (gchar*)"u", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_set_crtc_gamma_IN_ARG_red = {{-1, (gchar*)"red", (gchar*)"aq", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_set_crtc_gamma_IN_ARG_green = {{-1, (gchar*)"green", (gchar*)"aq", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_set_crtc_gamma_IN_ARG_blue = {{-1, (gchar*)"blue", (gchar*)"aq", NULL}, FALSE};

static const GDBusArgInfo* const _gd_dbus_display_config_method_info_set_crtc_gamma_IN_ARG_pointers[] = {
    &_gd_dbus_display_config_method_info_set_crtc_gamma_IN_ARG_serial.parent_struct, &_gd_dbus_display_config_method_info_set_crtc_gamma_IN_ARG_crtc.parent_struct,
    &_gd_dbus_display_config_method_info_set_crtc_gamma_IN_ARG_red.parent_struct, &_gd_dbus_display_config_method_info_set_crtc_gamma_IN_ARG_green.parent_struct, &_gd_dbus_display_config_method_info_set_crtc_gamma_IN_ARG_blue.parent_struct,
    NULL
};

static const _ExtendedGDBusMethodInfo _gd_dbus_display_config_method_info_set_crtc_gamma = {
    {-1, (gchar*)"SetCrtcGamma", (GDBusArgInfo**)&_gd_dbus_display_config_method_info_set_crtc_gamma_IN_ARG_pointers, NULL, NULL}, "handle-set-crtc-gamma", FALSE
};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_get_current_state_OUT_ARG_serial = {{-1, (gchar*)"serial", (gchar*)"u", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_get_current_state_OUT_ARG_monitors = {{-1, (gchar*)"monitors", (gchar*)"a((ssss)a(siiddada{sv})a{sv})", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_get_current_state_OUT_ARG_logical_monitors = {{-1, (gchar*)"logical_monitors", (gchar*)"a(iiduba(ssss)a{sv})", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_get_current_state_OUT_ARG_properties = {{-1, (gchar*)"properties", (gchar*)"a{sv}", NULL}, FALSE};

static const GDBusArgInfo* const _gd_dbus_display_config_method_info_get_current_state_OUT_ARG_pointers[] = {
    &_gd_dbus_display_config_method_info_get_current_state_OUT_ARG_serial.parent_struct, &_gd_dbus_display_config_method_info_get_current_state_OUT_ARG_monitors.parent_struct,
    &_gd_dbus_display_config_method_info_get_current_state_OUT_ARG_logical_monitors.parent_struct, &_gd_dbus_display_config_method_info_get_current_state_OUT_ARG_properties.parent_struct, NULL
};

static const _ExtendedGDBusMethodInfo _gd_dbus_display_config_method_info_get_current_state = {
    {-1, (gchar*)"GetCurrentState", NULL, (GDBusArgInfo**)&_gd_dbus_display_config_method_info_get_current_state_OUT_ARG_pointers, NULL}, "handle-get-current-state", FALSE
};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_apply_monitors_config_IN_ARG_serial = {{-1, (gchar*)"serial", (gchar*)"u", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_apply_monitors_config_IN_ARG_method = {{-1, (gchar*)"method", (gchar*)"u", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_apply_monitors_config_IN_ARG_logical_monitors = {{-1, (gchar*)"logical_monitors", (gchar*)"a(iiduba(ssa{sv}))", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_apply_monitors_config_IN_ARG_properties = {{-1, (gchar*)"properties", (gchar*)"a{sv}", NULL}, FALSE};

static const GDBusArgInfo* const _gd_dbus_display_config_method_info_apply_monitors_config_IN_ARG_pointers[] = {
    &_gd_dbus_display_config_method_info_apply_monitors_config_IN_ARG_serial.parent_struct, &_gd_dbus_display_config_method_info_apply_monitors_config_IN_ARG_method.parent_struct,
    &_gd_dbus_display_config_method_info_apply_monitors_config_IN_ARG_logical_monitors.parent_struct, &_gd_dbus_display_config_method_info_apply_monitors_config_IN_ARG_properties.parent_struct, NULL
};

static const _ExtendedGDBusMethodInfo _gd_dbus_display_config_method_info_apply_monitors_config = {
    {-1, (gchar*)"ApplyMonitorsConfig", (GDBusArgInfo**)&_gd_dbus_display_config_method_info_apply_monitors_config_IN_ARG_pointers, NULL, NULL}, "handle-apply-monitors-config", FALSE
};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_set_output_ctm_IN_ARG_serial = {{-1, (gchar*)"serial", (gchar*)"u", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_set_output_ctm_IN_ARG_output = {{-1, (gchar*)"output", (gchar*)"u", NULL}, FALSE};

static const _ExtendedGDBusArgInfo _gd_dbus_display_config_method_info_set_output_ctm_IN_ARG_ctm = {{-1, (gchar*)"ctm", (gchar*)"(ttttttttt)", NULL}, FALSE};

static const GDBusArgInfo* const _gd_dbus_display_config_method_info_set_output_ctm_IN_ARG_pointers[] = {
    &_gd_dbus_display_config_method_info_set_output_ctm_IN_ARG_serial.parent_struct, &_gd_dbus_display_config_method_info_set_output_ctm_IN_ARG_output.parent_struct,
    &_gd_dbus_display_config_method_info_set_output_ctm_IN_ARG_ctm.parent_struct, NULL
};

static const _ExtendedGDBusMethodInfo _gd_dbus_display_config_method_info_set_output_ctm = {
    {-1, (gchar*)"SetOutputCTM", (GDBusArgInfo**)&_gd_dbus_display_config_method_info_set_output_ctm_IN_ARG_pointers, NULL, NULL}, "handle-set-output-ctm", FALSE
};

static const GDBusMethodInfo* const _gd_dbus_display_config_method_info_pointers[] = {
    &_gd_dbus_display_config_method_info_get_resources.parent_struct, &_gd_dbus_display_config_method_info_change_backlight.parent_struct, &_gd_dbus_display_config_method_info_set_backlight.parent_struct,
    &_gd_dbus_display_config_method_info_get_crtc_gamma.parent_struct, &_gd_dbus_display_config_method_info_set_crtc_gamma.parent_struct, &_gd_dbus_display_config_method_info_get_current_state.parent_struct,
    &_gd_dbus_display_config_method_info_apply_monitors_config.parent_struct, &_gd_dbus_display_config_method_info_set_output_ctm.parent_struct, NULL
};

static const _ExtendedGDBusSignalInfo _gd_dbus_display_config_signal_info_monitors_changed = {{-1, (gchar*)"MonitorsChanged", NULL, NULL}, "monitors-changed"};

static const GDBusSignalInfo* const _gd_dbus_display_config_signal_info_pointers[] = {&_gd_dbus_display_config_signal_info_monitors_changed.parent_struct, NULL};

static const _ExtendedGDBusPropertyInfo _gd_dbus_display_config_property_info_backlight = {{-1, (gchar*)"Backlight", (gchar*)"(uaa{sv})", G_DBUS_PROPERTY_INFO_FLAGS_READABLE, NULL}, "backlight", FALSE, TRUE};

static const _ExtendedGDBusPropertyInfo _gd_dbus_display_config_property_info_power_save_mode = {
    {-1, (gchar*)"PowerSaveMode", (gchar*)"i", G_DBUS_PROPERTY_INFO_FLAGS_READABLE | G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE, NULL}, "power-save-mode", FALSE, TRUE
};

static const _ExtendedGDBusPropertyInfo _gd_dbus_display_config_property_info_panel_orientation_managed = {
    {-1, (gchar*)"PanelOrientationManaged", (gchar*)"b", G_DBUS_PROPERTY_INFO_FLAGS_READABLE, NULL}, "panel-orientation-managed", FALSE, TRUE
};

static const _ExtendedGDBusPropertyInfo _gd_dbus_display_config_property_info_apply_monitors_config_allowed = {
    {-1, (gchar*)"ApplyMonitorsConfigAllowed", (gchar*)"b", G_DBUS_PROPERTY_INFO_FLAGS_READABLE, NULL}, "apply-monitors-config-allowed", FALSE, TRUE
};

static const _ExtendedGDBusPropertyInfo _gd_dbus_display_config_property_info_night_light_supported = {{-1, (gchar*)"NightLightSupported", (gchar*)"b", G_DBUS_PROPERTY_INFO_FLAGS_READABLE, NULL}, "night-light-supported", FALSE, TRUE};

static const _ExtendedGDBusPropertyInfo _gd_dbus_display_config_property_info_has_external_monitor = {{-1, (gchar*)"HasExternalMonitor", (gchar*)"b", G_DBUS_PROPERTY_INFO_FLAGS_READABLE, NULL}, "has-external-monitor", FALSE, TRUE};

static const GDBusPropertyInfo* const _gd_dbus_display_config_property_info_pointers[] = {
    &_gd_dbus_display_config_property_info_backlight.parent_struct, &_gd_dbus_display_config_property_info_power_save_mode.parent_struct, &_gd_dbus_display_config_property_info_panel_orientation_managed.parent_struct,
    &_gd_dbus_display_config_property_info_apply_monitors_config_allowed.parent_struct, &_gd_dbus_display_config_property_info_night_light_supported.parent_struct, &_gd_dbus_display_config_property_info_has_external_monitor.parent_struct,
    NULL
};

static const _ExtendedGDBusInterfaceInfo _gd_dbus_display_config_interface_info = {
    {
        -1, (gchar*)"org.gnome.Mutter.DisplayConfig", (GDBusMethodInfo**)&_gd_dbus_display_config_method_info_pointers, (GDBusSignalInfo**)&_gd_dbus_display_config_signal_info_pointers,
        (GDBusPropertyInfo**)&_gd_dbus_display_config_property_info_pointers, NULL
    },
    "display-config",
};


/**
 * gd_dbus_display_config_interface_info:
 *
 * Gets a machine-readable description of the <link linkend="gdbus-interface-org-gnome-Mutter-DisplayConfig.top_of_page">org.gnome.Mutter.DisplayConfig</link> D-Bus interface.
 *
 * Returns: (transfer none): A #GDBusInterfaceInfo. Do not free.
 */
GDBusInterfaceInfo*
gd_dbus_display_config_interface_info(void)
{
    return (GDBusInterfaceInfo*)&_gd_dbus_display_config_interface_info.parent_struct;
}

/**
 * gd_dbus_display_config_override_properties:
 * @klass: The class structure for a #GObject derived class.
 * @property_id_begin: The property id to assign to the first overridden property.
 *
 * Overrides all #GObject properties in the #GDDBusDisplayConfig interface for a concrete class.
 * The properties are overridden in the order they are defined.
 *
 * Returns: The last property id.
 */
guint
gd_dbus_display_config_override_properties(GObjectClass* klass, guint property_id_begin)
{
    g_object_class_override_property(klass, property_id_begin++, "backlight");
    g_object_class_override_property(klass, property_id_begin++, "power-save-mode");
    g_object_class_override_property(klass, property_id_begin++, "panel-orientation-managed");
    g_object_class_override_property(klass, property_id_begin++, "apply-monitors-config-allowed");
    g_object_class_override_property(klass, property_id_begin++, "night-light-supported");
    g_object_class_override_property(klass, property_id_begin++, "has-external-monitor");
    return property_id_begin - 1;
}


inline static void
gd_dbus_display_config_signal_marshal_monitors_changed(GClosure* closure, GValue* return_value, unsigned int n_param_values, const GValue* param_values, void* invocation_hint, void* marshal_data)
{
    g_cclosure_marshal_VOID__VOID(closure, return_value, n_param_values, param_values, invocation_hint, marshal_data);
}

inline static void
gd_dbus_display_config_method_marshal_get_resources(GClosure* closure, GValue* return_value, unsigned int n_param_values, const GValue* param_values, void* invocation_hint, void* marshal_data)
{
    _g_dbus_codegen_marshal_BOOLEAN__OBJECT(closure, return_value, n_param_values, param_values, invocation_hint, marshal_data);
}

inline static void
gd_dbus_display_config_method_marshal_change_backlight(GClosure* closure, GValue* return_value, unsigned int n_param_values, const GValue* param_values, void* invocation_hint, void* marshal_data)
{
    _g_dbus_codegen_marshal_BOOLEAN__OBJECT_UINT_UINT_INT(closure, return_value, n_param_values, param_values, invocation_hint, marshal_data);
}

inline static void
gd_dbus_display_config_method_marshal_set_backlight(GClosure* closure, GValue* return_value, unsigned int n_param_values, const GValue* param_values, void* invocation_hint, void* marshal_data)
{
    _g_dbus_codegen_marshal_BOOLEAN__OBJECT_UINT_STRING_INT(closure, return_value, n_param_values, param_values, invocation_hint, marshal_data);
}

inline static void
gd_dbus_display_config_method_marshal_get_crtc_gamma(GClosure* closure, GValue* return_value, unsigned int n_param_values, const GValue* param_values, void* invocation_hint, void* marshal_data)
{
    _g_dbus_codegen_marshal_BOOLEAN__OBJECT_UINT_UINT(closure, return_value, n_param_values, param_values, invocation_hint, marshal_data);
}

inline static void
gd_dbus_display_config_method_marshal_set_crtc_gamma(GClosure* closure, GValue* return_value, unsigned int n_param_values, const GValue* param_values, void* invocation_hint, void* marshal_data)
{
    _g_dbus_codegen_marshal_BOOLEAN__OBJECT_UINT_UINT_VARIANT_VARIANT_VARIANT(closure, return_value, n_param_values, param_values, invocation_hint, marshal_data);
}

inline static void
gd_dbus_display_config_method_marshal_get_current_state(GClosure* closure, GValue* return_value, unsigned int n_param_values, const GValue* param_values, void* invocation_hint, void* marshal_data)
{
    _g_dbus_codegen_marshal_BOOLEAN__OBJECT(closure, return_value, n_param_values, param_values, invocation_hint, marshal_data);
}

inline static void
gd_dbus_display_config_method_marshal_apply_monitors_config(GClosure* closure, GValue* return_value, unsigned int n_param_values, const GValue* param_values, void* invocation_hint, void* marshal_data)
{
    _g_dbus_codegen_marshal_BOOLEAN__OBJECT_UINT_UINT_VARIANT_VARIANT(closure, return_value, n_param_values, param_values, invocation_hint, marshal_data);
}

inline static void
gd_dbus_display_config_method_marshal_set_output_ctm(GClosure* closure, GValue* return_value, unsigned int n_param_values, const GValue* param_values, void* invocation_hint, void* marshal_data)
{
    _g_dbus_codegen_marshal_BOOLEAN__OBJECT_UINT_UINT_VARIANT(closure, return_value, n_param_values, param_values, invocation_hint, marshal_data);
}


/**
 * GDDBusDisplayConfig:
 *
 * Abstract interface type for the D-Bus interface <link linkend="gdbus-interface-org-gnome-Mutter-DisplayConfig.top_of_page">org.gnome.Mutter.DisplayConfig</link>.
 */

/**
 * GDDBusDisplayConfigIface:
 * @parent_iface: The parent interface.
 * @handle_apply_monitors_config: Handler for the #GDDBusDisplayConfig::handle-apply-monitors-config signal.
 * @handle_change_backlight: Handler for the #GDDBusDisplayConfig::handle-change-backlight signal.
 * @handle_get_crtc_gamma: Handler for the #GDDBusDisplayConfig::handle-get-crtc-gamma signal.
 * @handle_get_current_state: Handler for the #GDDBusDisplayConfig::handle-get-current-state signal.
 * @handle_get_resources: Handler for the #GDDBusDisplayConfig::handle-get-resources signal.
 * @handle_set_backlight: Handler for the #GDDBusDisplayConfig::handle-set-backlight signal.
 * @handle_set_crtc_gamma: Handler for the #GDDBusDisplayConfig::handle-set-crtc-gamma signal.
 * @handle_set_output_ctm: Handler for the #GDDBusDisplayConfig::handle-set-output-ctm signal.
 * @get_apply_monitors_config_allowed: Getter for the #GDDBusDisplayConfig:apply-monitors-config-allowed property.
 * @get_backlight: Getter for the #GDDBusDisplayConfig:backlight property.
 * @get_has_external_monitor: Getter for the #GDDBusDisplayConfig:has-external-monitor property.
 * @get_night_light_supported: Getter for the #GDDBusDisplayConfig:night-light-supported property.
 * @get_panel_orientation_managed: Getter for the #GDDBusDisplayConfig:panel-orientation-managed property.
 * @get_power_save_mode: Getter for the #GDDBusDisplayConfig:power-save-mode property.
 * @monitors_changed: Handler for the #GDDBusDisplayConfig::monitors-changed signal.
 *
 * Virtual table for the D-Bus interface <link linkend="gdbus-interface-org-gnome-Mutter-DisplayConfig.top_of_page">org.gnome.Mutter.DisplayConfig</link>.
 */

typedef GDDBusDisplayConfigIface GDDBusDisplayConfigInterface;
G_DEFINE_INTERFACE(GDDBusDisplayConfig, gd_dbus_display_config, G_TYPE_OBJECT)

static void
gd_dbus_display_config_default_init(GDDBusDisplayConfigIface* iface)
{
    /* GObject signals for incoming D-Bus method calls: */
    /**
     * GDDBusDisplayConfig::handle-get-resources:
     * @object: A #GDDBusDisplayConfig.
     * @invocation: A #GDBusMethodInvocation.
     *
     * Signal emitted when a remote caller is invoking the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.GetResources">GetResources()</link> D-Bus method.
     *
     * If a signal handler returns %TRUE, it means the signal handler will handle the invocation (e.g. take a reference to @invocation and eventually call gd_dbus_display_config_complete_get_resources() or e.g. g_dbus_method_invocation_return_error() on it) and no other signal handlers will run. If no signal handler handles the invocation, the %G_DBUS_ERROR_UNKNOWN_METHOD error is returned.
     *
     * Returns: %G_DBUS_METHOD_INVOCATION_HANDLED or %TRUE if the invocation was handled, %G_DBUS_METHOD_INVOCATION_UNHANDLED or %FALSE to let other signal handlers run.
     */
    g_signal_new("handle-get-resources", G_TYPE_FROM_INTERFACE(iface), G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET(GDDBusDisplayConfigIface, handle_get_resources), g_signal_accumulator_true_handled, NULL,
                 gd_dbus_display_config_method_marshal_get_resources, G_TYPE_BOOLEAN, 1, G_TYPE_DBUS_METHOD_INVOCATION);

    /**
     * GDDBusDisplayConfig::handle-change-backlight:
     * @object: A #GDDBusDisplayConfig.
     * @invocation: A #GDBusMethodInvocation.
     * @arg_serial: Argument passed by remote caller.
     * @arg_output: Argument passed by remote caller.
     * @arg_value: Argument passed by remote caller.
     *
     * Signal emitted when a remote caller is invoking the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.ChangeBacklight">ChangeBacklight()</link> D-Bus method.
     *
     * If a signal handler returns %TRUE, it means the signal handler will handle the invocation (e.g. take a reference to @invocation and eventually call gd_dbus_display_config_complete_change_backlight() or e.g. g_dbus_method_invocation_return_error() on it) and no other signal handlers will run. If no signal handler handles the invocation, the %G_DBUS_ERROR_UNKNOWN_METHOD error is returned.
     *
     * Returns: %G_DBUS_METHOD_INVOCATION_HANDLED or %TRUE if the invocation was handled, %G_DBUS_METHOD_INVOCATION_UNHANDLED or %FALSE to let other signal handlers run.
     *
     * Deprecated: The D-Bus method has been deprecated.
     */
    g_signal_new("handle-change-backlight", G_TYPE_FROM_INTERFACE(iface), G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET(GDDBusDisplayConfigIface, handle_change_backlight), g_signal_accumulator_true_handled, NULL,
                 gd_dbus_display_config_method_marshal_change_backlight, G_TYPE_BOOLEAN, 4, G_TYPE_DBUS_METHOD_INVOCATION, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INT);

    /**
     * GDDBusDisplayConfig::handle-set-backlight:
     * @object: A #GDDBusDisplayConfig.
     * @invocation: A #GDBusMethodInvocation.
     * @arg_serial: Argument passed by remote caller.
     * @arg_connector: Argument passed by remote caller.
     * @arg_value: Argument passed by remote caller.
     *
     * Signal emitted when a remote caller is invoking the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.SetBacklight">SetBacklight()</link> D-Bus method.
     *
     * If a signal handler returns %TRUE, it means the signal handler will handle the invocation (e.g. take a reference to @invocation and eventually call gd_dbus_display_config_complete_set_backlight() or e.g. g_dbus_method_invocation_return_error() on it) and no other signal handlers will run. If no signal handler handles the invocation, the %G_DBUS_ERROR_UNKNOWN_METHOD error is returned.
     *
     * Returns: %G_DBUS_METHOD_INVOCATION_HANDLED or %TRUE if the invocation was handled, %G_DBUS_METHOD_INVOCATION_UNHANDLED or %FALSE to let other signal handlers run.
     */
    g_signal_new("handle-set-backlight", G_TYPE_FROM_INTERFACE(iface), G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET(GDDBusDisplayConfigIface, handle_set_backlight), g_signal_accumulator_true_handled, NULL,
                 gd_dbus_display_config_method_marshal_set_backlight, G_TYPE_BOOLEAN, 4, G_TYPE_DBUS_METHOD_INVOCATION, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_INT);

    /**
     * GDDBusDisplayConfig::handle-get-crtc-gamma:
     * @object: A #GDDBusDisplayConfig.
     * @invocation: A #GDBusMethodInvocation.
     * @arg_serial: Argument passed by remote caller.
     * @arg_crtc: Argument passed by remote caller.
     *
     * Signal emitted when a remote caller is invoking the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.GetCrtcGamma">GetCrtcGamma()</link> D-Bus method.
     *
     * If a signal handler returns %TRUE, it means the signal handler will handle the invocation (e.g. take a reference to @invocation and eventually call gd_dbus_display_config_complete_get_crtc_gamma() or e.g. g_dbus_method_invocation_return_error() on it) and no other signal handlers will run. If no signal handler handles the invocation, the %G_DBUS_ERROR_UNKNOWN_METHOD error is returned.
     *
     * Returns: %G_DBUS_METHOD_INVOCATION_HANDLED or %TRUE if the invocation was handled, %G_DBUS_METHOD_INVOCATION_UNHANDLED or %FALSE to let other signal handlers run.
     */
    g_signal_new("handle-get-crtc-gamma", G_TYPE_FROM_INTERFACE(iface), G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET(GDDBusDisplayConfigIface, handle_get_crtc_gamma), g_signal_accumulator_true_handled, NULL,
                 gd_dbus_display_config_method_marshal_get_crtc_gamma, G_TYPE_BOOLEAN, 3, G_TYPE_DBUS_METHOD_INVOCATION, G_TYPE_UINT, G_TYPE_UINT);

    /**
     * GDDBusDisplayConfig::handle-set-crtc-gamma:
     * @object: A #GDDBusDisplayConfig.
     * @invocation: A #GDBusMethodInvocation.
     * @arg_serial: Argument passed by remote caller.
     * @arg_crtc: Argument passed by remote caller.
     * @arg_red: Argument passed by remote caller.
     * @arg_green: Argument passed by remote caller.
     * @arg_blue: Argument passed by remote caller.
     *
     * Signal emitted when a remote caller is invoking the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.SetCrtcGamma">SetCrtcGamma()</link> D-Bus method.
     *
     * If a signal handler returns %TRUE, it means the signal handler will handle the invocation (e.g. take a reference to @invocation and eventually call gd_dbus_display_config_complete_set_crtc_gamma() or e.g. g_dbus_method_invocation_return_error() on it) and no other signal handlers will run. If no signal handler handles the invocation, the %G_DBUS_ERROR_UNKNOWN_METHOD error is returned.
     *
     * Returns: %G_DBUS_METHOD_INVOCATION_HANDLED or %TRUE if the invocation was handled, %G_DBUS_METHOD_INVOCATION_UNHANDLED or %FALSE to let other signal handlers run.
     */
    g_signal_new("handle-set-crtc-gamma", G_TYPE_FROM_INTERFACE(iface), G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET(GDDBusDisplayConfigIface, handle_set_crtc_gamma), g_signal_accumulator_true_handled, NULL,
                 gd_dbus_display_config_method_marshal_set_crtc_gamma, G_TYPE_BOOLEAN, 6, G_TYPE_DBUS_METHOD_INVOCATION, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_VARIANT, G_TYPE_VARIANT, G_TYPE_VARIANT);

    /**
     * GDDBusDisplayConfig::handle-get-current-state:
     * @object: A #GDDBusDisplayConfig.
     * @invocation: A #GDBusMethodInvocation.
     *
     * Signal emitted when a remote caller is invoking the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.GetCurrentState">GetCurrentState()</link> D-Bus method.
     *
     * If a signal handler returns %TRUE, it means the signal handler will handle the invocation (e.g. take a reference to @invocation and eventually call gd_dbus_display_config_complete_get_current_state() or e.g. g_dbus_method_invocation_return_error() on it) and no other signal handlers will run. If no signal handler handles the invocation, the %G_DBUS_ERROR_UNKNOWN_METHOD error is returned.
     *
     * Returns: %G_DBUS_METHOD_INVOCATION_HANDLED or %TRUE if the invocation was handled, %G_DBUS_METHOD_INVOCATION_UNHANDLED or %FALSE to let other signal handlers run.
     */
    g_signal_new("handle-get-current-state", G_TYPE_FROM_INTERFACE(iface), G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET(GDDBusDisplayConfigIface, handle_get_current_state), g_signal_accumulator_true_handled, NULL,
                 gd_dbus_display_config_method_marshal_get_current_state, G_TYPE_BOOLEAN, 1, G_TYPE_DBUS_METHOD_INVOCATION);

    /**
     * GDDBusDisplayConfig::handle-apply-monitors-config:
     * @object: A #GDDBusDisplayConfig.
     * @invocation: A #GDBusMethodInvocation.
     * @arg_serial: Argument passed by remote caller.
     * @arg_method: Argument passed by remote caller.
     * @arg_logical_monitors: Argument passed by remote caller.
     * @arg_properties: Argument passed by remote caller.
     *
     * Signal emitted when a remote caller is invoking the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.ApplyMonitorsConfig">ApplyMonitorsConfig()</link> D-Bus method.
     *
     * If a signal handler returns %TRUE, it means the signal handler will handle the invocation (e.g. take a reference to @invocation and eventually call gd_dbus_display_config_complete_apply_monitors_config() or e.g. g_dbus_method_invocation_return_error() on it) and no other signal handlers will run. If no signal handler handles the invocation, the %G_DBUS_ERROR_UNKNOWN_METHOD error is returned.
     *
     * Returns: %G_DBUS_METHOD_INVOCATION_HANDLED or %TRUE if the invocation was handled, %G_DBUS_METHOD_INVOCATION_UNHANDLED or %FALSE to let other signal handlers run.
     */
    g_signal_new("handle-apply-monitors-config", G_TYPE_FROM_INTERFACE(iface), G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET(GDDBusDisplayConfigIface, handle_apply_monitors_config), g_signal_accumulator_true_handled, NULL,
                 gd_dbus_display_config_method_marshal_apply_monitors_config, G_TYPE_BOOLEAN, 5, G_TYPE_DBUS_METHOD_INVOCATION, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_VARIANT, G_TYPE_VARIANT);

    /**
     * GDDBusDisplayConfig::handle-set-output-ctm:
     * @object: A #GDDBusDisplayConfig.
     * @invocation: A #GDBusMethodInvocation.
     * @arg_serial: Argument passed by remote caller.
     * @arg_output: Argument passed by remote caller.
     * @arg_ctm: Argument passed by remote caller.
     *
     * Signal emitted when a remote caller is invoking the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.SetOutputCTM">SetOutputCTM()</link> D-Bus method.
     *
     * If a signal handler returns %TRUE, it means the signal handler will handle the invocation (e.g. take a reference to @invocation and eventually call gd_dbus_display_config_complete_set_output_ctm() or e.g. g_dbus_method_invocation_return_error() on it) and no other signal handlers will run. If no signal handler handles the invocation, the %G_DBUS_ERROR_UNKNOWN_METHOD error is returned.
     *
     * Returns: %G_DBUS_METHOD_INVOCATION_HANDLED or %TRUE if the invocation was handled, %G_DBUS_METHOD_INVOCATION_UNHANDLED or %FALSE to let other signal handlers run.
     */
    g_signal_new("handle-set-output-ctm", G_TYPE_FROM_INTERFACE(iface), G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET(GDDBusDisplayConfigIface, handle_set_output_ctm), g_signal_accumulator_true_handled, NULL,
                 gd_dbus_display_config_method_marshal_set_output_ctm, G_TYPE_BOOLEAN, 4, G_TYPE_DBUS_METHOD_INVOCATION, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_VARIANT);

    /* GObject signals for received D-Bus signals: */
    /**
     * GDDBusDisplayConfig::monitors-changed:
     * @object: A #GDDBusDisplayConfig.
     *
     * On the client-side, this signal is emitted whenever the D-Bus signal <link linkend="gdbus-signal-org-gnome-Mutter-DisplayConfig.MonitorsChanged">"MonitorsChanged"</link> is received.
     *
     * On the service-side, this signal can be used with e.g. g_signal_emit_by_name() to make the object emit the D-Bus signal.
     */
    GD_DBUS__DISPLAY_CONFIG_SIGNALS[GD_DBUS__DISPLAY_CONFIG_MONITORS_CHANGED] = g_signal_new("monitors-changed", G_TYPE_FROM_INTERFACE(iface), G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET(GDDBusDisplayConfigIface, monitors_changed), NULL, NULL,
                                                                                             gd_dbus_display_config_signal_marshal_monitors_changed, G_TYPE_NONE, 0);

    /* GObject properties for D-Bus properties: */
    /**
     * GDDBusDisplayConfig:backlight:
     *
     * Represents the D-Bus property <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.Backlight">"Backlight"</link>.
     *
     * Since the D-Bus property for this #GObject property is readable but not writable, it is meaningful to read from it on both the client- and service-side. It is only meaningful, however, to write to it on the service-side.
     */
    g_object_interface_install_property(iface, g_param_spec_variant("backlight", "Backlight", "Backlight", G_VARIANT_TYPE("(uaa{sv})"), NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    /**
     * GDDBusDisplayConfig:power-save-mode:
     *
     * Represents the D-Bus property <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.PowerSaveMode">"PowerSaveMode"</link>.
     *
     * Since the D-Bus property for this #GObject property is both readable and writable, it is meaningful to both read from it and write to it on both the service- and client-side.
     */
    g_object_interface_install_property(iface, g_param_spec_int("power-save-mode", "PowerSaveMode", "PowerSaveMode", G_MININT32, G_MAXINT32, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    /**
     * GDDBusDisplayConfig:panel-orientation-managed:
     *
     * Represents the D-Bus property <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.PanelOrientationManaged">"PanelOrientationManaged"</link>.
     *
     * Since the D-Bus property for this #GObject property is readable but not writable, it is meaningful to read from it on both the client- and service-side. It is only meaningful, however, to write to it on the service-side.
     */
    g_object_interface_install_property(iface, g_param_spec_boolean("panel-orientation-managed", "PanelOrientationManaged", "PanelOrientationManaged", FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    /**
     * GDDBusDisplayConfig:apply-monitors-config-allowed:
     *
     * Represents the D-Bus property <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.ApplyMonitorsConfigAllowed">"ApplyMonitorsConfigAllowed"</link>.
     *
     * Since the D-Bus property for this #GObject property is readable but not writable, it is meaningful to read from it on both the client- and service-side. It is only meaningful, however, to write to it on the service-side.
     */
    g_object_interface_install_property(iface, g_param_spec_boolean("apply-monitors-config-allowed", "ApplyMonitorsConfigAllowed", "ApplyMonitorsConfigAllowed", FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    /**
     * GDDBusDisplayConfig:night-light-supported:
     *
     * Represents the D-Bus property <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.NightLightSupported">"NightLightSupported"</link>.
     *
     * Since the D-Bus property for this #GObject property is readable but not writable, it is meaningful to read from it on both the client- and service-side. It is only meaningful, however, to write to it on the service-side.
     */
    g_object_interface_install_property(iface, g_param_spec_boolean("night-light-supported", "NightLightSupported", "NightLightSupported", FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    /**
     * GDDBusDisplayConfig:has-external-monitor:
     *
     * Represents the D-Bus property <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.HasExternalMonitor">"HasExternalMonitor"</link>.
     *
     * Since the D-Bus property for this #GObject property is readable but not writable, it is meaningful to read from it on both the client- and service-side. It is only meaningful, however, to write to it on the service-side.
     */
    g_object_interface_install_property(iface, g_param_spec_boolean("has-external-monitor", "HasExternalMonitor", "HasExternalMonitor", FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

/**
 * gd_dbus_display_config_get_backlight: (skip)
 * @object: A #GDDBusDisplayConfig.
 *
 * Gets the value of the <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.Backlight">"Backlight"</link> D-Bus property.
 *
 * Since this D-Bus property is readable, it is meaningful to use this function on both the client- and service-side.
 *
 * The returned value is only valid until the property changes so on the client-side it is only safe to use this function on the thread where @object was constructed. Use gd_dbus_display_config_dup_backlight() if on another thread.
 *
 * Returns: (transfer none) (nullable): The property value or %NULL if the property is not set. Do not free the returned value, it belongs to @object.
 */
GVariant*
gd_dbus_display_config_get_backlight(GDDBusDisplayConfig* object)
{
    g_return_val_if_fail(GD_DBUS_IS_DISPLAY_CONFIG (object), NULL);

    return GD_DBUS_DISPLAY_CONFIG_GET_IFACE(object)->get_backlight(object);
}

/**
 * gd_dbus_display_config_dup_backlight: (skip)
 * @object: A #GDDBusDisplayConfig.
 *
 * Gets a copy of the <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.Backlight">"Backlight"</link> D-Bus property.
 *
 * Since this D-Bus property is readable, it is meaningful to use this function on both the client- and service-side.
 *
 * Returns: (transfer full) (nullable): The property value or %NULL if the property is not set. The returned value should be freed with g_variant_unref().
 */
GVariant*
gd_dbus_display_config_dup_backlight(GDDBusDisplayConfig* object)
{
    GVariant* value;
    g_object_get(G_OBJECT(object), "backlight", &value, NULL);
    return value;
}

/**
 * gd_dbus_display_config_set_backlight: (skip)
 * @object: A #GDDBusDisplayConfig.
 * @value: The value to set.
 *
 * Sets the <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.Backlight">"Backlight"</link> D-Bus property to @value.
 *
 * Since this D-Bus property is not writable, it is only meaningful to use this function on the service-side.
 */
void
gd_dbus_display_config_set_backlight(GDDBusDisplayConfig* object, GVariant* value)
{
    g_object_set(G_OBJECT(object), "backlight", value, NULL);
}

/**
 * gd_dbus_display_config_get_power_save_mode: (skip)
 * @object: A #GDDBusDisplayConfig.
 *
 * Gets the value of the <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.PowerSaveMode">"PowerSaveMode"</link> D-Bus property.
 *
 * Since this D-Bus property is both readable and writable, it is meaningful to use this function on both the client- and service-side.
 *
 * Returns: The property value.
 */
gint
gd_dbus_display_config_get_power_save_mode(GDDBusDisplayConfig* object)
{
    g_return_val_if_fail(GD_DBUS_IS_DISPLAY_CONFIG (object), 0);

    return GD_DBUS_DISPLAY_CONFIG_GET_IFACE(object)->get_power_save_mode(object);
}

/**
 * gd_dbus_display_config_set_power_save_mode: (skip)
 * @object: A #GDDBusDisplayConfig.
 * @value: The value to set.
 *
 * Sets the <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.PowerSaveMode">"PowerSaveMode"</link> D-Bus property to @value.
 *
 * Since this D-Bus property is both readable and writable, it is meaningful to use this function on both the client- and service-side.
 */
void
gd_dbus_display_config_set_power_save_mode(GDDBusDisplayConfig* object, gint value)
{
    g_object_set(G_OBJECT(object), "power-save-mode", value, NULL);
}

/**
 * gd_dbus_display_config_get_panel_orientation_managed: (skip)
 * @object: A #GDDBusDisplayConfig.
 *
 * Gets the value of the <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.PanelOrientationManaged">"PanelOrientationManaged"</link> D-Bus property.
 *
 * Since this D-Bus property is readable, it is meaningful to use this function on both the client- and service-side.
 *
 * Returns: The property value.
 */
gboolean
gd_dbus_display_config_get_panel_orientation_managed(GDDBusDisplayConfig* object)
{
    g_return_val_if_fail(GD_DBUS_IS_DISPLAY_CONFIG (object), FALSE);

    return GD_DBUS_DISPLAY_CONFIG_GET_IFACE(object)->get_panel_orientation_managed(object);
}

/**
 * gd_dbus_display_config_set_panel_orientation_managed: (skip)
 * @object: A #GDDBusDisplayConfig.
 * @value: The value to set.
 *
 * Sets the <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.PanelOrientationManaged">"PanelOrientationManaged"</link> D-Bus property to @value.
 *
 * Since this D-Bus property is not writable, it is only meaningful to use this function on the service-side.
 */
void
gd_dbus_display_config_set_panel_orientation_managed(GDDBusDisplayConfig* object, gboolean value)
{
    g_object_set(G_OBJECT(object), "panel-orientation-managed", value, NULL);
}

/**
 * gd_dbus_display_config_get_apply_monitors_config_allowed: (skip)
 * @object: A #GDDBusDisplayConfig.
 *
 * Gets the value of the <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.ApplyMonitorsConfigAllowed">"ApplyMonitorsConfigAllowed"</link> D-Bus property.
 *
 * Since this D-Bus property is readable, it is meaningful to use this function on both the client- and service-side.
 *
 * Returns: The property value.
 */
gboolean
gd_dbus_display_config_get_apply_monitors_config_allowed(GDDBusDisplayConfig* object)
{
    g_return_val_if_fail(GD_DBUS_IS_DISPLAY_CONFIG (object), FALSE);

    return GD_DBUS_DISPLAY_CONFIG_GET_IFACE(object)->get_apply_monitors_config_allowed(object);
}

/**
 * gd_dbus_display_config_set_apply_monitors_config_allowed: (skip)
 * @object: A #GDDBusDisplayConfig.
 * @value: The value to set.
 *
 * Sets the <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.ApplyMonitorsConfigAllowed">"ApplyMonitorsConfigAllowed"</link> D-Bus property to @value.
 *
 * Since this D-Bus property is not writable, it is only meaningful to use this function on the service-side.
 */
void
gd_dbus_display_config_set_apply_monitors_config_allowed(GDDBusDisplayConfig* object, gboolean value)
{
    g_object_set(G_OBJECT(object), "apply-monitors-config-allowed", value, NULL);
}

/**
 * gd_dbus_display_config_get_night_light_supported: (skip)
 * @object: A #GDDBusDisplayConfig.
 *
 * Gets the value of the <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.NightLightSupported">"NightLightSupported"</link> D-Bus property.
 *
 * Since this D-Bus property is readable, it is meaningful to use this function on both the client- and service-side.
 *
 * Returns: The property value.
 */
gboolean
gd_dbus_display_config_get_night_light_supported(GDDBusDisplayConfig* object)
{
    g_return_val_if_fail(GD_DBUS_IS_DISPLAY_CONFIG (object), FALSE);

    return GD_DBUS_DISPLAY_CONFIG_GET_IFACE(object)->get_night_light_supported(object);
}

/**
 * gd_dbus_display_config_set_night_light_supported: (skip)
 * @object: A #GDDBusDisplayConfig.
 * @value: The value to set.
 *
 * Sets the <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.NightLightSupported">"NightLightSupported"</link> D-Bus property to @value.
 *
 * Since this D-Bus property is not writable, it is only meaningful to use this function on the service-side.
 */
void
gd_dbus_display_config_set_night_light_supported(GDDBusDisplayConfig* object, gboolean value)
{
    g_object_set(G_OBJECT(object), "night-light-supported", value, NULL);
}

/**
 * gd_dbus_display_config_get_has_external_monitor: (skip)
 * @object: A #GDDBusDisplayConfig.
 *
 * Gets the value of the <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.HasExternalMonitor">"HasExternalMonitor"</link> D-Bus property.
 *
 * Since this D-Bus property is readable, it is meaningful to use this function on both the client- and service-side.
 *
 * Returns: The property value.
 */
gboolean
gd_dbus_display_config_get_has_external_monitor(GDDBusDisplayConfig* object)
{
    g_return_val_if_fail(GD_DBUS_IS_DISPLAY_CONFIG (object), FALSE);

    return GD_DBUS_DISPLAY_CONFIG_GET_IFACE(object)->get_has_external_monitor(object);
}

/**
 * gd_dbus_display_config_set_has_external_monitor: (skip)
 * @object: A #GDDBusDisplayConfig.
 * @value: The value to set.
 *
 * Sets the <link linkend="gdbus-property-org-gnome-Mutter-DisplayConfig.HasExternalMonitor">"HasExternalMonitor"</link> D-Bus property to @value.
 *
 * Since this D-Bus property is not writable, it is only meaningful to use this function on the service-side.
 */
void
gd_dbus_display_config_set_has_external_monitor(GDDBusDisplayConfig* object, gboolean value)
{
    g_object_set(G_OBJECT(object), "has-external-monitor", value, NULL);
}

/**
 * gd_dbus_display_config_emit_monitors_changed:
 * @object: A #GDDBusDisplayConfig.
 *
 * Emits the <link linkend="gdbus-signal-org-gnome-Mutter-DisplayConfig.MonitorsChanged">"MonitorsChanged"</link> D-Bus signal.
 */
void
gd_dbus_display_config_emit_monitors_changed(GDDBusDisplayConfig* object)
{
    g_signal_emit(object, GD_DBUS__DISPLAY_CONFIG_SIGNALS[GD_DBUS__DISPLAY_CONFIG_MONITORS_CHANGED], 0);
}

/**
 * gd_dbus_display_config_call_get_resources:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied or %NULL.
 * @user_data: User data to pass to @callback.
 *
 * Asynchronously invokes the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.GetResources">GetResources()</link> D-Bus method on @proxy.
 * When the operation is finished, @callback will be invoked in the thread-default main loop of the thread you are calling this method from (see g_main_context_push_thread_default()).
 * You can then call gd_dbus_display_config_call_get_resources_finish() to get the result of the operation.
 *
 * See gd_dbus_display_config_call_get_resources_sync() for the synchronous, blocking version of this method.
 */
void
gd_dbus_display_config_call_get_resources(GDDBusDisplayConfig* proxy, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_dbus_proxy_call(G_DBUS_PROXY(proxy), "GetResources", g_variant_new("()"), G_DBUS_CALL_FLAGS_NONE, -1, cancellable, callback, user_data);
}

/**
 * gd_dbus_display_config_call_get_resources_finish:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @out_serial: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_crtcs: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_outputs: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_modes: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_max_screen_width: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_max_screen_height: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @res: The #GAsyncResult obtained from the #GAsyncReadyCallback passed to gd_dbus_display_config_call_get_resources().
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with gd_dbus_display_config_call_get_resources().
 *
 * Returns: (skip): %TRUE if the call succeeded, %FALSE if @error is set.
 */
gboolean
gd_dbus_display_config_call_get_resources_finish(GDDBusDisplayConfig* proxy, guint* out_serial, GVariant** out_crtcs, GVariant** out_outputs, GVariant** out_modes, gint* out_max_screen_width, gint* out_max_screen_height, GAsyncResult* res,
                                                 GError** error)
{
    GVariant* _ret;
    _ret = g_dbus_proxy_call_finish(G_DBUS_PROXY(proxy), res, error);
    if (_ret == NULL) goto _out;
    g_variant_get(_ret, "(u@a(uxiiiiiuaua{sv})@a(uxiausauaua{sv})@a(uxuudu)ii)", out_serial, out_crtcs, out_outputs, out_modes, out_max_screen_width, out_max_screen_height);
    g_variant_unref(_ret);
_out:
    return _ret != NULL;
}

/**
 * gd_dbus_display_config_call_get_resources_sync:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @out_serial: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_crtcs: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_outputs: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_modes: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_max_screen_width: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_max_screen_height: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Synchronously invokes the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.GetResources">GetResources()</link> D-Bus method on @proxy. The calling thread is blocked until a reply is received.
 *
 * See gd_dbus_display_config_call_get_resources() for the asynchronous version of this method.
 *
 * Returns: (skip): %TRUE if the call succeeded, %FALSE if @error is set.
 */
gboolean
gd_dbus_display_config_call_get_resources_sync(GDDBusDisplayConfig* proxy, guint* out_serial, GVariant** out_crtcs, GVariant** out_outputs, GVariant** out_modes, gint* out_max_screen_width, gint* out_max_screen_height,
                                               GCancellable* cancellable, GError** error)
{
    GVariant* _ret;
    _ret = g_dbus_proxy_call_sync(G_DBUS_PROXY(proxy), "GetResources", g_variant_new("()"), G_DBUS_CALL_FLAGS_NONE, -1, cancellable, error);
    if (_ret == NULL) goto _out;
    g_variant_get(_ret, "(u@a(uxiiiiiuaua{sv})@a(uxiausauaua{sv})@a(uxuudu)ii)", out_serial, out_crtcs, out_outputs, out_modes, out_max_screen_width, out_max_screen_height);
    g_variant_unref(_ret);
_out:
    return _ret != NULL;
}

/**
 * gd_dbus_display_config_call_change_backlight:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @arg_serial: Argument to pass with the method invocation.
 * @arg_output: Argument to pass with the method invocation.
 * @arg_value: Argument to pass with the method invocation.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied or %NULL.
 * @user_data: User data to pass to @callback.
 *
 * Asynchronously invokes the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.ChangeBacklight">ChangeBacklight()</link> D-Bus method on @proxy.
 * When the operation is finished, @callback will be invoked in the thread-default main loop of the thread you are calling this method from (see g_main_context_push_thread_default()).
 * You can then call gd_dbus_display_config_call_change_backlight_finish() to get the result of the operation.
 *
 * See gd_dbus_display_config_call_change_backlight_sync() for the synchronous, blocking version of this method.
 *
 * Deprecated: The D-Bus method has been deprecated.
 */
void
gd_dbus_display_config_call_change_backlight(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_output, gint arg_value, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_dbus_proxy_call(G_DBUS_PROXY(proxy), "ChangeBacklight", g_variant_new("(uui)", arg_serial, arg_output, arg_value), G_DBUS_CALL_FLAGS_NONE, -1, cancellable, callback, user_data);
}

/**
 * gd_dbus_display_config_call_change_backlight_finish:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @out_new_value: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @res: The #GAsyncResult obtained from the #GAsyncReadyCallback passed to gd_dbus_display_config_call_change_backlight().
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with gd_dbus_display_config_call_change_backlight().
 *
 * Returns: (skip): %TRUE if the call succeeded, %FALSE if @error is set.
 *
 * Deprecated: The D-Bus method has been deprecated.
 */
gboolean
gd_dbus_display_config_call_change_backlight_finish(GDDBusDisplayConfig* proxy, gint* out_new_value, GAsyncResult* res, GError** error)
{
    GVariant* _ret;
    _ret = g_dbus_proxy_call_finish(G_DBUS_PROXY(proxy), res, error);
    if (_ret == NULL) goto _out;
    g_variant_get(_ret, "(i)", out_new_value);
    g_variant_unref(_ret);
_out:
    return _ret != NULL;
}

/**
 * gd_dbus_display_config_call_change_backlight_sync:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @arg_serial: Argument to pass with the method invocation.
 * @arg_output: Argument to pass with the method invocation.
 * @arg_value: Argument to pass with the method invocation.
 * @out_new_value: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Synchronously invokes the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.ChangeBacklight">ChangeBacklight()</link> D-Bus method on @proxy. The calling thread is blocked until a reply is received.
 *
 * See gd_dbus_display_config_call_change_backlight() for the asynchronous version of this method.
 *
 * Returns: (skip): %TRUE if the call succeeded, %FALSE if @error is set.
 *
 * Deprecated: The D-Bus method has been deprecated.
 */
gboolean
gd_dbus_display_config_call_change_backlight_sync(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_output, gint arg_value, gint* out_new_value, GCancellable* cancellable, GError** error)
{
    GVariant* _ret;
    _ret = g_dbus_proxy_call_sync(G_DBUS_PROXY(proxy), "ChangeBacklight", g_variant_new("(uui)", arg_serial, arg_output, arg_value), G_DBUS_CALL_FLAGS_NONE, -1, cancellable, error);
    if (_ret == NULL) goto _out;
    g_variant_get(_ret, "(i)", out_new_value);
    g_variant_unref(_ret);
_out:
    return _ret != NULL;
}

/**
 * gd_dbus_display_config_call_set_backlight:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @arg_serial: Argument to pass with the method invocation.
 * @arg_connector: Argument to pass with the method invocation.
 * @arg_value: Argument to pass with the method invocation.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied or %NULL.
 * @user_data: User data to pass to @callback.
 *
 * Asynchronously invokes the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.SetBacklight">SetBacklight()</link> D-Bus method on @proxy.
 * When the operation is finished, @callback will be invoked in the thread-default main loop of the thread you are calling this method from (see g_main_context_push_thread_default()).
 * You can then call gd_dbus_display_config_call_set_backlight_finish() to get the result of the operation.
 *
 * See gd_dbus_display_config_call_set_backlight_sync() for the synchronous, blocking version of this method.
 */
void
gd_dbus_display_config_call_set_backlight(GDDBusDisplayConfig* proxy, guint arg_serial, const gchar* arg_connector, gint arg_value, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_dbus_proxy_call(G_DBUS_PROXY(proxy), "SetBacklight", g_variant_new("(usi)", arg_serial, arg_connector, arg_value), G_DBUS_CALL_FLAGS_NONE, -1, cancellable, callback, user_data);
}

/**
 * gd_dbus_display_config_call_set_backlight_finish:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @res: The #GAsyncResult obtained from the #GAsyncReadyCallback passed to gd_dbus_display_config_call_set_backlight().
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with gd_dbus_display_config_call_set_backlight().
 *
 * Returns: (skip): %TRUE if the call succeeded, %FALSE if @error is set.
 */
gboolean
gd_dbus_display_config_call_set_backlight_finish(GDDBusDisplayConfig* proxy, GAsyncResult* res, GError** error)
{
    GVariant* _ret;
    _ret = g_dbus_proxy_call_finish(G_DBUS_PROXY(proxy), res, error);
    if (_ret == NULL) goto _out;
    g_variant_get(_ret, "()");
    g_variant_unref(_ret);
_out:
    return _ret != NULL;
}

/**
 * gd_dbus_display_config_call_set_backlight_sync:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @arg_serial: Argument to pass with the method invocation.
 * @arg_connector: Argument to pass with the method invocation.
 * @arg_value: Argument to pass with the method invocation.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Synchronously invokes the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.SetBacklight">SetBacklight()</link> D-Bus method on @proxy. The calling thread is blocked until a reply is received.
 *
 * See gd_dbus_display_config_call_set_backlight() for the asynchronous version of this method.
 *
 * Returns: (skip): %TRUE if the call succeeded, %FALSE if @error is set.
 */
gboolean
gd_dbus_display_config_call_set_backlight_sync(GDDBusDisplayConfig* proxy, guint arg_serial, const gchar* arg_connector, gint arg_value, GCancellable* cancellable, GError** error)
{
    GVariant* _ret;
    _ret = g_dbus_proxy_call_sync(G_DBUS_PROXY(proxy), "SetBacklight", g_variant_new("(usi)", arg_serial, arg_connector, arg_value), G_DBUS_CALL_FLAGS_NONE, -1, cancellable, error);
    if (_ret == NULL) goto _out;
    g_variant_get(_ret, "()");
    g_variant_unref(_ret);
_out:
    return _ret != NULL;
}

/**
 * gd_dbus_display_config_call_get_crtc_gamma:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @arg_serial: Argument to pass with the method invocation.
 * @arg_crtc: Argument to pass with the method invocation.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied or %NULL.
 * @user_data: User data to pass to @callback.
 *
 * Asynchronously invokes the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.GetCrtcGamma">GetCrtcGamma()</link> D-Bus method on @proxy.
 * When the operation is finished, @callback will be invoked in the thread-default main loop of the thread you are calling this method from (see g_main_context_push_thread_default()).
 * You can then call gd_dbus_display_config_call_get_crtc_gamma_finish() to get the result of the operation.
 *
 * See gd_dbus_display_config_call_get_crtc_gamma_sync() for the synchronous, blocking version of this method.
 */
void
gd_dbus_display_config_call_get_crtc_gamma(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_crtc, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_dbus_proxy_call(G_DBUS_PROXY(proxy), "GetCrtcGamma", g_variant_new("(uu)", arg_serial, arg_crtc), G_DBUS_CALL_FLAGS_NONE, -1, cancellable, callback, user_data);
}

/**
 * gd_dbus_display_config_call_get_crtc_gamma_finish:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @out_red: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_green: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_blue: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @res: The #GAsyncResult obtained from the #GAsyncReadyCallback passed to gd_dbus_display_config_call_get_crtc_gamma().
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with gd_dbus_display_config_call_get_crtc_gamma().
 *
 * Returns: (skip): %TRUE if the call succeeded, %FALSE if @error is set.
 */
gboolean
gd_dbus_display_config_call_get_crtc_gamma_finish(GDDBusDisplayConfig* proxy, GVariant** out_red, GVariant** out_green, GVariant** out_blue, GAsyncResult* res, GError** error)
{
    GVariant* _ret;
    _ret = g_dbus_proxy_call_finish(G_DBUS_PROXY(proxy), res, error);
    if (_ret == NULL) goto _out;
    g_variant_get(_ret, "(@aq@aq@aq)", out_red, out_green, out_blue);
    g_variant_unref(_ret);
_out:
    return _ret != NULL;
}

/**
 * gd_dbus_display_config_call_get_crtc_gamma_sync:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @arg_serial: Argument to pass with the method invocation.
 * @arg_crtc: Argument to pass with the method invocation.
 * @out_red: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_green: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_blue: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Synchronously invokes the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.GetCrtcGamma">GetCrtcGamma()</link> D-Bus method on @proxy. The calling thread is blocked until a reply is received.
 *
 * See gd_dbus_display_config_call_get_crtc_gamma() for the asynchronous version of this method.
 *
 * Returns: (skip): %TRUE if the call succeeded, %FALSE if @error is set.
 */
gboolean
gd_dbus_display_config_call_get_crtc_gamma_sync(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_crtc, GVariant** out_red, GVariant** out_green, GVariant** out_blue, GCancellable* cancellable, GError** error)
{
    GVariant* _ret;
    _ret = g_dbus_proxy_call_sync(G_DBUS_PROXY(proxy), "GetCrtcGamma", g_variant_new("(uu)", arg_serial, arg_crtc), G_DBUS_CALL_FLAGS_NONE, -1, cancellable, error);
    if (_ret == NULL) goto _out;
    g_variant_get(_ret, "(@aq@aq@aq)", out_red, out_green, out_blue);
    g_variant_unref(_ret);
_out:
    return _ret != NULL;
}

/**
 * gd_dbus_display_config_call_set_crtc_gamma:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @arg_serial: Argument to pass with the method invocation.
 * @arg_crtc: Argument to pass with the method invocation.
 * @arg_red: Argument to pass with the method invocation.
 * @arg_green: Argument to pass with the method invocation.
 * @arg_blue: Argument to pass with the method invocation.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied or %NULL.
 * @user_data: User data to pass to @callback.
 *
 * Asynchronously invokes the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.SetCrtcGamma">SetCrtcGamma()</link> D-Bus method on @proxy.
 * When the operation is finished, @callback will be invoked in the thread-default main loop of the thread you are calling this method from (see g_main_context_push_thread_default()).
 * You can then call gd_dbus_display_config_call_set_crtc_gamma_finish() to get the result of the operation.
 *
 * See gd_dbus_display_config_call_set_crtc_gamma_sync() for the synchronous, blocking version of this method.
 */
void
gd_dbus_display_config_call_set_crtc_gamma(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_crtc, GVariant* arg_red, GVariant* arg_green, GVariant* arg_blue, GCancellable* cancellable, GAsyncReadyCallback callback,
                                           gpointer user_data)
{
    g_dbus_proxy_call(G_DBUS_PROXY(proxy), "SetCrtcGamma", g_variant_new("(uu@aq@aq@aq)", arg_serial, arg_crtc, arg_red, arg_green, arg_blue), G_DBUS_CALL_FLAGS_NONE, -1, cancellable, callback, user_data);
}

/**
 * gd_dbus_display_config_call_set_crtc_gamma_finish:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @res: The #GAsyncResult obtained from the #GAsyncReadyCallback passed to gd_dbus_display_config_call_set_crtc_gamma().
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with gd_dbus_display_config_call_set_crtc_gamma().
 *
 * Returns: (skip): %TRUE if the call succeeded, %FALSE if @error is set.
 */
gboolean
gd_dbus_display_config_call_set_crtc_gamma_finish(GDDBusDisplayConfig* proxy, GAsyncResult* res, GError** error)
{
    GVariant* _ret;
    _ret = g_dbus_proxy_call_finish(G_DBUS_PROXY(proxy), res, error);
    if (_ret == NULL) goto _out;
    g_variant_get(_ret, "()");
    g_variant_unref(_ret);
_out:
    return _ret != NULL;
}

/**
 * gd_dbus_display_config_call_set_crtc_gamma_sync:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @arg_serial: Argument to pass with the method invocation.
 * @arg_crtc: Argument to pass with the method invocation.
 * @arg_red: Argument to pass with the method invocation.
 * @arg_green: Argument to pass with the method invocation.
 * @arg_blue: Argument to pass with the method invocation.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Synchronously invokes the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.SetCrtcGamma">SetCrtcGamma()</link> D-Bus method on @proxy. The calling thread is blocked until a reply is received.
 *
 * See gd_dbus_display_config_call_set_crtc_gamma() for the asynchronous version of this method.
 *
 * Returns: (skip): %TRUE if the call succeeded, %FALSE if @error is set.
 */
gboolean
gd_dbus_display_config_call_set_crtc_gamma_sync(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_crtc, GVariant* arg_red, GVariant* arg_green, GVariant* arg_blue, GCancellable* cancellable, GError** error)
{
    GVariant* _ret;
    _ret = g_dbus_proxy_call_sync(G_DBUS_PROXY(proxy), "SetCrtcGamma", g_variant_new("(uu@aq@aq@aq)", arg_serial, arg_crtc, arg_red, arg_green, arg_blue), G_DBUS_CALL_FLAGS_NONE, -1, cancellable, error);
    if (_ret == NULL) goto _out;
    g_variant_get(_ret, "()");
    g_variant_unref(_ret);
_out:
    return _ret != NULL;
}

/**
 * gd_dbus_display_config_call_get_current_state:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied or %NULL.
 * @user_data: User data to pass to @callback.
 *
 * Asynchronously invokes the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.GetCurrentState">GetCurrentState()</link> D-Bus method on @proxy.
 * When the operation is finished, @callback will be invoked in the thread-default main loop of the thread you are calling this method from (see g_main_context_push_thread_default()).
 * You can then call gd_dbus_display_config_call_get_current_state_finish() to get the result of the operation.
 *
 * See gd_dbus_display_config_call_get_current_state_sync() for the synchronous, blocking version of this method.
 */
void
gd_dbus_display_config_call_get_current_state(GDDBusDisplayConfig* proxy, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_dbus_proxy_call(G_DBUS_PROXY(proxy), "GetCurrentState", g_variant_new("()"), G_DBUS_CALL_FLAGS_NONE, -1, cancellable, callback, user_data);
}

/**
 * gd_dbus_display_config_call_get_current_state_finish:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @out_serial: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_monitors: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_logical_monitors: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_properties: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @res: The #GAsyncResult obtained from the #GAsyncReadyCallback passed to gd_dbus_display_config_call_get_current_state().
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with gd_dbus_display_config_call_get_current_state().
 *
 * Returns: (skip): %TRUE if the call succeeded, %FALSE if @error is set.
 */
gboolean
gd_dbus_display_config_call_get_current_state_finish(GDDBusDisplayConfig* proxy, guint* out_serial, GVariant** out_monitors, GVariant** out_logical_monitors, GVariant** out_properties, GAsyncResult* res, GError** error)
{
    GVariant* _ret;
    _ret = g_dbus_proxy_call_finish(G_DBUS_PROXY(proxy), res, error);
    if (_ret == NULL) goto _out;
    g_variant_get(_ret, "(u@a((ssss)a(siiddada{sv})a{sv})@a(iiduba(ssss)a{sv})@a{sv})", out_serial, out_monitors, out_logical_monitors, out_properties);
    g_variant_unref(_ret);
_out:
    return _ret != NULL;
}

/**
 * gd_dbus_display_config_call_get_current_state_sync:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @out_serial: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_monitors: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_logical_monitors: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @out_properties: (out) (optional): Return location for return parameter or %NULL to ignore.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Synchronously invokes the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.GetCurrentState">GetCurrentState()</link> D-Bus method on @proxy. The calling thread is blocked until a reply is received.
 *
 * See gd_dbus_display_config_call_get_current_state() for the asynchronous version of this method.
 *
 * Returns: (skip): %TRUE if the call succeeded, %FALSE if @error is set.
 */
gboolean
gd_dbus_display_config_call_get_current_state_sync(GDDBusDisplayConfig* proxy, guint* out_serial, GVariant** out_monitors, GVariant** out_logical_monitors, GVariant** out_properties, GCancellable* cancellable, GError** error)
{
    GVariant* _ret;
    _ret = g_dbus_proxy_call_sync(G_DBUS_PROXY(proxy), "GetCurrentState", g_variant_new("()"), G_DBUS_CALL_FLAGS_NONE, -1, cancellable, error);
    if (_ret == NULL) goto _out;
    g_variant_get(_ret, "(u@a((ssss)a(siiddada{sv})a{sv})@a(iiduba(ssss)a{sv})@a{sv})", out_serial, out_monitors, out_logical_monitors, out_properties);
    g_variant_unref(_ret);
_out:
    return _ret != NULL;
}

/**
 * gd_dbus_display_config_call_apply_monitors_config:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @arg_serial: Argument to pass with the method invocation.
 * @arg_method: Argument to pass with the method invocation.
 * @arg_logical_monitors: Argument to pass with the method invocation.
 * @arg_properties: Argument to pass with the method invocation.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied or %NULL.
 * @user_data: User data to pass to @callback.
 *
 * Asynchronously invokes the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.ApplyMonitorsConfig">ApplyMonitorsConfig()</link> D-Bus method on @proxy.
 * When the operation is finished, @callback will be invoked in the thread-default main loop of the thread you are calling this method from (see g_main_context_push_thread_default()).
 * You can then call gd_dbus_display_config_call_apply_monitors_config_finish() to get the result of the operation.
 *
 * See gd_dbus_display_config_call_apply_monitors_config_sync() for the synchronous, blocking version of this method.
 */
void
gd_dbus_display_config_call_apply_monitors_config(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_method, GVariant* arg_logical_monitors, GVariant* arg_properties, GCancellable* cancellable, GAsyncReadyCallback callback,
                                                  gpointer user_data)
{
    g_dbus_proxy_call(G_DBUS_PROXY(proxy), "ApplyMonitorsConfig", g_variant_new("(uu@a(iiduba(ssa{sv}))@a{sv})", arg_serial, arg_method, arg_logical_monitors, arg_properties), G_DBUS_CALL_FLAGS_NONE, -1, cancellable, callback, user_data);
}

/**
 * gd_dbus_display_config_call_apply_monitors_config_finish:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @res: The #GAsyncResult obtained from the #GAsyncReadyCallback passed to gd_dbus_display_config_call_apply_monitors_config().
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with gd_dbus_display_config_call_apply_monitors_config().
 *
 * Returns: (skip): %TRUE if the call succeeded, %FALSE if @error is set.
 */
gboolean
gd_dbus_display_config_call_apply_monitors_config_finish(GDDBusDisplayConfig* proxy, GAsyncResult* res, GError** error)
{
    GVariant* _ret;
    _ret = g_dbus_proxy_call_finish(G_DBUS_PROXY(proxy), res, error);
    if (_ret == NULL) goto _out;
    g_variant_get(_ret, "()");
    g_variant_unref(_ret);
_out:
    return _ret != NULL;
}

/**
 * gd_dbus_display_config_call_apply_monitors_config_sync:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @arg_serial: Argument to pass with the method invocation.
 * @arg_method: Argument to pass with the method invocation.
 * @arg_logical_monitors: Argument to pass with the method invocation.
 * @arg_properties: Argument to pass with the method invocation.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Synchronously invokes the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.ApplyMonitorsConfig">ApplyMonitorsConfig()</link> D-Bus method on @proxy. The calling thread is blocked until a reply is received.
 *
 * See gd_dbus_display_config_call_apply_monitors_config() for the asynchronous version of this method.
 *
 * Returns: (skip): %TRUE if the call succeeded, %FALSE if @error is set.
 */
gboolean
gd_dbus_display_config_call_apply_monitors_config_sync(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_method, GVariant* arg_logical_monitors, GVariant* arg_properties, GCancellable* cancellable, GError** error)
{
    GVariant* _ret;
    _ret = g_dbus_proxy_call_sync(G_DBUS_PROXY(proxy), "ApplyMonitorsConfig", g_variant_new("(uu@a(iiduba(ssa{sv}))@a{sv})", arg_serial, arg_method, arg_logical_monitors, arg_properties), G_DBUS_CALL_FLAGS_NONE, -1, cancellable, error);
    if (_ret == NULL) goto _out;
    g_variant_get(_ret, "()");
    g_variant_unref(_ret);
_out:
    return _ret != NULL;
}

/**
 * gd_dbus_display_config_call_set_output_ctm:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @arg_serial: Argument to pass with the method invocation.
 * @arg_output: Argument to pass with the method invocation.
 * @arg_ctm: Argument to pass with the method invocation.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied or %NULL.
 * @user_data: User data to pass to @callback.
 *
 * Asynchronously invokes the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.SetOutputCTM">SetOutputCTM()</link> D-Bus method on @proxy.
 * When the operation is finished, @callback will be invoked in the thread-default main loop of the thread you are calling this method from (see g_main_context_push_thread_default()).
 * You can then call gd_dbus_display_config_call_set_output_ctm_finish() to get the result of the operation.
 *
 * See gd_dbus_display_config_call_set_output_ctm_sync() for the synchronous, blocking version of this method.
 */
void
gd_dbus_display_config_call_set_output_ctm(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_output, GVariant* arg_ctm, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_dbus_proxy_call(G_DBUS_PROXY(proxy), "SetOutputCTM", g_variant_new("(uu@(ttttttttt))", arg_serial, arg_output, arg_ctm), G_DBUS_CALL_FLAGS_NONE, -1, cancellable, callback, user_data);
}

/**
 * gd_dbus_display_config_call_set_output_ctm_finish:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @res: The #GAsyncResult obtained from the #GAsyncReadyCallback passed to gd_dbus_display_config_call_set_output_ctm().
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with gd_dbus_display_config_call_set_output_ctm().
 *
 * Returns: (skip): %TRUE if the call succeeded, %FALSE if @error is set.
 */
gboolean
gd_dbus_display_config_call_set_output_ctm_finish(GDDBusDisplayConfig* proxy, GAsyncResult* res, GError** error)
{
    GVariant* _ret;
    _ret = g_dbus_proxy_call_finish(G_DBUS_PROXY(proxy), res, error);
    if (_ret == NULL) goto _out;
    g_variant_get(_ret, "()");
    g_variant_unref(_ret);
_out:
    return _ret != NULL;
}

/**
 * gd_dbus_display_config_call_set_output_ctm_sync:
 * @proxy: A #GDDBusDisplayConfigProxy.
 * @arg_serial: Argument to pass with the method invocation.
 * @arg_output: Argument to pass with the method invocation.
 * @arg_ctm: Argument to pass with the method invocation.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Synchronously invokes the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.SetOutputCTM">SetOutputCTM()</link> D-Bus method on @proxy. The calling thread is blocked until a reply is received.
 *
 * See gd_dbus_display_config_call_set_output_ctm() for the asynchronous version of this method.
 *
 * Returns: (skip): %TRUE if the call succeeded, %FALSE if @error is set.
 */
gboolean
gd_dbus_display_config_call_set_output_ctm_sync(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_output, GVariant* arg_ctm, GCancellable* cancellable, GError** error)
{
    GVariant* _ret;
    _ret = g_dbus_proxy_call_sync(G_DBUS_PROXY(proxy), "SetOutputCTM", g_variant_new("(uu@(ttttttttt))", arg_serial, arg_output, arg_ctm), G_DBUS_CALL_FLAGS_NONE, -1, cancellable, error);
    if (_ret == NULL) goto _out;
    g_variant_get(_ret, "()");
    g_variant_unref(_ret);
_out:
    return _ret != NULL;
}

/**
 * gd_dbus_display_config_complete_get_resources:
 * @object: A #GDDBusDisplayConfig.
 * @invocation: (transfer full): A #GDBusMethodInvocation.
 * @serial: Parameter to return.
 * @crtcs: Parameter to return.
 * @outputs: Parameter to return.
 * @modes: Parameter to return.
 * @max_screen_width: Parameter to return.
 * @max_screen_height: Parameter to return.
 *
 * Helper function used in service implementations to finish handling invocations of the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.GetResources">GetResources()</link> D-Bus method. If you instead want to finish handling an invocation by returning an error, use g_dbus_method_invocation_return_error() or similar.
 *
 * This method will free @invocation, you cannot use it afterwards.
 */
void
gd_dbus_display_config_complete_get_resources(GDDBusDisplayConfig* object G_GNUC_UNUSED, GDBusMethodInvocation* invocation, guint serial, GVariant* crtcs, GVariant* outputs, GVariant* modes, gint max_screen_width, gint max_screen_height)
{
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(u@a(uxiiiiiuaua{sv})@a(uxiausauaua{sv})@a(uxuudu)ii)", serial, crtcs, outputs, modes, max_screen_width, max_screen_height));
}

/**
 * gd_dbus_display_config_complete_change_backlight:
 * @object: A #GDDBusDisplayConfig.
 * @invocation: (transfer full): A #GDBusMethodInvocation.
 * @new_value: Parameter to return.
 *
 * Helper function used in service implementations to finish handling invocations of the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.ChangeBacklight">ChangeBacklight()</link> D-Bus method. If you instead want to finish handling an invocation by returning an error, use g_dbus_method_invocation_return_error() or similar.
 *
 * This method will free @invocation, you cannot use it afterwards.
 *
 * Deprecated: The D-Bus method has been deprecated.
 */
void
gd_dbus_display_config_complete_change_backlight(GDDBusDisplayConfig* object G_GNUC_UNUSED, GDBusMethodInvocation* invocation, gint new_value)
{
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(i)", new_value));
}

/**
 * gd_dbus_display_config_complete_set_backlight:
 * @object: A #GDDBusDisplayConfig.
 * @invocation: (transfer full): A #GDBusMethodInvocation.
 *
 * Helper function used in service implementations to finish handling invocations of the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.SetBacklight">SetBacklight()</link> D-Bus method. If you instead want to finish handling an invocation by returning an error, use g_dbus_method_invocation_return_error() or similar.
 *
 * This method will free @invocation, you cannot use it afterwards.
 */
void
gd_dbus_display_config_complete_set_backlight(GDDBusDisplayConfig* object G_GNUC_UNUSED, GDBusMethodInvocation* invocation)
{
    g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
}

/**
 * gd_dbus_display_config_complete_get_crtc_gamma:
 * @object: A #GDDBusDisplayConfig.
 * @invocation: (transfer full): A #GDBusMethodInvocation.
 * @red: Parameter to return.
 * @green: Parameter to return.
 * @blue: Parameter to return.
 *
 * Helper function used in service implementations to finish handling invocations of the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.GetCrtcGamma">GetCrtcGamma()</link> D-Bus method. If you instead want to finish handling an invocation by returning an error, use g_dbus_method_invocation_return_error() or similar.
 *
 * This method will free @invocation, you cannot use it afterwards.
 */
void
gd_dbus_display_config_complete_get_crtc_gamma(GDDBusDisplayConfig* object G_GNUC_UNUSED, GDBusMethodInvocation* invocation, GVariant* red, GVariant* green, GVariant* blue)
{
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(@aq@aq@aq)", red, green, blue));
}

/**
 * gd_dbus_display_config_complete_set_crtc_gamma:
 * @object: A #GDDBusDisplayConfig.
 * @invocation: (transfer full): A #GDBusMethodInvocation.
 *
 * Helper function used in service implementations to finish handling invocations of the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.SetCrtcGamma">SetCrtcGamma()</link> D-Bus method. If you instead want to finish handling an invocation by returning an error, use g_dbus_method_invocation_return_error() or similar.
 *
 * This method will free @invocation, you cannot use it afterwards.
 */
void
gd_dbus_display_config_complete_set_crtc_gamma(GDDBusDisplayConfig* object G_GNUC_UNUSED, GDBusMethodInvocation* invocation)
{
    g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
}

/**
 * gd_dbus_display_config_complete_get_current_state:
 * @object: A #GDDBusDisplayConfig.
 * @invocation: (transfer full): A #GDBusMethodInvocation.
 * @serial: Parameter to return.
 * @monitors: Parameter to return.
 * @logical_monitors: Parameter to return.
 * @properties: Parameter to return.
 *
 * Helper function used in service implementations to finish handling invocations of the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.GetCurrentState">GetCurrentState()</link> D-Bus method. If you instead want to finish handling an invocation by returning an error, use g_dbus_method_invocation_return_error() or similar.
 *
 * This method will free @invocation, you cannot use it afterwards.
 */
void
gd_dbus_display_config_complete_get_current_state(GDDBusDisplayConfig* object G_GNUC_UNUSED, GDBusMethodInvocation* invocation, guint serial, GVariant* monitors, GVariant* logical_monitors, GVariant* properties)
{
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(u@a((ssss)a(siiddada{sv})a{sv})@a(iiduba(ssss)a{sv})@a{sv})", serial, monitors, logical_monitors, properties));
}

/**
 * gd_dbus_display_config_complete_apply_monitors_config:
 * @object: A #GDDBusDisplayConfig.
 * @invocation: (transfer full): A #GDBusMethodInvocation.
 *
 * Helper function used in service implementations to finish handling invocations of the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.ApplyMonitorsConfig">ApplyMonitorsConfig()</link> D-Bus method. If you instead want to finish handling an invocation by returning an error, use g_dbus_method_invocation_return_error() or similar.
 *
 * This method will free @invocation, you cannot use it afterwards.
 */
void
gd_dbus_display_config_complete_apply_monitors_config(GDDBusDisplayConfig* object G_GNUC_UNUSED, GDBusMethodInvocation* invocation)
{
    g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
}

/**
 * gd_dbus_display_config_complete_set_output_ctm:
 * @object: A #GDDBusDisplayConfig.
 * @invocation: (transfer full): A #GDBusMethodInvocation.
 *
 * Helper function used in service implementations to finish handling invocations of the <link linkend="gdbus-method-org-gnome-Mutter-DisplayConfig.SetOutputCTM">SetOutputCTM()</link> D-Bus method. If you instead want to finish handling an invocation by returning an error, use g_dbus_method_invocation_return_error() or similar.
 *
 * This method will free @invocation, you cannot use it afterwards.
 */
void
gd_dbus_display_config_complete_set_output_ctm(GDDBusDisplayConfig* object G_GNUC_UNUSED, GDBusMethodInvocation* invocation)
{
    g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
}

/* ------------------------------------------------------------------------ */

/**
 * GDDBusDisplayConfigProxy:
 *
 * The #GDDBusDisplayConfigProxy structure contains only private data and should only be accessed using the provided API.
 */

/**
 * GDDBusDisplayConfigProxyClass:
 * @parent_class: The parent class.
 *
 * Class structure for #GDDBusDisplayConfigProxy.
 */

struct _GDDBusDisplayConfigProxyPrivate
{
    GData* qdata;
};

static void gd_dbus_display_config_proxy_iface_init(GDDBusDisplayConfigIface* iface);

#if GLIB_VERSION_MAX_ALLOWED >= GLIB_VERSION_2_38
G_DEFINE_TYPE_WITH_CODE(GDDBusDisplayConfigProxy, gd_dbus_display_config_proxy, G_TYPE_DBUS_PROXY, G_ADD_PRIVATE (GDDBusDisplayConfigProxy) G_IMPLEMENT_INTERFACE (GD_DBUS_TYPE_DISPLAY_CONFIG, gd_dbus_display_config_proxy_iface_init))

#else
G_DEFINE_TYPE_WITH_CODE (GDDBusDisplayConfigProxy, gd_dbus_display_config_proxy, G_TYPE_DBUS_PROXY,
                         G_IMPLEMENT_INTERFACE (GD_DBUS_TYPE_DISPLAY_CONFIG, gd_dbus_display_config_proxy_iface_init))

#endif
static void
gd_dbus_display_config_proxy_finalize(GObject* object)
{
    GDDBusDisplayConfigProxy* proxy = GD_DBUS_DISPLAY_CONFIG_PROXY(object);
    g_datalist_clear(&proxy->priv->qdata);
    G_OBJECT_CLASS(gd_dbus_display_config_proxy_parent_class)->finalize(object);
}

static void
gd_dbus_display_config_proxy_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec G_GNUC_UNUSED)
{
    const _ExtendedGDBusPropertyInfo* info;
    GVariant* variant;
    g_assert(prop_id != 0 && prop_id - 1 < 6);
    info = (const _ExtendedGDBusPropertyInfo*)_gd_dbus_display_config_property_info_pointers[prop_id - 1];
    variant = g_dbus_proxy_get_cached_property(G_DBUS_PROXY(object), info->parent_struct.name);
    if (info->use_gvariant) {
        g_value_set_variant(value, variant);
    }
    else {
        if (variant != NULL) g_dbus_gvariant_to_gvalue(variant, value);
    }
    if (variant != NULL) g_variant_unref(variant);
}

static void
gd_dbus_display_config_proxy_set_property_cb(GDBusProxy* proxy, GAsyncResult* res, gpointer user_data)
{
    const _ExtendedGDBusPropertyInfo* info = user_data;
    GError* error;
    GVariant* _ret;
    error = NULL;
    _ret = g_dbus_proxy_call_finish(proxy, res, &error);
    if (!_ret) {
        g_warning("Error setting property '%s' on interface org.gnome.Mutter.DisplayConfig: %s (%s, %d)", info->parent_struct.name, error->message, g_quark_to_string (error->domain), error->code);
        g_error_free(error);
    }
    else {
        g_variant_unref(_ret);
    }
}

static void
gd_dbus_display_config_proxy_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec G_GNUC_UNUSED)
{
    const _ExtendedGDBusPropertyInfo* info;
    GVariant* variant;
    g_assert(prop_id != 0 && prop_id - 1 < 6);
    info = (const _ExtendedGDBusPropertyInfo*)_gd_dbus_display_config_property_info_pointers[prop_id - 1];
    variant = g_dbus_gvalue_to_gvariant(value, G_VARIANT_TYPE(info->parent_struct.signature));
    g_dbus_proxy_call(G_DBUS_PROXY(object), "org.freedesktop.DBus.Properties.Set", g_variant_new("(ssv)", "org.gnome.Mutter.DisplayConfig", info->parent_struct.name, variant), G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                      (GAsyncReadyCallback)gd_dbus_display_config_proxy_set_property_cb, (GDBusPropertyInfo*)&info->parent_struct);
    g_variant_unref(variant);
}

static void
gd_dbus_display_config_proxy_g_signal(GDBusProxy* proxy, const gchar* sender_name G_GNUC_UNUSED, const gchar* signal_name, GVariant* parameters)
{
    _ExtendedGDBusSignalInfo* info;
    GVariantIter iter;
    GVariant* child;
    GValue* paramv;
    gsize num_params;
    gsize n;
    guint signal_id;
    info = (_ExtendedGDBusSignalInfo*)g_dbus_interface_info_lookup_signal((GDBusInterfaceInfo*)&_gd_dbus_display_config_interface_info.parent_struct, signal_name);
    if (info == NULL) return;
    num_params = g_variant_n_children(parameters);
    paramv = g_new0(GValue, num_params + 1);
    g_value_init(&paramv[0], GD_DBUS_TYPE_DISPLAY_CONFIG);
    g_value_set_object(&paramv[0], proxy);
    g_variant_iter_init(&iter, parameters);
    n = 1;
    while ((child = g_variant_iter_next_value(&iter)) != NULL) {
        _ExtendedGDBusArgInfo* arg_info = (_ExtendedGDBusArgInfo*)info->parent_struct.args[n - 1];
        if (arg_info->use_gvariant) {
            g_value_init(&paramv[n], G_TYPE_VARIANT);
            g_value_set_variant(&paramv[n], child);
            n++;
        }
        else g_dbus_gvariant_to_gvalue(child, &paramv[n++]);
        g_variant_unref(child);
    }
    signal_id = g_signal_lookup(info->signal_name, GD_DBUS_TYPE_DISPLAY_CONFIG);
    g_signal_emitv(paramv, signal_id, 0, NULL);
    for (n = 0; n < num_params + 1; n++) g_value_unset(&paramv[n]);
    g_free(paramv);
}

static void
gd_dbus_display_config_proxy_g_properties_changed(GDBusProxy* _proxy, GVariant* changed_properties, const gchar* const * invalidated_properties)
{
    GDDBusDisplayConfigProxy* proxy = GD_DBUS_DISPLAY_CONFIG_PROXY(_proxy);
    guint n;
    const gchar* key;
    GVariantIter* iter;
    _ExtendedGDBusPropertyInfo* info;
    g_variant_get(changed_properties, "a{sv}", &iter);
    while (g_variant_iter_next(iter, "{&sv}", &key, NULL)) {
        info = (_ExtendedGDBusPropertyInfo*)g_dbus_interface_info_lookup_property((GDBusInterfaceInfo*)&_gd_dbus_display_config_interface_info.parent_struct, key);
        g_datalist_remove_data(&proxy->priv->qdata, key);
        if (info != NULL) g_object_notify(G_OBJECT(proxy), info->hyphen_name);
    }
    g_variant_iter_free(iter);
    for (n = 0; invalidated_properties[n] != NULL; n++) {
        info = (_ExtendedGDBusPropertyInfo*)g_dbus_interface_info_lookup_property((GDBusInterfaceInfo*)&_gd_dbus_display_config_interface_info.parent_struct, invalidated_properties[n]);
        g_datalist_remove_data(&proxy->priv->qdata, invalidated_properties[n]);
        if (info != NULL) g_object_notify(G_OBJECT(proxy), info->hyphen_name);
    }
}

static GVariant*
gd_dbus_display_config_proxy_get_backlight(GDDBusDisplayConfig* object)
{
    GDDBusDisplayConfigProxy* proxy = GD_DBUS_DISPLAY_CONFIG_PROXY(object);
    GVariant* variant;
    GVariant* value = NULL;
    variant = g_dbus_proxy_get_cached_property(G_DBUS_PROXY(proxy), "Backlight");
    value = variant;
    if (variant != NULL) g_variant_unref(variant);
    return value;
}

static gint
gd_dbus_display_config_proxy_get_power_save_mode(GDDBusDisplayConfig* object)
{
    GDDBusDisplayConfigProxy* proxy = GD_DBUS_DISPLAY_CONFIG_PROXY(object);
    GVariant* variant;
    gint value = 0;
    variant = g_dbus_proxy_get_cached_property(G_DBUS_PROXY(proxy), "PowerSaveMode");
    if (variant != NULL) {
        value = g_variant_get_int32(variant);
        g_variant_unref(variant);
    }
    return value;
}

static gboolean
gd_dbus_display_config_proxy_get_panel_orientation_managed(GDDBusDisplayConfig* object)
{
    GDDBusDisplayConfigProxy* proxy = GD_DBUS_DISPLAY_CONFIG_PROXY(object);
    GVariant* variant;
    gboolean value = FALSE;
    variant = g_dbus_proxy_get_cached_property(G_DBUS_PROXY(proxy), "PanelOrientationManaged");
    if (variant != NULL) {
        value = g_variant_get_boolean(variant);
        g_variant_unref(variant);
    }
    return value;
}

static gboolean
gd_dbus_display_config_proxy_get_apply_monitors_config_allowed(GDDBusDisplayConfig* object)
{
    GDDBusDisplayConfigProxy* proxy = GD_DBUS_DISPLAY_CONFIG_PROXY(object);
    GVariant* variant;
    gboolean value = FALSE;
    variant = g_dbus_proxy_get_cached_property(G_DBUS_PROXY(proxy), "ApplyMonitorsConfigAllowed");
    if (variant != NULL) {
        value = g_variant_get_boolean(variant);
        g_variant_unref(variant);
    }
    return value;
}

static gboolean
gd_dbus_display_config_proxy_get_night_light_supported(GDDBusDisplayConfig* object)
{
    GDDBusDisplayConfigProxy* proxy = GD_DBUS_DISPLAY_CONFIG_PROXY(object);
    GVariant* variant;
    gboolean value = FALSE;
    variant = g_dbus_proxy_get_cached_property(G_DBUS_PROXY(proxy), "NightLightSupported");
    if (variant != NULL) {
        value = g_variant_get_boolean(variant);
        g_variant_unref(variant);
    }
    return value;
}

static gboolean
gd_dbus_display_config_proxy_get_has_external_monitor(GDDBusDisplayConfig* object)
{
    GDDBusDisplayConfigProxy* proxy = GD_DBUS_DISPLAY_CONFIG_PROXY(object);
    GVariant* variant;
    gboolean value = FALSE;
    variant = g_dbus_proxy_get_cached_property(G_DBUS_PROXY(proxy), "HasExternalMonitor");
    if (variant != NULL) {
        value = g_variant_get_boolean(variant);
        g_variant_unref(variant);
    }
    return value;
}

static void
gd_dbus_display_config_proxy_init(GDDBusDisplayConfigProxy* proxy)
{
#if GLIB_VERSION_MAX_ALLOWED >= GLIB_VERSION_2_38
    proxy->priv = gd_dbus_display_config_proxy_get_instance_private(proxy);
#else
  proxy->priv = G_TYPE_INSTANCE_GET_PRIVATE (proxy, GD_DBUS_TYPE_DISPLAY_CONFIG_PROXY, GDDBusDisplayConfigProxyPrivate);
#endif

    g_dbus_proxy_set_interface_info(G_DBUS_PROXY(proxy), gd_dbus_display_config_interface_info());
}

static void
gd_dbus_display_config_proxy_class_init(GDDBusDisplayConfigProxyClass* klass)
{
    GObjectClass* gobject_class;
    GDBusProxyClass* proxy_class;

    gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = gd_dbus_display_config_proxy_finalize;
    gobject_class->get_property = gd_dbus_display_config_proxy_get_property;
    gobject_class->set_property = gd_dbus_display_config_proxy_set_property;

    proxy_class = G_DBUS_PROXY_CLASS(klass);
    proxy_class->g_signal = gd_dbus_display_config_proxy_g_signal;
    proxy_class->g_properties_changed = gd_dbus_display_config_proxy_g_properties_changed;

    gd_dbus_display_config_override_properties(gobject_class, 1);

#if GLIB_VERSION_MAX_ALLOWED < GLIB_VERSION_2_38
  g_type_class_add_private (klass, sizeof (GDDBusDisplayConfigProxyPrivate));
#endif
}

static void
gd_dbus_display_config_proxy_iface_init(GDDBusDisplayConfigIface* iface)
{
    iface->get_backlight = gd_dbus_display_config_proxy_get_backlight;
    iface->get_power_save_mode = gd_dbus_display_config_proxy_get_power_save_mode;
    iface->get_panel_orientation_managed = gd_dbus_display_config_proxy_get_panel_orientation_managed;
    iface->get_apply_monitors_config_allowed = gd_dbus_display_config_proxy_get_apply_monitors_config_allowed;
    iface->get_night_light_supported = gd_dbus_display_config_proxy_get_night_light_supported;
    iface->get_has_external_monitor = gd_dbus_display_config_proxy_get_has_external_monitor;
}

/**
 * gd_dbus_display_config_proxy_new:
 * @connection: A #GDBusConnection.
 * @flags: Flags from the #GDBusProxyFlags enumeration.
 * @name: (nullable): A bus name (well-known or unique) or %NULL if @connection is not a message bus connection.
 * @object_path: An object path.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: User data to pass to @callback.
 *
 * Asynchronously creates a proxy for the D-Bus interface <link linkend="gdbus-interface-org-gnome-Mutter-DisplayConfig.top_of_page">org.gnome.Mutter.DisplayConfig</link>. See g_dbus_proxy_new() for more details.
 *
 * When the operation is finished, @callback will be invoked in the thread-default main loop of the thread you are calling this method from (see g_main_context_push_thread_default()).
 * You can then call gd_dbus_display_config_proxy_new_finish() to get the result of the operation.
 *
 * See gd_dbus_display_config_proxy_new_sync() for the synchronous, blocking version of this constructor.
 */
void
gd_dbus_display_config_proxy_new(GDBusConnection* connection, GDBusProxyFlags flags, const gchar* name, const gchar* object_path, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_async_initable_new_async(GD_DBUS_TYPE_DISPLAY_CONFIG_PROXY, G_PRIORITY_DEFAULT, cancellable, callback, user_data, "g-flags", flags, "g-name", name, "g-connection", connection, "g-object-path", object_path, "g-interface-name",
                               "org.gnome.Mutter.DisplayConfig", NULL);
}

/**
 * gd_dbus_display_config_proxy_new_finish:
 * @res: The #GAsyncResult obtained from the #GAsyncReadyCallback passed to gd_dbus_display_config_proxy_new().
 * @error: Return location for error or %NULL
 *
 * Finishes an operation started with gd_dbus_display_config_proxy_new().
 *
 * Returns: (transfer full) (type GDDBusDisplayConfigProxy): The constructed proxy object or %NULL if @error is set.
 */
GDDBusDisplayConfig*
gd_dbus_display_config_proxy_new_finish(GAsyncResult* res, GError** error)
{
    GObject* ret;
    GObject* source_object;
    source_object = g_async_result_get_source_object(res);
    ret = g_async_initable_new_finish(G_ASYNC_INITABLE(source_object), res, error);
    g_object_unref(source_object);
    if (ret != NULL) return GD_DBUS_DISPLAY_CONFIG(ret);
    else return NULL;
}

/**
 * gd_dbus_display_config_proxy_new_sync:
 * @connection: A #GDBusConnection.
 * @flags: Flags from the #GDBusProxyFlags enumeration.
 * @name: (nullable): A bus name (well-known or unique) or %NULL if @connection is not a message bus connection.
 * @object_path: An object path.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @error: Return location for error or %NULL
 *
 * Synchronously creates a proxy for the D-Bus interface <link linkend="gdbus-interface-org-gnome-Mutter-DisplayConfig.top_of_page">org.gnome.Mutter.DisplayConfig</link>. See g_dbus_proxy_new_sync() for more details.
 *
 * The calling thread is blocked until a reply is received.
 *
 * See gd_dbus_display_config_proxy_new() for the asynchronous version of this constructor.
 *
 * Returns: (transfer full) (type GDDBusDisplayConfigProxy): The constructed proxy object or %NULL if @error is set.
 */
GDDBusDisplayConfig*
gd_dbus_display_config_proxy_new_sync(GDBusConnection* connection, GDBusProxyFlags flags, const gchar* name, const gchar* object_path, GCancellable* cancellable, GError** error)
{
    GInitable* ret;
    ret = g_initable_new(GD_DBUS_TYPE_DISPLAY_CONFIG_PROXY, cancellable, error, "g-flags", flags, "g-name", name, "g-connection", connection, "g-object-path", object_path, "g-interface-name", "org.gnome.Mutter.DisplayConfig", NULL);
    if (ret != NULL) return GD_DBUS_DISPLAY_CONFIG(ret);
    else return NULL;
}


/**
 * gd_dbus_display_config_proxy_new_for_bus:
 * @bus_type: A #GBusType.
 * @flags: Flags from the #GDBusProxyFlags enumeration.
 * @name: A bus name (well-known or unique).
 * @object_path: An object path.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: User data to pass to @callback.
 *
 * Like gd_dbus_display_config_proxy_new() but takes a #GBusType instead of a #GDBusConnection.
 *
 * When the operation is finished, @callback will be invoked in the thread-default main loop of the thread you are calling this method from (see g_main_context_push_thread_default()).
 * You can then call gd_dbus_display_config_proxy_new_for_bus_finish() to get the result of the operation.
 *
 * See gd_dbus_display_config_proxy_new_for_bus_sync() for the synchronous, blocking version of this constructor.
 */
void
gd_dbus_display_config_proxy_new_for_bus(GBusType bus_type, GDBusProxyFlags flags, const gchar* name, const gchar* object_path, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_async_initable_new_async(GD_DBUS_TYPE_DISPLAY_CONFIG_PROXY, G_PRIORITY_DEFAULT, cancellable, callback, user_data, "g-flags", flags, "g-name", name, "g-bus-type", bus_type, "g-object-path", object_path, "g-interface-name",
                               "org.gnome.Mutter.DisplayConfig", NULL);
}

/**
 * gd_dbus_display_config_proxy_new_for_bus_finish:
 * @res: The #GAsyncResult obtained from the #GAsyncReadyCallback passed to gd_dbus_display_config_proxy_new_for_bus().
 * @error: Return location for error or %NULL
 *
 * Finishes an operation started with gd_dbus_display_config_proxy_new_for_bus().
 *
 * Returns: (transfer full) (type GDDBusDisplayConfigProxy): The constructed proxy object or %NULL if @error is set.
 */
GDDBusDisplayConfig*
gd_dbus_display_config_proxy_new_for_bus_finish(GAsyncResult* res, GError** error)
{
    GObject* ret;
    GObject* source_object;
    source_object = g_async_result_get_source_object(res);
    ret = g_async_initable_new_finish(G_ASYNC_INITABLE(source_object), res, error);
    g_object_unref(source_object);
    if (ret != NULL) return GD_DBUS_DISPLAY_CONFIG(ret);
    else return NULL;
}

/**
 * gd_dbus_display_config_proxy_new_for_bus_sync:
 * @bus_type: A #GBusType.
 * @flags: Flags from the #GDBusProxyFlags enumeration.
 * @name: A bus name (well-known or unique).
 * @object_path: An object path.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @error: Return location for error or %NULL
 *
 * Like gd_dbus_display_config_proxy_new_sync() but takes a #GBusType instead of a #GDBusConnection.
 *
 * The calling thread is blocked until a reply is received.
 *
 * See gd_dbus_display_config_proxy_new_for_bus() for the asynchronous version of this constructor.
 *
 * Returns: (transfer full) (type GDDBusDisplayConfigProxy): The constructed proxy object or %NULL if @error is set.
 */
GDDBusDisplayConfig*
gd_dbus_display_config_proxy_new_for_bus_sync(GBusType bus_type, GDBusProxyFlags flags, const gchar* name, const gchar* object_path, GCancellable* cancellable, GError** error)
{
    GInitable* ret;
    ret = g_initable_new(GD_DBUS_TYPE_DISPLAY_CONFIG_PROXY, cancellable, error, "g-flags", flags, "g-name", name, "g-bus-type", bus_type, "g-object-path", object_path, "g-interface-name", "org.gnome.Mutter.DisplayConfig", NULL);
    if (ret != NULL) return GD_DBUS_DISPLAY_CONFIG(ret);
    else return NULL;
}


/* ------------------------------------------------------------------------ */

/**
 * GDDBusDisplayConfigSkeleton:
 *
 * The #GDDBusDisplayConfigSkeleton structure contains only private data and should only be accessed using the provided API.
 */

/**
 * GDDBusDisplayConfigSkeletonClass:
 * @parent_class: The parent class.
 *
 * Class structure for #GDDBusDisplayConfigSkeleton.
 */

struct _GDDBusDisplayConfigSkeletonPrivate
{
    GValue* properties;
    GList* changed_properties;
    GSource* changed_properties_idle_source;
    GMainContext* context;
    GMutex lock;
};

static void
_gd_dbus_display_config_skeleton_handle_method_call(GDBusConnection* connection G_GNUC_UNUSED, const gchar* sender G_GNUC_UNUSED, const gchar* object_path G_GNUC_UNUSED, const gchar* interface_name, const gchar* method_name,
                                                    GVariant* parameters, GDBusMethodInvocation* invocation, gpointer user_data)
{
    GDDBusDisplayConfigSkeleton* skeleton = GD_DBUS_DISPLAY_CONFIG_SKELETON(user_data);
    _ExtendedGDBusMethodInfo* info;
    GVariantIter iter;
    GVariant* child;
    GValue* paramv;
    gsize num_params;
    guint num_extra;
    gsize n;
    guint signal_id;
    GValue return_value = G_VALUE_INIT;
    info = (_ExtendedGDBusMethodInfo*)g_dbus_method_invocation_get_method_info(invocation);
    g_assert(info != NULL);
    num_params = g_variant_n_children(parameters);
    num_extra = info->pass_fdlist ? 3 : 2;
    paramv = g_new0(GValue, num_params + num_extra);
    n = 0;
    g_value_init(&paramv[n], GD_DBUS_TYPE_DISPLAY_CONFIG);
    g_value_set_object(&paramv[n++], skeleton);
    g_value_init(&paramv[n], G_TYPE_DBUS_METHOD_INVOCATION);
    g_value_set_object(&paramv[n++], invocation);
    if (info->pass_fdlist) {
#ifdef G_OS_UNIX
        g_value_init(&paramv[n], G_TYPE_UNIX_FD_LIST);
        g_value_set_object(&paramv[n++], g_dbus_message_get_unix_fd_list(g_dbus_method_invocation_get_message(invocation)));
#else
      g_assert_not_reached ();
#endif
    }
    g_variant_iter_init(&iter, parameters);
    while ((child = g_variant_iter_next_value(&iter)) != NULL) {
        _ExtendedGDBusArgInfo* arg_info = (_ExtendedGDBusArgInfo*)info->parent_struct.in_args[n - num_extra];
        if (arg_info->use_gvariant) {
            g_value_init(&paramv[n], G_TYPE_VARIANT);
            g_value_set_variant(&paramv[n], child);
            n++;
        }
        else g_dbus_gvariant_to_gvalue(child, &paramv[n++]);
        g_variant_unref(child);
    }
    signal_id = g_signal_lookup(info->signal_name, GD_DBUS_TYPE_DISPLAY_CONFIG);
    g_value_init(&return_value, G_TYPE_BOOLEAN);
    g_signal_emitv(paramv, signal_id, 0, &return_value);
    if (!g_value_get_boolean(&return_value)) g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD, "Method %s is not implemented on interface %s", method_name, interface_name);
    g_value_unset(&return_value);
    for (n = 0; n < num_params + num_extra; n++) g_value_unset(&paramv[n]);
    g_free(paramv);
}

static GVariant*
_gd_dbus_display_config_skeleton_handle_get_property(GDBusConnection* connection G_GNUC_UNUSED, const gchar* sender G_GNUC_UNUSED, const gchar* object_path G_GNUC_UNUSED, const gchar* interface_name G_GNUC_UNUSED,
                                                     const gchar* property_name, GError** error, gpointer user_data)
{
    GDDBusDisplayConfigSkeleton* skeleton = GD_DBUS_DISPLAY_CONFIG_SKELETON(user_data);
    GValue value = G_VALUE_INIT;
    GParamSpec* pspec;
    _ExtendedGDBusPropertyInfo* info;
    GVariant* ret;
    ret = NULL;
    info = (_ExtendedGDBusPropertyInfo*)g_dbus_interface_info_lookup_property((GDBusInterfaceInfo*)&_gd_dbus_display_config_interface_info.parent_struct, property_name);
    g_assert(info != NULL);
    pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(skeleton), info->hyphen_name);
    if (pspec == NULL) {
        g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS, "No property with name %s", property_name);
    }
    else {
        g_value_init(&value, pspec->value_type);
        g_object_get_property(G_OBJECT(skeleton), info->hyphen_name, &value);
        ret = g_dbus_gvalue_to_gvariant(&value, G_VARIANT_TYPE(info->parent_struct.signature));
        g_value_unset(&value);
    }
    return ret;
}

static gboolean
_gd_dbus_display_config_skeleton_handle_set_property(GDBusConnection* connection G_GNUC_UNUSED, const gchar* sender G_GNUC_UNUSED, const gchar* object_path G_GNUC_UNUSED, const gchar* interface_name G_GNUC_UNUSED,
                                                     const gchar* property_name, GVariant* variant, GError** error, gpointer user_data)
{
    GDDBusDisplayConfigSkeleton* skeleton = GD_DBUS_DISPLAY_CONFIG_SKELETON(user_data);
    GValue value = G_VALUE_INIT;
    GParamSpec* pspec;
    _ExtendedGDBusPropertyInfo* info;
    gboolean ret;
    ret = FALSE;
    info = (_ExtendedGDBusPropertyInfo*)g_dbus_interface_info_lookup_property((GDBusInterfaceInfo*)&_gd_dbus_display_config_interface_info.parent_struct, property_name);
    g_assert(info != NULL);
    pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(skeleton), info->hyphen_name);
    if (pspec == NULL) {
        g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS, "No property with name %s", property_name);
    }
    else {
        if (info->use_gvariant) g_value_set_variant(&value, variant);
        else g_dbus_gvariant_to_gvalue(variant, &value);
        g_object_set_property(G_OBJECT(skeleton), info->hyphen_name, &value);
        g_value_unset(&value);
        ret = TRUE;
    }
    return ret;
}

static const GDBusInterfaceVTable _gd_dbus_display_config_skeleton_vtable = {
    _gd_dbus_display_config_skeleton_handle_method_call, _gd_dbus_display_config_skeleton_handle_get_property, _gd_dbus_display_config_skeleton_handle_set_property, {NULL}
};

static GDBusInterfaceInfo*
gd_dbus_display_config_skeleton_dbus_interface_get_info(GDBusInterfaceSkeleton* skeleton G_GNUC_UNUSED)
{
    return gd_dbus_display_config_interface_info();
}

static GDBusInterfaceVTable*
gd_dbus_display_config_skeleton_dbus_interface_get_vtable(GDBusInterfaceSkeleton* skeleton G_GNUC_UNUSED)
{
    return (GDBusInterfaceVTable*)&_gd_dbus_display_config_skeleton_vtable;
}

static GVariant*
gd_dbus_display_config_skeleton_dbus_interface_get_properties(GDBusInterfaceSkeleton* _skeleton)
{
    GDDBusDisplayConfigSkeleton* skeleton = GD_DBUS_DISPLAY_CONFIG_SKELETON(_skeleton);

    GVariantBuilder builder;
    guint n;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
    if (_gd_dbus_display_config_interface_info.parent_struct.properties == NULL) goto out;
    for (n = 0; _gd_dbus_display_config_interface_info.parent_struct.properties[n] != NULL; n++) {
        GDBusPropertyInfo* info = _gd_dbus_display_config_interface_info.parent_struct.properties[n];
        if (info->flags & G_DBUS_PROPERTY_INFO_FLAGS_READABLE) {
            GVariant* value;
            value = _gd_dbus_display_config_skeleton_handle_get_property(g_dbus_interface_skeleton_get_connection(G_DBUS_INTERFACE_SKELETON(skeleton)), NULL, g_dbus_interface_skeleton_get_object_path(G_DBUS_INTERFACE_SKELETON(skeleton)),
                                                                         "org.gnome.Mutter.DisplayConfig", info->name, NULL, skeleton);
            if (value != NULL) {
                g_variant_take_ref(value);
                g_variant_builder_add(&builder, "{sv}", info->name, value);
                g_variant_unref(value);
            }
        }
    }
out:
    return g_variant_builder_end(&builder);
}

static gboolean _gd_dbus_display_config_emit_changed(gpointer user_data);

static void
gd_dbus_display_config_skeleton_dbus_interface_flush(GDBusInterfaceSkeleton* _skeleton)
{
    GDDBusDisplayConfigSkeleton* skeleton = GD_DBUS_DISPLAY_CONFIG_SKELETON(_skeleton);
    gboolean emit_changed = FALSE;

    g_mutex_lock(&skeleton->priv->lock);
    if (skeleton->priv->changed_properties_idle_source != NULL) {
        g_source_destroy(skeleton->priv->changed_properties_idle_source);
        skeleton->priv->changed_properties_idle_source = NULL;
        emit_changed = TRUE;
    }
    g_mutex_unlock(&skeleton->priv->lock);

    if (emit_changed) _gd_dbus_display_config_emit_changed(skeleton);
}

static void
_gd_dbus_display_config_on_signal_monitors_changed(GDDBusDisplayConfig* object)
{
    GDDBusDisplayConfigSkeleton* skeleton = GD_DBUS_DISPLAY_CONFIG_SKELETON(object);

    GList * connections, * l;
    GVariant* signal_variant;
    connections = g_dbus_interface_skeleton_get_connections(G_DBUS_INTERFACE_SKELETON(skeleton));

    signal_variant = g_variant_ref_sink(g_variant_new("()"));
    for (l = connections; l != NULL; l = l->next) {
        GDBusConnection* connection = l->data;
        g_dbus_connection_emit_signal(connection, NULL, g_dbus_interface_skeleton_get_object_path(G_DBUS_INTERFACE_SKELETON(skeleton)), "org.gnome.Mutter.DisplayConfig", "MonitorsChanged", signal_variant, NULL);
    }
    g_variant_unref(signal_variant);
    g_list_free_full(connections, g_object_unref);
}

static void gd_dbus_display_config_skeleton_iface_init(GDDBusDisplayConfigIface* iface);
#if GLIB_VERSION_MAX_ALLOWED >= GLIB_VERSION_2_38
G_DEFINE_TYPE_WITH_CODE(GDDBusDisplayConfigSkeleton, gd_dbus_display_config_skeleton, G_TYPE_DBUS_INTERFACE_SKELETON,
                        G_ADD_PRIVATE (GDDBusDisplayConfigSkeleton) G_IMPLEMENT_INTERFACE (GD_DBUS_TYPE_DISPLAY_CONFIG, gd_dbus_display_config_skeleton_iface_init))

#else
G_DEFINE_TYPE_WITH_CODE (GDDBusDisplayConfigSkeleton, gd_dbus_display_config_skeleton, G_TYPE_DBUS_INTERFACE_SKELETON,
                         G_IMPLEMENT_INTERFACE (GD_DBUS_TYPE_DISPLAY_CONFIG, gd_dbus_display_config_skeleton_iface_init))

#endif
static void
gd_dbus_display_config_skeleton_finalize(GObject* object)
{
    GDDBusDisplayConfigSkeleton* skeleton = GD_DBUS_DISPLAY_CONFIG_SKELETON(object);
    guint n;
    for (n = 0; n < 6; n++) g_value_unset(&skeleton->priv->properties[n]);
    g_free(skeleton->priv->properties);
    g_list_free_full(skeleton->priv->changed_properties, (GDestroyNotify)_changed_property_free);
    if (skeleton->priv->changed_properties_idle_source != NULL) g_source_destroy(skeleton->priv->changed_properties_idle_source);
    g_main_context_unref(skeleton->priv->context);
    g_mutex_clear(&skeleton->priv->lock);
    G_OBJECT_CLASS(gd_dbus_display_config_skeleton_parent_class)->finalize(object);
}

static void
gd_dbus_display_config_skeleton_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec G_GNUC_UNUSED)
{
    GDDBusDisplayConfigSkeleton* skeleton = GD_DBUS_DISPLAY_CONFIG_SKELETON(object);
    g_assert(prop_id != 0 && prop_id - 1 < 6);
    g_mutex_lock(&skeleton->priv->lock);
    g_value_copy(&skeleton->priv->properties[prop_id - 1], value);
    g_mutex_unlock(&skeleton->priv->lock);
}

static gboolean
_gd_dbus_display_config_emit_changed(gpointer user_data)
{
    GDDBusDisplayConfigSkeleton* skeleton = GD_DBUS_DISPLAY_CONFIG_SKELETON(user_data);
    GList* l;
    GVariantBuilder builder;
    GVariantBuilder invalidated_builder;
    guint num_changes;

    g_mutex_lock(&skeleton->priv->lock);
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_init(&invalidated_builder, G_VARIANT_TYPE("as"));
    for (l = skeleton->priv->changed_properties, num_changes = 0; l != NULL; l = l->next) {
        ChangedProperty* cp = l->data;
        GVariant* variant;
        const GValue* cur_value;

        cur_value = &skeleton->priv->properties[cp->prop_id - 1];
        if (!_g_value_equal(cur_value, &cp->orig_value)) {
            variant = g_dbus_gvalue_to_gvariant(cur_value, G_VARIANT_TYPE(cp->info->parent_struct.signature));
            g_variant_builder_add(&builder, "{sv}", cp->info->parent_struct.name, variant);
            g_variant_unref(variant);
            num_changes++;
        }
    }
    if (num_changes > 0) {
        GList * connections, * ll;
        GVariant* signal_variant;
        signal_variant = g_variant_ref_sink(g_variant_new("(sa{sv}as)", "org.gnome.Mutter.DisplayConfig", &builder, &invalidated_builder));
        connections = g_dbus_interface_skeleton_get_connections(G_DBUS_INTERFACE_SKELETON(skeleton));
        for (ll = connections; ll != NULL; ll = ll->next) {
            GDBusConnection* connection = ll->data;

            g_dbus_connection_emit_signal(connection, NULL, g_dbus_interface_skeleton_get_object_path(G_DBUS_INTERFACE_SKELETON(skeleton)), "org.freedesktop.DBus.Properties", "PropertiesChanged", signal_variant, NULL);
        }
        g_variant_unref(signal_variant);
        g_list_free_full(connections, g_object_unref);
    }
    else {
        g_variant_builder_clear(&builder);
        g_variant_builder_clear(&invalidated_builder);
    }
    g_list_free_full(skeleton->priv->changed_properties, (GDestroyNotify)_changed_property_free);
    skeleton->priv->changed_properties = NULL;
    skeleton->priv->changed_properties_idle_source = NULL;
    g_mutex_unlock(&skeleton->priv->lock);
    return FALSE;
}

static void
_gd_dbus_display_config_schedule_emit_changed(GDDBusDisplayConfigSkeleton* skeleton, const _ExtendedGDBusPropertyInfo* info, guint prop_id, const GValue* orig_value)
{
    ChangedProperty* cp;
    GList* l;
    cp = NULL;
    for (l = skeleton->priv->changed_properties; l != NULL; l = l->next) {
        ChangedProperty* i_cp = l->data;
        if (i_cp->info == info) {
            cp = i_cp;
            break;
        }
    }
    if (cp == NULL) {
        cp = g_new0(ChangedProperty, 1);
        cp->prop_id = prop_id;
        cp->info = info;
        skeleton->priv->changed_properties = g_list_prepend(skeleton->priv->changed_properties, cp);
        g_value_init(&cp->orig_value, G_VALUE_TYPE(orig_value));
        g_value_copy(orig_value, &cp->orig_value);
    }
}

static void
gd_dbus_display_config_skeleton_notify(GObject* object, GParamSpec* pspec G_GNUC_UNUSED)
{
    GDDBusDisplayConfigSkeleton* skeleton = GD_DBUS_DISPLAY_CONFIG_SKELETON(object);
    g_mutex_lock(&skeleton->priv->lock);
    if (skeleton->priv->changed_properties != NULL && skeleton->priv->changed_properties_idle_source == NULL) {
        skeleton->priv->changed_properties_idle_source = g_idle_source_new();
        g_source_set_priority(skeleton->priv->changed_properties_idle_source, G_PRIORITY_DEFAULT);
        g_source_set_callback(skeleton->priv->changed_properties_idle_source, _gd_dbus_display_config_emit_changed, g_object_ref(skeleton), (GDestroyNotify)g_object_unref);
        g_source_set_name(skeleton->priv->changed_properties_idle_source, "[generated] _gd_dbus_display_config_emit_changed");
        g_source_attach(skeleton->priv->changed_properties_idle_source, skeleton->priv->context);
        g_source_unref(skeleton->priv->changed_properties_idle_source);
    }
    g_mutex_unlock(&skeleton->priv->lock);
}

static void
gd_dbus_display_config_skeleton_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    const _ExtendedGDBusPropertyInfo* info;
    GDDBusDisplayConfigSkeleton* skeleton = GD_DBUS_DISPLAY_CONFIG_SKELETON(object);
    g_assert(prop_id != 0 && prop_id - 1 < 6);
    info = (const _ExtendedGDBusPropertyInfo*)_gd_dbus_display_config_property_info_pointers[prop_id - 1];
    g_mutex_lock(&skeleton->priv->lock);
    g_object_freeze_notify(object);
    if (!_g_value_equal(value, &skeleton->priv->properties[prop_id - 1])) {
        if (g_dbus_interface_skeleton_get_connection(G_DBUS_INTERFACE_SKELETON(skeleton)) != NULL && info->emits_changed_signal) _gd_dbus_display_config_schedule_emit_changed(
            skeleton, info, prop_id, &skeleton->priv->properties[prop_id - 1]);
        g_value_copy(value, &skeleton->priv->properties[prop_id - 1]);
        g_object_notify_by_pspec(object, pspec);
    }
    g_mutex_unlock(&skeleton->priv->lock);
    g_object_thaw_notify(object);
}

static void
gd_dbus_display_config_skeleton_init(GDDBusDisplayConfigSkeleton* skeleton)
{
#if GLIB_VERSION_MAX_ALLOWED >= GLIB_VERSION_2_38
    skeleton->priv = gd_dbus_display_config_skeleton_get_instance_private(skeleton);
#else
  skeleton->priv = G_TYPE_INSTANCE_GET_PRIVATE (skeleton, GD_DBUS_TYPE_DISPLAY_CONFIG_SKELETON, GDDBusDisplayConfigSkeletonPrivate);
#endif

    g_mutex_init(&skeleton->priv->lock);
    skeleton->priv->context = g_main_context_ref_thread_default();
    skeleton->priv->properties = g_new0(GValue, 6);
    g_value_init(&skeleton->priv->properties[0], G_TYPE_VARIANT);
    g_value_init(&skeleton->priv->properties[1], G_TYPE_INT);
    g_value_init(&skeleton->priv->properties[2], G_TYPE_BOOLEAN);
    g_value_init(&skeleton->priv->properties[3], G_TYPE_BOOLEAN);
    g_value_init(&skeleton->priv->properties[4], G_TYPE_BOOLEAN);
    g_value_init(&skeleton->priv->properties[5], G_TYPE_BOOLEAN);
}

static GVariant*
gd_dbus_display_config_skeleton_get_backlight(GDDBusDisplayConfig* object)
{
    GDDBusDisplayConfigSkeleton* skeleton = GD_DBUS_DISPLAY_CONFIG_SKELETON(object);
    GVariant* value;
    g_mutex_lock(&skeleton->priv->lock);
    value = g_marshal_value_peek_variant(&(skeleton->priv->properties[0]));
    g_mutex_unlock(&skeleton->priv->lock);
    return value;
}

static gint
gd_dbus_display_config_skeleton_get_power_save_mode(GDDBusDisplayConfig* object)
{
    GDDBusDisplayConfigSkeleton* skeleton = GD_DBUS_DISPLAY_CONFIG_SKELETON(object);
    gint value;
    g_mutex_lock(&skeleton->priv->lock);
    value = g_marshal_value_peek_int(&(skeleton->priv->properties[1]));
    g_mutex_unlock(&skeleton->priv->lock);
    return value;
}

static gboolean
gd_dbus_display_config_skeleton_get_panel_orientation_managed(GDDBusDisplayConfig* object)
{
    GDDBusDisplayConfigSkeleton* skeleton = GD_DBUS_DISPLAY_CONFIG_SKELETON(object);
    gboolean value;
    g_mutex_lock(&skeleton->priv->lock);
    value = g_marshal_value_peek_boolean(&(skeleton->priv->properties[2]));
    g_mutex_unlock(&skeleton->priv->lock);
    return value;
}

static gboolean
gd_dbus_display_config_skeleton_get_apply_monitors_config_allowed(GDDBusDisplayConfig* object)
{
    GDDBusDisplayConfigSkeleton* skeleton = GD_DBUS_DISPLAY_CONFIG_SKELETON(object);
    gboolean value;
    g_mutex_lock(&skeleton->priv->lock);
    value = g_marshal_value_peek_boolean(&(skeleton->priv->properties[3]));
    g_mutex_unlock(&skeleton->priv->lock);
    return value;
}

static gboolean
gd_dbus_display_config_skeleton_get_night_light_supported(GDDBusDisplayConfig* object)
{
    GDDBusDisplayConfigSkeleton* skeleton = GD_DBUS_DISPLAY_CONFIG_SKELETON(object);
    gboolean value;
    g_mutex_lock(&skeleton->priv->lock);
    value = g_marshal_value_peek_boolean(&(skeleton->priv->properties[4]));
    g_mutex_unlock(&skeleton->priv->lock);
    return value;
}

static gboolean
gd_dbus_display_config_skeleton_get_has_external_monitor(GDDBusDisplayConfig* object)
{
    GDDBusDisplayConfigSkeleton* skeleton = GD_DBUS_DISPLAY_CONFIG_SKELETON(object);
    gboolean value;
    g_mutex_lock(&skeleton->priv->lock);
    value = g_marshal_value_peek_boolean(&(skeleton->priv->properties[5]));
    g_mutex_unlock(&skeleton->priv->lock);
    return value;
}

static void
gd_dbus_display_config_skeleton_class_init(GDDBusDisplayConfigSkeletonClass* klass)
{
    GObjectClass* gobject_class;
    GDBusInterfaceSkeletonClass* skeleton_class;

    gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = gd_dbus_display_config_skeleton_finalize;
    gobject_class->get_property = gd_dbus_display_config_skeleton_get_property;
    gobject_class->set_property = gd_dbus_display_config_skeleton_set_property;
    gobject_class->notify = gd_dbus_display_config_skeleton_notify;


    gd_dbus_display_config_override_properties(gobject_class, 1);

    skeleton_class = G_DBUS_INTERFACE_SKELETON_CLASS(klass);
    skeleton_class->get_info = gd_dbus_display_config_skeleton_dbus_interface_get_info;
    skeleton_class->get_properties = gd_dbus_display_config_skeleton_dbus_interface_get_properties;
    skeleton_class->flush = gd_dbus_display_config_skeleton_dbus_interface_flush;
    skeleton_class->get_vtable = gd_dbus_display_config_skeleton_dbus_interface_get_vtable;

#if GLIB_VERSION_MAX_ALLOWED < GLIB_VERSION_2_38
  g_type_class_add_private (klass, sizeof (GDDBusDisplayConfigSkeletonPrivate));
#endif
}

static void
gd_dbus_display_config_skeleton_iface_init(GDDBusDisplayConfigIface* iface)
{
    iface->monitors_changed = _gd_dbus_display_config_on_signal_monitors_changed;
    iface->get_backlight = gd_dbus_display_config_skeleton_get_backlight;
    iface->get_power_save_mode = gd_dbus_display_config_skeleton_get_power_save_mode;
    iface->get_panel_orientation_managed = gd_dbus_display_config_skeleton_get_panel_orientation_managed;
    iface->get_apply_monitors_config_allowed = gd_dbus_display_config_skeleton_get_apply_monitors_config_allowed;
    iface->get_night_light_supported = gd_dbus_display_config_skeleton_get_night_light_supported;
    iface->get_has_external_monitor = gd_dbus_display_config_skeleton_get_has_external_monitor;
}

/**
 * gd_dbus_display_config_skeleton_new:
 *
 * Creates a skeleton object for the D-Bus interface <link linkend="gdbus-interface-org-gnome-Mutter-DisplayConfig.top_of_page">org.gnome.Mutter.DisplayConfig</link>.
 *
 * Returns: (transfer full) (type GDDBusDisplayConfigSkeleton): The skeleton object.
 */
GDDBusDisplayConfig*
gd_dbus_display_config_skeleton_new(void)
{
    return GD_DBUS_DISPLAY_CONFIG(g_object_new (GD_DBUS_TYPE_DISPLAY_CONFIG_SKELETON, NULL));
}
