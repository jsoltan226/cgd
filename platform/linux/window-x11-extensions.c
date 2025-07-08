#define P_INTERNAL_GUARD__
#include "window-x11-extensions.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "libxcb-rtld.h"
#undef P_INTERNAL_GUARD__
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <string.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <xcb/xcb.h>

#define MODULE_NAME "window-x11-extensions"

static xcb_query_extension_reply_t * query_extension(const char *name,
    xcb_connection_t *conn, const struct libxcb *xcb);

void X11_extension_store_init(struct x11_extension_store *ext_store,
    xcb_connection_t *conn, const struct libxcb *xcb)
{
    u_check_params(ext_store != NULL && conn != NULL && xcb != NULL &&
        xcb->_voidp_xcb_query_extension != NULL &&
        xcb->_voidp_xcb_query_extension_reply != NULL);

    atomic_store(&ext_store->ready_, false);
    atomic_store(&ext_store->initialized_, true);

    static const char *const ext_names[X11_EXT_MAX_] = {
        [X11_EXT_XINPUT] = X11_XINPUT_EXT_NAME,
        [X11_EXT_SHM] = X11_SHM_EXT_NAME,
        [X11_EXT_PRESENT] = X11_PRESENT_EXT_NAME,
    };
    const bool ext_lib_loaded[X11_EXT_MAX_] = {
        [X11_EXT_XINPUT] = xcb->xinput.loaded_,
        [X11_EXT_SHM] = xcb->shm.loaded_,
        [X11_EXT_PRESENT] = xcb->present.loaded_,
    };

    for (u32 i = 0; i < X11_EXT_MAX_; i++) {
        struct x11_extension *const curr_ext = &ext_store->extensions[i];

        curr_ext->name = ext_names[i];
        curr_ext->available = false;
        curr_ext->major_opcode = 0;
        curr_ext->first_event = 0;
        curr_ext->first_error = 0;

        if (!ext_lib_loaded[i]) {
            s_log_debug("X \"%s\" extension library not loaded",
                ext_names[i]);
            continue;
        }

        xcb_query_extension_reply_t *reply =
            query_extension(ext_names[i], conn, xcb);
        if (reply != NULL) {
            curr_ext->available = reply->present;
            curr_ext->major_opcode = reply->major_opcode;
            curr_ext->first_event = reply->first_event;
            curr_ext->first_error = reply->first_error;
            u_nfree(&reply);
        } /* else extension not available */
    }

    atomic_store(&ext_store->ready_, true);
}

void X11_extension_get_data(const struct x11_extension_store *ext_store,
    enum x11_extension_name ext_name, struct x11_extension *o)
{
    u_check_params(ext_store != NULL && atomic_load(&ext_store->initialized_) &&
        ext_name >= 0 && ext_name < X11_EXT_MAX_ && o != NULL);

    s_assert(atomic_load(&ext_store->ready_),
        "Extension store not properly initialized!");

    memcpy(o, &ext_store->extensions[ext_name], sizeof(struct x11_extension));
}

bool X11_extension_is_available(const struct x11_extension_store *ext_store,
    enum x11_extension_name ext_name)
{
    u_check_params(ext_store != NULL && atomic_load(&ext_store->initialized_));
    s_assert(atomic_load(&ext_store->ready_),
        "Extension store not properly initialized!");

    return ext_store->extensions[ext_name].available;
}

enum x11_extension_name X11_extension_get_name_by_opcode(
    const struct x11_extension_store *ext_store, u8 major_opcode
)
{
    u_check_params(ext_store != NULL && atomic_load(&ext_store->initialized_));
    s_assert(atomic_load(&ext_store->ready_),
        "Extension store not properly initialized!");

    for (u32 i = 0; i < X11_EXT_MAX_; i++) {
        if (major_opcode == ext_store->extensions[i].major_opcode)
            return i;
    }

    return -1;
}

void X11_extension_store_destroy(struct x11_extension_store *ext_store)
{
    u_check_params(ext_store != NULL);

    if (!atomic_load(&ext_store->initialized_)) return;

    memset(ext_store, 0, sizeof(struct x11_extension_store));
    atomic_store(&ext_store->initialized_, false);
    atomic_store(&ext_store->ready_, false);
}

static xcb_query_extension_reply_t * query_extension(const char *name,
    xcb_connection_t *conn, const struct libxcb *xcb)
{
    xcb_query_extension_cookie_t cookie = xcb->xcb_query_extension(conn,
        strlen(name), name);
    xcb_query_extension_reply_t *reply =
        xcb->xcb_query_extension_reply(conn, cookie, NULL);
    if (reply == NULL) {
        s_log_error("xcb_query_extension(\"%s\") failed!", name);
        return NULL;
    }

    s_log_debug("extension \"%s\" present: %d, opcode: %u, "
        "first event: %u, first error: %u",
        name, reply->present, reply->major_opcode,
        reply->first_event, reply->first_error);

    return reply;
}
