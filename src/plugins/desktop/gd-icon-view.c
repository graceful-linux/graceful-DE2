//
// Created by dingjing on 25-7-1.
//
#include "gd-icon-view.h"

#include <gdk/gdkx.h>
#include <glib/gi18n.h>

#include "gd-nautilus2-gen.dbus.h"
#include "gd-file-manager-gen.dbus.h"
#include "gd-create-folder-dialog.h"
#include "gd-desktop-enum-types.h"
#include "gd-desktop-enums.h"
#include "gd-dummy-icon.h"
#include "gd-home-icon.h"
#include "gd-icon.h"
#include "gd-monitor-view.h"
#include "gd-trash-icon.h"
#include "gd-utils.h"
#include "gd-workarea-watcher.h"

typedef struct
{
    GtkWidget* icon;

    GtkWidget* view;
} GDIconInfo;

struct _GDIconView
{
    GtkEventBox parent;

    GDWorkareaWatcher* workarea_watcher;

    GDThumbnailFactory* thumbnail_factory;

    GtkGesture* multi_press;
    GtkGesture* drag;

    GFile* desktop;
    GFileMonitor* monitor;

    GSettings* settings;

    GDPlacement placement;
    GDSortBy sort_by;

    GtkWidget* fixed;

    GtkWidget* dummy_icon;

    GCancellable* cancellable;

    GList* icons;

    GDIconInfo* home_info;
    GDIconInfo* trash_info;

    GList* selected_icons;

    GtkCssProvider* rubberband_css;
    GtkStyleContext* rubberband_style;
    GdkRectangle rubberband_rect;
    GList* rubberband_icons;

    GDNautilus2Gen* nautilus;
    GDFileManagerGen* file_manager;

    GtkWidget* create_folder_dialog;

    GDIcon* last_selected_icon;
    GDIcon* extend_from_icon;

    GPtrArray* drag_rectagles;
};

enum
{
    SELECT_ALL,
    UNSELECT_ALL,
    ACTIVATE,
    RENAME,
    TRASH,
    DELETE,
    TOGGLE,
    MOVE,
    LAST_SIGNAL
};

static guint view_signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(GDIconView, gd_icon_view, GTK_TYPE_EVENT_BOX)

static GVariant*
get_platform_data(GDIconView* self, guint32 timestamp)
{
    GVariantBuilder builder;
    GtkWidget* toplevel;
    GdkWindow* window;
    char* parent_handle;

    g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

    toplevel = gtk_widget_get_toplevel(GTK_WIDGET(self));
    window = gtk_widget_get_window(toplevel);
    parent_handle = g_strdup_printf("x11:%lx", gdk_x11_window_get_xid(window));

    g_variant_builder_add(&builder, "{sv}", "parent-handle", g_variant_new_take_string(parent_handle));

    g_variant_builder_add(&builder, "{sv}", "timestamp", g_variant_new_uint32(timestamp));

    g_variant_builder_add(&builder, "{sv}", "window-position", g_variant_new_string("center"));

    return g_variant_builder_end(&builder);
}

static char*
build_attributes_list(const char* first, ...)
{
    GString* attributes;
    va_list args;
    const char* attribute;

    attributes = g_string_new(first);
    va_start(args, first);

    while ((attribute = va_arg(args, const char *)) != NULL) g_string_append_printf(attributes, ",%s", attribute);

    va_end(args);

    return g_string_free(attributes, FALSE);
}

static GDIconInfo*
gd_icon_info_new(GtkWidget* icon)
{
    GDIconInfo* info;

    info = g_new0(GDIconInfo, 1);
    info->icon = g_object_ref_sink(icon);

    info->view = NULL;

    return info;
}

static void
gd_icon_info_free(gpointer data)
{
    GDIconInfo* info;

    info = (GDIconInfo*)data;

    g_clear_pointer(&info->icon, g_object_unref);
    g_free(info);
}

static GList*
get_monitor_views(GDIconView* self)
{
    GList* views;
    GList* children;
    GList* l;

    views = NULL;

    children = gtk_container_get_children(GTK_CONTAINER(self->fixed));

    for (l = children; l != NULL; l = l->next) {
        GDMonitorView* view;

        view = GD_MONITOR_VIEW(l->data);

        if (gd_monitor_view_is_primary(view)) views = g_list_prepend(views, view);
        else views = g_list_append(views, view);
    }

    g_list_free(children);
    return views;
}

static void
add_icons(GDIconView* self)
{
    GList* views;
    GList* view;
    GList* l;

    views = get_monitor_views(self);
    view = views;

    for (l = self->icons; l != NULL; l = l->next) {
        GDIconInfo* info;

        info = (GDIconInfo*)l->data;

        if (gd_icon_is_hidden(GD_ICON(info->icon))) continue;

        if (info->view != NULL) continue;

        while (view != NULL) {
            if (!gd_monitor_view_add_icon(GD_MONITOR_VIEW(view->data), info->icon)) {
                view = view->next;
                continue;
            }

            info->view = view->data;

            break;
        }

        if (view == NULL) break;
    }

    g_list_free(views);
}

static int
compare_directory(GDIcon* a, GDIcon* b, GDIconView* self)
{
    bool is_dir_a;
    bool is_dir_b;

    is_dir_a = gd_icon_get_file_type(a) == G_FILE_TYPE_DIRECTORY;
    is_dir_b = gd_icon_get_file_type(b) == G_FILE_TYPE_DIRECTORY;

    if (is_dir_a != is_dir_b) return is_dir_a ? -1 : 1;

    return 0;
}

static int
compare_name(GDIcon* a, GDIcon* b, GDIconView* self)
{
    const char* name_a;
    const char* name_b;

    name_a = gd_icon_get_name_collated(a);
    name_b = gd_icon_get_name_collated(b);

    return g_strcmp0(name_a, name_b);
}

static int
compare_modified(GDIcon* a, GDIcon* b, GDIconView* self)
{
    guint64 modified_a;
    guint64 modified_b;

    modified_a = gd_icon_get_time_modified(a);
    modified_b = gd_icon_get_time_modified(b);

    if (modified_a < modified_b) return -1;
    else if (modified_a > modified_b) return 1;

    return 0;
}

static int
compare_size(GDIcon* a, GDIcon* b, GDIconView* self)
{
    guint64 size_a;
    guint64 size_b;

    size_a = gd_icon_get_size(a);
    size_b = gd_icon_get_size(b);

    if (size_a < size_b) return -1;
    else if (size_a > size_b) return 1;

    return 0;
}

static int
compare_name_func(gconstpointer a, gconstpointer b, gpointer user_data)
{
    GDIconView* self;
    GDIconInfo* info_a;
    GDIcon* icon_a;
    GDIconInfo* info_b;
    GDIcon* icon_b;
    int result;

    self = GD_ICON_VIEW(user_data);

    info_a = (GDIconInfo*)a;
    icon_a = GD_ICON(info_a->icon);

    info_b = (GDIconInfo*)b;
    icon_b = GD_ICON(info_b->icon);

    result = compare_directory(icon_a, icon_b, self);

    if (result == 0) result = compare_name(icon_a, icon_b, self);

    return result;
}

static int
compare_modified_func(gconstpointer a, gconstpointer b, gpointer user_data)
{
    GDIconView* self;
    GDIconInfo* info_a;
    GDIcon* icon_a;
    GDIconInfo* info_b;
    GDIcon* icon_b;
    int result;

    self = GD_ICON_VIEW(user_data);

    info_a = (GDIconInfo*)a;
    icon_a = GD_ICON(info_a->icon);

    info_b = (GDIconInfo*)b;
    icon_b = GD_ICON(info_b->icon);

    result = compare_directory(icon_a, icon_b, self);

    if (result == 0) result = compare_modified(icon_a, icon_b, self);

    return result;
}

static int
compare_size_func(gconstpointer a, gconstpointer b, gpointer user_data)
{
    GDIconView* self;
    GDIconInfo* info_a;
    GDIcon* icon_a;
    GDIconInfo* info_b;
    GDIcon* icon_b;
    int result;

    self = GD_ICON_VIEW(user_data);

    info_a = (GDIconInfo*)a;
    icon_a = GD_ICON(info_a->icon);

    info_b = (GDIconInfo*)b;
    icon_b = GD_ICON(info_b->icon);

    result = compare_directory(icon_a, icon_b, self);

    if (result == 0) result = compare_size(icon_a, icon_b, self);

    return result;
}

static bool
sort_icons(GDIconView* self)
{
    GList* old_list;
    bool changed;
    GList* l1;
    GList* l2;

    old_list = g_list_copy(self->icons);

    if (self->sort_by == GD_SORT_BY_NAME) self->icons = g_list_sort_with_data(self->icons, compare_name_func, self);
    else if (self->sort_by == GD_SORT_BY_DATE_MODIFIED) self->icons = g_list_sort_with_data(self->icons, compare_modified_func, self);
    else if (self->sort_by == GD_SORT_BY_SIZE) self->icons = g_list_sort_with_data(self->icons, compare_size_func, self);

    changed = FALSE;
    for (l1 = self->icons, l2 = old_list; l1 != NULL; l1 = l1->next, l2 = l2->next) {
        if (l1->data == l2->data) continue;

        changed = TRUE;
        break;
    }

    g_list_free(old_list);
    return changed;
}

static void
remove_and_readd_icons(GDIconView* self)
{
    GList* l;

    for (l = self->icons; l != NULL; l = l->next) {
        GDIconInfo* info;

        info = (GDIconInfo*)l->data;

        if (info->view == NULL) continue;

        gd_monitor_view_remove_icon(GD_MONITOR_VIEW(info->view), info->icon);
        info->view = NULL;
    }

    add_icons(self);
}

