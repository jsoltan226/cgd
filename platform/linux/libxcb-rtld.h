#ifndef LIBXCB_RTLD_H_
#define LIBXCB_RTLD_H_

#include <xcb/bigreq.h>
#include <xcb/present.h>
#include <xcb/shm.h>
#include <xcb/xcbext.h>
#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include <core/int.h>
#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xinput.h>
#include <xcb/present.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>

#define LIBXCB_SO_NAME "libxcb"
#define LIBXCB_SYM_LIST                                                        \
    X_FN_(xcb_generic_error_t *, xcb_request_check,                            \
        xcb_connection_t *c, xcb_void_cookie_t cookie                          \
    )                                                                          \
    X_FN_(xcb_connection_t *, xcb_connect, const char *dpyname, int *screenp)  \
    X_FN_(int, xcb_connection_has_error, xcb_connection_t *c)                  \
    X_FN_(const struct xcb_setup_t *, xcb_get_setup, xcb_connection_t *c)      \
    X_FN_(xcb_screen_iterator_t, xcb_setup_roots_iterator,                     \
        const xcb_setup_t *R                                                   \
    )                                                                          \
    X_FN_(uint32_t, xcb_generate_id, xcb_connection_t *c)                      \
    X_FN_(xcb_void_cookie_t, xcb_create_window_checked, xcb_connection_t *c,   \
        uint8_t depth, xcb_window_t wid, xcb_window_t parent,                  \
        int16_t x, int16_t y, uint16_t width, uint16_t height,                 \
        uint16_t border_width, uint16_t _class, xcb_visualid_t visual,         \
        uint32_t value_mask, const void *value_list                            \
    )                                                                          \
    X_FN_(xcb_void_cookie_t, xcb_change_property_checked,                      \
        xcb_connection_t *c, uint8_t mode, xcb_window_t window,                \
        xcb_atom_t property, xcb_atom_t type, uint8_t format,                  \
        uint32_t data_len, const void *data                                    \
    )                                                                          \
    X_FN_(xcb_void_cookie_t, xcb_create_gc_checked,                            \
        xcb_connection_t *c, xcb_gcontext_t cid, xcb_drawable_t drawable,      \
        uint32_t value_mask, const void *value_list                            \
    )                                                                          \
    X_FN_(xcb_void_cookie_t, xcb_map_window_checked,                           \
        xcb_connection_t *c, xcb_window_t window                               \
    )                                                                          \
    X_FN_(int, xcb_flush, xcb_connection_t *c)                                 \
    X_FN_(xcb_void_cookie_t, xcb_destroy_window,                               \
        xcb_connection_t *c, xcb_window_t window                               \
    )                                                                          \
    X_FN_(void, xcb_disconnect, xcb_connection_t *c)                           \
    X_FN_(xcb_intern_atom_cookie_t, xcb_intern_atom,                           \
        xcb_connection_t *c, uint8_t only_if_exists,                           \
        uint16_t name_len, const char *name                                    \
    )                                                                          \
    X_FN_(xcb_intern_atom_reply_t *, xcb_intern_atom_reply,                    \
        xcb_connection_t *c, xcb_intern_atom_cookie_t cookie,                  \
        xcb_generic_error_t **e                                                \
    )                                                                          \
    X_FN_(xcb_generic_event_t *, xcb_wait_for_event, xcb_connection_t *c)      \
    X_FN_(xcb_void_cookie_t, xcb_send_event,                                   \
        xcb_connection_t *conn, uint8_t propagate, xcb_window_t destination,   \
        uint32_t event_mask, const char *event                                 \
    )                                                                          \
    X_FN_(xcb_void_cookie_t, xcb_free_gc,                                      \
        xcb_connection_t *conn, xcb_gcontext_t gc                              \
    )                                                                          \
    X_FN_(xcb_void_cookie_t, xcb_free_pixmap_checked,                          \
        xcb_connection_t *conn, xcb_pixmap_t pixmap                            \
    )                                                                          \
    X_FN_(xcb_query_extension_cookie_t, xcb_query_extension,                   \
        xcb_connection_t *conn, uint16_t name_len, const char *name            \
    )                                                                          \
    X_FN_(xcb_query_extension_reply_t *, xcb_query_extension_reply,            \
        xcb_connection_t *c, xcb_query_extension_cookie_t cookie,              \
        xcb_generic_error_t **e                                                \
    )                                                                          \
    X_FN_(const struct xcb_query_extension_reply_t *, xcb_get_extension_data,  \
        xcb_connection_t *c, xcb_extension_t *ext                              \
    )                                                                          \
    X_FN_(xcb_big_requests_enable_cookie_t, xcb_big_requests_enable,           \
        xcb_connection_t *c                                                    \
    )                                                                          \
    X_FN_(xcb_big_requests_enable_reply_t *, xcb_big_requests_enable_reply,    \
        xcb_connection_t *c, xcb_big_requests_enable_cookie_t cookie,          \
        xcb_generic_error_t **e                                                \
    )                                                                          \
    X_FN_(xcb_void_cookie_t, xcb_put_image,                                    \
        xcb_connection_t *c, uint8_t format, xcb_drawable_t drawable,          \
        xcb_gcontext_t gc, uint16_t width, uint16_t height,                    \
        int16_t dst_x, int16_t dst_y, uint8_t left_pad, uint8_t depth,         \
        uint32_t data_len, const uint8_t *data                                 \
    )                                                                          \
    X_FN_(uint32_t, xcb_get_maximum_request_length, xcb_connection_t *conn)    \

