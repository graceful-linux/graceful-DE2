//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_MONITOR_SPEC_PRIVATE_H
#define graceful_DE2_GD_MONITOR_SPEC_PRIVATE_H
#include "macros/macros.h"
#include "gd-monitor-manager-types-private.h"

#include <glib.h>


C_BEGIN_EXTERN_C

struct _GDMonitorSpec
{
    gchar* connector;
    gchar* vendor;
    gchar* product;
    gchar* serial;
};

GDMonitorSpec* gd_monitor_spec_clone(GDMonitorSpec* spec);

guint gd_monitor_spec_hash(gconstpointer key);

bool gd_monitor_spec_equals(GDMonitorSpec* spec, GDMonitorSpec* other_spec);

gint gd_monitor_spec_compare(GDMonitorSpec* spec_a, GDMonitorSpec* spec_b);

void gd_monitor_spec_free(GDMonitorSpec* spec);

bool gd_verify_monitor_spec(GDMonitorSpec* spec, GError** error);


C_END_EXTERN_C

#endif // graceful_DE2_GD_MONITOR_SPEC_PRIVATE_H
