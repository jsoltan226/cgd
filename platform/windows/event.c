#include "../event.h"
#include "../thread.h"
#include <core/int.h>
#include <core/util.h>
#include <core/vector.h>
#include <stdbool.h>

#define MODULE_NAME "event"

static VECTOR(struct p_event) g_event_queue = NULL;
static p_mt_mutex_t g_event_queue_mutex = P_MT_MUTEX_INITIALIZER;

static void setup_event_queue(bool warn);
static void destroy_event_queue();

i32 p_event_poll(struct p_event *o)
{
    u_check_params(o != NULL);
    u32 n_events = 0;

    p_mt_mutex_lock(&g_event_queue_mutex);

    if (g_event_queue == NULL)
        setup_event_queue(true);

    n_events = vector_size(g_event_queue);
    if (n_events == 0)
        goto ret;

    memcpy(o,
        vector_end(g_event_queue) - sizeof(struct p_event),
        sizeof(struct p_event)
    );
    vector_pop_back(g_event_queue);

    if (o->type == P_EVENT_QUIT)
        s_log_debug("Caught QUIT event");

ret:
    p_mt_mutex_unlock(&g_event_queue_mutex);
    return n_events;
}

void p_event_send(const struct p_event *ev)
{
    u_check_params(ev != NULL);

    p_mt_mutex_lock(&g_event_queue_mutex);

    switch (ev->type) {
        case P_EVENT_CTL_INIT_:
            setup_event_queue(false);
            break;
        case P_EVENT_CTL_DESTROY_:
            destroy_event_queue();
            break;
        default:
            if (g_event_queue == NULL)
                setup_event_queue(true);
            vector_push_back(g_event_queue, *ev);
            break;
    }

    p_mt_mutex_unlock(&g_event_queue_mutex);
}

static void setup_event_queue(bool warn)
{
    if (g_event_queue != NULL) {
        s_log_warn("%s: Event queue already initialized!", __func__);
        return;
    } else if (warn) {
        s_log_warn("Event queue does not exist, initializing...");
    }

    g_event_queue = vector_new(struct p_event);
}

static void destroy_event_queue()
{
    if (g_event_queue == NULL) {
        s_log_warn("%s: Event queue already destroyed!", __func__);
        return;
    }
    s_log_debug("Destroying event queue...");

    vector_destroy(&g_event_queue);
    g_event_queue = NULL;
}
