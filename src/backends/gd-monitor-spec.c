//
// Created by dingjing on 25-6-28.
//
#include "gd-monitor-spec-private.h"

#include <gio/gio.h>


GDMonitorSpec*
gd_monitor_spec_clone(GDMonitorSpec* spec)
{
    GDMonitorSpec* new_spec;

    new_spec = g_new0(GDMonitorSpec, 1);

    new_spec->connector = g_strdup(spec->connector);
    new_spec->vendor = g_strdup(spec->vendor);
    new_spec->product = g_strdup(spec->product);
    new_spec->serial = g_strdup(spec->serial);

    return new_spec;
}

guint
gd_monitor_spec_hash(gconstpointer key)
{
    const GDMonitorSpec* monitor_spec = key;

    return (g_str_hash(monitor_spec->connector) + g_str_hash(monitor_spec->vendor) + g_str_hash(monitor_spec->product) + g_str_hash(monitor_spec->serial));
}

bool
gd_monitor_spec_equals(GDMonitorSpec* spec, GDMonitorSpec* other_spec)
{
    return (g_str_equal(spec->connector, other_spec->connector) && g_str_equal(spec->vendor, other_spec->vendor) && g_str_equal(spec->product, other_spec->product) && g_str_equal(spec->serial, other_spec->serial));
}

gint
gd_monitor_spec_compare(GDMonitorSpec* spec_a, GDMonitorSpec* spec_b)
{
    gint ret;

    ret = g_strcmp0(spec_a->connector, spec_b->connector);
    if (ret != 0) return ret;

    ret = g_strcmp0(spec_a->vendor, spec_b->vendor);
    if (ret != 0) return ret;

    ret = g_strcmp0(spec_a->product, spec_b->product);
    if (ret != 0) return ret;

    return g_strcmp0(spec_a->serial, spec_b->serial);
}

void
gd_monitor_spec_free(GDMonitorSpec* spec)
{
    g_free(spec->connector);
    g_free(spec->vendor);
    g_free(spec->product);
    g_free(spec->serial);
    g_free(spec);
}

bool
gd_verify_monitor_spec(GDMonitorSpec* spec, GError** error)
{
    if (spec->connector && spec->vendor && spec->product && spec->serial) {
        return TRUE;
    }
    else {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Monitor spec incomplete");

        return FALSE;
    }
}