static void
resort_icons(GDIconView* self, bool force)
{
    if (!sort_icons(self) && !force) return;

    remove_and_readd_icons(self);
}

static void
unselect_cb(gpointer data, gpointer user_data)
{
    gd_icon_set_selected(data, FALSE);
}

static void
unselect_icons(GDIconView* self)
{
    if (self->selected_icons == NULL) return;

    g_list_foreach(self->selected_icons, unselect_cb, NULL);
    g_clear_pointer(&self->selected_icons, g_list_free);
}

static void
icon_selected_cb(GDIcon* icon, GDIconView* self)
{
    if (gd_icon_get_selected(icon)) self->selected_icons = g_list_append(self->selected_icons, icon);
    else self->selected_icons = g_list_remove(self->selected_icons, icon);
}

static void
icon_has_focus_cb(GtkWidget* widget, GParamSpec* pspec, GDIconView* self)
{
    if (gtk_widget_has_focus(widget)) self->last_selected_icon = GD_ICON(widget);
}

static void
icon_clicked_cb(GtkWidget* widget, GDIconView* self)
{
    self->last_selected_icon = GD_ICON(widget);
    self->extend_from_icon = NULL;
}

static void
show_item_properties_cb(GObject* object, GAsyncResult* res, gpointer user_data)
{
    GError* error;

    error = NULL;
    gd_file_manager_gen_call_show_item_properties_finish(GD_FILE_MANAGER_GEN(object), res, &error);

    if (error != NULL) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            g_warning("Error showing properties: %s", error->message);
        g_error_free(error);
    }
}

static void
empty_trash_cb(GObject* object, GAsyncResult* res, gpointer user_data)
{
    GError* error;

    error = NULL;
    gd_nautilus2_gen_call_empty_trash_finish(GD_NAUTILUS2_GEN(object), res, &error);

    if (error != NULL) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            g_warning("Error emptying trash: %s", error->message);
        g_error_free(error);
    }
}

static void
trash_uris_cb(GObject* object, GAsyncResult* res, gpointer user_data)
{
    GError* error;

    error = NULL;
    gd_nautilus2_gen_call_trash_uris_finish(GD_NAUTILUS2_GEN(object), res, &error);

    if (error != NULL) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            g_warning("Error moving files to trash: %s", error->message);
        g_error_free(error);
    }
}

static void
delete_uris_cb(GObject* object, GAsyncResult* res, gpointer user_data)
{
    GError* error;

    error = NULL;
    gd_nautilus2_gen_call_delete_uris_finish(GD_NAUTILUS2_GEN(object), res, &error);

    if (error != NULL) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            g_warning("Error deleting files: %s", error->message);
        g_error_free(error);
    }
}

static void
rename_uri_cb(GObject* object, GAsyncResult* res, gpointer user_data)
{
    GError* error;

    error = NULL;
    gd_nautilus2_gen_call_rename_uri_finish(GD_NAUTILUS2_GEN(object), res, &error);

    if (error != NULL) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            g_warning("Error renaming file: %s", error->message);
        g_error_free(error);
    }
}

static void
copy_uris_cb(GObject* object, GAsyncResult* res, gpointer user_data)
{
    GError* error;

    error = NULL;
    gd_nautilus2_gen_call_copy_uris_finish(GD_NAUTILUS2_GEN(object), res, &error);

    if (error != NULL) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            g_warning("Error copying uris: %s", error->message);
        g_error_free(error);
    }
}

static void
move_uris_cb(GObject* object, GAsyncResult* res, gpointer user_data)
{
    GError* error;

    error = NULL;
    gd_nautilus2_gen_call_move_uris_finish(GD_NAUTILUS2_GEN(object), res, &error);

    if (error != NULL) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            g_warning("Error moving uris: %s", error->message);
        g_error_free(error);
    }
}

static GDIconInfo*
create_icon_info(GDIconView* self, GtkWidget* icon)
{
    g_signal_connect(icon, "selected", G_CALLBACK (icon_selected_cb), self);

    g_signal_connect(icon, "notify::has-focus", G_CALLBACK (icon_has_focus_cb), self);

    g_signal_connect(icon, "clicked", G_CALLBACK (icon_clicked_cb), self);

    g_settings_bind(self->settings, "icon-size", icon, "icon-size", G_SETTINGS_BIND_GET);

    g_settings_bind(self->settings, "extra-text-width", icon, "extra-text-width", G_SETTINGS_BIND_GET);

    return gd_icon_info_new(icon);
}

static void
icon_changed_cb(GDIcon* icon, GDIconView* self)
{
    if (self->placement == GD_PLACEMENT_AUTO_ARRANGE_ICONS) resort_icons(self, FALSE);
}

static void
query_info_cb(GObject* object, GAsyncResult* res, gpointer user_data)
{
    GFile* file;
    GFileInfo* file_info;
    GError* error;
    GDIconView* self;
    GtkWidget* icon;
    GDIconInfo* icon_info;

    file = G_FILE(object);

    error = NULL;
    file_info = g_file_query_info_finish(file, res, &error);

    if (error != NULL) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            g_warning("%s", error->message);

        g_error_free(error);
        return;
    }

    self = GD_ICON_VIEW(user_data);

    icon = gd_icon_new(self, file, file_info);
    g_object_unref(file_info);

    g_signal_connect(icon, "changed", G_CALLBACK (icon_changed_cb), self);

    icon_info = create_icon_info(self, icon);
    self->icons = g_list_prepend(self->icons, icon_info);

    if (self->placement == GD_PLACEMENT_AUTO_ARRANGE_ICONS) resort_icons(self, TRUE);
    else add_icons(self);

    gd_icon_update(GD_ICON(icon));
}

static GDIconInfo*
find_icon_info_by_file(GDIconView* self, GFile* file)
{
    GList* l;

    for (l = self->icons; l != NULL; l = l->next) {
        GDIconInfo* info;
        GFile* icon_file;

        info = (GDIconInfo*)l->data;
        icon_file = gd_icon_get_file(GD_ICON(info->icon));

        if (g_file_equal(icon_file, file)) return info;
    }

    return NULL;
}

static void
remove_icon_from_view(GDIconView* self, GDIconInfo* info)
{
    GDIcon* icon;

    if (info->view == NULL) return;

    icon = GD_ICON(info->icon);

    gd_monitor_view_remove_icon(GD_MONITOR_VIEW(info->view), GTK_WIDGET(icon));
    info->view = NULL;

    self->selected_icons = g_list_remove(self->selected_icons, icon);
    self->rubberband_icons = g_list_remove(self->rubberband_icons, icon);

    if (icon == self->last_selected_icon) self->last_selected_icon = NULL;

    if (icon == self->extend_from_icon) self->extend_from_icon = NULL;
}

static void
remove_icon(GDIconView* self, GDIconInfo* info)
{
    remove_icon_from_view(self, info);

    self->icons = g_list_remove(self->icons, info);
    gd_icon_info_free(info);

    if (self->placement == GD_PLACEMENT_AUTO_ARRANGE_ICONS) remove_and_readd_icons(self);
}

static void
file_deleted(GDIconView* self, GFile* deleted_file)
{
    GDIconInfo* info;

    info = find_icon_info_by_file(self, deleted_file);

    if (info == NULL) return;

    remove_icon(self, info);
}

static void
file_created(GDIconView* self, GFile* created_file)
{
    char* attributes;

    attributes = gd_icon_view_get_file_attributes(self);

    g_file_query_info_async(created_file, attributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_LOW, self->cancellable, query_info_cb, self);

    g_free(attributes);
}

static void
file_renamed(GDIconView* self, GFile* old_file, GFile* new_file)
{
    GDIconInfo* old_info;
    GDIconInfo* new_info;

    old_info = find_icon_info_by_file(self, old_file);
    new_info = find_icon_info_by_file(self, new_file);

    if (old_info != NULL && new_info != NULL) {
        gd_icon_set_file(GD_ICON(new_info->icon), new_file);
        remove_icon(self, old_info);
    }
    else if (old_info != NULL) {
        gd_icon_set_file(GD_ICON(old_info->icon), new_file);
    }
    else if (new_info != NULL) {
        gd_icon_set_file(GD_ICON(new_info->icon), new_file);
    }
}

static void
desktop_changed_cb(GFileMonitor* monitor, GFile* file, GFile* other_file, GFileMonitorEvent event_type, GDIconView* self)
{
    switch (event_type) {
    case G_FILE_MONITOR_EVENT_CHANGED:
        break;

    case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
        break;

    case G_FILE_MONITOR_EVENT_DELETED:
        file_deleted(self, file);
        break;

    case G_FILE_MONITOR_EVENT_CREATED:
        file_created(self, file);
        break;

    case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
        break;

    case G_FILE_MONITOR_EVENT_PRE_UNMOUNT:
        break;

    case G_FILE_MONITOR_EVENT_UNMOUNTED:
        break;

    case G_FILE_MONITOR_EVENT_MOVED:
        break;

    case G_FILE_MONITOR_EVENT_RENAMED:
        file_renamed(self, file, other_file);
        break;

    case G_FILE_MONITOR_EVENT_MOVED_IN:
        file_created(self, file);
        break;

    case G_FILE_MONITOR_EVENT_MOVED_OUT:
        file_deleted(self, file);
        break;

    default:
        break;
    }
}

static void
create_folder_dialog_validate_cb(GDCreateFolderDialog* dialog, const char* folder_name, GDIconView* self)
{
    char* message;
    bool valid;

    message = NULL;
    valid = gd_icon_view_validate_new_name(self, G_FILE_TYPE_DIRECTORY, folder_name, &message);

    gd_create_folder_dialog_set_valid(dialog, valid, message);
    g_free(message);
}

