//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_CONFIRM_DISPLAY_CHANGE_DIALOG_H
#define graceful_DE2_GD_CONFIRM_DISPLAY_CHANGE_DIALOG_H
#include "macros/macros.h"

#include <gtk/gtk.h>

C_BEGIN_EXTERN_C

#define GD_TYPE_CONFIRM_DISPLAY_CHANGE_DIALOG (gd_confirm_display_change_dialog_get_type ())
G_DECLARE_FINAL_TYPE (GDConfirmDisplayChangeDialog, gd_confirm_display_change_dialog, GD, CONFIRM_DISPLAY_CHANGE_DIALOG, GtkWindow)

GtkWidget *gd_confirm_display_change_dialog_new (gint timeout);

C_END_EXTERN_C

#endif // graceful_DE2_GD_CONFIRM_DISPLAY_CHANGE_DIALOG_H
