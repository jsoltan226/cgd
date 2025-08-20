#ifndef WINDOW_X11_EVENT_H_
#define WINDOW_X11_EVENT_H_

#include <platform/common/guard.h>

extern void * window_X11_event_listener_fn(void *arg);

extern void * window_X11_render_software_malloced_present_thread_fn(void *arg);

#endif /* WINDOW_X11_EVENT_H_ */
