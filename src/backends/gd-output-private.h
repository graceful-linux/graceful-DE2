//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_OUTPUT_PRIVATE_H
#define graceful_DE2_GD_OUTPUT_PRIVATE_H
#include <stdint.h>
#include <glib-object.h>

#include "macros/macros.h"
#include "gd-output-info-private.h"

C_BEGIN_EXTERN_C

typedef struct _GDOutputCtm
{
    uint64_t matrix[9];
} GDOutputCtm;

typedef struct
{
    GDOutput* output;
    gboolean is_primary;
    gboolean is_presentation;
    gboolean is_underscanning;
    gboolean has_max_bpc;
    unsigned int max_bpc;
} GDOutputAssignment;

#define GD_TYPE_OUTPUT (gd_output_get_type ())
G_DECLARE_DERIVABLE_TYPE(GDOutput, gd_output, GD, OUTPUT, GObject)

struct _GDOutputClass
{
    GObjectClass parent_class;
};

uint64_t gd_output_get_id(GDOutput* self);

GDGpu* gd_output_get_gpu(GDOutput* self);

const GDOutputInfo* gd_output_get_info(GDOutput* self);

GDMonitor* gd_output_get_monitor(GDOutput* self);

void gd_output_set_monitor(GDOutput* self, GDMonitor* monitor);

void gd_output_unset_monitor(GDOutput* self);

const char* gd_output_get_name(GDOutput* self);

void gd_output_assign_crtc(GDOutput* self, GDCrtc* crtc, const GDOutputAssignment* output_assignment);

void gd_output_unassign_crtc(GDOutput* self);

GDCrtc* gd_output_get_assigned_crtc(GDOutput* self);

gboolean gd_output_is_laptop(GDOutput* self);

GDMonitorTransform gd_output_logical_to_crtc_transform(GDOutput* self, GDMonitorTransform transform);

GDMonitorTransform gd_output_crtc_to_logical_transform(GDOutput* self, GDMonitorTransform transform);

gboolean gd_output_is_primary(GDOutput* self);

gboolean gd_output_is_presentation(GDOutput* self);

gboolean gd_output_is_underscanning(GDOutput* self);

gboolean gd_output_get_max_bpc(GDOutput* self, unsigned int* max_bpc);

void gd_output_set_backlight(GDOutput* self, int backlight);

int gd_output_get_backlight(GDOutput* self);

void gd_output_add_possible_clone(GDOutput* self, GDOutput* possible_clone);

static inline GDOutputAssignment*
gd_find_output_assignment(GDOutputAssignment** outputs, unsigned int n_outputs, GDOutput* output)
{
    unsigned int i;

    for (i = 0; i < n_outputs; i++) {
        GDOutputAssignment* output_assignment;

        output_assignment = outputs[i];

        if (output == output_assignment->output) return output_assignment;
    }

    return NULL;
}


#endif // graceful_DE2_GD_OUTPUT_PRIVATE_H
