//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_BG_H
#define graceful_DE2_GD_BG_H
#include "macros/macros.h"

#include <gdk/gdk.h>
#include <gio/gio.h>
#include <gdesktop-enums.h>

C_BEGIN_EXTERN_C


#define GD_TYPE_BG (gd_bg_get_type ())
G_DECLARE_FINAL_TYPE (GDBG, gd_bg, GD, BG, GObject)

GDBG            *gd_bg_new                            (const char                *schema_id);
void             gd_bg_load_from_preferences          (GDBG                      *self);
void             gd_bg_save_to_preferences            (GDBG                      *self);
void             gd_bg_set_filename                   (GDBG                      *self, const char* filename);
void             gd_bg_set_placement                  (GDBG                      *self, GDesktopBackgroundStyle placement);
void             gd_bg_set_rgba                       (GDBG                      *self, GDesktopBackgroundShading type, GdkRGBA* primary, GdkRGBA* secondary);
cairo_surface_t *gd_bg_create_surface                 (GDBG                      *self, GdkWindow* window, int width, int height, bool root);
void             gd_bg_set_surface_as_root            (GdkDisplay                *display, cairo_surface_t* surface);
cairo_surface_t *gd_bg_get_surface_from_root          (GdkDisplay                *display, int width, int height);
GdkRGBA         *gd_bg_get_average_color_from_surface (cairo_surface_t           *surface);


C_END_EXTERN_C


#endif // graceful_DE2_GD_BG_H
