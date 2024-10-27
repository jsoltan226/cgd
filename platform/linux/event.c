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
#include <signal.h>

#define MODULE_NAME "event"

static VECTOR(struct p_event) g_event_queue = NULL;
static p_mt_mutex_t g_event_queue_mutex = P_MT_MUTEX_INITIALIZER;

static void setup_event_queue(bool warn);
static void destroy_event_queue();
static void SIGTERM_handler(i32 sig_num);

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

    /** Set the signal handler for SIGTERM and SIGINT **/
    struct sigaction sa = { 0 };

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

static void destroy_event_queue()
{
    if (g_event_queue == NULL) {
        s_log_warn("%s: Event queue already destroyed!", __func__);
        return;
    }
    s_log_debug("Destroying event queue...");

    vector_destroy(&g_event_queue);
    g_event_queue = NULL;

    /* Restore default signal handlers */
    struct sigaction sa = { .sa_handler = SIG_DFL };
    if (sigaction(SIGTERM, &sa, NULL) || sigaction(SIGINT, &sa, NULL)) {
        s_log_fatal(MODULE_NAME, __func__,
            "Failed to deregister signal handlers for SIGINT and SIGTERM: %s",
            strerror(errno));
    }
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
