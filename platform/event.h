#ifndef P_EVENT_H_
#define P_EVENT_H_

#include <core/int.h>

enum p_event_type {
    P_EVENT_NONE = 0,
    P_EVENT_QUIT,

    P_EVENT_CTL_INIT_,
    P_EVENT_CTL_DESTROY_,
};

struct p_event {
    enum p_event_type type;

    union {
    };
};

/* Reads the last event from the queue, if it exists
 * Returns the number of events left in the queue
 * (return value 0 means that there are no more events left) */
i32 p_event_poll(struct p_event *o);

/* Pushes an event to the queue. */
void p_event_send(const struct p_event *ev);

#endif /* P_EVENT_H_ */
