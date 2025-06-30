//
// Created by dingjing on 25-6-28.
//
#include <gio/gio.h>

#include "gd-monitor-spec-private.h"
#include "gd-monitor-config-private.h"


GDMonitorConfig* gd_monitor_config_new(GDMonitor* monitor, GDMonitorMode* mode)
{
    GDMonitorSpec* spec;
    GDMonitorModeSpec* mode_spec;
    GDMonitorConfig* config;

    spec = gd_monitor_get_spec(monitor);
    mode_spec = gd_monitor_mode_get_spec(mode);

    config = g_new0(GDMonitorConfig, 1);
    config->monitor_spec = gd_monitor_spec_clone(spec);
    config->mode_spec = g_memdup2(mode_spec, sizeof (GDMonitorModeSpec));
    config->enable_underscanning = gd_monitor_is_underscanning(monitor);

    config->has_max_bpc = gd_monitor_get_max_bpc(monitor, &config->max_bpc);

    return config;
}

void gd_monitor_config_free(GDMonitorConfig* config)
{
    if (config->monitor_spec != NULL) gd_monitor_spec_free(config->monitor_spec);

    g_free(config->mode_spec);
    g_free(config);
}

bool gd_verify_monitor_config(GDMonitorConfig* config, GError** error)
{
    if (config->monitor_spec && config->mode_spec) {
        return TRUE;
    }
    else {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Monitor config incomplete");
        return FALSE;
    }
}
