//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_OUTPUT_XRANDR_PRIVATE_H
#define graceful_DE2_GD_OUTPUT_XRANDR_PRIVATE_H
#include <X11/extensions/Xrandr.h>

#include "macros/macros.h"
#include "gd-output-private.h"
#include "gd-gpu-xrandr-private.h"


#define GD_TYPE_OUTPUT_XRANDR (gd_output_xrandr_get_type ())
G_DECLARE_FINAL_TYPE(GDOutputXrandr, gd_output_xrandr, GD, OUTPUT_XRANDR, GDOutput)

GDOutputXrandr* gd_output_xrandr_new(GDGpuXrandr* gpu_xrandr, XRROutputInfo* xrandr_output, RROutput output_id, RROutput primary_output);

GBytes* gd_output_xrandr_read_edid(GDOutputXrandr* self);

void gd_output_xrandr_apply_mode(GDOutputXrandr* self);

void gd_output_xrandr_set_ctm(GDOutputXrandr* self, const GDOutputCtm* ctm);


#endif // graceful_DE2_GD_OUTPUT_XRANDR_PRIVATE_H
