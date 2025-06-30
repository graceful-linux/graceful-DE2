//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_MONITOR_TRANSFORM_H
#define graceful_DE2_GD_MONITOR_TRANSFORM_H
#include "macros/macros.h"

#include "gd-orientation-manager-private.h"

C_BEGIN_EXTERN_C

typedef enum
{
    GD_MONITOR_TRANSFORM_NORMAL,
    GD_MONITOR_TRANSFORM_90,
    GD_MONITOR_TRANSFORM_180,
    GD_MONITOR_TRANSFORM_270,
    GD_MONITOR_TRANSFORM_FLIPPED,
    GD_MONITOR_TRANSFORM_FLIPPED_90,
    GD_MONITOR_TRANSFORM_FLIPPED_180,
    GD_MONITOR_TRANSFORM_FLIPPED_270,
} GDMonitorTransform;

#define GD_MONITOR_N_TRANSFORMS (GD_MONITOR_TRANSFORM_FLIPPED_270 + 1)
#define GD_MONITOR_ALL_TRANSFORMS ((1 << GD_MONITOR_N_TRANSFORMS) - 1)


static inline bool gd_monitor_transform_is_rotated (GDMonitorTransform transform)
{
    return (transform % 2);
}

static inline bool gd_monitor_transform_is_flipped (GDMonitorTransform transform)
{
    return (transform >= GD_MONITOR_TRANSFORM_FLIPPED);
}

GDMonitorTransform gd_monitor_transform_from_orientation (GDOrientation      orientation);
GDMonitorTransform gd_monitor_transform_invert           (GDMonitorTransform transform);
GDMonitorTransform gd_monitor_transform_transform        (GDMonitorTransform transform, GDMonitorTransform other);


C_END_EXTERN_C

#endif // graceful_DE2_GD_MONITOR_TRANSFORM_H
