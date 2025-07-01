//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_UTILS_H
#define graceful_DE2_GD_UTILS_H
#include <glib.h>
#include <gio/gdesktopappinfo.h>

#include "macros/macros.h"

C_BEGIN_EXTERN_C

bool    gd_launch_app_info     (GDesktopAppInfo* appInfo, GError** error);
bool    gd_launch_desktop_file (const char* desktopFile, GError** error);
bool    gd_launch_uri          (const char* uri, GError** error);
double  gd_get_nautilus_scale  (void);

C_END_EXTERN_C

#endif // graceful_DE2_GD_UTILS_H
