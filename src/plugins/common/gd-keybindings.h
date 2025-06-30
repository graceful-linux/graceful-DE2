//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_KEY_BINDINGS_H
#define graceful_DE2_GD_KEY_BINDINGS_H
#include "macros/macros.h"

#include <gtk/gtk.h>

C_BEGIN_EXTERN_C

typedef enum
{
    GD_KEYBINDING_NONE,
    GD_KEYBINDING_ISO_NEXT_GROUP,
    GD_KEYBINDING_ISO_FIRST_GROUP,
    GD_KEYBINDING_ISO_LAST_GROUP
} GDKeybindingType;

#define GD_TYPE_KEYBINDINGS gd_keybindings_get_type ()
G_DECLARE_FINAL_TYPE (GDKeybindings, gd_keybindings, GD, KEYBINDINGS, GObject)

GDKeybindings   *gd_keybindings_new            (void);
void             gd_keybindings_grab_iso_group (GDKeybindings *keybindings, const gchar   *iso_group);
guint            gd_keybindings_grab           (GDKeybindings *keybindings, const gchar   *accelerator);
bool             gd_keybindings_ungrab         (GDKeybindings *keybindings, guint action);
guint            gd_keybindings_get_keyval     (GDKeybindings *keybindings, guint action);
GdkModifierType  gd_keybindings_get_modifiers  (GDKeybindings *keybindings, guint action);

C_END_EXTERN_C

#endif // graceful_DE2_GD_KEY_BINDINGS_H
