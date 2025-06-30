//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_DBUS_DISPLAY_CONFIG_H
#define graceful_DE2_GD_DBUS_DISPLAY_CONFIG_H
#include "macros/macros.h"

#include <gio/gio.h>

C_BEGIN_EXTERN_C


/* ------------------------------------------------------------------------ */
/* Declarations for org.gnome.Mutter.DisplayConfig */

#define GD_DBUS_TYPE_DISPLAY_CONFIG (gd_dbus_display_config_get_type ())
#define GD_DBUS_DISPLAY_CONFIG(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GD_DBUS_TYPE_DISPLAY_CONFIG, GDDBusDisplayConfig))
#define GD_DBUS_IS_DISPLAY_CONFIG(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GD_DBUS_TYPE_DISPLAY_CONFIG))
#define GD_DBUS_DISPLAY_CONFIG_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), GD_DBUS_TYPE_DISPLAY_CONFIG, GDDBusDisplayConfigIface))

struct _GDDBusDisplayConfig;
typedef struct _GDDBusDisplayConfig GDDBusDisplayConfig;
typedef struct _GDDBusDisplayConfigIface GDDBusDisplayConfigIface;

struct _GDDBusDisplayConfigIface
{
    GTypeInterface parent_iface;



    gboolean (*handle_apply_monitors_config)(GDDBusDisplayConfig* object, GDBusMethodInvocation* invocation, guint arg_serial, guint arg_method, GVariant* arg_logical_monitors, GVariant* arg_properties);

    gboolean (*handle_change_backlight)(GDDBusDisplayConfig* object, GDBusMethodInvocation* invocation, guint arg_serial, guint arg_output, gint arg_value);

    gboolean (*handle_get_crtc_gamma)(GDDBusDisplayConfig* object, GDBusMethodInvocation* invocation, guint arg_serial, guint arg_crtc);

    gboolean (*handle_get_current_state)(GDDBusDisplayConfig* object, GDBusMethodInvocation* invocation);

    gboolean (*handle_get_resources)(GDDBusDisplayConfig* object, GDBusMethodInvocation* invocation);

    gboolean (*handle_set_backlight)(GDDBusDisplayConfig* object, GDBusMethodInvocation* invocation, guint arg_serial, const gchar* arg_connector, gint arg_value);

    gboolean (*handle_set_crtc_gamma)(GDDBusDisplayConfig* object, GDBusMethodInvocation* invocation, guint arg_serial, guint arg_crtc, GVariant* arg_red, GVariant* arg_green, GVariant* arg_blue);

    gboolean (*handle_set_output_ctm)(GDDBusDisplayConfig* object, GDBusMethodInvocation* invocation, guint arg_serial, guint arg_output, GVariant* arg_ctm);

    gboolean (*get_apply_monitors_config_allowed)(GDDBusDisplayConfig* object);

    GVariant* (*get_backlight)(GDDBusDisplayConfig* object);

    gboolean (*get_has_external_monitor)(GDDBusDisplayConfig* object);

    gboolean (*get_night_light_supported)(GDDBusDisplayConfig* object);

    gboolean (*get_panel_orientation_managed)(GDDBusDisplayConfig* object);

    gint (*get_power_save_mode)(GDDBusDisplayConfig* object);

    void (*monitors_changed)(GDDBusDisplayConfig* object);
};

GType gd_dbus_display_config_get_type(void) G_GNUC_CONST;

GDBusInterfaceInfo* gd_dbus_display_config_interface_info(void);
guint gd_dbus_display_config_override_properties(GObjectClass* klass, guint property_id_begin);


/* D-Bus method call completion functions: */
void gd_dbus_display_config_complete_get_resources(GDDBusDisplayConfig* object, GDBusMethodInvocation* invocation, guint serial, GVariant* crtcs, GVariant* outputs, GVariant* modes, gint max_screen_width, gint max_screen_height);

G_GNUC_DEPRECATED void gd_dbus_display_config_complete_change_backlight(GDDBusDisplayConfig* object, GDBusMethodInvocation* invocation, gint new_value);