static void
create_folder_cb(GObject* object, GAsyncResult* res, gpointer user_data)
{
    GError* error;

    error = NULL;
    gd_nautilus2_gen_call_create_folder_finish(GD_NAUTILUS2_GEN(object), res, &error);

    if (error != NULL) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            g_warning("Error creating new folder: %s", error->message);
        g_error_free(error);
    }
}

static void
create_folder_dialog_response_cb(GtkDialog* dialog, gint response_id, GDIconView* self)
{
    GDCreateFolderDialog* folder_dialog;

    if (response_id != GTK_RESPONSE_ACCEPT) return;

    folder_dialog = GD_CREATE_FOLDER_DIALOG(dialog);

    if (self->nautilus != NULL) {
        char* parent_uri;
        char* folder_name;
        guint32 timestamp;

        parent_uri = g_file_get_uri(self->desktop);
        folder_name = gd_create_folder_dialog_get_folder_name(folder_dialog);
        timestamp = gtk_get_current_event_time();

        gd_nautilus2_gen_call_create_folder(self->nautilus, parent_uri, folder_name, get_platform_data(self, timestamp), self->cancellable, create_folder_cb, NULL);

        g_free(parent_uri);
        g_free(folder_name);
    }
    else {
        g_assert_not_reached();
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void
create_folder_dialog_destroy_cb(GtkWidget* widget, GDIconView* self)
{
    self->create_folder_dialog = NULL;
}

static void
new_folder_cb(GtkMenuItem* item, GDIconView* self)
{
    GtkWidget* dialog;

    if (self->nautilus == NULL) return;

    if (self->create_folder_dialog != NULL) {
        gtk_window_present(GTK_WINDOW(self->create_folder_dialog));
        return;
    }

    dialog = gd_create_folder_dialog_new();
    self->create_folder_dialog = dialog;

    g_signal_connect(dialog, "validate", G_CALLBACK (create_folder_dialog_validate_cb), self);

    g_signal_connect(dialog, "response", G_CALLBACK (create_folder_dialog_response_cb), self);

    g_signal_connect(dialog, "destroy", G_CALLBACK (create_folder_dialog_destroy_cb), self);

    gtk_window_present(GTK_WINDOW(dialog));
}

static void
change_background_cb(GtkMenuItem* item, GDIconView* self)
{
    GError* error;

    error = NULL;
    if (!gd_launch_desktop_file("gnome-background-panel.desktop", &error)) {
        g_warning("%s", error->message);
        g_error_free(error);
    }
}

static void
display_settings_cb(GtkMenuItem* item, GDIconView* self)
{
    GError* error;

    error = NULL;
    if (!gd_launch_desktop_file("gnome-display-panel.desktop", &error)) {
        g_warning("%s", error->message);
        g_error_free(error);
    }
}

static void
open_terminal_cb(GtkMenuItem* item, GDIconView* self)
{
    GError* error;

    error = NULL;
    if (!gd_launch_desktop_file("org.gnome.Terminal.desktop", &error)) {
        g_warning("%s", error->message);
        g_error_free(error);
    }
}

typedef struct
{
    GDIconView* self;
    GDPlacement placement;
} GDPlacementData;

static void
free_placement_data(gpointer data, GClosure* closure)
{
    g_free(data);
}

static void
placement_cb(GtkCheckMenuItem* item, GDPlacementData* data)
{
    if (!gtk_check_menu_item_get_active(item)) return;

    g_settings_set_enum(data->self->settings, "placement", data->placement);
}

static const char*
placement_to_string(GDPlacement placement)
{
    const char* string;

    string = NULL;

    switch (placement) {
    case GD_PLACEMENT_AUTO_ARRANGE_ICONS:
        string = _("Auto arrange icons");
        break;

    case GD_PLACEMENT_ALIGN_ICONS_TO_GRID:
        string = _("Align icons to grid");
        break;

    case GD_PLACEMENT_FREE:
        string = C_("Free placement of icons", "Free");
        break;

    case GD_PLACEMENT_LAST: default:
        break;
    }

    g_assert(string != NULL);

    return string;
}

static bool
is_placement_implemented(GDPlacement placement)
{
    switch (placement) {
    case GD_PLACEMENT_AUTO_ARRANGE_ICONS:
        return TRUE;

    case GD_PLACEMENT_ALIGN_ICONS_TO_GRID:
    case GD_PLACEMENT_FREE:
    case GD_PLACEMENT_LAST: default:
        break;
    }

    return FALSE;
}

static void
append_placement_submenu(GDIconView* self, GtkWidget* parent)
{
    GtkWidget* menu;
    GtkStyleContext* context;
    GSList* group;
    GDPlacement i;

    menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent), menu);

    context = gtk_widget_get_style_context(menu);
    gtk_style_context_add_class(context, GTK_STYLE_CLASS_CONTEXT_MENU);

    group = NULL;

    for (i = 0; i < GD_PLACEMENT_LAST; i++) {
        const char* label;
        GtkWidget* item;
        GDPlacementData* data;

        label = placement_to_string(i);

        item = gtk_radio_menu_item_new_with_label(group, label);
        group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));

        if (i == self->placement) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);

        gtk_widget_set_sensitive(item, is_placement_implemented(i));

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        gtk_widget_show(item);

        data = g_new0(GDPlacementData, 1);

        data->self = self;
        data->placement = i;

        g_signal_connect_data(item, "toggled", G_CALLBACK(placement_cb), data, free_placement_data, 0);
    }
}

typedef struct
{
    GDIconView* self;
    GDSortBy sort_by;
} GDSortByData;

static void
free_sort_by_data(gpointer data, GClosure* closure)
{
    g_free(data);
}

static void
sort_by_toggled_cb(GtkCheckMenuItem* item, GDSortByData* data)
{
    if (!gtk_check_menu_item_get_active(item)) return;

    g_settings_set_enum(data->self->settings, "sort-by", data->sort_by);
}

static void
sort_by_activate_cb(GtkMenuItem* item, GDSortByData* data)
{
    g_settings_set_enum(data->self->settings, "sort-by", data->sort_by);
    resort_icons(data->self, FALSE);
}

static const char*
sort_by_to_string(GDSortBy sort_by)
{
    const char* string;

    string = NULL;

    switch (sort_by) {
    case GD_SORT_BY_NAME:
        string = _("Name");
        break;

    case GD_SORT_BY_DATE_MODIFIED:
        string = _("Date modified");
        break;

    case GD_SORT_BY_SIZE:
        string = _("Size");
        break;

    case GD_SORT_BY_LAST: default:
        break;
    }

    g_assert(string != NULL);

    return string;
}

static void
append_sort_by_submenu(GDIconView* self, GtkWidget* parent)
{
    GtkWidget* menu;
    GtkStyleContext* context;
    GSList* group;
    GDSortBy i;

    menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent), menu);

    context = gtk_widget_get_style_context(menu);
    gtk_style_context_add_class(context, GTK_STYLE_CLASS_CONTEXT_MENU);

    group = NULL;

    for (i = 0; i < GD_SORT_BY_LAST; i++) {
        const char* label;
        GtkWidget* item;
        GDSortByData* data;

        label = sort_by_to_string(i);

        if (self->placement == GD_PLACEMENT_AUTO_ARRANGE_ICONS) {
            item = gtk_radio_menu_item_new_with_label(group, label);
            group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));

            if (i == self->sort_by) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
        }
        else {
            item = gtk_menu_item_new_with_label(label);
        }

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        gtk_widget_show(item);

        data = g_new0(GDSortByData, 1);

        data->self = self;
        data->sort_by = i;

        if (self->placement == GD_PLACEMENT_AUTO_ARRANGE_ICONS) {
            g_signal_connect_data(item, "toggled", G_CALLBACK(sort_by_toggled_cb), data, free_sort_by_data, 0);
        }
        else {
            g_signal_connect_data(item, "activate", G_CALLBACK(sort_by_activate_cb), data, free_sort_by_data, 0);
        }
    }
}

static GtkWidget*
create_popup_menu(GDIconView* self)
{
    GtkWidget* popup_menu;
    GtkStyleContext* context;
    GtkWidget* item;

    popup_menu = gtk_menu_new();

    context = gtk_widget_get_style_context(popup_menu);
    gtk_style_context_add_class(context, GTK_STYLE_CLASS_CONTEXT_MENU);

    item = gtk_menu_item_new_with_label(_("New Folder"));
    gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), item);
    gtk_widget_show(item);

    g_signal_connect(item, "activate", G_CALLBACK (new_folder_cb), self);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), item);
    gtk_widget_show(item);

    item = gtk_menu_item_new_with_label(_("Placement"));
    gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), item);
    gtk_widget_show(item);

    append_placement_submenu(self, item);

    item = gtk_menu_item_new_with_label(_("Sort by"));
    gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), item);
    gtk_widget_show(item);

    append_sort_by_submenu(self, item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), item);
    gtk_widget_show(item);

    item = gtk_menu_item_new_with_label(_("Change Background"));
    gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), item);
    gtk_widget_show(item);

    g_signal_connect(item, "activate", G_CALLBACK (change_background_cb), self);

    item = gtk_menu_item_new_with_label(_("Display Settings"));
    gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), item);
    gtk_widget_show(item);

    g_signal_connect(item, "activate", G_CALLBACK (display_settings_cb), self);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), item);
    gtk_widget_show(item);

    item = gtk_menu_item_new_with_label(_("Open Terminal"));
    gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), item);
    gtk_widget_show(item);

    g_signal_connect(item, "activate", G_CALLBACK (open_terminal_cb), self);

    return popup_menu;
}

