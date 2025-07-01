//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_THUMBNAIL_FACTORY_H
#define graceful_DE2_GD_THUMBNAIL_FACTORY_H
#include <gio/gio.h>
#include "macros/macros.h"

C_BEGIN_EXTERN_C

typedef enum
{
    GD_THUMBNAIL_ERROR_CAN_NOT_THUMBNAIL,
    GD_THUMBNAIL_ERROR_HAS_FAILED_THUMBNAIL,
    GD_THUMBNAIL_ERROR_GENERATION_FAILED
} GDThumbnailError;

#define GD_THUMBNAIL_ERROR (gd_thumbnail_error_quark ())
GQuark gd_thumbnail_error_quark (void);

#define GD_TYPE_THUMBNAIL_FACTORY (gd_thumbnail_factory_get_type ())
G_DECLARE_FINAL_TYPE (GDThumbnailFactory, gd_thumbnail_factory, GD, THUMBNAIL_FACTORY, GObject)

GDThumbnailFactory *gd_thumbnail_factory_new         (void);

void                gd_thumbnail_factory_load_async  (GDThumbnailFactory   *self,
                                                      const char           *uri,
                                                      const char           *content_type,
                                                      guint64               time_modified,
                                                      GCancellable         *cancellable,
                                                      GAsyncReadyCallback   callback,
                                                      gpointer              user_data);

GIcon              *gd_thumbnail_factory_load_finish (GDThumbnailFactory   *self,
                                                      GAsyncResult         *result,
                                                      GError              **error);

C_END_EXTERN_C

#endif // graceful_DE2_GD_THUMBNAIL_FACTORY_H
