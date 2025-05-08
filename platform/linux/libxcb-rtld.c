#include "../thread.h"
#include "../librtld.h"
#include <core/log.h>
#include <core/int.h>
#include <core/util.h>
#include <string.h>
#include <stdbool.h>
#define P_INTERNAL_GUARD__
#include "libxcb-rtld.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "libxcb-rtld"

#define LIBXCB_RTLD_ERROR_MAGIC -1

static struct p_lib *g_libxcb_lib = NULL;
static struct p_lib *g_libxcb_image_lib = NULL;
static struct p_lib *g_libxcb_icccm_lib = NULL;
static struct p_lib *g_libxcb_keysyms_lib = NULL;

static struct p_lib *g_libxcb_input_lib = NULL;
static bool g_libxcb_input_ok = true;
static struct p_lib *g_libxcb_shm_lib = NULL;
static bool g_libxcb_shm_ok = true;
static struct p_lib *g_libxcb_present_lib = NULL;
static bool g_libxcb_present_ok = true;

static struct libxcb g_libxcb_syms = { 0 };
static i32 g_n_active_handles = 0;
static p_mt_mutex_t g_mutex = P_MT_MUTEX_INITIALIZER;

#define X_FN_(ret_type, name, ...) #name,
#define X_V_(type, name) #name,
static const char *libxcb_sym_names[] = {
    LIBXCB_SYM_LIST
    NULL
};

static const char *libxcb_image_sym_names[] = {
    LIBXCB_IMAGE_SYM_LIST
    NULL
};

static const char *libxcb_icccm_sym_names[] = {
    LIBXCB_ICCCM_SYM_LIST
    NULL
};

static const char *libxcb_keysyms_sym_names[] = {
    LIBXCB_KEYSYMS_SYM_LIST
    NULL
};

static const char *libxcb_input_sym_names[] = {
    LIBXCB_INPUT_SYM_LIST
    NULL
};

static const char *libxcb_shm_sym_names[] = {
    LIBXCB_SHM_SYM_LIST
    NULL
};

static const char *libxcb_present_sym_names[] = {
    LIBXCB_PRESENT_SYM_LIST
    NULL
};

#undef X_FN_
#undef X_V_

/* Note: Both of these functions assume that `g_mutex` is locked
 * throughout their entire execution */
static i32 do_load_libraries(void);
static void do_unload_libraries(void);

i32 libxcb_load(struct libxcb *o)
{
    i32 ret = 0;
    u_check_params(o != NULL);
    memset(o, 0, sizeof(struct libxcb));

    p_mt_mutex_lock(&g_mutex);

    if (g_n_active_handles == LIBXCB_RTLD_ERROR_MAGIC) {
        ret = -1;
        goto end;
    }

    if (g_libxcb_lib == NULL ||
        g_libxcb_image_lib == NULL ||
        g_libxcb_icccm_lib == NULL ||
        g_libxcb_keysyms_lib == NULL ||
        (g_libxcb_input_lib == NULL && g_libxcb_input_ok) ||
        (g_libxcb_shm_lib == NULL && g_libxcb_shm_ok) ||
        (g_libxcb_present_lib == NULL && g_libxcb_present_ok)
    ) {
        if (do_load_libraries()) {
            ret = 1;
            goto end;
        }
    }

    memcpy(o, &g_libxcb_syms, sizeof(struct libxcb));
    o->handleno_ = g_n_active_handles;
    g_n_active_handles++;

end:
    p_mt_mutex_unlock(&g_mutex);
    o->loaded_ = ret == 0;
    return ret;
}

void libxcb_unload(struct libxcb *xcb)
{
    if (xcb == NULL) return;
    p_mt_mutex_lock(&g_mutex);

    if (g_n_active_handles == LIBXCB_RTLD_ERROR_MAGIC) goto end;
    else if (g_n_active_handles <= 0) goto end;

    /* Sanity check */
    if (!xcb->loaded_ || xcb->handleno_ >= g_n_active_handles) {
        s_log_error("%s: Invalid libxcb handle. Not unloading!", __func__);
        goto end;
    }

    if (--g_n_active_handles == 0)
        do_unload_libraries();

end:
    p_mt_mutex_unlock(&g_mutex);
}