static void
multi_press_pressed_cb(GtkGestureMultiPress* gesture, gint n_press, gdouble x, gdouble y, GDIconView* self)
{
    guint button;
    GdkEventSequence* sequence;
    const GdkEvent* event;

    button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
    sequence = gtk_gesture_single_get_current_sequence(GTK_GESTURE_SINGLE(gesture));
    event = gtk_gesture_get_last_event(GTK_GESTURE(gesture), sequence);

    if (event == NULL) return;

    if (!gtk_widget_has_focus(GTK_WIDGET(self))) gtk_widget_grab_focus(GTK_WIDGET(self));

    if (button == GDK_BUTTON_PRIMARY) {
        GdkModifierType state;
        bool control_pressed;
        bool shift_pressed;

        gdk_event_get_state(event, &state);

        control_pressed = (state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK;
        shift_pressed = (state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK;

        if (!control_pressed && !shift_pressed) {
            g_clear_pointer(&self->rubberband_icons, g_list_free);
            unselect_icons(self);
        }
    }
    else if (button == GDK_BUTTON_SECONDARY) {
        GtkWidget* popup_menu;

        unselect_icons(self);

        popup_menu = create_popup_menu(self);
        g_object_ref_sink(popup_menu);

        gtk_menu_popup_at_pointer(GTK_MENU(popup_menu), event);
        g_object_unref(popup_menu);
    }
}

static void
drag_begin_cb(GtkGestureDrag* gesture, gdouble start_x, gdouble start_y, GDIconView* self)
{
    self->rubberband_rect = (GdkRectangle){0};

    gtk_widget_queue_draw(GTK_WIDGET(self));
}

static void
drag_end_cb(GtkGestureDrag* gesture, gdouble offset_x, gdouble offset_y, GDIconView* self)
{
    g_clear_pointer(&self->rubberband_icons, g_list_free);
    gtk_widget_queue_draw(GTK_WIDGET(self));
}

static void
drag_update_cb(GtkGestureDrag* gesture, gdouble offset_x, gdouble offset_y, GDIconView* self)
{
    double start_x;
    double start_y;
    GdkRectangle old_rect;

    gtk_gesture_drag_get_start_point(GTK_GESTURE_DRAG(self->drag), &start_x, &start_y);

    old_rect = self->rubberband_rect;
    self->rubberband_rect = (GdkRectangle){.x = start_x, .y = start_y, .width = ABS(offset_x), .height = ABS(offset_y)};

    if (offset_x < 0) self->rubberband_rect.x += offset_x;

    if (offset_y < 0) self->rubberband_rect.y += offset_y;

    if ((self->rubberband_rect.x > old_rect.x || self->rubberband_rect.y > old_rect.y || self->rubberband_rect.width < old_rect.width || self->rubberband_rect.height < old_rect.height) && self->rubberband_icons != NULL) {
        GList* rubberband_icons;
        GList* l;

        rubberband_icons = g_list_copy(self->rubberband_icons);

        for (l = rubberband_icons; l != NULL; l = l->next) {
            GDIcon* icon;
            GtkAllocation allocation;

            icon = l->data;

            gtk_widget_get_allocation(GTK_WIDGET(icon), &allocation);

            if (!gdk_rectangle_intersect(&self->rubberband_rect, &allocation, NULL)) {
                if (gd_icon_get_selected(icon)) gd_icon_set_selected(icon, FALSE);
                else gd_icon_set_selected(icon, TRUE);

                self->rubberband_icons = g_list_remove(self->rubberband_icons, icon);
            }
        }

        g_list_free(rubberband_icons);
    }

    if (self->rubberband_rect.x < old_rect.x || self->rubberband_rect.y < old_rect.y || self->rubberband_rect.width > old_rect.width || self->rubberband_rect.height > old_rect.height) {
        GList* rubberband_icons;
        GList* monitor_views;
        GList* l;

        rubberband_icons = NULL;
        monitor_views = get_monitor_views(self);

        for (l = monitor_views; l != NULL; l = l->next) {
            GDMonitorView* monitor_view;
            GtkAllocation allocation;
            GdkRectangle rect;

            monitor_view = l->data;

            gtk_widget_get_allocation(GTK_WIDGET(monitor_view), &allocation);

            if (gdk_rectangle_intersect(&self->rubberband_rect, &allocation, &rect)) {
                GList* icons;

                icons = gd_monitor_view_get_icons(monitor_view, &rect);
                rubberband_icons = g_list_concat(rubberband_icons, icons);
            }
        }

        g_list_free(monitor_views);

        if (rubberband_icons != NULL) {
            for (l = rubberband_icons; l != NULL; l = l->next) {
                GDIcon* icon;

                icon = l->data;

                if (g_list_find(self->rubberband_icons, icon) != NULL) continue;

                self->rubberband_icons = g_list_prepend(self->rubberband_icons, icon);

                if (gd_icon_get_selected(icon)) gd_icon_set_selected(icon, FALSE);
                else gd_icon_set_selected(icon, TRUE);
            }

            g_list_free(rubberband_icons);
        }
    }

    gtk_widget_queue_draw(GTK_WIDGET(self));
}

static void
view_foreach_cb(GtkWidget* widget, gpointer user_data)
{
    GDIconView* self;
    GList* l;

    self = GD_ICON_VIEW(user_data);

    for (l = self->icons; l != NULL; l = l->next) {
        GDIconInfo* info;

        info = (GDIconInfo*)l->data;

        if (info->icon == widget) {
            gtk_container_remove(GTK_CONTAINER(info->view), widget);
            info->view = NULL;
            break;
        }
    }
}

static void
size_changed_cb(GtkWidget* view, GDIconView* self)
{
    gtk_container_foreach(GTK_CONTAINER(view), view_foreach_cb, self);
    add_icons(self);
}

static void
append_home_icon(GDIconView* self)
{
    GError* error;
    GtkWidget* icon;

    if (self->home_info != NULL) return;

    error = NULL;
    icon = gd_home_icon_new(self, &error);

    if (error != NULL) {
        g_warning("%s", error->message);
        g_error_free(error);
        return;
    }

    self->home_info = create_icon_info(self, icon);
    self->icons = g_list_prepend(self->icons, self->home_info);
}

static void
append_trash_icon(GDIconView* self)
{
    GError* error;
    GtkWidget* icon;

    if (self->trash_info != NULL) return;

    error = NULL;
    icon = gd_trash_icon_new(self, &error);

    if (error != NULL) {
        g_warning("%s", error->message);
        g_error_free(error);
        return;
    }

    self->trash_info = create_icon_info(self, icon);
    self->icons = g_list_prepend(self->icons, self->trash_info);
}

static void
next_files_cb(GObject* object, GAsyncResult* res, gpointer user_data)
{
    GFileEnumerator* enumerator;
    GList* files;
    GError* error;
    GDIconView* self;
    GList* l;

    enumerator = G_FILE_ENUMERATOR(object);

    error = NULL;
    files = g_file_enumerator_next_files_finish(enumerator, res, &error);

    if (error != NULL) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            g_warning("%s", error->message);

        g_error_free(error);
        return;
    }

    self = GD_ICON_VIEW(user_data);

    for (l = files; l != NULL; l = l->next) {
        GFileInfo* info;
        GFile* file;
        GtkWidget* icon;
        GDIconInfo* icon_info;

        info = l->data;
        file = g_file_enumerator_get_child(enumerator, info);

        icon = gd_icon_new(self, file, info);
        g_object_unref(file);

        g_signal_connect(icon, "changed", G_CALLBACK (icon_changed_cb), self);

        icon_info = create_icon_info(self, icon);
        self->icons = g_list_prepend(self->icons, icon_info);
    }

    if (g_settings_get_boolean(self->settings, "show-home")) append_home_icon(self);

    if (g_settings_get_boolean(self->settings, "show-trash")) append_trash_icon(self);

    g_list_free_full(files, g_object_unref);

    if (self->placement == GD_PLACEMENT_AUTO_ARRANGE_ICONS) sort_icons(self);

    add_icons(self);
}

static void
enumerate_children_cb(GObject* object, GAsyncResult* res, gpointer user_data)
{
    GFileEnumerator* enumerator;
    GError* error;
    GDIconView* self;

    error = NULL;
    enumerator = g_file_enumerate_children_finish(G_FILE(object), res, &error);

    if (error != NULL) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            g_warning("%s", error->message);

        g_error_free(error);
        return;
    }

    self = GD_ICON_VIEW(user_data);

    g_file_enumerator_next_files_async(enumerator, G_MAXINT32, G_PRIORITY_LOW, self->cancellable, next_files_cb, user_data);

    g_object_unref(enumerator);
}

static void
enumerate_desktop(GDIconView* self)
{
    char* attributes;

    attributes = gd_icon_view_get_file_attributes(self);

    g_file_enumerate_children_async(self->desktop, attributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_LOW, self->cancellable, enumerate_children_cb, self);

    g_free(attributes);
}

static void
nautilus_ready_cb(GObject* object, GAsyncResult* res, gpointer user_data)

{
    GError* error;
    GDNautilus2Gen* nautilus;
    GDIconView* self;

    error = NULL;
    nautilus = gd_nautilus2_gen_proxy_new_for_bus_finish(res, &error);

    if (error != NULL) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            g_warning("%s", error->message);

        g_error_free(error);
        return;
    }

    self = GD_ICON_VIEW(user_data);
    self->nautilus = nautilus;
}

static void
file_manager_ready_cb(GObject* object, GAsyncResult* res, gpointer user_data)

