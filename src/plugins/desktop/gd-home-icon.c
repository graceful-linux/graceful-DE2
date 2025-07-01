//
// Created by dingjing on 25-6-30.
//
#include "gd-home-icon.h"

#include <glib/gi18n.h>

struct _GDHomeIcon
{
    GDIcon parent;
};

G_DEFINE_TYPE(GDHomeIcon, gd_home_icon, GD_TYPE_ICON)static GIcon* gd_home_icon_get_icon(GDIcon* icon, bool* is_thumbnail)
{
    GFileInfo* info;

    info = gd_icon_get_file_info(icon);
    *is_thumbnail = FALSE;

    return g_file_info_get_icon(info);
}

static const char*
gd_home_icon_get_text(GDIcon* icon)
{
    return _("Home");
}

static bool
gd_home_icon_can_delete(GDIcon* icon)
{
    return FALSE;
}

static bool
gd_home_icon_can_rename(GDIcon* icon)
{
    return FALSE;
}

static void
gd_home_icon_class_init(GDHomeIconClass* self_class)
{
    GDIconClass* icon_class;

    icon_class = GD_ICON_CLASS(self_class);

    icon_class->get_icon = gd_home_icon_get_icon;
    icon_class->get_text = gd_home_icon_get_text;
    icon_class->can_delete = gd_home_icon_can_delete;
    icon_class->can_rename = gd_home_icon_can_rename;
}

static void
gd_home_icon_init(GDHomeIcon* self)
{
}

GtkWidget*
gd_home_icon_new(GDIconView* icon_view, GError** error)
{
    GFile* file;
    char* attributes;
    GFileQueryInfoFlags flags;
    GFileInfo* info;
    GtkWidget* widget;

    file = g_file_new_for_path(g_get_home_dir());

    attributes = gd_icon_view_get_file_attributes(icon_view);
    flags = G_FILE_QUERY_INFO_NONE;

    info = g_file_query_info(file, attributes, flags, NULL, error);
    g_free(attributes);

    if (info == NULL) {
        g_object_unref(file);
        return NULL;
    }

    widget = g_object_new(GD_TYPE_HOME_ICON, "icon-view", icon_view, "file", file, "info", info, NULL);

    g_object_unref(file);
    g_object_unref(info);

    return widget;
}
