//
// Created by dingjing on 25-6-27.
//

#ifndef graceful_DE2_GD_APPLICATION_H
#define graceful_DE2_GD_APPLICATION_H
#include "macros/macros.h"

#include <glib-object.h>


C_BEGIN_EXTERN_C

#define GD_TYPE_APPLICATION         gd_application_get_type()
G_DECLARE_FINAL_TYPE(GDApplication, gd_application, GD, APPLICATION, GObject)


GDApplication*  gd_application_new (void);


C_END_EXTERN_C

#endif // graceful_DE2_GD_APPLICATION_H
