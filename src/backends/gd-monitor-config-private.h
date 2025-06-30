//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_MONITOR_CONFIG_PRIVATE_H
#define graceful_DE2_GD_MONITOR_CONFIG_PRIVATE_H
#include "macros/macros.h"

#include "gd-monitor-private.h"
#include "gd-monitor-manager-types-private.h"


C_BEGIN_EXTERN_C

typedef struct
{
    GDMonitorSpec*      monitor_spec;
    GDMonitorModeSpec*  mode_spec;
    bool                enable_underscanning;
    bool                has_max_bpc;
    unsigned int        max_bpc;
} GDMonitorConfig;


GDMonitorConfig* gd_monitor_config_new(GDMonitor* monitor, GDMonitorMode* mode);
void gd_monitor_config_free(GDMonitorConfig* config);
bool gd_verify_monitor_config(GDMonitorConfig* config, GError** error);

C_END_EXTERN_C

#endif // graceful_DE2_GD_MONITOR_CONFIG_PRIVATE_H
