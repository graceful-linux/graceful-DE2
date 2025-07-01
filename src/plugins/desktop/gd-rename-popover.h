//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_RENAME_POPOVER_H
#define graceful_DE2_GD_RENAME_POPOVER_H
#include <gtk/gtk.h>
#include "macros/macros.h"

C_BEGIN_EXTERN_C

#define GD_TYPE_RENAME_POPOVER (gd_rename_popover_get_type ())
G_DECLARE_FINAL_TYPE (GDRenamePopover, gd_rename_popover, GD, RENAME_POPOVER, GtkPopover)

GtkWidget *gd_rename_popover_new       (GtkWidget       *relative_to,
                                        GFileType        file_type,
                                        const char      *name);

void       gd_rename_popover_set_valid (GDRenamePopover *self,
                                        bool         valid,
                                        const char      *message);

char      *gd_rename_popover_get_name  (GDRenamePopover *self);

C_END_EXTERN_C

#endif // graceful_DE2_GD_RENAME_POPOVER_H