{
    GError* error;
    GDFileManagerGen* file_manager;
    GDIconView* self;

    error = NULL;
    file_manager = gd_file_manager_gen_proxy_new_for_bus_finish(res, &error);

    if (error != NULL) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            g_warning("%s", error->message);

        g_error_free(error);
        return;
    }

    self = GD_ICON_VIEW(user_data);
    self->file_manager = file_manager;
}

static GtkWidget*
find_monitor_view_by_monitor(GDIconView* self, GdkMonitor* monitor)
{
    GDMonitorView* view;
    GList* children;
    GList* l;

    children = gtk_container_get_children(GTK_CONTAINER(self->fixed));

    for (l = children; l != NULL; l = l->next) {
        view = GD_MONITOR_VIEW(l->data);

        if (gd_monitor_view_get_monitor(view) == monitor) break;

        view = NULL;
    }

    g_list_free(children);

    return GTK_WIDGET(view);
}

static void
workarea_watcher_changed_cb(GDWorkareaWatcher* workarea_watcher, GDIconView* self)
{
    GList* children;
    GList* l;

    children = gtk_container_get_children(GTK_CONTAINER(self->fixed));

    for (l = children; l != NULL; l = l->next) {
        GDMonitorView* view;
        GdkMonitor* monitor;
        GdkRectangle workarea;

        view = GD_MONITOR_VIEW(l->data);

        monitor = gd_monitor_view_get_monitor(view);
        gdk_monitor_get_workarea(monitor, &workarea);

        gd_monitor_view_set_size(view, workarea.width, workarea.height);
        gtk_fixed_move(GTK_FIXED(self->fixed), l->data, workarea.x, workarea.y);
    }

    g_list_free(children);
}

static void
create_monitor_view(GDIconView* self, GdkMonitor* monitor)
{
    guint column_spacing;
    guint row_spacing;
    GdkRectangle workarea;
    GtkWidget* view;

    column_spacing = g_settings_get_uint(self->settings, "column-spacing");
    row_spacing = g_settings_get_uint(self->settings, "row-spacing");

    gdk_monitor_get_workarea(monitor, &workarea);

    view = gd_monitor_view_new(monitor, self, GD_DUMMY_ICON(self->dummy_icon), column_spacing, row_spacing);

    gd_monitor_view_set_placement(GD_MONITOR_VIEW(view), self->placement);

    gd_monitor_view_set_size(GD_MONITOR_VIEW(view), workarea.width, workarea.height);

    g_signal_connect(view, "size-changed", G_CALLBACK (size_changed_cb), self);

    gtk_fixed_put(GTK_FIXED(self->fixed), view, workarea.x, workarea.y);
    gtk_widget_show(view);

    g_settings_bind(self->settings, "column-spacing", view, "column-spacing", G_SETTINGS_BIND_GET);

    g_settings_bind(self->settings, "row-spacing", view, "row-spacing", G_SETTINGS_BIND_GET);
}

static void
monitor_added_cb(GdkDisplay* display, GdkMonitor* monitor, GDIconView* self)
{
    create_monitor_view(self, monitor);
}

static void
monitor_removed_cb(GdkDisplay* display, GdkMonitor* monitor, GDIconView* self)
{
    GtkWidget* view;
    GList* l;

    view = find_monitor_view_by_monitor(self, monitor);
    if (view == NULL) return;

    for (l = self->icons; l != NULL; l = l->next) {
        GDIconInfo* info;

        info = (GDIconInfo*)l->data;

        if (info->view != view) continue;

        gd_monitor_view_remove_icon(GD_MONITOR_VIEW(view), info->icon);
        info->view = NULL;
    }

    gtk_widget_destroy(view);
}

static GDPlacement
warn_if_placement_not_implemented(GDIconView* self, GDPlacement placement)
{
    if (!is_placement_implemented(placement)) {
        g_warning("Placement mode `%s` is not implemented!", placement_to_string (placement));

        placement = GD_PLACEMENT_AUTO_ARRANGE_ICONS;
        g_settings_set_enum(self->settings, "placement", placement);
    }

    return placement;
}

static void
placement_changed_cb(GSettings* settings, const char* key, GDIconView* self)
{
    GDPlacement placement;
    GList* monitor_views;
    GList* l;

    placement = g_settings_get_enum(settings, key);
    placement = warn_if_placement_not_implemented(self, placement);

    if (self->placement == placement) return;

    self->placement = placement;

    monitor_views = get_monitor_views(self);
    for (l = monitor_views; l != NULL; l = l->next) gd_monitor_view_set_placement(GD_MONITOR_VIEW(l->data), placement);
    g_list_free(monitor_views);

    if (placement == GD_PLACEMENT_AUTO_ARRANGE_ICONS) resort_icons(self, FALSE);
    else if (placement == GD_PLACEMENT_ALIGN_ICONS_TO_GRID) remove_and_readd_icons(self);
}

static void
sort_by_changed_cb(GSettings* settings, const char* key, GDIconView* self)
{
    GDSortBy sort_by;

    sort_by = g_settings_get_enum(settings, key);

    if (self->sort_by == sort_by) return;

    self->sort_by = sort_by;

    if (self->placement == GD_PLACEMENT_AUTO_ARRANGE_ICONS) resort_icons(self, FALSE);
}

static void
show_home_changed_cb(GSettings* settings, const char* key, GDIconView* self)
{
    if (g_settings_get_boolean(self->settings, key)) {
        append_home_icon(self);

        if (self->placement == GD_PLACEMENT_AUTO_ARRANGE_ICONS) resort_icons(self, TRUE);
    }
    else if (self->home_info != NULL) {
        remove_icon_from_view(self, self->home_info);

        self->icons = g_list_remove(self->icons, self->home_info);
        gd_icon_info_free(self->home_info);
        self->home_info = NULL;

        if (self->placement == GD_PLACEMENT_AUTO_ARRANGE_ICONS) remove_and_readd_icons(self);
    }
}

static void
show_trash_changed_cb(GSettings* settings, const char* key, GDIconView* self)
{
    if (g_settings_get_boolean(self->settings, key)) {
        append_trash_icon(self);

        if (self->placement == GD_PLACEMENT_AUTO_ARRANGE_ICONS) resort_icons(self, TRUE);
    }
    else if (self->trash_info != NULL) {
        remove_icon_from_view(self, self->trash_info);

        self->icons = g_list_remove(self->icons, self->trash_info);
        gd_icon_info_free(self->trash_info);
        self->trash_info = NULL;

        if (self->placement == GD_PLACEMENT_AUTO_ARRANGE_ICONS) remove_and_readd_icons(self);
    }
}

static char**
get_selected_uris(GDIconView* self)
{
    int n_uris;
    char** uris;
    GList* l;
    int i;

    if (self->selected_icons == NULL) return NULL;

    n_uris = g_list_length(self->selected_icons);
    uris = g_new0(char *, n_uris + 1);

    for (l = self->selected_icons, i = 0; l != NULL; l = l->next) {
        GFile* file;

        file = gd_icon_get_file(GD_ICON(l->data));
        uris[i++] = g_file_get_uri(file);
    }

    return uris;
}

static void
select_cb(gpointer data, gpointer user_data)
{
    GDIconInfo* info;

    info = data;

    gd_icon_set_selected(GD_ICON(info->icon), TRUE);
}

static void
select_all_cb(GDIconView* self, gpointer user_data)
{
    g_list_foreach(self->icons, select_cb, NULL);
}

static void
unselect_all_cb(GDIconView* self, gpointer user_data)
{
    unselect_icons(self);
}

static void
activate_cb(GDIconView* self, gpointer user_data)
{
    GList* l;

    if (self->selected_icons == NULL) return;

    for (l = self->selected_icons; l != NULL; l = l->next) gd_icon_open(GD_ICON(l->data));
}

static void
rename_cb(GDIconView* self, gpointer user_data)
{
    if (self->selected_icons == NULL || g_list_length(self->selected_icons) != 1) return;

    gd_icon_rename(GD_ICON(self->selected_icons->data));
}

static void
trash_cb(GDIconView* self, gpointer user_data)
{
    bool can_delete;
    GList* l;
    char** uris;

    if (self->selected_icons == NULL) return;

    can_delete = TRUE;
    for (l = self->selected_icons; l != NULL; l = l->next) {
        if (!GD_ICON_GET_CLASS(l->data)->can_delete(GD_ICON(l->data))) {
            can_delete = FALSE;
            break;
        }
    }

    if (!can_delete) return;

    uris = get_selected_uris(self);
    if (uris == NULL) return;

    gd_icon_view_move_to_trash(self, (const char* const *)uris, gtk_get_current_event_time());
    g_strfreev(uris);
}

static void
delete_cb(GDIconView* self, gpointer user_data)
{
    bool can_delete;
    GList* l;
    char** uris;

    if (self->selected_icons == NULL) return;

    can_delete = TRUE;
    for (l = self->selected_icons; l != NULL; l = l->next) {
        if (!GD_ICON_GET_CLASS(l->data)->can_delete(GD_ICON(l->data))) {
            can_delete = FALSE;
            break;
        }
    }

    if (!can_delete) return;

    uris = get_selected_uris(self);
    if (uris == NULL) return;

    gd_icon_view_delete(self, (const char* const *)uris, gtk_get_current_event_time());

    g_strfreev(uris);
}

static void
toggle_cb(GDIconView* self, gpointer user_data)
{
    if (self->last_selected_icon == NULL) return;

    if (gd_icon_get_selected(self->last_selected_icon)) gd_icon_set_selected(self->last_selected_icon, FALSE);
    else gd_icon_set_selected(self->last_selected_icon, TRUE);
}

