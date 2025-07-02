//
// Created by dingjing on 25-7-2.
//

#ifndef graceful_DE2_ND_STACK_H
#define graceful_DE2_ND_STACK_H
#include "macros/macros.h"

#include <gtk/gtk.h>
#include "gd-bubble.h"

C_BEGIN_EXTERN_C

#define ND_TYPE_STACK         (nd_stack_get_type ())
#define ND_STACK(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ND_TYPE_STACK, NdStack))
#define ND_STACK_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), ND_TYPE_STACK, NdStackClass))
#define ND_IS_STACK(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ND_TYPE_STACK))
#define ND_IS_STACK_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ND_TYPE_STACK))
#define ND_STACK_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ND_TYPE_STACK, NdStackClass))

typedef struct NdStackPrivate NdStackPrivate;

typedef struct
{
    GObject         parent;
    NdStackPrivate* priv;
} NdStack;

typedef struct
{
    GObjectClass   parentClass;
} NdStackClass;

typedef enum
{
    ND_STACK_LOCATION_UNKNOWN = -1,
    ND_STACK_LOCATION_TOP_LEFT,
    ND_STACK_LOCATION_TOP_RIGHT,
    ND_STACK_LOCATION_BOTTOM_LEFT,
    ND_STACK_LOCATION_BOTTOM_RIGHT,
    ND_STACK_LOCATION_DEFAULT = ND_STACK_LOCATION_TOP_RIGHT
} NdStackLocation;

GType           nd_stack_get_type              (void);
NdStack*        nd_stack_new                   (GdkMonitor* monitor);
void            nd_stack_set_location          (NdStack* stack, NdStackLocation location);
void            nd_stack_add_bubble            (NdStack* stack, GDBubble* bubble);
void            nd_stack_remove_bubble         (NdStack* stack, GDBubble* bubble);
void            nd_stack_remove_all            (NdStack* stack);
GList*          nd_stack_get_bubbles           (NdStack* stack);
void            nd_stack_queue_update_position (NdStack* stack);

C_END_EXTERN_C

#endif // graceful_DE2_ND_STACK_H
