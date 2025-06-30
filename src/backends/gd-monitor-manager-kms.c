//
// Created by dingjing on 25-6-28.
//
#include "gd-monitor-manager-kms-private.h"

#include <gio/gio.h>

#include "gd-monitor-manager.h"


struct _GDMonitorManagerKms
{
    GDMonitorManager parent;
};

static void
initable_iface_init(GInitableIface* initable_iface);

G_DEFINE_TYPE_WITH_CODE(GDMonitorManagerKms, gd_monitor_manager_kms, GD_TYPE_MONITOR_MANAGER, G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init))

static bool
gd_monitor_manager_kms_initable_init(GInitable* initable, GCancellable* cancellable, GError** error)
{
    return TRUE;
}

static void
initable_iface_init(GInitableIface* initable_iface)
{
    initable_iface->init = (void*) gd_monitor_manager_kms_initable_init;
}

static GBytes*
gd_monitor_manager_kms_read_edid(GDMonitorManager* manager, GDOutput* output)
{
    return NULL;
}

static void
gd_monitor_manager_kms_ensure_initial_config(GDMonitorManager* manager)
{
}

static bool
gd_monitor_manager_kms_apply_monitors_config(GDMonitorManager* manager, GDMonitorsConfig* config, GDMonitorsConfigMethod method, GError** error)
{
    g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Not implemented");
    return FALSE;
}

static void
gd_monitor_manager_kms_set_power_save_mode(GDMonitorManager* manager, GDPowerSave mode)
{
}

static void
gd_monitor_manager_kms_get_crtc_gamma(GDMonitorManager* manager, GDCrtc* crtc, gsize* size, gushort** red, gushort** green, gushort** blue)
{
}

static void
gd_monitor_manager_kms_set_crtc_gamma(GDMonitorManager* manager, GDCrtc* crtc, gsize size, gushort* red, gushort* green, gushort* blue)
{
}

static bool
gd_monitor_manager_kms_is_transform_handled(GDMonitorManager* manager, GDCrtc* crtc, GDMonitorTransform transform)
{
    return FALSE;
}

static gfloat
gd_monitor_manager_kms_calculate_monitor_mode_scale(GDMonitorManager* manager, GDLogicalMonitorLayoutMode layout_mode, GDMonitor* monitor, GDMonitorMode* monitor_mode)
{
    return 1.0;
}

static gfloat*
gd_monitor_manager_kms_calculate_supported_scales(GDMonitorManager* manager, GDLogicalMonitorLayoutMode layout_mode, GDMonitor* monitor, GDMonitorMode* monitor_mode, gint* n_supported_scales)
{
    *n_supported_scales = 0;
    return NULL;
}

static GDMonitorManagerCapability
gd_monitor_manager_kms_get_capabilities(GDMonitorManager* manager)
{
    return GD_MONITOR_MANAGER_CAPABILITY_NONE;
}

static bool
gd_monitor_manager_kms_get_max_screen_size(GDMonitorManager* manager, gint* max_width, gint* max_height)
{
    return FALSE;
}

static GDLogicalMonitorLayoutMode
gd_monitor_manager_kms_get_default_layout_mode(GDMonitorManager* manager)
{
    return GD_LOGICAL_MONITOR_LAYOUT_MODE_PHYSICAL;
}

static void
gd_monitor_manager_kms_class_init(GDMonitorManagerKmsClass* kms_class)
{
    GDMonitorManagerClass* manager_class;

    manager_class = GD_MONITOR_MANAGER_CLASS(kms_class);

    manager_class->read_edid = gd_monitor_manager_kms_read_edid;
    manager_class->ensure_initial_config = gd_monitor_manager_kms_ensure_initial_config;
    manager_class->apply_monitors_config = gd_monitor_manager_kms_apply_monitors_config;
    manager_class->set_power_save_mode = gd_monitor_manager_kms_set_power_save_mode;
    manager_class->get_crtc_gamma = gd_monitor_manager_kms_get_crtc_gamma;
    manager_class->set_crtc_gamma = gd_monitor_manager_kms_set_crtc_gamma;
    manager_class->is_transform_handled = gd_monitor_manager_kms_is_transform_handled;
    manager_class->calculate_monitor_mode_scale = gd_monitor_manager_kms_calculate_monitor_mode_scale;
    manager_class->calculate_supported_scales = gd_monitor_manager_kms_calculate_supported_scales;
    manager_class->get_capabilities = gd_monitor_manager_kms_get_capabilities;
    manager_class->get_max_screen_size = gd_monitor_manager_kms_get_max_screen_size;
    manager_class->get_default_layout_mode = gd_monitor_manager_kms_get_default_layout_mode;
}

static void
gd_monitor_manager_kms_init(GDMonitorManagerKms* kms)
{
}
