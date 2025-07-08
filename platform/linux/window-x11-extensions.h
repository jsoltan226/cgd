#ifndef WINDOW_X11_EXTENSION_STORE_H_
#define WINDOW_X11_EXTENSION_STORE_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#define P_INTERNAL_GUARD__
#include "libxcb-rtld.h"
#undef P_INTERNAL_GUARD__
#include <core/int.h>
#include <stdbool.h>
#include <xcb/xproto.h>

enum x11_extension_name {
    X11_EXT_NULL_ = -1,
    X11_EXT_XINPUT = 0,
    X11_EXT_SHM,
    X11_EXT_PRESENT,
    X11_EXT_MAX_
};

struct x11_extension {
    bool available;

    const char *name;
    u8 major_opcode;
    u8 first_event, first_error;
};

struct x11_extension_store {
    _Atomic bool initialized_, ready_;
    struct x11_extension extensions[X11_EXT_MAX_];
};

/* Important note: THIS API IS NOT THREAD SAFE!
 * The initialization and destruction of the extension store
 * MUST be done while NO OTHER THAN THE "MAIN" THREAD
 * is running/has access to it.
 *
 * `X11_extension_get_*`, however, can be called from any thread
 * as it doesn't write to the store,
 * because it's assumed that the store was initialized before any "side" thread
 * even started. */

void X11_extension_store_init(struct x11_extension_store *ext_store,
    xcb_connection_t *conn, const struct libxcb *xcb);

void X11_extension_get_data(const struct x11_extension_store *ext_store,
    enum x11_extension_name ext_name, struct x11_extension *o);
bool X11_extension_is_available(const struct x11_extension_store *ext_store,
    enum x11_extension_name ext_name);
enum x11_extension_name X11_extension_get_name_by_opcode(
    const struct x11_extension_store *ext_store, u8 major_opcode
);

void X11_extension_store_destroy(struct x11_extension_store *ext_store);

#endif /* WINDOW_X11_EXTENSION_STORE_H_ */
