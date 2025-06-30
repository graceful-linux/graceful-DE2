//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_EDID_PRIVATE_H
#define graceful_DE2_GD_EDID_PRIVATE_H
#include "macros/macros.h"

C_BEGIN_EXTERN_C typedef struct _GDEdidInfo GDEdidInfo;
typedef struct _GDEdidTiming GDEdidTiming;
typedef struct _GDEdidDetailedTiming GDEdidDetailedTiming;
typedef struct _GDEdidHdrStaticMetadata GDEdidHdrStaticMetadata;

typedef enum
{
    GD_EDID_INTERFACE_UNDEFINED,
    GD_EDID_INTERFACE_DVI,
    GD_EDID_INTERFACE_HDMI_A,
    GD_EDID_INTERFACE_HDMI_B,
    GD_EDID_INTERFACE_MDDI,
    GD_EDID_INTERFACE_DISPLAY_PORT
} GDEdidInterface;

typedef enum
{
    GD_EDID_COLOR_TYPE_UNDEFINED,
    GD_EDID_COLOR_TYPE_MONOCHROME,
    GD_EDID_COLOR_TYPE_RGB,
    GD_EDID_COLOR_TYPE_OTHER_COLOR
} GDEdidColorType;

typedef enum
{
    GD_EDID_STEREO_TYPE_NO_STEREO,
    GD_EDID_STEREO_TYPE_FIELD_RIGHT,
    GD_EDID_STEREO_TYPE_FIELD_LEFT,
    GD_EDID_STEREO_TYPE_TWO_WAY_RIGHT_ON_EVEN,
    GD_EDID_STEREO_TYPE_TWO_WAY_LEFT_ON_EVEN,
    GD_EDID_STEREO_TYPE_FOUR_WAY_INTERLEAVED,
    GD_EDID_STEREO_TYPE_SIDE_BY_SIDE
} GDEdidStereoType;

typedef enum
{
    GD_EDID_COLORIMETRY_XVYCC601 = (1 << 0),
    GD_EDID_COLORIMETRY_XVYCC709 = (1 << 1),
    GD_EDID_COLORIMETRY_SYCC601 = (1 << 2),
    GD_EDID_COLORIMETRY_OPYCC601 = (1 << 3),
    GD_EDID_COLORIMETRY_OPRGB = (1 << 4),
    GD_EDID_COLORIMETRY_BT2020CYCC = (1 << 5),
    GD_EDID_COLORIMETRY_BT2020YCC = (1 << 6),
    GD_EDID_COLORIMETRY_BT2020RGB = (1 << 7),
    GD_EDID_COLORIMETRY_ST2113RGB = (1 << 14),
    GD_EDID_COLORIMETRY_ICTCP = (1 << 15),
} GDEdidColorimetry;

typedef enum
{
    GD_EDID_TF_TRADITIONAL_GAMMA_SDR = (1 << 0),
    GD_EDID_TF_TRADITIONAL_GAMMA_HDR = (1 << 1),
    GD_EDID_TF_PQ = (1 << 2),
    GD_EDID_TF_HLG = (1 << 3),
} GDEdidTransferFunction;

typedef enum { GD_EDID_STATIC_METADATA_TYPE1 = 0, } GDEdidStaticMetadataType;

struct _GDEdidTiming
{
    int width;
    int height;
    int frequency;
};

struct _GDEdidDetailedTiming
{
    int pixel_clock;
    int h_addr;
    int h_blank;
    int h_sync;
    int h_front_porch;
    int v_addr;
    int v_blank;
    int v_sync;
    int v_front_porch;
    int width_mm;
    int height_mm;
    int right_border;
    int top_border;
    int interlaced;
    GDEdidStereoType stereo;

    int digital_sync;

    union
    {
        struct
        {
            int bipolar;
            int serrations;
            int sync_on_green;
        } analog;

        struct
        {
            int composite;
            int serrations;
            int negative_vsync;
            int negative_hsync;
        } digital;
    } connector;
};

struct _GDEdidHdrStaticMetadata
{
    int available;
    int max_luminance;
    int min_luminance;
    int max_fal;
    GDEdidTransferFunction tf;
    GDEdidStaticMetadataType sm;
};

struct _GDEdidInfo
{
    int checksum;
    char manufacturer_code[4];
    int product_code;
    unsigned int serial_number;

    int production_week; /* -1 if not specified */
    int production_year; /* -1 if not specified */
    int model_year; /* -1 if not specified */

    int major_version;
    int minor_version;

    int is_digital;

    union
    {
        struct
        {
            int bits_per_primary;
            GDEdidInterface interface;
            int rgb444;
            int ycrcb444;
            int ycrcb422;
        } digital;

        struct
        {
            double video_signal_level;
            double sync_signal_level;
            double total_signal_level;

            int blank_to_black;

            int separate_hv_sync;
            int composite_sync_on_h;
            int composite_sync_on_green;
            int serration_on_vsync;
            GDEdidColorType color_type;
        } analog;
    } connector;

    int width_mm; /* -1 if not specified */
    int height_mm; /* -1 if not specified */
    double aspect_ratio; /* -1.0 if not specififed */

    double gamma; /* -1.0 if not specified */

    int standby;
    int suspend;
    int active_off;

    int srgb_is_standard;
    int preferred_timing_includes_native;
    int continuous_frequency;

    double red_x;
    double red_y;
    double green_x;
    double green_y;
    double blue_x;
    double blue_y;
    double white_x;
    double white_y;

    GDEdidTiming established[24]; /* Terminated by 0x0x0 */
    GDEdidTiming standard[8];

    int n_detailed_timings;
    GDEdidDetailedTiming detailed_timings[4]; /* If monitor has a preferred
                                             * mode, it is the first one
                                             * (whether it has, is
                                             * determined by the
                                             * preferred_timing_includes
                                             * bit.
                                             */

    /* Optional product description */
    char dsc_serial_number[14];
    char dsc_product_name[14];
    char dsc_string[14]; /* Unspecified ASCII data */

    GDEdidColorimetry colorimetry;
    GDEdidHdrStaticMetadata hdr_static_metadata;
};

GDEdidInfo* gd_edid_info_new_parse(const uint8_t* data);


C_END_EXTERN_C

#endif // graceful_DE2_GD_EDID_PRIVATE_H