void gd_dbus_display_config_complete_set_backlight(GDDBusDisplayConfig* object, GDBusMethodInvocation* invocation);

void gd_dbus_display_config_complete_get_crtc_gamma(GDDBusDisplayConfig* object, GDBusMethodInvocation* invocation, GVariant* red, GVariant* green, GVariant* blue);

void gd_dbus_display_config_complete_set_crtc_gamma(GDDBusDisplayConfig* object, GDBusMethodInvocation* invocation);

void gd_dbus_display_config_complete_get_current_state(GDDBusDisplayConfig* object, GDBusMethodInvocation* invocation, guint serial, GVariant* monitors, GVariant* logical_monitors, GVariant* properties);

void gd_dbus_display_config_complete_apply_monitors_config(GDDBusDisplayConfig* object, GDBusMethodInvocation* invocation);

void gd_dbus_display_config_complete_set_output_ctm(GDDBusDisplayConfig* object, GDBusMethodInvocation* invocation);



/* D-Bus signal emissions functions: */
void gd_dbus_display_config_emit_monitors_changed(GDDBusDisplayConfig* object);



/* D-Bus method calls: */
void gd_dbus_display_config_call_get_resources(GDDBusDisplayConfig* proxy, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data);

gboolean gd_dbus_display_config_call_get_resources_finish(GDDBusDisplayConfig* proxy, guint* out_serial, GVariant** out_crtcs, GVariant** out_outputs, GVariant** out_modes, gint* out_max_screen_width, gint* out_max_screen_height,
                                                          GAsyncResult* res, GError** error);

gboolean gd_dbus_display_config_call_get_resources_sync(GDDBusDisplayConfig* proxy, guint* out_serial, GVariant** out_crtcs, GVariant** out_outputs, GVariant** out_modes, gint* out_max_screen_width, gint* out_max_screen_height,
                                                        GCancellable* cancellable, GError** error);

G_GNUC_DEPRECATED void gd_dbus_display_config_call_change_backlight(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_output, gint arg_value, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data);

G_GNUC_DEPRECATED gboolean gd_dbus_display_config_call_change_backlight_finish(GDDBusDisplayConfig* proxy, gint* out_new_value, GAsyncResult* res, GError** error);

G_GNUC_DEPRECATED gboolean gd_dbus_display_config_call_change_backlight_sync(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_output, gint arg_value, gint* out_new_value, GCancellable* cancellable, GError** error);

void gd_dbus_display_config_call_set_backlight(GDDBusDisplayConfig* proxy, guint arg_serial, const gchar* arg_connector, gint arg_value, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data);

gboolean gd_dbus_display_config_call_set_backlight_finish(GDDBusDisplayConfig* proxy, GAsyncResult* res, GError** error);

gboolean gd_dbus_display_config_call_set_backlight_sync(GDDBusDisplayConfig* proxy, guint arg_serial, const gchar* arg_connector, gint arg_value, GCancellable* cancellable, GError** error);

void gd_dbus_display_config_call_get_crtc_gamma(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_crtc, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data);

gboolean gd_dbus_display_config_call_get_crtc_gamma_finish(GDDBusDisplayConfig* proxy, GVariant** out_red, GVariant** out_green, GVariant** out_blue, GAsyncResult* res, GError** error);

gboolean gd_dbus_display_config_call_get_crtc_gamma_sync(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_crtc, GVariant** out_red, GVariant** out_green, GVariant** out_blue, GCancellable* cancellable, GError** error);

void gd_dbus_display_config_call_set_crtc_gamma(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_crtc, GVariant* arg_red, GVariant* arg_green, GVariant* arg_blue, GCancellable* cancellable, GAsyncReadyCallback callback,
                                                gpointer user_data);

gboolean gd_dbus_display_config_call_set_crtc_gamma_finish(GDDBusDisplayConfig* proxy, GAsyncResult* res, GError** error);

gboolean gd_dbus_display_config_call_set_crtc_gamma_sync(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_crtc, GVariant* arg_red, GVariant* arg_green, GVariant* arg_blue, GCancellable* cancellable, GError** error);

void gd_dbus_display_config_call_get_current_state(GDDBusDisplayConfig* proxy, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data);

