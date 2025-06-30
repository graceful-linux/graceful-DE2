//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_OUTPUT_INFO_PRIVATE_H
#define graceful_DE2_GD_OUTPUT_INFO_PRIVATE_H
#include "macros/macros.h"

#include <glib.h>

#include "gd-crtc-private.h"
// #include "gd-monitor-transform.h"
// #include "gd-monitor-manager-enums-private.h"
// #include "gd-monitor-manager-types-private.h"


C_BEGIN_EXTERN_C

typedef struct
{
    uint32_t group_id;
    uint32_t flags;
    uint32_t max_h_tiles;
    uint32_t max_v_tiles;
    uint32_t loc_h_tile;
    uint32_t loc_v_tile;
    uint32_t tile_w;
    uint32_t tile_h;
} GDTileInfo;

struct _GDOutputInfo
{
    grefcount ref_count;

    char* name;
    char* vendor;
    char* product;
    char* serial;
    int width_mm;
    int height_mm;

    GDConnectorType connector_type;
    GDMonitorTransform panel_orientation_transform;

    GDCrtcMode* preferred_mode;
    GDCrtcMode** modes;
    guint n_modes;

    GDCrtc** possible_crtcs;
    guint n_possible_crtcs;

    GDOutput** possible_clones;
    guint n_possible_clones;

    int backlight_min;
    int backlight_max;

    bool supports_underscanning;
    bool supports_color_transform;

    unsigned int max_bpc_min;
    unsigned int max_bpc_max;

    /* Get a new preferred mode on hotplug events, to handle
     * dynamic guest resizing
     */
    bool hotplug_mode_update;
    int suggested_x;
    int suggested_y;

    GDTileInfo tile_info;
};

#define GD_TYPE_OUTPUT_INFO (gd_output_info_get_type ())

GType gd_output_info_get_type(void);

GDOutputInfo* gd_output_info_new(void);

GDOutputInfo* gd_output_info_ref(GDOutputInfo* self);

void gd_output_info_unref(GDOutputInfo* self);

void gd_output_info_parse_edid(GDOutputInfo* self, GBytes* edid);


C_END_EXTERN_C

#endif // graceful_DE2_GD_OUTPUT_INFO_PRIVATE_H
