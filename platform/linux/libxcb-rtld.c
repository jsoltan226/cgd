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

#define ERROR_MAGIC -1

static struct p_lib *g_libxcb_lib = NULL;
static struct p_lib *g_libxcb_image_lib = NULL;
static struct p_lib *g_libxcb_icccm_lib = NULL;
static struct p_lib *g_libxcb_input_lib = NULL;
static struct p_lib *g_libxcb_keysyms_lib = NULL;

static struct p_lib *g_libxcb_shm_lib = NULL;
static bool g_libxcb_shm_ok = true;

static struct libxcb g_libxcb_syms = { 0 };
static i32 g_n_active_handles = 0;
static p_mt_mutex_t g_mutex = P_MT_MUTEX_INITIALIZER;

#define X_(ret_type, name, ...) #name,
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

static const char *libxcb_input_sym_names[] = {
    LIBXCB_INPUT_SYM_LIST
    NULL
};

static const char *libxcb_keysyms_sym_names[] = {
    LIBXCB_KEYSYMS_SYM_LIST
    NULL
};

static const char *libxcb_shm_sym_names[] = {
    LIBXCB_SHM_SYM_LIST
    NULL
};

#undef X_

i32 libxcb_load(struct libxcb *o)
{
    i32 ret = 0;
    u_check_params(o != NULL);
    memset(o, 0, sizeof(struct libxcb));

    p_mt_mutex_lock(&g_mutex);

    if (g_n_active_handles == ERROR_MAGIC) {
        ret = -1;
        goto end;
    }

    if (g_libxcb_lib == NULL ||
        g_libxcb_image_lib == NULL ||
        g_libxcb_icccm_lib == NULL ||
        g_libxcb_input_lib == NULL ||
        g_libxcb_keysyms_lib == NULL ||
        (g_libxcb_shm_lib == NULL && g_libxcb_shm_ok)
    ) {
        g_n_active_handles = 0;
        memset(&g_libxcb_syms, 0, sizeof(struct libxcb));

        *(i32 *)&g_libxcb_syms.handleno_ = -1; /* Cast away const */
        *(bool *)&g_libxcb_syms.failed_ = false; /* Cast away const */

        g_libxcb_lib = p_librtld_load(LIBXCB_SO_NAME, libxcb_sym_names);

        g_libxcb_image_lib = p_librtld_load(LIBXCB_IMAGE_SO_NAME,
                libxcb_image_sym_names);

        g_libxcb_icccm_lib = p_librtld_load(LIBXCB_ICCCM_SO_NAME,
                libxcb_icccm_sym_names);

        g_libxcb_input_lib = p_librtld_load(LIBXCB_INPUT_SO_NAME,
                libxcb_input_sym_names);

        g_libxcb_keysyms_lib = p_librtld_load(LIBXCB_KEYSYMS_SO_NAME,
                libxcb_keysyms_sym_names);

        if (g_libxcb_lib == NULL ||
            g_libxcb_image_lib == NULL ||
            g_libxcb_icccm_lib == NULL ||
            g_libxcb_input_lib == NULL ||
            g_libxcb_keysyms_lib == NULL
        ) {
            g_n_active_handles = ERROR_MAGIC;
            ret = 1;
            goto end;
        }

        g_libxcb_shm_lib = p_librtld_load(LIBXCB_SHM_SO_NAME,
            libxcb_shm_sym_names);
        if (g_libxcb_shm_lib == NULL)
            g_libxcb_shm_ok = false;

#define X_(ret_type, name, ...) g_libxcb_syms.name = \
                p_librtld_get_sym_handle(g_libxcb_lib, #name);
            LIBXCB_SYM_LIST
#undef X_
#define X_(ret_type, name, ...) g_libxcb_syms.name = \
                p_librtld_get_sym_handle(g_libxcb_image_lib, #name);
            LIBXCB_IMAGE_SYM_LIST
#undef X_
#define X_(ret_type, name, ...) g_libxcb_syms.name = \
                p_librtld_get_sym_handle(g_libxcb_icccm_lib, #name);
            LIBXCB_ICCCM_SYM_LIST
#undef X_
#define X_(ret_type, name, ...) g_libxcb_syms.name = \
                p_librtld_get_sym_handle(g_libxcb_input_lib, #name);
            LIBXCB_INPUT_SYM_LIST
#undef X_
#define X_(ret_type, name, ...) g_libxcb_syms.name = \
                p_librtld_get_sym_handle(g_libxcb_keysyms_lib, #name);
            LIBXCB_KEYSYMS_SYM_LIST
#undef X_
        if (g_libxcb_shm_ok) {
#define X_(ret_type, name, ...) g_libxcb_syms.shm.name = \
                p_librtld_get_sym_handle(g_libxcb_shm_lib, #name);
            LIBXCB_SHM_SYM_LIST
#undef X_
            /* Cast away const */
            *(bool *)&g_libxcb_syms.shm.has_shm_extension_ = true;
        } else {
            memset(&g_libxcb_syms.shm, 0, sizeof(struct libxcb_shm));
            /* Cast away const */
            *(bool *)&g_libxcb_syms.shm.has_shm_extension_ = false;
        }
    }

    memcpy(o, &g_libxcb_syms, sizeof(struct libxcb));
    *(i32 *)&o->handleno_ = g_n_active_handles; /* Cast away const */
    g_n_active_handles++;

end:
    p_mt_mutex_unlock(&g_mutex);
    *(bool *)&o->failed_ = ret != 0; /* Cast away const */
    return ret;
}

void libxcb_unload(struct libxcb *xcb)
{
    if (xcb == NULL || xcb->failed_) return;
    p_mt_mutex_lock(&g_mutex);

    if (g_n_active_handles == ERROR_MAGIC) goto end;
    else if (g_n_active_handles <= 0) goto end;

    /* Sanity check */
    if (xcb->handleno_ >= g_n_active_handles) {
        s_log_error("%s: Invalid libxcb handle. Not unloading!", __func__);
        goto end;
    }

    if (--g_n_active_handles == 0) {
        /* `p_librtld_close` automatically handles invalid arguments,
         * so we don't have to check if the handles are NULL */
        p_librtld_close(&g_libxcb_lib);
        p_librtld_close(&g_libxcb_icccm_lib);
        p_librtld_close(&g_libxcb_image_lib);
        p_librtld_close(&g_libxcb_input_lib);
        p_librtld_close(&g_libxcb_keysyms_lib);
        p_librtld_close(&g_libxcb_shm_lib);

        memset(&g_libxcb_syms, 0, sizeof(struct libxcb));
    }

end:
    p_mt_mutex_unlock(&g_mutex);
}
