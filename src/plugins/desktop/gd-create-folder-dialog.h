//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_CREATE_FOLDER_DIALOG_H
#define graceful_DE2_GD_CREATE_FOLDER_DIALOG_H
#include "macros/macros.h"
#include <gtk/gtk.h>

C_BEGIN_EXTERN_C

#define GD_TYPE_CREATE_FOLDER_DIALOG (gd_create_folder_dialog_get_type ())
G_DECLARE_FINAL_TYPE (GDCreateFolderDialog, gd_create_folder_dialog, GD, CREATE_FOLDER_DIALOG, GtkDialog)

GtkWidget *gd_create_folder_dialog_new             (void);
void       gd_create_folder_dialog_set_valid       (GDCreateFolderDialog *self, bool valid, const char* message);
char      *gd_create_folder_dialog_get_folder_name (GDCreateFolderDialog *self);

C_END_EXTERN_C

#endif // graceful_DE2_GD_CREATE_FOLDER_DIALOG_H
