//
// Created by dingjing on 25-7-1.
//
#include "gd-thumbnail-factory.h"

#include <libgnome-desktop/gnome-desktop-thumbnail.h>

typedef struct
{
    GDThumbnailFactory* self;

    char* uri;
    char* content_type;
    guint64 time_modified;
} GDLoadData;

struct _GDThumbnailFactory
{
    GObject parent;

    GnomeDesktopThumbnailFactory* factory;
};

G_DEFINE_QUARK(gf-thumbnail-error-quark, gd_thumbnail_error)

G_DEFINE_TYPE(GDThumbnailFactory, gd_thumbnail_factory, G_TYPE_OBJECT)

static GDLoadData*
gd_load_data_new(GDThumbnailFactory* self, const char* uri, const char* content_type, guint64 time_modified)
{
    GDLoadData* data;

    data = g_new0(GDLoadData, 1);
    data->self = g_object_ref(self);

    data->uri = g_strdup(uri);
    data->content_type = g_strdup(content_type);
    data->time_modified = time_modified;

    return data;
}

static void
gd_load_data_free(gpointer data)
{
    GDLoadData* load_data;

    load_data = data;

    g_object_unref(load_data->self);
    g_free(load_data->uri);
    g_free(load_data->content_type);
    g_free(load_data);
}

static void
load_icon_in_thread(GTask* task, gpointer source_object, gpointer task_data, GCancellable* cancellable)
{
    GDLoadData* data;
    char* path;
    GdkPixbuf* pixbuf;

    data = g_task_get_task_data(task);

    if (!gnome_desktop_thumbnail_factory_can_thumbnail(data->self->factory, data->uri, data->content_type, data->time_modified)) {
        g_task_return_new_error(task, GD_THUMBNAIL_ERROR, GD_THUMBNAIL_ERROR_CAN_NOT_THUMBNAIL, "Can not thumbnail this file");
        return;
    }

    path = gnome_desktop_thumbnail_factory_lookup(data->self->factory, data->uri, data->time_modified);

    if (path != NULL) {
        GFile* file;

        file = g_file_new_for_path(path);
        g_free(path);

        g_task_return_pointer(task, g_file_icon_new(file), g_object_unref);
        g_object_unref(file);
        return;
    }

    if (gnome_desktop_thumbnail_factory_has_valid_failed_thumbnail(data->self->factory, data->uri, data->time_modified)) {
        g_task_return_new_error(task, GD_THUMBNAIL_ERROR, GD_THUMBNAIL_ERROR_HAS_FAILED_THUMBNAIL, "File has valid failed thumbnail");
        return;
    }

    pixbuf = gnome_desktop_thumbnail_factory_generate_thumbnail(data->self->factory, data->uri, data->content_type, NULL, NULL);

    if (pixbuf != NULL) {
        gnome_desktop_thumbnail_factory_save_thumbnail(data->self->factory, pixbuf, data->uri, data->time_modified, NULL, NULL);

        g_task_return_pointer(task, pixbuf, g_object_unref);
        return;
    }

    gnome_desktop_thumbnail_factory_create_failed_thumbnail(data->self->factory, data->uri, data->time_modified, NULL, NULL);

    g_task_return_new_error(task, GD_THUMBNAIL_ERROR, GD_THUMBNAIL_ERROR_GENERATION_FAILED, "Thumbnail generation failed");
}

static void
gd_thumbnail_factory_dispose(GObject* object)
{
    GDThumbnailFactory* self;

    self = GD_THUMBNAIL_FACTORY(object);

    g_clear_object(&self->factory);

    G_OBJECT_CLASS(gd_thumbnail_factory_parent_class)->dispose(object);
}

static void
gd_thumbnail_factory_class_init(GDThumbnailFactoryClass* self_class)
{
    GObjectClass* object_class;

    object_class = G_OBJECT_CLASS(self_class);

    object_class->dispose = gd_thumbnail_factory_dispose;
}

static void
gd_thumbnail_factory_init(GDThumbnailFactory* self)
{
    GnomeDesktopThumbnailSize thumbnail_size;

    thumbnail_size = GNOME_DESKTOP_THUMBNAIL_SIZE_LARGE;
    self->factory = gnome_desktop_thumbnail_factory_new(thumbnail_size);
}

GDThumbnailFactory*
gd_thumbnail_factory_new(void)
{
    return g_object_new(GD_TYPE_THUMBNAIL_FACTORY, NULL);
}

void
gd_thumbnail_factory_load_async(GDThumbnailFactory* self, const char* uri, const char* content_type, guint64 time_modified, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask* task;
    GDLoadData* data;

    task = g_task_new(self, cancellable, callback, user_data);

    data = gd_load_data_new(self, uri, content_type, time_modified);
    g_task_set_task_data(task, data, gd_load_data_free);

    g_task_run_in_thread(task, load_icon_in_thread);
    g_object_unref(task);
}

GIcon*
gd_thumbnail_factory_load_finish(GDThumbnailFactory* self, GAsyncResult* result, GError** error)
{
    g_return_val_if_fail(g_task_is_valid (result, self), NULL);

    return g_task_propagate_pointer(G_TASK(result), error);
}