static GDMonitorView*
find_monitor_view_up(GDIconView* self, GList* views, GdkRectangle* current)
{
    GList* l;

    for (l = views; l != NULL; l = l->next) {
        GDMonitorView* view;
        GdkMonitor* monitor;
        GdkRectangle geometry;

        view = GD_MONITOR_VIEW(l->data);
        monitor = gd_monitor_view_get_monitor(view);

        gdk_monitor_get_geometry(monitor, &geometry);

        if (current->y == geometry.y + geometry.height) return view;
    }

    return NULL;
}

static GDMonitorView*
find_monitor_view_down(GDIconView* self, GList* views, GdkRectangle* current)
{
    GList* l;

    for (l = views; l != NULL; l = l->next) {
        GDMonitorView* view;
        GdkMonitor* monitor;
        GdkRectangle geometry;

        view = GD_MONITOR_VIEW(l->data);
        monitor = gd_monitor_view_get_monitor(view);

        gdk_monitor_get_geometry(monitor, &geometry);

        if (current->y + current->height == geometry.y) return view;
    }

    return NULL;
}

static GDMonitorView*
find_monitor_view_left(GDIconView* self, GList* views, GdkRectangle* current)
{
    GList* l;

    for (l = views; l != NULL; l = l->next) {
        GDMonitorView* view;
        GdkMonitor* monitor;
        GdkRectangle geometry;

        view = GD_MONITOR_VIEW(l->data);
        monitor = gd_monitor_view_get_monitor(view);

        gdk_monitor_get_geometry(monitor, &geometry);

        if (current->x == geometry.x + geometry.width) return view;
    }

    return NULL;
}

static GDMonitorView*
find_monitor_view_right(GDIconView* self, GList* views, GdkRectangle* current)
{
    GList* l;

    for (l = views; l != NULL; l = l->next) {
        GDMonitorView* view;
        GdkMonitor* monitor;
        GdkRectangle geometry;

        view = GD_MONITOR_VIEW(l->data);
        monitor = gd_monitor_view_get_monitor(view);

        gdk_monitor_get_geometry(monitor, &geometry);

        if (current->x + current->width == geometry.x) return view;
    }

    return NULL;
}

static GDMonitorView*
find_next_view(GDIconView* self, GDMonitorView* next_to, GtkDirectionType direction)
{
    GList* views;
    GList* children;
    GList* l;
    GdkMonitor* monitor;
    GdkRectangle geometry;
    GDMonitorView* next_view;

    views = NULL;

    children = gtk_container_get_children(GTK_CONTAINER(self->fixed));
    for (l = children; l != NULL; l = l->next) {
        GDMonitorView* view;

        view = GD_MONITOR_VIEW(l->data);
        if (view == next_to) continue;

        views = g_list_prepend(views, view);
    }

    g_list_free(children);

    if (views == NULL) return NULL;

    monitor = gd_monitor_view_get_monitor(next_to);
    gdk_monitor_get_geometry(monitor, &geometry);

    next_view = NULL;

    switch (direction) {
    case GTK_DIR_UP:
        next_view = find_monitor_view_up(self, views, &geometry);
        break;

    case GTK_DIR_DOWN:
        next_view = find_monitor_view_down(self, views, &geometry);
        break;

    case GTK_DIR_LEFT:
        next_view = find_monitor_view_left(self, views, &geometry);
        break;

    case GTK_DIR_RIGHT:
        next_view = find_monitor_view_right(self, views, &geometry);
        break;

    case GTK_DIR_TAB_FORWARD:
    case GTK_DIR_TAB_BACKWARD: default:
        break;
    }

    g_list_free(views);

    return next_view;
}

static void
move_cb(GDIconView* self, GtkDirectionType direction, gpointer user_data)
{
    GdkModifierType state;
    bool extend;
    bool modify;
    GDMonitorView* view;
    GDIcon* next_icon;

    extend = FALSE;
    modify = FALSE;
    view = NULL;

    if (self->icons == NULL) return;

    if (gtk_get_current_event_state(&state)) {
        GdkModifierType extend_mod_mask;
        GdkModifierType modify_mod_mask;

        extend_mod_mask = gtk_widget_get_modifier_mask(GTK_WIDGET(self), GDK_MODIFIER_INTENT_EXTEND_SELECTION);
        modify_mod_mask = gtk_widget_get_modifier_mask(GTK_WIDGET(self), GDK_MODIFIER_INTENT_MODIFY_SELECTION);

        if ((state & extend_mod_mask) == extend_mod_mask) extend = TRUE;

        if ((state & modify_mod_mask) == modify_mod_mask) modify = TRUE;
    }

    if (self->last_selected_icon != NULL) {
        GList* l;

        for (l = self->icons; l != NULL; l = l->next) {
            GDIconInfo* info;

            info = (GDIconInfo*)l->data;

            if (GD_ICON(info->icon) == self->last_selected_icon) {
                view = GD_MONITOR_VIEW(info->view);
                break;
            }
        }
    }
    else {
        GList* children;
        GList* l;

        children = gtk_container_get_children(GTK_CONTAINER(self->fixed));

        for (l = children; l != NULL; l = l->next) {
            GDMonitorView* tmp;

            tmp = GD_MONITOR_VIEW(l->data);

            if (gd_monitor_view_is_primary(tmp)) {
                view = tmp;
                break;
            }
        }

        g_list_free(children);
    }

    g_assert(view != NULL);

    do {
        next_icon = gd_monitor_view_find_next_icon(view, self->last_selected_icon, direction);

        if (next_icon == NULL) view = find_next_view(self, view, direction);
    } while (next_icon == NULL && view != NULL);

    if (next_icon == NULL) {
        gtk_widget_error_bell(GTK_WIDGET(self));
        return;
    }

    if (!modify) unselect_icons(self);

    if (extend) {
        GdkRectangle rect1;
        GdkRectangle rect2;
        GdkRectangle area;
        GList* children;
        GList* l;

        if (self->extend_from_icon == NULL) self->extend_from_icon = self->last_selected_icon;

        gtk_widget_get_allocation(GTK_WIDGET(self->extend_from_icon), &rect1);
        gtk_widget_get_allocation(GTK_WIDGET(next_icon), &rect2);

        gdk_rectangle_union(&rect1, &rect2, &area);

        children = gtk_container_get_children(GTK_CONTAINER(self->fixed));

        for (l = children; l != NULL; l = l->next) gd_monitor_view_select_icons(GD_MONITOR_VIEW(l->data), &area);

        g_list_free(children);
    }

    if (!extend && !modify) {
        self->extend_from_icon = NULL;
        gd_icon_set_selected(next_icon, TRUE);
    }

    gtk_widget_grab_focus(GTK_WIDGET(next_icon));
}

static GtkWidget*
create_dummy_icon(GDIconView* self)
{
    GtkWidget* widget;

    widget = gd_dummy_icon_new(self);

    g_settings_bind(self->settings, "icon-size", widget, "icon-size", G_SETTINGS_BIND_GET);

    g_settings_bind(self->settings, "extra-text-width", widget, "extra-text-width", G_SETTINGS_BIND_GET);

    return widget;
}

static void
gd_icon_view_dispose(GObject* object)
{
    GDIconView* self;

    self = GD_ICON_VIEW(object);

    g_clear_object(&self->workarea_watcher);

    g_clear_object(&self->thumbnail_factory);

    g_clear_object(&self->multi_press);
    g_clear_object(&self->drag);

    g_clear_object(&self->desktop);
    g_clear_object(&self->monitor);
    g_clear_object(&self->settings);

    g_cancellable_cancel(self->cancellable);
    g_clear_object(&self->cancellable);

    if (self->icons != NULL) {
        g_list_free_full(self->icons, gd_icon_info_free);
        self->icons = NULL;
    }

    g_clear_pointer(&self->selected_icons, g_list_free);

    g_clear_object(&self->rubberband_css);
    g_clear_object(&self->rubberband_style);
    g_clear_pointer(&self->rubberband_icons, g_list_free);

    g_clear_object(&self->nautilus);
    g_clear_object(&self->file_manager);

    g_clear_pointer(&self->create_folder_dialog, gtk_widget_destroy);

    g_clear_pointer(&self->drag_rectagles, g_ptr_array_unref);

    G_OBJECT_CLASS(gd_icon_view_parent_class)->dispose(object);
}

static void
gd_icon_view_finalize(GObject* object)
{
    GDIconView* self;

    self = GD_ICON_VIEW(object);

    g_clear_pointer(&self->dummy_icon, gtk_widget_unparent);

    G_OBJECT_CLASS(gd_icon_view_parent_class)->finalize(object);
}

static void
ensure_rubberband_style(GDIconView* self)
{
    GtkWidgetPath* path;
    GdkScreen* screen;
    GtkStyleProvider* provider;
    guint priority;

    if (self->rubberband_style != NULL) return;

    self->rubberband_css = gtk_css_provider_new();
    self->rubberband_style = gtk_style_context_new();

    path = gtk_widget_path_new();

    gtk_widget_path_append_type(path, G_TYPE_NONE);
    gtk_widget_path_iter_set_object_name(path, -1, "rubberband");
    gtk_widget_path_iter_add_class(path, -1, GTK_STYLE_CLASS_RUBBERBAND);

    gtk_style_context_set_path(self->rubberband_style, path);
    gtk_widget_path_unref(path);

    screen = gtk_widget_get_screen(GTK_WIDGET(self));
    provider = GTK_STYLE_PROVIDER(self->rubberband_css);
    priority = GTK_STYLE_PROVIDER_PRIORITY_APPLICATION;

    gtk_style_context_add_provider_for_screen(screen, provider, priority);
}

