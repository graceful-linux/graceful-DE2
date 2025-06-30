//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_RECTANGLE_PRIVATE_H
#define graceful_DE2_GD_RECTANGLE_PRIVATE_H
#include "gd-rectangle.h"

bool gd_rectangle_overlaps_with_region         (const GList* spanningRects, const GDRectangle* rect);
bool gd_rectangle_is_adjacent_to_any_in_region (const GList* spanningRects, GDRectangle* rect);
bool gd_rectangle_is_adjacent_to               (const GDRectangle* rect, const GDRectangle* other);

#endif // graceful_DE2_GD_RECTANGLE_PRIVATE_H