static i32 do_load_libraries(void)
{
    g_n_active_handles = 0;
    memset(&g_libxcb_syms, 0, sizeof(struct libxcb));

    g_libxcb_syms.handleno_ = -1;
    g_libxcb_syms.loaded_ = false;

    g_libxcb_lib = p_librtld_load(LIBXCB_SO_NAME, libxcb_sym_names);

    g_libxcb_image_lib = p_librtld_load(LIBXCB_IMAGE_SO_NAME,
            libxcb_image_sym_names);

    g_libxcb_icccm_lib = p_librtld_load(LIBXCB_ICCCM_SO_NAME,
            libxcb_icccm_sym_names);

    g_libxcb_keysyms_lib = p_librtld_load(LIBXCB_KEYSYMS_SO_NAME,
            libxcb_keysyms_sym_names);

    if (g_libxcb_lib == NULL ||
        g_libxcb_image_lib == NULL ||
        g_libxcb_icccm_lib == NULL ||
        g_libxcb_keysyms_lib == NULL
    ) {
        g_n_active_handles = LIBXCB_RTLD_ERROR_MAGIC;
        return 1;
    }

    g_libxcb_input_lib = p_librtld_load(LIBXCB_INPUT_SO_NAME,
            libxcb_input_sym_names);
    if (g_libxcb_input_lib == NULL)
        g_libxcb_input_ok = false;

    g_libxcb_shm_lib = p_librtld_load(LIBXCB_SHM_SO_NAME,
        libxcb_shm_sym_names);
    if (g_libxcb_shm_lib == NULL)
        g_libxcb_shm_ok = false;

    g_libxcb_present_lib = p_librtld_load(LIBXCB_PRESENT_SO_NAME,
        libxcb_present_sym_names);
    if (g_libxcb_present_lib == NULL)
        g_libxcb_present_ok = false;

#define X_FN_(ret_type, name, ...) curr_syms._voidp_##name = \
            p_librtld_load_sym(curr_lib, #name);
#define X_V_(type, name) curr_syms.name = p_librtld_load_sym(curr_lib, #name);

    /* Retrieve the core API symbols */
#define curr_syms g_libxcb_syms

#define curr_lib g_libxcb_lib
        LIBXCB_SYM_LIST
#undef curr_lib
#define curr_lib g_libxcb_image_lib
        LIBXCB_IMAGE_SYM_LIST
#undef curr_lib
#define curr_lib g_libxcb_icccm_lib
        LIBXCB_ICCCM_SYM_LIST
#undef curr_lib
#define curr_lib g_libxcb_keysyms_lib
        LIBXCB_KEYSYMS_SYM_LIST
#undef curr_lib

#undef curr_syms

    /* Retrieve the extension symbols */
    if (g_libxcb_input_ok) {
#define curr_syms g_libxcb_syms.xinput
#define curr_lib g_libxcb_input_lib
        LIBXCB_INPUT_SYM_LIST
#undef curr_lib
#undef curr_syms
        g_libxcb_syms.xinput.loaded_ = true;
    } else {
        memset(&g_libxcb_syms.xinput, 0, sizeof(struct libxcb_xinput));
        g_libxcb_syms.xinput.loaded_ = false;
    }
    if (g_libxcb_shm_ok) {
#define curr_syms g_libxcb_syms.shm
#define curr_lib g_libxcb_shm_lib
        LIBXCB_SHM_SYM_LIST
#undef curr_lib
#undef curr_syms
        g_libxcb_syms.shm.loaded_ = true;
    } else {
        memset(&g_libxcb_syms.shm, 0, sizeof(struct libxcb_shm));
        g_libxcb_syms.shm.loaded_ = false;
    }

    if (g_libxcb_present_ok) {
#define curr_syms g_libxcb_syms.present
#define curr_lib g_libxcb_present_lib
        LIBXCB_PRESENT_SYM_LIST
#undef curr_lib
#undef curr_syms
        g_libxcb_syms.present.loaded_ = true;
    } else {
        memset(&g_libxcb_syms.present, 0, sizeof(struct libxcb_present));
        g_libxcb_syms.present.loaded_ = false;
    }

    return 0;
}

static void do_unload_libraries(void)
{
    /* `p_librtld_close` automatically handles invalid arguments,
     * so we don't have to check if the handles are NULL */
    p_librtld_close(&g_libxcb_lib);
    p_librtld_close(&g_libxcb_icccm_lib);
    p_librtld_close(&g_libxcb_image_lib);
    p_librtld_close(&g_libxcb_keysyms_lib);
    p_librtld_close(&g_libxcb_input_lib);
    p_librtld_close(&g_libxcb_shm_lib);
    p_librtld_close(&g_libxcb_present_lib);

    memset(&g_libxcb_syms, 0, sizeof(struct libxcb));
}
