//
// Created by dingjing on 25-6-27.
//

#ifndef graceful_DE2_GD_SESSION_H
#define graceful_DE2_GD_SESSION_H
#include "macros/macros.h"
#include <glib-object.h>

#define GD_TYPE_SESSION         (gd_session_get_type())
G_DECLARE_FINAL_TYPE(GDSession, gd_session, GD, SESSION, GObject)


C_BEGIN_EXTERN_C

GDSession*      gd_session_new              (const char* startupId);
void            gd_session_set_environment  (GDSession* self, const char* name, const char* value);
void            gd_session_register         (GDSession* self);

C_END_EXTERN_C

#endif // graceful_DE2_GD_SESSION_H
