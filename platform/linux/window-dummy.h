#ifndef WINDOW_DUMMY_H_
#define WINDOW_DUMMY_H_

#include <core/pixel.h>

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

struct window_dummy {
};

void window_dummy_init(struct window_dummy *win);
void window_dummy_destroy(struct window_dummy *win);

#endif /* WINDOW_DUMMY_H_ */
