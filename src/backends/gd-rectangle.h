//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_RECTANGLE_H
#define graceful_DE2_GD_RECTANGLE_H
#include "macros/macros.h"
#include <glib.h>

C_BEGIN_EXTERN_C

typedef struct
{
    gint x;
    gint y;
    gint width;
    gint height;
} GDRectangle;

bool gd_rectangle_equal         (const GDRectangle* src1, const GDRectangle* src2);
bool gd_rectangle_vert_overlap  (const GDRectangle* rect1, const GDRectangle* rect2);
bool gd_rectangle_horiz_overlap (const GDRectangle* rect1, const GDRectangle* rect2);
bool gd_rectangle_contains_rect (const GDRectangle* outerRect, const GDRectangle* innerRect);

C_END_EXTERN_C

#endif // graceful_DE2_GD_RECTANGLE_H
