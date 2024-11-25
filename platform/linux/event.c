#define _GNU_SOURCE
#include "../event.h"
#include "../thread.h"
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <core/vector.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <signal.h>

#define MODULE_NAME "event"

static VECTOR(struct p_event) g_event_queue = NULL;
static p_mt_mutex_t g_event_queue_mutex = P_MT_MUTEX_INITIALIZER;
static atomic_flag g_signal_handler_running = ATOMIC_FLAG_INIT;
static _Atomic bool g_caught_SIGTERM = false;

static void setup_event_queue(bool warn);
static void destroy_event_queue(void);
static void SIGTERM_handler(i32 sig_num);

i32 p_event_poll(struct p_event *o)
{
    if (atomic_load(&g_caught_SIGTERM)) {
        p_event_send(&(const struct p_event) { .type = P_EVENT_QUIT });
        atomic_store(&g_caught_SIGTERM, false);
    }

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
    } else if (!warn) {
        s_log_debug("Initializing the event queue...");
    }

    g_event_queue = vector_new(struct p_event);

    /** Set the signal handler for SIGTERM and SIGINT **/
    struct sigaction sa = { 0 };

    /* Do not set this signal handler for other threads */
    sa.sa_flags |= SA_NODEFER;

    /* Set the handler */
    sa.sa_handler = SIGTERM_handler;

    /* Block SIGTERM and SIGINT while our handler is running */
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGINT);

    /* We don't want to set any special flags */
    sa.sa_flags = 0;

    /* Finally, set the configuration for both SIGTERM and SIGINT  */
    if (sigaction(SIGTERM, &sa, NULL) || sigaction(SIGINT, &sa, NULL)) {
        s_log_error("Failed to set signal handler: %s", strerror(errno));
        s_log_fatal(MODULE_NAME, __func__, "Failed to set up the event queue!");
    }
}

static void destroy_event_queue(void)
{
    if (g_event_queue == NULL) {
        s_log_warn("%s: Event queue already destroyed!", __func__);
        return;
    }
    s_log_debug("Destroying event queue...");

    atomic_flag_test_and_set(&g_signal_handler_running);

    vector_destroy(&g_event_queue);
    g_event_queue = NULL;

    /* Do NOT restore the default signal handlers */
}

static void SIGTERM_handler(i32 sig_num)
{
    if (atomic_flag_test_and_set(&g_signal_handler_running))
        return;

    if (!atomic_load(&g_caught_SIGTERM))
        atomic_store(&g_caught_SIGTERM, true);

    atomic_flag_clear(&g_signal_handler_running);
}
