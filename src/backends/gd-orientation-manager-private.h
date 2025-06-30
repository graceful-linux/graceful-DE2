//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_ORIENTATION_MANAGER_PRIVATE_H
#define graceful_DE2_GD_ORIENTATION_MANAGER_PRIVATE_H
#include <glib-object.h>
#include "macros/macros.h"

C_BEGIN_EXTERN_C

typedef enum
{
    GD_ORIENTATION_UNDEFINED,
    GD_ORIENTATION_NORMAL,
    GD_ORIENTATION_BOTTOM_UP,
    GD_ORIENTATION_LEFT_UP,
    GD_ORIENTATION_RIGHT_UP
} GDOrientation;

#define GD_TYPE_ORIENTATION_MANAGER (gd_orientation_manager_get_type ())

G_DECLARE_FINAL_TYPE(GDOrientationManager, gd_orientation_manager, GD, ORIENTATION_MANAGER, GObject)

GDOrientationManager* gd_orientation_manager_new(void);

GDOrientation gd_orientation_manager_get_orientation(GDOrientationManager* manager);

bool gd_orientation_manager_has_accelerometer(GDOrientationManager* self);


C_END_EXTERN_C

#endif // graceful_DE2_GD_ORIENTATION_MANAGER_PRIVATE_H
