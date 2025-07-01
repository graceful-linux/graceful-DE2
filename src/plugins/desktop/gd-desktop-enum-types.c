//
// Created by dingjing on 25-6-30.
//
#include "gd-desktop-enum-types.h"
#include "gd-desktop-enums.h"

GType
gd_icon_size_get_type(void)
{
    static gsize g_enum_type_id;
    static const GEnumValue values[] = {
        {GD_ICON_SIZE_16PX, "GD_ICON_SIZE_16PX", "16px"}, {GD_ICON_SIZE_22PX, "GD_ICON_SIZE_22PX", "22px"}, {GD_ICON_SIZE_24PX, "GD_ICON_SIZE_24PX", "24px"}, {GD_ICON_SIZE_32PX, "GD_ICON_SIZE_32PX", "32px"},
        {GD_ICON_SIZE_48PX, "GD_ICON_SIZE_48PX", "48px"}, {GD_ICON_SIZE_64PX, "GD_ICON_SIZE_64PX", "64px"}, {GD_ICON_SIZE_72PX, "GD_ICON_SIZE_72PX", "72px"}, {GD_ICON_SIZE_96PX, "GD_ICON_SIZE_96PX", "96px"},
        {GD_ICON_SIZE_128PX, "GD_ICON_SIZE_128PX", "128px"}, {0, NULL, NULL}
    };

    if (g_once_init_enter(&g_enum_type_id)) {
        const gchar* string;
        GType id;

        string = g_intern_static_string("GDIconSize");
        id = g_enum_register_static(string, values);

        g_once_init_leave(&g_enum_type_id, id);
    }

    return g_enum_type_id;
}

GType
gd_placement_get_type(void)
{
    static gsize g_enum_type_id;
    static const GEnumValue values[] = {
        {GD_PLACEMENT_AUTO_ARRANGE_ICONS, "GD_PLACEMENT_AUTO_ARRANGE_ICONS", "auto-arrange-icons"}, {GD_PLACEMENT_ALIGN_ICONS_TO_GRID, "GD_PLACEMENT_ALIGN_ICONS_TO_GRID", "align-icons-to-grid"},
        {GD_PLACEMENT_FREE, "GD_PLACEMENT_FREE", "free"}, {0, NULL, NULL}
    };

    if (g_once_init_enter(&g_enum_type_id)) {
        const gchar* string;
        GType id;

        string = g_intern_static_string("GDPlacement");
        id = g_enum_register_static(string, values);

        g_once_init_leave(&g_enum_type_id, id);
    }

    return g_enum_type_id;
}

GType
gd_sort_by_get_type(void)
{
    static gsize g_enum_type_id;
    static const GEnumValue values[] = {{GD_SORT_BY_NAME, "GD_SORT_BY_NAME", "name"}, {GD_SORT_BY_DATE_MODIFIED, "GD_SORT_BY_DATE_MODIFIED", "date-modified"}, {GD_SORT_BY_SIZE, "GD_SORT_BY_SIZE", "size"}, {0, NULL, NULL}};

    if (g_once_init_enter(&g_enum_type_id)) {
        const gchar* string;
        GType id;

        string = g_intern_static_string("GDSortBy");
        id = g_enum_register_static(string, values);

        g_once_init_leave(&g_enum_type_id, id);
    }

    return g_enum_type_id;
}

/* Generated data ends here */
