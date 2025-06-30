//
// Created by dingjing on 25-6-28.
//
#include "gd-monitor-transform.h"

#include <glib.h>


static GDMonitorTransform
gd_monitor_transform_flip(GDMonitorTransform transform)
{
    switch (transform) {
    case GD_MONITOR_TRANSFORM_NORMAL:
        return GD_MONITOR_TRANSFORM_FLIPPED;
    case GD_MONITOR_TRANSFORM_90:
        return GD_MONITOR_TRANSFORM_FLIPPED_270;
    case GD_MONITOR_TRANSFORM_180:
        return GD_MONITOR_TRANSFORM_FLIPPED_180;
    case GD_MONITOR_TRANSFORM_270:
        return GD_MONITOR_TRANSFORM_FLIPPED_90;
    case GD_MONITOR_TRANSFORM_FLIPPED:
        return GD_MONITOR_TRANSFORM_NORMAL;
    case GD_MONITOR_TRANSFORM_FLIPPED_90:
        return GD_MONITOR_TRANSFORM_270;
    case GD_MONITOR_TRANSFORM_FLIPPED_180:
        return GD_MONITOR_TRANSFORM_180;
    case GD_MONITOR_TRANSFORM_FLIPPED_270:
        return GD_MONITOR_TRANSFORM_90;

    default:
        break;
    }

    g_assert_not_reached();
    return 0;
}

GDMonitorTransform
gd_monitor_transform_from_orientation(GDOrientation orientation)
{
    switch (orientation) {
    case GD_ORIENTATION_BOTTOM_UP:
        return GD_MONITOR_TRANSFORM_180;

    case GD_ORIENTATION_LEFT_UP:
        return GD_MONITOR_TRANSFORM_90;

    case GD_ORIENTATION_RIGHT_UP:
        return GD_MONITOR_TRANSFORM_270;

    case GD_ORIENTATION_UNDEFINED:
    case GD_ORIENTATION_NORMAL: default:
        break;
    }

    return GD_MONITOR_TRANSFORM_NORMAL;
}

GDMonitorTransform
gd_monitor_transform_invert(GDMonitorTransform transform)
{
    GDMonitorTransform inverted_transform;

    switch (transform) {
    case GD_MONITOR_TRANSFORM_90:
        inverted_transform = GD_MONITOR_TRANSFORM_270;
        break;

    case GD_MONITOR_TRANSFORM_270:
        inverted_transform = GD_MONITOR_TRANSFORM_90;
        break;

    case GD_MONITOR_TRANSFORM_NORMAL:
    case GD_MONITOR_TRANSFORM_180:
    case GD_MONITOR_TRANSFORM_FLIPPED:
    case GD_MONITOR_TRANSFORM_FLIPPED_90:
    case GD_MONITOR_TRANSFORM_FLIPPED_180:
    case GD_MONITOR_TRANSFORM_FLIPPED_270:
        inverted_transform = transform;
        break;

    default:
        g_assert_not_reached();
        break;
    }

    return inverted_transform;
}

GDMonitorTransform
gd_monitor_transform_transform(GDMonitorTransform transform, GDMonitorTransform other)
{
    GDMonitorTransform new_transform;
    gboolean needs_flip;

    if (gd_monitor_transform_is_flipped(other)) new_transform = gd_monitor_transform_flip(transform);
    else new_transform = transform;

    needs_flip = FALSE;
    if (gd_monitor_transform_is_flipped(new_transform)) needs_flip = TRUE;

    new_transform += other;
    new_transform %= GD_MONITOR_TRANSFORM_FLIPPED;

    if (needs_flip) new_transform += GD_MONITOR_TRANSFORM_FLIPPED;

    return new_transform;
}
