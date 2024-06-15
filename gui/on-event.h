#ifndef ON_EVENT_H_
#define ON_EVENT_H_
#include "core/int.h"
#include <stdlib.h>

#define ONEVENT_OBJ_ARGV_LEN    16
#define ONEVENT_OBJ_ARGV_SIZE   (ONEVENT_OBJ_ARGV_LEN * sizeof(u64))

typedef i32(*on_event_fn)(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);

struct on_event_obj {
    on_event_fn fn;
    u64 argv_buf[ONEVENT_OBJ_ARGV_LEN];
};

#define on_event_execute(oeObj) do {    \
    if (oeObj.fn != NULL)               \
        oeObj.fn(oeObj.argv_buf);       \
} while (0);

#endif /* ON_EVENT_H_ */