gboolean gd_dbus_display_config_call_get_current_state_finish(GDDBusDisplayConfig* proxy, guint* out_serial, GVariant** out_monitors, GVariant** out_logical_monitors, GVariant** out_properties, GAsyncResult* res, GError** error);

gboolean gd_dbus_display_config_call_get_current_state_sync(GDDBusDisplayConfig* proxy, guint* out_serial, GVariant** out_monitors, GVariant** out_logical_monitors, GVariant** out_properties, GCancellable* cancellable, GError** error);

void gd_dbus_display_config_call_apply_monitors_config(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_method, GVariant* arg_logical_monitors, GVariant* arg_properties, GCancellable* cancellable, GAsyncReadyCallback callback,
                                                       gpointer user_data);

gboolean gd_dbus_display_config_call_apply_monitors_config_finish(GDDBusDisplayConfig* proxy, GAsyncResult* res, GError** error);

gboolean gd_dbus_display_config_call_apply_monitors_config_sync(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_method, GVariant* arg_logical_monitors, GVariant* arg_properties, GCancellable* cancellable, GError** error);

void gd_dbus_display_config_call_set_output_ctm(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_output, GVariant* arg_ctm, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data);

gboolean gd_dbus_display_config_call_set_output_ctm_finish(GDDBusDisplayConfig* proxy, GAsyncResult* res, GError** error);

gboolean gd_dbus_display_config_call_set_output_ctm_sync(GDDBusDisplayConfig* proxy, guint arg_serial, guint arg_output, GVariant* arg_ctm, GCancellable* cancellable, GError** error);



/* D-Bus property accessors: */
GVariant* gd_dbus_display_config_get_backlight(GDDBusDisplayConfig* object);
GVariant* gd_dbus_display_config_dup_backlight(GDDBusDisplayConfig* object);
void gd_dbus_display_config_set_backlight(GDDBusDisplayConfig* object, GVariant* value);

gint gd_dbus_display_config_get_power_save_mode(GDDBusDisplayConfig* object);
void gd_dbus_display_config_set_power_save_mode(GDDBusDisplayConfig* object, gint value);

gboolean gd_dbus_display_config_get_panel_orientation_managed(GDDBusDisplayConfig* object);
void gd_dbus_display_config_set_panel_orientation_managed(GDDBusDisplayConfig* object, gboolean value);

gboolean gd_dbus_display_config_get_apply_monitors_config_allowed(GDDBusDisplayConfig* object);
void gd_dbus_display_config_set_apply_monitors_config_allowed(GDDBusDisplayConfig* object, gboolean value);

gboolean gd_dbus_display_config_get_night_light_supported(GDDBusDisplayConfig* object);
void gd_dbus_display_config_set_night_light_supported(GDDBusDisplayConfig* object, gboolean value);

gboolean gd_dbus_display_config_get_has_external_monitor(GDDBusDisplayConfig* object);
void gd_dbus_display_config_set_has_external_monitor(GDDBusDisplayConfig* object, gboolean value);


/* ---- */

#define GD_DBUS_TYPE_DISPLAY_CONFIG_PROXY (gd_dbus_display_config_proxy_get_type ())
#define GD_DBUS_DISPLAY_CONFIG_PROXY(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GD_DBUS_TYPE_DISPLAY_CONFIG_PROXY, GDDBusDisplayConfigProxy))
#define GD_DBUS_DISPLAY_CONFIG_PROXY_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), GD_DBUS_TYPE_DISPLAY_CONFIG_PROXY, GDDBusDisplayConfigProxyClass))
#define GD_DBUS_DISPLAY_CONFIG_PROXY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GD_DBUS_TYPE_DISPLAY_CONFIG_PROXY, GDDBusDisplayConfigProxyClass))
#define GD_DBUS_IS_DISPLAY_CONFIG_PROXY(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GD_DBUS_TYPE_DISPLAY_CONFIG_PROXY))
#define GD_DBUS_IS_DISPLAY_CONFIG_PROXY_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GD_DBUS_TYPE_DISPLAY_CONFIG_PROXY))

