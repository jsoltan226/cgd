#define _GNU_SOURCE
#include "../event.h"
#include "core/datastruct/vector.h"
#include "core/int.h"
#include "core/log.h"
#include "core/util.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#define MODULE_NAME "event"

static VECTOR(struct p_event) g_event_queue = NULL;
static pthread_mutex_t g_event_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

static i32 setup_event_queue();
static void SIGTERM_handler(i32 sig_num);

i32 p_event_poll(struct p_event *o)
{
    u_check_params(o != NULL);
    u32 n_events = 0;

    pthread_mutex_lock(&g_event_queue_mutex);

    if (g_event_queue == NULL && setup_event_queue() != 0)
        s_log_fatal(MODULE_NAME, __func__, "Failed to set up the event queue!");

    n_events = vector_size(g_event_queue);
    if (n_events == 0)
        goto ret;

    memcpy(o,
        vector_end(g_event_queue) - sizeof(struct p_event),
        sizeof(struct p_event)
    );
    vector_pop_back(g_event_queue);

    if (o->type == P_EVENT_QUIT) {
        s_log_info("Caught QUIT event");
        vector_destroy(g_event_queue);
        g_event_queue = NULL;
    }

ret:
    pthread_mutex_unlock(&g_event_queue_mutex);
    return n_events;
}

void p_event_send(const struct p_event *ev)
{
    u_check_params(ev != NULL);

    pthread_mutex_lock(&g_event_queue_mutex);
    if (g_event_queue == NULL)
        g_event_queue = vector_new(struct p_event);

    vector_push_back(g_event_queue, *ev);

    pthread_mutex_unlock(&g_event_queue_mutex);
}

static i32 setup_event_queue()
{
    s_log_debug("Event queue does not exist, initializing...");
    g_event_queue = vector_new(struct p_event);

    /** Set the signal handler for SIGTERM and SIGINT **/
    struct sigaction sa;

    /* Set the handler */
    sa.sa_handler = SIGTERM_handler;

    /* Empty the set of signals being blocked while our handler is running */
    sigemptyset(&sa.sa_mask);

    /* Add SIGTERM to the set of blocked signals */
    sigaddset(&sa.sa_mask, SIGTERM);

    /* We don't want to set any special flags */
    sa.sa_flags = 0;

    /* Don't leave anything uninitialized */
    sa.sa_restorer = NULL;

    /* Finally, set the configuration for both SIGTERM and SIGINT  */
    if (
        sigaction(SIGTERM, &sa, NULL) ||
        sigaction(SIGINT, &sa, NULL)
    ) {
        s_log_error("Failed to set signal handler: %s", strerror(errno));
        return 1;
    }

    return 0;
}

static void SIGTERM_handler(i32 sig_num)
{
    if (sig_num == SIGTERM)
        s_log_debug("Caught SIGTERM");
    else if (sig_num == SIGINT)
        s_log_debug("Caught SIGINT");
    else
        s_log_warn("Caught unexpected signal %i", sig_num);

    const struct p_event ev = {
        .type = P_EVENT_QUIT
    };
    p_event_send(&ev);
}
