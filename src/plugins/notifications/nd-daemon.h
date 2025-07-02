//
// Created by dingjing on 25-7-2.
//

#ifndef graceful_DE2_ND_DAEMON_H
#define graceful_DE2_ND_DAEMON_H
#include "macros/macros.h"

#include <glib-object.h>

C_BEGIN_EXTERN_C

#define ND_TYPE_DAEMON nd_daemon_get_type ()
G_DECLARE_FINAL_TYPE (NdDaemon, nd_daemon, ND, DAEMON, GObject)

NdDaemon *nd_daemon_new (void);

C_END_EXTERN_C

#endif // graceful_DE2_ND_DAEMON_H
