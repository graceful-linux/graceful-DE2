//
// Created by dingjing on 25-6-28.
//
#include "gd-rectangle.h"

#include "gd-rectangle-private.h"


static bool gd_rectangle_overlap(const GDRectangle* rect1, const GDRectangle* rect2)
{
    g_return_val_if_fail(rect1 != NULL, FALSE);
    g_return_val_if_fail(rect2 != NULL, FALSE);

    return !((rect1->x + rect1->width <= rect2->x) || (rect2->x + rect2->width <= rect1->x) || (rect1->y + rect1->height <= rect2->y) || (rect2->y + rect2->height <= rect1->y));
}

bool gd_rectangle_equal(const GDRectangle* src1, const GDRectangle* src2)
{
    return ((src1->x == src2->x) && (src1->y == src2->y) && (src1->width == src2->width) && (src1->height == src2->height));
}

bool gd_rectangle_vert_overlap(const GDRectangle* rect1, const GDRectangle* rect2)
{
    return (rect1->y < rect2->y + rect2->height && rect2->y < rect1->y + rect1->height);
}

bool gd_rectangle_horiz_overlap(const GDRectangle* rect1, const GDRectangle* rect2)
{
    return (rect1->x < rect2->x + rect2->width && rect2->x < rect1->x + rect1->width);
}

bool gd_rectangle_contains_rect(const GDRectangle* outer_rect, const GDRectangle* inner_rect)
{
    return inner_rect->x >= outer_rect->x && inner_rect->y >= outer_rect->y && inner_rect->x + inner_rect->width <= outer_rect->x + outer_rect->width && inner_rect->y + inner_rect->height <= outer_rect->y + outer_rect->height;
}

bool gd_rectangle_overlaps_with_region(const GList* spanning_rects, const GDRectangle* rect)
{
    const GList* temp;
    bool overlaps;

    temp = spanning_rects;
    overlaps = FALSE;
    while (!overlaps && temp != NULL) {
        overlaps = overlaps || gd_rectangle_overlap(temp->data, rect);
        temp = temp->next;
    }

    return overlaps;
}

bool gd_rectangle_is_adjacent_to_any_in_region(const GList* spanning_rects, GDRectangle* rect)
{
    const GList* l;

    for (l = spanning_rects; l; l = l->next) {
        GDRectangle* other = (GDRectangle*)l->data;

        if (rect == other || gd_rectangle_equal(rect, other)) continue;

        if (gd_rectangle_is_adjacent_to(rect, other)) return TRUE;
    }

    return FALSE;
}

bool gd_rectangle_is_adjacent_to(const GDRectangle* rect, const GDRectangle* other)
{
    gint rect_x1 = rect->x;
    gint rect_y1 = rect->y;
    gint rect_x2 = rect->x + rect->width;
    gint rect_y2 = rect->y + rect->height;
    gint other_x1 = other->x;
    gint other_y1 = other->y;
    gint other_x2 = other->x + other->width;
    gint other_y2 = other->y + other->height;

    if ((rect_x1 == other_x2 || rect_x2 == other_x1) && !(rect_y2 <= other_y1 || rect_y1 >= other_y2)) return TRUE;
    else if ((rect_y1 == other_y2 || rect_y2 == other_y1) && !(rect_x2 <= other_x1 || rect_x1 >= other_x2)) return TRUE;
    else return FALSE;
}