static bool
gd_icon_view_draw(GtkWidget* widget, cairo_t* cr)
{
    GDIconView* self;

    self = GD_ICON_VIEW(widget);

    GTK_WIDGET_CLASS(gd_icon_view_parent_class)->draw(widget, cr);

    if (self->drag_rectagles != NULL && self->drag_rectagles->len > 1) {
        GdkDisplay* display;
        GdkSeat* seat;
        int x;
        int y;
        guint i;

        display = gtk_widget_get_display(widget);
        seat = gdk_display_get_default_seat(display);

        gdk_window_get_device_position(gtk_widget_get_window(widget), gdk_seat_get_pointer(seat), &x, &y, NULL);

        ensure_rubberband_style(self);

        cairo_save(cr);

        for (i = 0; i < self->drag_rectagles->len; i++) {
            GdkRectangle* rect;

            rect = g_ptr_array_index(self->drag_rectagles, i);

            gtk_render_background(self->rubberband_style, cr, x + rect->x, y + rect->y, rect->width, rect->height);

            gtk_render_frame(self->rubberband_style, cr, x + rect->x, y + rect->y, rect->width, rect->height);
        }

        cairo_restore(cr);
    }
    else if (gtk_gesture_is_recognized(GTK_GESTURE(self->drag))) {
        ensure_rubberband_style(self);

        cairo_save(cr);

        gtk_render_background(self->rubberband_style, cr, self->rubberband_rect.x, self->rubberband_rect.y, self->rubberband_rect.width, self->rubberband_rect.height);

        gtk_render_frame(self->rubberband_style, cr, self->rubberband_rect.x, self->rubberband_rect.y, self->rubberband_rect.width, self->rubberband_rect.height);

        cairo_restore(cr);
    }

    return TRUE;
}

static bool
gd_icon_view_popup_menu(GtkWidget* widget)
{
    GDIconView* self;

    self = GD_ICON_VIEW(widget);

    if (self->selected_icons == NULL) {
        GtkWidget* popup_menu;

        popup_menu = create_popup_menu(self);
        g_object_ref_sink(popup_menu);

        gtk_menu_popup_at_pointer(GTK_MENU(popup_menu), NULL);
        g_object_unref(popup_menu);

        return TRUE;
    }
    else {
        gd_icon_popup_menu(GD_ICON(self->selected_icons->data));
    }

    return FALSE;
}

static void
gd_icon_view_forall(GtkContainer* container, bool include_internals, GtkCallback callback, gpointer callback_data)
{
    GDIconView* self;

    self = GD_ICON_VIEW(container);

    GTK_CONTAINER_CLASS(gd_icon_view_parent_class)->forall(container, include_internals, callback, callback_data);

    if (include_internals) (*callback)(self->dummy_icon, callback_data);
}

static void
install_signals(void)
{
    view_signals[SELECT_ALL] = g_signal_new("select-all", GD_TYPE_ICON_VIEW, G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);

    view_signals[UNSELECT_ALL] = g_signal_new("unselect-all", GD_TYPE_ICON_VIEW, G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);

    view_signals[ACTIVATE] = g_signal_new("activate", GD_TYPE_ICON_VIEW, G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);

    view_signals[RENAME] = g_signal_new("rename", GD_TYPE_ICON_VIEW, G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);

    view_signals[TRASH] = g_signal_new("trash", GD_TYPE_ICON_VIEW, G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);

    view_signals[DELETE] = g_signal_new("delete", GD_TYPE_ICON_VIEW, G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);

    view_signals[TOGGLE] = g_signal_new("toggle", GD_TYPE_ICON_VIEW, G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);

    view_signals[MOVE] = g_signal_new("move", GD_TYPE_ICON_VIEW, G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, GTK_TYPE_DIRECTION_TYPE);
}

static void
add_move_binding(GtkBindingSet* binding_set, guint keyval, GdkModifierType modifiers, GtkDirectionType direction)
{
    gtk_binding_entry_add_signal(binding_set, keyval, modifiers, "move", 1, GTK_TYPE_DIRECTION_TYPE, direction);

    gtk_binding_entry_add_signal(binding_set, keyval, GDK_SHIFT_MASK, "move", 1, GTK_TYPE_DIRECTION_TYPE, direction);

    gtk_binding_entry_add_signal(binding_set, keyval, GDK_CONTROL_MASK, "move", 1, GTK_TYPE_DIRECTION_TYPE, direction);
}

static void
add_bindings(GtkBindingSet* binding_set)
{
    GdkModifierType modifiers;

    /* Select all */

    modifiers = GDK_CONTROL_MASK;
    gtk_binding_entry_add_signal(binding_set, GDK_KEY_a, modifiers, "select-all", 0);

    /* Unselect all */

    modifiers = GDK_CONTROL_MASK | GDK_SHIFT_MASK;
    gtk_binding_entry_add_signal(binding_set, GDK_KEY_a, modifiers, "unselect-all", 0);

    /* Activate */

    modifiers = 0;
    gtk_binding_entry_add_signal(binding_set, GDK_KEY_Return, modifiers, "activate", 0);

    modifiers = 0;
    gtk_binding_entry_add_signal(binding_set, GDK_KEY_ISO_Enter, modifiers, "activate", 0);

    modifiers = 0;
    gtk_binding_entry_add_signal(binding_set, GDK_KEY_KP_Enter, modifiers, "activate", 0);

    /* Rename */

    modifiers = 0;
    gtk_binding_entry_add_signal(binding_set, GDK_KEY_F2, modifiers, "rename", 0);

    /* Trash */

    modifiers = 0;
    gtk_binding_entry_add_signal(binding_set, GDK_KEY_Delete, modifiers, "trash", 0);

    modifiers = 0;
    gtk_binding_entry_add_signal(binding_set, GDK_KEY_KP_Delete, modifiers, "trash", 0);

    /* Delete */

    modifiers = GDK_SHIFT_MASK;
    gtk_binding_entry_add_signal(binding_set, GDK_KEY_Delete, modifiers, "delete", 0);

    modifiers = GDK_SHIFT_MASK;
    gtk_binding_entry_add_signal(binding_set, GDK_KEY_KP_Delete, modifiers, "delete", 0);

    /* Toggle */

    modifiers = GDK_CONTROL_MASK;
    gtk_binding_entry_add_signal(binding_set, GDK_KEY_space, modifiers, "toggle", 0);

    modifiers = GDK_CONTROL_MASK;
    gtk_binding_entry_add_signal(binding_set, GDK_KEY_KP_Space, modifiers, "toggle", 0);

    /* Move */

    modifiers = 0;
    add_move_binding(binding_set, GDK_KEY_Up, modifiers, GTK_DIR_UP);

    modifiers = 0;
    add_move_binding(binding_set, GDK_KEY_KP_Up, modifiers, GTK_DIR_UP);

    modifiers = 0;
    add_move_binding(binding_set, GDK_KEY_Down, modifiers, GTK_DIR_DOWN);

    modifiers = 0;
    add_move_binding(binding_set, GDK_KEY_KP_Down, modifiers, GTK_DIR_DOWN);

    modifiers = 0;
    add_move_binding(binding_set, GDK_KEY_Left, modifiers, GTK_DIR_LEFT);

    modifiers = 0;
    add_move_binding(binding_set, GDK_KEY_KP_Left, modifiers, GTK_DIR_LEFT);

    modifiers = 0;
    add_move_binding(binding_set, GDK_KEY_Right, modifiers, GTK_DIR_RIGHT);

    modifiers = 0;
    add_move_binding(binding_set, GDK_KEY_KP_Right, modifiers, GTK_DIR_RIGHT);
}

static void
gd_icon_view_class_init(GDIconViewClass* self_class)
{
    GObjectClass* object_class;
    GtkWidgetClass* widget_class;
    GtkContainerClass* container_class;
    GtkBindingSet* binding_set;

    object_class = G_OBJECT_CLASS(self_class);
    widget_class = GTK_WIDGET_CLASS(self_class);
    container_class = GTK_CONTAINER_CLASS(self_class);

    object_class->dispose = gd_icon_view_dispose;
    object_class->finalize = gd_icon_view_finalize;

    widget_class->draw = (void*) gd_icon_view_draw;
    widget_class->popup_menu = (void*) gd_icon_view_popup_menu;

    container_class->forall = (void*) gd_icon_view_forall;

    install_signals();

    binding_set = gtk_binding_set_by_class(widget_class);
    add_bindings(binding_set);
}

