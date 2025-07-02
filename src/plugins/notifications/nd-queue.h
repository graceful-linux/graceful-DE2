//
// Created by dingjing on 25-7-2.
//

#ifndef graceful_DE2_ND_QUEUE_H
#define graceful_DE2_ND_QUEUE_H
#include "macros/macros.h"

#include <glib-object.h>

#include "nd-notification.h"

C_BEGIN_EXTERN_C

#define ND_TYPE_QUEUE         (nd_queue_get_type ())
#define ND_QUEUE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ND_TYPE_QUEUE, NdQueue))
#define ND_QUEUE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), ND_TYPE_QUEUE, NdQueueClass))
#define ND_IS_QUEUE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ND_TYPE_QUEUE))
#define ND_IS_QUEUE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ND_TYPE_QUEUE))
#define ND_QUEUE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ND_TYPE_QUEUE, NdQueueClass))

typedef struct NdQueuePrivate NdQueuePrivate;

typedef struct
{
    GObject           parent;
    NdQueuePrivate *priv;
} NdQueue;

typedef struct
{
    GObjectClass   parent_class;
} NdQueueClass;

GType               nd_queue_get_type                       (void);
NdQueue*            nd_queue_new                            (void);
guint               nd_queue_length                         (NdQueue* queue);
NdNotification*     nd_queue_lookup                         (NdQueue* queue, guint id);
void                nd_queue_add                            (NdQueue* queue, NdNotification* notification);
void                nd_queue_remove_for_id                  (NdQueue* queue, guint id);

C_END_EXTERN_C

#endif // graceful_DE2_ND_QUEUE_H
