#ifndef WINDOW_DUMMY_H_
#define WINDOW_DUMMY_H_

#include <platform/common/guard.h>
#include <core/pixel.h>

struct window_dummy {
    u64 dummy;
};

i32 window_dummy_init(struct window_dummy *win, const u32 flags);
void window_dummy_destroy(struct window_dummy *win);

#endif /* WINDOW_DUMMY_H_ */
