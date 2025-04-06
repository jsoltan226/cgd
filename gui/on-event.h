#ifndef ON_EVENT_H_
#define ON_EVENT_H_

#include <core/int.h>
#include <stdlib.h>

#define ONEVENT_OBJ_ARG_LEN    16
#define ONEVENT_OBJ_ARG_SIZE   (ONEVENT_OBJ_ARG_LEN * sizeof(u64))

typedef i32 (*on_event_fn)(const u64 arg[ONEVENT_OBJ_ARG_LEN]);

struct on_event_obj {
    on_event_fn fn;
    u64 arg[ONEVENT_OBJ_ARG_LEN];
};

#define on_event_execute(oe) do {   \
    if (oe.fn != NULL)              \
        oe.fn(oe.arg);              \
} while (0);

#endif /* ON_EVENT_H_ */
