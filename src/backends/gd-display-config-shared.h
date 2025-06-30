//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_DISPLAY_CONFIG_SHARED_H
#define graceful_DE2_GD_DISPLAY_CONFIG_SHARED_H
#include "macros/macros.h"


C_BEGIN_EXTERN_C

typedef enum
{
    GD_POWER_SAVE_UNSUPPORTED = -1,

    GD_POWER_SAVE_ON = 0,
    GD_POWER_SAVE_STANDBY,
    GD_POWER_SAVE_SUSPEND,
    GD_POWER_SAVE_OFF,
} GDPowerSave;

C_END_EXTERN_C

#endif // graceful_DE2_GD_DISPLAY_CONFIG_SHARED_H
