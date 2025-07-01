//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_ICON_VIEW_H
#define graceful_DE2_GD_ICON_VIEW_H
#include <gtk/gtk.h>
#include "macros/macros.h"
#include "gd-thumbnail-factory.h"

C_BEGIN_EXTERN_C

#define GD_TYPE_ICON_VIEW (gd_icon_view_get_type ())
G_DECLARE_FINAL_TYPE (GDIconView, gd_icon_view, GD, ICON_VIEW, GtkEventBox)

GtkWidget          *gd_icon_view_new                      (void);

GDThumbnailFactory *gd_icon_view_get_thumbnail_factory    (GDIconView          *self);

char               *gd_icon_view_get_file_attributes      (GDIconView          *self);

char               *gd_icon_view_get_desktop_uri          (GDIconView          *self);

void                gd_icon_view_set_representative_color (GDIconView          *self,
                                                           GdkRGBA             *color);

void                gd_icon_view_set_drag_rectangles      (GDIconView          *self,
                                                           GPtrArray           *rectangles);

void                gd_icon_view_clear_selection          (GDIconView          *self);

GList              *gd_icon_view_get_selected_icons       (GDIconView          *self);

void                gd_icon_view_show_item_properties     (GDIconView          *self,
                                                           const char * const  *uris);

void                gd_icon_view_empty_trash              (GDIconView          *self,
                                                           guint32              timestamp);

bool            gd_icon_view_validate_new_name        (GDIconView          *self,
                                                           GFileType            file_type,
                                                           const char          *new_name,
                                                           char               **message);

void                gd_icon_view_move_to_trash            (GDIconView          *self,
                                                           const char * const  *uris,
                                                           guint32              timestamp);

void                gd_icon_view_delete                   (GDIconView          *self,
                                                           const char * const  *uris,
                                                           guint32              timestamp);

void                gd_icon_view_rename_file              (GDIconView          *self,
                                                           const char          *uri,
                                                           const char          *new_name,
                                                           guint32              timestamp);

void                gd_icon_view_copy_uris                (GDIconView          *self,
                                                           const char * const  *uris,
                                                           const char          *destination,
                                                           guint32              timestamp);

void                gd_icon_view_move_uris                (GDIconView          *self,
                                                           const char * const  *uris,
                                                           const char          *destination,
                                                           guint32              timestamp);
C_END_EXTERN_C

#endif // graceful_DE2_GD_ICON_VIEW_H
