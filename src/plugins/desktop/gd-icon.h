//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_ICON_H
#define graceful_DE2_GD_ICON_H
#include "gd-icon-view.h"

C_BEGIN_EXTERN_C

#define GD_TYPE_ICON (gd_icon_get_type ())
G_DECLARE_DERIVABLE_TYPE (GDIcon, gd_icon, GD, ICON, GtkButton)

struct _GDIconClass
{
    GtkButtonClass parent_class;

    void         (* create_file_monitor) (GDIcon   *self);

    GIcon      * (* get_icon)            (GDIcon   *self,
                                          bool *is_thumbnail);

    const char * (* get_text)            (GDIcon   *self);

    bool     (* can_delete)          (GDIcon   *self);

    bool     (* can_rename)          (GDIcon   *self);
};

GtkWidget  *gd_icon_new               (GDIconView *icon_view,
                                       GFile      *file,
                                       GFileInfo  *info);

GtkWidget  *gd_icon_get_image         (GDIcon     *self);

void        gd_icon_get_press         (GDIcon     *self,
                                       double     *x,
                                       double     *y);

void        gd_icon_set_file          (GDIcon     *self,
                                       GFile      *file);

GFile      *gd_icon_get_file          (GDIcon     *self);

GFileInfo  *gd_icon_get_file_info     (GDIcon     *self);

const char *gd_icon_get_name          (GDIcon     *self);

const char *gd_icon_get_name_collated (GDIcon     *self);

GFileType   gd_icon_get_file_type     (GDIcon     *self);

guint64     gd_icon_get_time_modified (GDIcon     *self);

guint64     gd_icon_get_size          (GDIcon     *self);

bool    gd_icon_is_hidden         (GDIcon     *self);

void        gd_icon_set_selected      (GDIcon     *self,
                                       bool    selected);

bool    gd_icon_get_selected      (GDIcon     *self);

void        gd_icon_open              (GDIcon     *self);

void        gd_icon_rename            (GDIcon     *self);

void        gd_icon_popup_menu        (GDIcon     *self);

void        gd_icon_update            (GDIcon     *self);

C_END_EXTERN_C

#endif // graceful_DE2_GD_ICON_H