static void
gd_icon_view_init(GDIconView* self)
{
    const char* desktop_dir;
    GError* error;
    GdkDisplay* display;
    int n_monitors;
    int i;

    g_signal_connect(self, "select-all", G_CALLBACK (select_all_cb), NULL);
    g_signal_connect(self, "unselect-all", G_CALLBACK (unselect_all_cb), NULL);
    g_signal_connect(self, "activate", G_CALLBACK (activate_cb), NULL);
    g_signal_connect(self, "rename", G_CALLBACK (rename_cb), NULL);
    g_signal_connect(self, "trash", G_CALLBACK (trash_cb), NULL);
    g_signal_connect(self, "delete", G_CALLBACK (delete_cb), NULL);
    g_signal_connect(self, "toggle", G_CALLBACK (toggle_cb), NULL);
    g_signal_connect(self, "move", G_CALLBACK (move_cb), NULL);

    self->workarea_watcher = gd_workarea_watcher_new();

    g_signal_connect(self->workarea_watcher, "changed", G_CALLBACK (workarea_watcher_changed_cb), self);

    self->thumbnail_factory = gd_thumbnail_factory_new();

    self->multi_press = gtk_gesture_multi_press_new(GTK_WIDGET(self));
    self->drag = gtk_gesture_drag_new(GTK_WIDGET(self));

    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(self->multi_press), 0);

    g_signal_connect(self->multi_press, "pressed", G_CALLBACK (multi_press_pressed_cb), self);

    g_signal_connect(self->drag, "drag-begin", G_CALLBACK (drag_begin_cb), self);

    g_signal_connect(self->drag, "drag-end", G_CALLBACK (drag_end_cb), self);

    g_signal_connect(self->drag, "drag-update", G_CALLBACK (drag_update_cb), self);

    desktop_dir = g_get_user_special_dir(G_USER_DIRECTORY_DESKTOP);
    self->desktop = g_file_new_for_path(desktop_dir);

    error = NULL;
    self->monitor = g_file_monitor_directory(self->desktop, G_FILE_MONITOR_WATCH_MOVES, NULL, &error);

    if (error != NULL) {
        g_warning("%s", error->message);
        g_error_free(error);
        return;
    }

    g_signal_connect(self->monitor, "changed", G_CALLBACK (desktop_changed_cb), self);

    self->settings = g_settings_new("org.gnome.gnome-flashback.desktop.icons");

    g_signal_connect(self->settings, "changed::placement", G_CALLBACK (placement_changed_cb), self);

    g_signal_connect(self->settings, "changed::sort-by", G_CALLBACK (sort_by_changed_cb), self);

    g_signal_connect(self->settings, "changed::show-home", G_CALLBACK (show_home_changed_cb), self);

    g_signal_connect(self->settings, "changed::show-trash", G_CALLBACK (show_trash_changed_cb), self);

    self->placement = g_settings_get_enum(self->settings, "placement");
    self->placement = warn_if_placement_not_implemented(self, self->placement);
    self->sort_by = g_settings_get_enum(self->settings, "sort-by");

    self->fixed = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(self), self->fixed);
    gtk_widget_show(self->fixed);

    self->dummy_icon = create_dummy_icon(self);
    gtk_widget_set_parent(self->dummy_icon, GTK_WIDGET(self));
    gtk_widget_show(self->dummy_icon);

    self->cancellable = g_cancellable_new();

    gd_nautilus2_gen_proxy_new_for_bus(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION, "org.gnome.Nautilus", "/org/gnome/Nautilus/FileOperations2", self->cancellable, nautilus_ready_cb, self);

    gd_file_manager_gen_proxy_new_for_bus(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION, "org.freedesktop.FileManager1", "/org/freedesktop/FileManager1", self->cancellable, file_manager_ready_cb, self);

    display = gdk_display_get_default();
    n_monitors = gdk_display_get_n_monitors(display);

    g_signal_connect_object(display, "monitor-added", G_CALLBACK(monitor_added_cb), self, 0);

    g_signal_connect_object(display, "monitor-removed", G_CALLBACK(monitor_removed_cb), self, 0);

    for (i = 0; i < n_monitors; i++) {
        GdkMonitor* monitor;

        monitor = gdk_display_get_monitor(display, i);
        create_monitor_view(self, monitor);
    }

    enumerate_desktop(self);
}

GtkWidget*
gd_icon_view_new(void)
{
    return g_object_new(GD_TYPE_ICON_VIEW, "can-focus", TRUE, NULL);
}

GDThumbnailFactory*
gd_icon_view_get_thumbnail_factory(GDIconView* self)
{
    return self->thumbnail_factory;
}

char*
gd_icon_view_get_file_attributes(GDIconView* self)
{
    return build_attributes_list(G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME, G_FILE_ATTRIBUTE_STANDARD_ICON, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN,
                                 G_FILE_ATTRIBUTE_STANDARD_IS_BACKUP, G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_ATTRIBUTE_TIME_MODIFIED, NULL);
}

char*
gd_icon_view_get_desktop_uri(GDIconView* self)
{
    return g_file_get_uri(self->desktop);
}

void
gd_icon_view_set_representative_color(GDIconView* self, GdkRGBA* color)
{
    ensure_rubberband_style(self);

    if (color != NULL) {
        double shade;
        GdkRGBA background;
        GdkRGBA border;
        char* background_css;
        char* border_css;
        char* css;

        shade = color->green < 0.5 ? 1.1 : 0.9;

        background = *color;
        background.alpha = 0.6;

        border.red = color->red * shade;
        border.green = color->green * shade;
        border.blue = color->green * shade;
        border.alpha = 1.0;

        background_css = gdk_rgba_to_string(&background);
        border_css = gdk_rgba_to_string(&border);

        css = g_strdup_printf(".rubberband { background-color: %s; border: 1px solid %s; }", background_css, border_css);

        g_free(background_css);
        g_free(border_css);

        gtk_css_provider_load_from_data(self->rubberband_css, css, -1, NULL);
        g_free(css);
    }
    else {
        gtk_css_provider_load_from_data(self->rubberband_css, "", -1, NULL);
    }
}

void
gd_icon_view_set_drag_rectangles(GDIconView* self, GPtrArray* rectangles)
{
    g_clear_pointer(&self->drag_rectagles, g_ptr_array_unref);

    if (rectangles != NULL) self->drag_rectagles = g_ptr_array_ref(rectangles);

    gtk_widget_queue_draw(GTK_WIDGET(self));
}

void
gd_icon_view_clear_selection(GDIconView* self)
{
    unselect_icons(self);
}

GList*
gd_icon_view_get_selected_icons(GDIconView* self)
{
    return self->selected_icons;
}

void
gd_icon_view_show_item_properties(GDIconView* self, const char* const * uris)
{
    if (self->file_manager == NULL) return;

    gd_file_manager_gen_call_show_item_properties(self->file_manager, uris, "", self->cancellable, show_item_properties_cb, NULL);
}

void
gd_icon_view_empty_trash(GDIconView* self, guint32 timestamp)
{
    if (self->nautilus == NULL) return;

    gd_nautilus2_gen_call_empty_trash(self->nautilus, TRUE, get_platform_data(self, timestamp), self->cancellable, empty_trash_cb, NULL);
}

bool
gd_icon_view_validate_new_name(GDIconView* self, GFileType file_type, const char* new_name, char** message)
{
    bool is_dir;
    char* text;
    bool valid;
    GList* l;

    g_assert(message != NULL && *message == NULL);

    is_dir = file_type == G_FILE_TYPE_DIRECTORY;
    text = g_strstrip(g_strdup (new_name));
    valid = TRUE;

    if (*text == '\0') {
        valid = FALSE;
    }
    else if (g_strstr_len(text, -1, "/") != NULL) {
        if (is_dir) *message = g_strdup(_("Folder names cannot contain “/”."));
        else *message = g_strdup(_("File names cannot contain “/”."));

        valid = FALSE;
    }
    else if (g_strcmp0(text, ".") == 0) {
        if (is_dir) *message = g_strdup(_("A folder cannot be called “.”."));
        else *message = g_strdup(_("A file cannot be called “.”."));

        valid = FALSE;
    }
    else if (g_strcmp0(text, "..") == 0) {
        if (is_dir) *message = g_strdup(_("A folder cannot be called “..”."));
        else *message = g_strdup(_("A file cannot be called “..”."));

        valid = FALSE;
    }

    for (l = self->icons; l != NULL; l = l->next) {
        GDIconInfo* info;
        const char* name;

        info = l->data;

        name = gd_icon_get_name(GD_ICON(info->icon));
        if (g_strcmp0(name, text) == 0) {
            if (is_dir) *message = g_strdup(_("A folder with that name already exists."));
            else *message = g_strdup(_("A file with that name already exists."));

            valid = FALSE;
            break;
        }
    }

    if (*message == NULL && g_str_has_prefix(text, ".")) {
        if (is_dir) *message = g_strdup(_("Folders with “.” at the beginning of their name are hidden."));
        else *message = g_strdup(_("Files with “.” at the beginning of their name are hidden."));
    }

    g_free(text);

    return valid;
}

void
gd_icon_view_move_to_trash(GDIconView* self, const char* const * uris, guint32 timestamp)
{
    if (self->nautilus == NULL) return;

    gd_nautilus2_gen_call_trash_uris(self->nautilus, uris, get_platform_data(self, timestamp), self->cancellable, trash_uris_cb, NULL);
}

void
gd_icon_view_delete(GDIconView* self, const char* const * uris, guint32 timestamp)
{
    if (self->nautilus == NULL) return;

    gd_nautilus2_gen_call_delete_uris(self->nautilus, uris, get_platform_data(self, timestamp), self->cancellable, delete_uris_cb, NULL);
}

void
gd_icon_view_rename_file(GDIconView* self, const char* uri, const char* new_name, guint32 timestamp)
{
    if (self->nautilus == NULL) return;

    gd_nautilus2_gen_call_rename_uri(self->nautilus, uri, new_name, get_platform_data(self, timestamp), self->cancellable, rename_uri_cb, NULL);
}

void
gd_icon_view_copy_uris(GDIconView* self, const char* const * uris, const char* destination, guint32 timestamp)
{
    if (self->nautilus == NULL) return;

    gd_nautilus2_gen_call_copy_uris(self->nautilus, uris, destination, get_platform_data(self, timestamp), self->cancellable, copy_uris_cb, NULL);
}

void
gd_icon_view_move_uris(GDIconView* self, const char* const * uris, const char* destination, guint32 timestamp)
{
    if (self->nautilus == NULL) return;

    gd_nautilus2_gen_call_move_uris(self->nautilus, uris, destination, get_platform_data(self, timestamp), self->cancellable, move_uris_cb, NULL);
}
