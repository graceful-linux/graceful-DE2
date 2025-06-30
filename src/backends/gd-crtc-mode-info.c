//
// Created by dingjing on 25-6-28.
//
#include "gd-crtc-mode-info-private.h"


G_DEFINE_BOXED_TYPE (GDCrtcModeInfo, gd_crtc_mode_info, gd_crtc_mode_info_ref, gd_crtc_mode_info_unref)


GDCrtcModeInfo * gd_crtc_mode_info_new (void)
{
    GDCrtcModeInfo *self;

    self = g_new0 (GDCrtcModeInfo, 1);
    g_ref_count_init (&self->ref_count);

    return self;
}

GDCrtcModeInfo* gd_crtc_mode_info_ref (GDCrtcModeInfo *self)
{
    g_ref_count_inc (&self->ref_count);

    return self;
}

void gd_crtc_mode_info_unref (GDCrtcModeInfo *self)
{
    if (g_ref_count_dec (&self->ref_count)) {
        g_free (self);
    }
}
