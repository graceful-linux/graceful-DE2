//
// Created by dingjing on 25-7-3.
//

#ifndef graceful_DE2_MAIN_PANEL_H
#define graceful_DE2_MAIN_PANEL_H
#include "macros/macros.h"

#include <glib-object.h>


C_BEGIN_EXTERN_C

#define GD_TYPE_PANEL       (gd_panel_get_type())
G_DECLARE_FINAL_TYPE(GDPanel, gd_panel, GD, PANEL, GObject)

GDPanel*  gd_panel_new    (const char* startupId);

C_END_EXTERN_C


#endif // graceful_DE2_MAIN_PANEL_H
