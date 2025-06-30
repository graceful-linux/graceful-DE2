//
// Created by dingjing on 25-6-30.
//
#include "gd-common-enum-types.h"

#include "gd-keybindings.h"

GType
gd_keybinding_type_get_type(void)
{
    static gsize g_enum_type_id;
    static const GEnumValue values[] = {
        {GD_KEYBINDING_NONE, "GD_KEYBINDING_NONE", "none"},
        {GD_KEYBINDING_ISO_NEXT_GROUP, "GD_KEYBINDING_ISO_NEXT_GROUP", "iso-next-group"},
        {GD_KEYBINDING_ISO_FIRST_GROUP, "GD_KEYBINDING_ISO_FIRST_GROUP", "iso-first-group"},
        {GD_KEYBINDING_ISO_LAST_GROUP, "GD_KEYBINDING_ISO_LAST_GROUP", "iso-last-group"},
        {0, NULL, NULL}
    };

    if (g_once_init_enter(&g_enum_type_id)) {
        const gchar* string;
        GType id;

        string = g_intern_static_string("GDKeybindingType");
        id = g_enum_register_static(string, values);

        g_once_init_leave(&g_enum_type_id, id);
    }

    return g_enum_type_id;
}

/* Generated data ends here */