#define LIBXCB_ICCCM_SO_NAME "libxcb-icccm"
#define LIBXCB_ICCCM_SYM_LIST                                                  \
    X_FN_(xcb_void_cookie_t, xcb_icccm_set_wm_normal_hints_checked,            \
        xcb_connection_t *c, xcb_window_t window, xcb_size_hints_t *hints      \
    )                                                                          \
    X_FN_(void, xcb_icccm_size_hints_set_min_size,                             \
        xcb_size_hints_t *hints, int32_t min_width, int32_t min_height         \
    )                                                                          \
    X_FN_(void, xcb_icccm_size_hints_set_max_size,                             \
        xcb_size_hints_t *hints, int32_t min_width, int32_t min_height         \
    )                                                                          \

#define LIBXCB_KEYSYMS_SO_NAME "libxcb-keysyms"
#define LIBXCB_KEYSYMS_SYM_LIST                                                \
    X_FN_(xcb_key_symbols_t *, xcb_key_symbols_alloc, xcb_connection_t *c)     \
    X_FN_(xcb_keysym_t, xcb_key_symbols_get_keysym,                            \
        xcb_key_symbols_t *syms, xcb_keycode_t keycode, int col                \
    )                                                                          \
    X_FN_(void, xcb_key_symbols_free, xcb_key_symbols_t *syms)                 \

#define LIBXCB_ERRORS_SO_NAME "libxcb-errors"
#define LIBXCB_ERRORS_SYM_LIST


#define X11_XINPUT_EXT_NAME "XInputExtension"
#define LIBXCB_INPUT_SO_NAME "libxcb-xinput"
#define LIBXCB_INPUT_SYM_LIST                                                  \
    X_FN_(xcb_input_xi_query_version_cookie_t, xcb_input_xi_query_version,     \
        xcb_connection_t *c, uint16_t major_version, uint16_t minor_version    \
    )                                                                          \
    X_FN_(xcb_input_xi_query_version_reply_t *,                                \
            xcb_input_xi_query_version_reply,                                  \
        xcb_connection_t *c, xcb_input_xi_query_version_cookie_t cookie,       \
        xcb_generic_error_t **e                                                \
    )                                                                          \
    X_FN_(xcb_void_cookie_t, xcb_input_xi_select_events_checked,               \
        xcb_connection_t *conn, xcb_window_t window,                           \
        uint16_t num_mask, const xcb_input_event_mask_t *masks                 \
    )                                                                          \
    X_FN_(xcb_input_xi_query_device_cookie_t, xcb_input_xi_query_device,       \
        xcb_connection_t *conn, xcb_input_device_id_t deviceid                 \
    )                                                                          \
    X_FN_(xcb_input_xi_query_device_reply_t *,                                 \
            xcb_input_xi_query_device_reply,                                   \
        xcb_connection_t *conn, xcb_input_xi_query_device_cookie_t cookie,     \
        xcb_generic_error_t **e                                                \
    )                                                                          \
    X_FN_(xcb_input_xi_device_info_iterator_t,                                 \
       xcb_input_xi_query_device_infos_iterator,                               \
        const xcb_input_xi_query_device_reply_t *R                             \
    )                                                                          \
    X_FN_(void, xcb_input_xi_device_info_next,                                 \
        xcb_input_xi_device_info_iterator_t *i                                 \
    )                                                                          \
                                                                               \
    X_V_(xcb_extension_t, xcb_input_id)                                        \