typedef struct _GDDBusDisplayConfigProxy GDDBusDisplayConfigProxy;
typedef struct _GDDBusDisplayConfigProxyClass GDDBusDisplayConfigProxyClass;
typedef struct _GDDBusDisplayConfigProxyPrivate GDDBusDisplayConfigProxyPrivate;

struct _GDDBusDisplayConfigProxy
{
    /*< private >*/
    GDBusProxy parent_instance;
    GDDBusDisplayConfigProxyPrivate* priv;
};

struct _GDDBusDisplayConfigProxyClass
{
    GDBusProxyClass parent_class;
};

GType gd_dbus_display_config_proxy_get_type(void) G_GNUC_CONST;

#if GLIB_CHECK_VERSION(2, 44, 0)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GDDBusDisplayConfigProxy, g_object_unref)
#endif

void gd_dbus_display_config_proxy_new(GDBusConnection* connection, GDBusProxyFlags flags, const gchar* name, const gchar* object_path, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data);
GDDBusDisplayConfig* gd_dbus_display_config_proxy_new_finish(GAsyncResult* res, GError** error);
GDDBusDisplayConfig* gd_dbus_display_config_proxy_new_sync(GDBusConnection* connection, GDBusProxyFlags flags, const gchar* name, const gchar* object_path, GCancellable* cancellable, GError** error);

void gd_dbus_display_config_proxy_new_for_bus(GBusType bus_type, GDBusProxyFlags flags, const gchar* name, const gchar* object_path, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data);
GDDBusDisplayConfig* gd_dbus_display_config_proxy_new_for_bus_finish(GAsyncResult* res, GError** error);
GDDBusDisplayConfig* gd_dbus_display_config_proxy_new_for_bus_sync(GBusType bus_type, GDBusProxyFlags flags, const gchar* name, const gchar* object_path, GCancellable* cancellable, GError** error);


/* ---- */

#define GD_DBUS_TYPE_DISPLAY_CONFIG_SKELETON (gd_dbus_display_config_skeleton_get_type ())
#define GD_DBUS_DISPLAY_CONFIG_SKELETON(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GD_DBUS_TYPE_DISPLAY_CONFIG_SKELETON, GDDBusDisplayConfigSkeleton))
#define GD_DBUS_DISPLAY_CONFIG_SKELETON_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), GD_DBUS_TYPE_DISPLAY_CONFIG_SKELETON, GDDBusDisplayConfigSkeletonClass))
#define GD_DBUS_DISPLAY_CONFIG_SKELETON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GD_DBUS_TYPE_DISPLAY_CONFIG_SKELETON, GDDBusDisplayConfigSkeletonClass))
#define GD_DBUS_IS_DISPLAY_CONFIG_SKELETON(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GD_DBUS_TYPE_DISPLAY_CONFIG_SKELETON))
#define GD_DBUS_IS_DISPLAY_CONFIG_SKELETON_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GD_DBUS_TYPE_DISPLAY_CONFIG_SKELETON))

typedef struct _GDDBusDisplayConfigSkeleton GDDBusDisplayConfigSkeleton;
typedef struct _GDDBusDisplayConfigSkeletonClass GDDBusDisplayConfigSkeletonClass;
typedef struct _GDDBusDisplayConfigSkeletonPrivate GDDBusDisplayConfigSkeletonPrivate;

struct _GDDBusDisplayConfigSkeleton
{
    /*< private >*/
    GDBusInterfaceSkeleton parent_instance;
    GDDBusDisplayConfigSkeletonPrivate* priv;
};

struct _GDDBusDisplayConfigSkeletonClass
{
    GDBusInterfaceSkeletonClass parent_class;
};

GType gd_dbus_display_config_skeleton_get_type(void) G_GNUC_CONST;

#if GLIB_CHECK_VERSION(2, 44, 0)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GDDBusDisplayConfigSkeleton, g_object_unref)
#endif

GDDBusDisplayConfig* gd_dbus_display_config_skeleton_new(void);

C_END_EXTERN_C

#endif // graceful_DE2_GD_DBUS_DISPLAY_CONFIG_H
