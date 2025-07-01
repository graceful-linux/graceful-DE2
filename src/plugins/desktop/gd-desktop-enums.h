//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_DESKTOP_ENUMS_H
#define graceful_DE2_GD_DESKTOP_ENUMS_H
#include "macros/macros.h"

C_BEGIN_EXTERN_C

typedef enum
{
    GD_ICON_SIZE_16PX = 16,
    GD_ICON_SIZE_22PX = 22,
    GD_ICON_SIZE_24PX = 24,
    GD_ICON_SIZE_32PX = 32,
    GD_ICON_SIZE_48PX = 48,
    GD_ICON_SIZE_64PX = 64,
    GD_ICON_SIZE_72PX = 72,
    GD_ICON_SIZE_96PX = 96,
    GD_ICON_SIZE_128PX = 128
  } GDIconSize;

typedef enum
{
    GD_PLACEMENT_AUTO_ARRANGE_ICONS,
    GD_PLACEMENT_ALIGN_ICONS_TO_GRID,
    GD_PLACEMENT_FREE,

    GD_PLACEMENT_LAST /*< skip >*/
  } GDPlacement;

typedef enum
{
    GD_SORT_BY_NAME,
    GD_SORT_BY_DATE_MODIFIED,
    GD_SORT_BY_SIZE,

    GD_SORT_BY_LAST /*< skip >*/
} GDSortBy;

C_END_EXTERN_C

#endif // graceful_DE2_GD_DESKTOP_ENUMS_H