#define X11_SHM_EXT_NAME "MIT-SHM"
#define LIBXCB_SHM_SO_NAME "libxcb-shm"
#define LIBXCB_SHM_SYM_LIST                                                    \
    X_FN_(xcb_void_cookie_t, xcb_shm_attach_checked,                           \
        xcb_connection_t *c, xcb_shm_seg_t shmseg,                             \
        uint32_t shmid, uint8_t read_only                                      \
    )                                                                          \
    X_FN_(xcb_void_cookie_t, xcb_shm_put_image,                                \
        xcb_connection_t *c, xcb_drawable_t drawable, xcb_gcontext_t gc,       \
        uint16_t total_width, uint16_t total_height,                           \
        uint16_t src_x, uint16_t src_y, uint16_t src_w, uint16_t src_h,        \
        int16_t dst_x, int16_t dst_y, uint8_t depth, uint8_t format,           \
        uint8_t send_event, xcb_shm_seg_t shmseg, uint32_t offset              \
    )                                                                          \
    X_FN_(xcb_void_cookie_t, xcb_shm_detach,                                   \
        xcb_connection_t *c, xcb_shm_seg_t shmseg                              \
    )                                                                          \
    X_FN_(xcb_void_cookie_t, xcb_shm_create_pixmap,                            \
        xcb_connection_t *conn, xcb_pixmap_t pid, xcb_drawable_t drawable,     \
        uint16_t width, uint16_t height, uint8_t depth,                        \
        xcb_shm_seg_t shmseg, uint32_t offset                                  \
    )                                                                          \
    X_FN_(xcb_shm_query_version_cookie_t, xcb_shm_query_version,               \
        xcb_connection_t *conn                                                 \
    )                                                                          \
    X_FN_(xcb_shm_query_version_reply_t *, xcb_shm_query_version_reply,        \
        xcb_connection_t *conn, xcb_shm_query_version_cookie_t cookie,         \
        xcb_generic_error_t **e                                                \
    )                                                                          \
    X_V_(xcb_extension_t, xcb_shm_id)                                          \

#define X11_PRESENT_EXT_NAME "Present"
#define LIBXCB_PRESENT_SO_NAME "libxcb-present"
#define LIBXCB_PRESENT_SYM_LIST                                                \
    X_FN_(xcb_present_query_version_cookie_t, xcb_present_query_version,       \
        xcb_connection_t *c, uint32_t major_version, uint32_t minor_version    \
    )                                                                          \
    X_FN_(xcb_present_query_version_reply_t *,                                 \
            xcb_present_query_version_reply,                                   \
        xcb_connection_t *c, xcb_present_query_version_cookie_t cookie,        \
        xcb_generic_error_t **e                                                \
    )                                                                          \
    X_FN_(xcb_void_cookie_t, xcb_present_pixmap,                               \
        xcb_connection_t *c, xcb_window_t window, xcb_pixmap_t pixmap,         \
        uint32_t serial, xcb_xfixes_region_t valid,                            \
        xcb_xfixes_region_t update, int16_t x_off, int16_t y_off,              \
        xcb_randr_crtc_t target_crtc, xcb_sync_fence_t wait_fence,             \
        xcb_sync_fence_t idle_fence, uint32_t options,                         \
        uint64_t target_msc, uint64_t divisor, uint64_t remainder,             \
        uint32_t notifies_len, const xcb_present_notify_t *notifies            \
    )                                                                          \
    X_FN_(xcb_void_cookie_t, xcb_present_select_input_checked,                 \
        xcb_connection_t *c, xcb_present_event_t eid,                          \
        xcb_window_t window, uint32_t event_mask                               \
    )                                                                          \
    X_V_(xcb_extension_t, xcb_present_id)                                      \

struct libxcb {
#define X_FN_(ret_type, name, ...) \
    union { ret_type (*name) (__VA_ARGS__); void *_voidp_##name; };

#define X_V_(type, name) type *name;

    bool loaded_;
    LIBXCB_SYM_LIST
    LIBXCB_ICCCM_SYM_LIST
    LIBXCB_KEYSYMS_SYM_LIST

    struct libxcb_xinput {
        bool loaded_;
        LIBXCB_INPUT_SYM_LIST
    } xinput;
    struct libxcb_shm {
        bool loaded_;
        LIBXCB_SHM_SYM_LIST
    } shm;
    struct libxcb_present {
        bool loaded_;
        LIBXCB_PRESENT_SYM_LIST
    } present;
#undef X_V_
#undef X_FN_

    i32 handleno_;
};

i32 libxcb_load(struct libxcb *o);
void libxcb_unload(struct libxcb *xcb);

#endif /* LIBXCB_RTLD_H_ */
