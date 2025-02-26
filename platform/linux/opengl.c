#define _GNU_SOURCE
#define P_OPENGL_LOADER_INTERNAL_GUARD__
#include "../opengl.h"
#undef P_OPENGL_LOADER_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-internal.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-dri.h"
#undef P_INTERNAL_GUARD__
#include "../window.h"
#include "../librtld.h"
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <core/vector.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

#define MODULE_NAME "opengl"

#define EGL_FUNCTION_LIST                                                   \
    X_(EGLDisplay, eglGetDisplay, EGLNativeDisplayType display_id)          \
    X_(EGLDisplay, eglGetPlatformDisplay,                                   \
        EGLenum platform, void *native_display, const EGLAttrib *attr_list  \
    )                                                                       \
    X_(EGLBoolean, eglInitialize,                                           \
        EGLDisplay display, EGLint *major, EGLint *minor                    \
    )                                                                       \
    X_(EGLBoolean, eglTerminate, EGLDisplay display)                        \
    X_(EGLBoolean, eglBindAPI, EGLenum api)                                 \
    X_(EGLContext, eglCreateContext,                                        \
        EGLDisplay dpy, EGLConfig config, EGLContext share_context,         \
        const EGLint *attrib_list                                           \
    )                                                                       \
    X_(EGLint, eglGetError, void)                                           \
    X_(EGLBoolean, eglChooseConfig,                                         \
        EGLDisplay dpy, const EGLint *attrib_list,                          \
        EGLConfig *configs, EGLint config_size, EGLint *num_config          \
    )                                                                       \
    X_(EGLBoolean, eglMakeCurrent,                                          \
        EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx    \
    )                                                                       \
    X_(EGLSurface, eglCreatePlatformWindowSurface,                          \
        EGLDisplay dpy, EGLConfig config, void * native_window,             \
        EGLAttrib const *attrib_list                                        \
    )                                                                       \
    X_(EGLBoolean, eglDestroySurface, EGLDisplay dpy, EGLSurface surface)   \
    X_(const char *, eglQueryString, EGLDisplay display, EGLint name)       \
    X_(EGLBoolean, eglSwapBuffers, EGLDisplay dpy, EGLSurface surface)      \


#define X_(return_type, name, ...) return_type (*name)(__VA_ARGS__);
struct egl_functions {
    EGL_FUNCTION_LIST
    void * (*eglGetProcAddress)(const char *procName);
};
#undef X_

struct p_opengl_ctx {
    struct p_opengl_functions functions;

    /* Only used in `p_opengl_swap_buffers` and `p_opengl_destroy_context`
     * to check that the provided window is the same with which the context
     * was created, so it can be read-only */
    const struct p_window *bound_win;

    struct egl_ctx {
        struct p_lib *lib;
        struct egl_functions fns;
        EGLDisplay dpy;
        i32 ver_major, ver_minor;
        EGLSurface surface;
        EGLContext ctx;
    } egl;
};

static i32 init_egl(struct egl_ctx *egl, struct p_window *win);
static void terminate_egl(struct egl_ctx *egl);

static i32 check_egl_support(const struct egl_ctx *egl, enum window_bit *o);

static const char * egl_get_last_error(const struct egl_ctx *egl);
static i32 egl_get_native_platform_and_display(const struct egl_ctx *egl,
    const struct p_window *win, const enum window_bit supported_windows,
    EGLenum *platform_o, u64 *display_o);
static i32 egl_get_native_window(const struct egl_ctx *egl,
    const struct p_window *win, u64 *o);

static i32 load_opengl_functions(struct p_opengl_functions *o,
    void * (*eglGetProcAddress)(const char *procName));

struct p_opengl_ctx * p_opengl_create_context(struct p_window *win)
{
    struct p_opengl_ctx *ctx = calloc(1, sizeof(struct p_opengl_ctx));
    s_assert(ctx != NULL, "calloc() failed for new OpenGL context");

    ctx->bound_win = win;
    if (p_window_set_acceleration(win, P_WINDOW_ACCELERATION_OPENGL))
        goto_error("Failed to set window acceleration to OpenGL.");

    if (init_egl(&ctx->egl, win))
        goto_error("Failed to initialize EGL.");

    if (load_opengl_functions(&ctx->functions, ctx->egl.fns.eglGetProcAddress))
        goto_error("Failed to load OpenGL functions.");

    return ctx;

err:
    p_opengl_destroy_context(&ctx, win);
    return NULL;
}

i32 p_opengl_get_functions(struct p_opengl_ctx *ctx,
    struct p_opengl_functions *o)
{
    if (ctx == NULL || o == NULL) return -1;

    memcpy(o, &ctx->functions, sizeof(struct p_opengl_functions));
    return 0;
}

void p_opengl_swap_buffers(struct p_opengl_ctx *ctx, struct p_window *win)
{
    u_check_params(ctx != NULL && win != NULL);
    if (ctx->bound_win != win) {
        s_log_error("Attempt to swap buffers with the wrong window.");
        return;
    }

    EGLBoolean r = ctx->egl.fns.eglSwapBuffers(ctx->egl.dpy, ctx->egl.surface);
    s_assert(r != EGL_FALSE, "Failed to swap buffers: %s",
        egl_get_last_error(&ctx->egl));

    switch (ctx->bound_win->type) {
    case WINDOW_TYPE_X11: break;
    case WINDOW_TYPE_DRI:
        window_dri_set_egl_buffers_swapped(&win->dri);
        break;
    case WINDOW_TYPE_FBDEV:
    case WINDOW_TYPE_DUMMY:
        break; /* Not supported */
    }
}

void p_opengl_destroy_context(struct p_opengl_ctx **ctx_p,
    struct p_window *win)
{
    if (ctx_p == NULL || *ctx_p == NULL) return;
    struct p_opengl_ctx *ctx = *ctx_p;

    if (ctx->bound_win != win) {
        s_log_error("Attempt to destroy context with the wrong window.");
        return;
    }

    terminate_egl(&ctx->egl);

    if (ctx->bound_win != NULL)
        p_window_set_acceleration(win, P_WINDOW_ACCELERATION_NONE);

    memset(ctx, 0, sizeof(struct p_opengl_ctx));
    u_nfree(ctx_p);
}

static i32 init_egl(struct egl_ctx *egl, struct p_window *win)
{
    memset(egl, 0, sizeof(struct egl_ctx));

    egl->lib = p_librtld_load_lib_explicit("libEGL", NULL, NULL, NULL);
    if (egl->lib == NULL)
        goto_error("Failed to load the EGL library.");

    egl->fns.eglGetProcAddress =
        p_librtld_load_sym(egl->lib, "eglGetProcAddress");
    if (egl->fns.eglGetProcAddress == NULL)
        goto_error("Failed to load the eglGetProcAddress function.");

#define X_(return_type, name, ...)                              \
    egl->fns.name = egl->fns.eglGetProcAddress(#name);          \
    if (egl->fns.name == NULL)                                  \
        goto_error("Failed to load EGL function %s", #name);    \

    EGL_FUNCTION_LIST
#undef X_

    enum window_bit supported_window_types = 0;
    if (check_egl_support(egl, &supported_window_types))
        goto_error("The EGL implementation doen't support required features.");

    EGLenum platform;
    u64 native_dpy;
    if (egl_get_native_platform_and_display(egl, win, supported_window_types,
            &platform, &native_dpy))
        goto_error("No supported EGL native display for \"%s\".",
            window_type_strings[win->type]);

    egl->dpy = egl->fns.eglGetPlatformDisplay(platform,
        (void *)native_dpy, NULL);
    if (egl->dpy == EGL_NO_DISPLAY)
        goto_error("Couldn't get the EGL display.");

    if (!egl->fns.eglInitialize(egl->dpy, &egl->ver_major, &egl->ver_minor))
        goto_error("Failed to initialize the EGL display connection: %s",
            egl_get_last_error(egl));

    EGLConfig cfg;
    EGLint n_cfgs;
    const EGLint cfg_attr_list[] = {
        EGL_CONFORMANT, EGL_OPENGL_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_BUFFER_SIZE, 32,
        EGL_NONE
    };
    if (!egl->fns.eglChooseConfig(egl->dpy, cfg_attr_list, &cfg, 1, &n_cfgs))
        goto_error("Failed to choose EGL config.");

    if (egl->fns.eglBindAPI(EGL_OPENGL_API) == 0)
        goto_error("Failed to select OpenGL as the EGL API.");

    u64 native_window;
    if (egl_get_native_window(egl->ctx, win, &native_window))
        goto_error("No supported EGL native window for \"%s\"",
            window_type_strings[win->type]);

    egl->surface = egl->fns.eglCreatePlatformWindowSurface(egl->dpy,
        cfg, (void *)native_window, NULL);
    if (egl->surface == EGL_NO_SURFACE)
        goto_error("Failed to create EGL surface: %s", egl_get_last_error(egl));

    EGLint ctx_attr_list[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 3,
        EGL_NONE,
    };
    egl->ctx = egl->fns.eglCreateContext(egl->dpy, cfg,
        EGL_NO_CONTEXT, ctx_attr_list);
    if (egl->ctx == EGL_NO_CONTEXT)
        goto_error("Failed to create the EGL context: %s",
            egl_get_last_error(egl));

    if (egl->fns.eglMakeCurrent(egl->dpy,
            egl->surface, egl->surface, egl->ctx) == 0)
        goto_error("eglMakeCurrent failed (%s)", egl_get_last_error(egl));

    s_log_debug("Initialized EGL (version %i.%i)",
        egl->ver_major, egl->ver_minor);
    return 0;

err:
    /* `terminate_egl` will be called by `p_opengl_destroy_context` */
    return 1;
}

static void terminate_egl(struct egl_ctx *egl)
{
    if (egl == NULL) return;

    if (egl->surface != EGL_NO_SURFACE && egl->fns.eglDestroySurface != NULL)
        (void) egl->fns.eglDestroySurface(egl->dpy, egl->surface);

    if (egl->dpy != EGL_NO_DISPLAY && egl->fns.eglTerminate != NULL)
        (void) egl->fns.eglTerminate(egl->dpy);

    if (egl->lib != NULL)
        p_librtld_close(&egl->lib);

    memset(egl, 0, sizeof(struct egl_ctx));
    egl->dpy = EGL_NO_DISPLAY;
    egl->surface = EGL_NO_SURFACE;
}

static i32 check_egl_support(const struct egl_ctx *egl, enum window_bit *o)
{
    *o = 0;
    const char *exts = egl->fns.eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (exts == NULL) {
        s_log_error("Failed to get the list of supported EGL extensions: %s",
            egl_get_last_error(egl));
        return 1;
    }

    if (strstr(exts, "EGL_EXT_platform_xcb"))
        *o |= WINDOW_BIT_X11;

    if (strstr(exts, "EGL_KHR_platform_gbm"))
        *o |= WINDOW_BIT_DRI;

    if (*o == 0) {
        s_log_error("No window types are supported "
            "by this EGL implementation.");
        return 1;
    }

    return 0;
}

static const char * egl_get_last_error(const struct egl_ctx *egl)
{
    u_check_params(egl != NULL && egl->fns.eglGetError != NULL);

#define EGL_ERROR_LIST_         \
    X_(EGL_SUCCESS)             \
    X_(EGL_NOT_INITIALIZED)     \
    X_(EGL_BAD_ACCESS)          \
    X_(EGL_BAD_ALLOC)           \
    X_(EGL_BAD_ATTRIBUTE)       \
    X_(EGL_BAD_CONTEXT)         \
    X_(EGL_BAD_CONFIG)          \
    X_(EGL_BAD_CURRENT_SURFACE) \
    X_(EGL_BAD_DISPLAY)         \
    X_(EGL_BAD_SURFACE)         \
    X_(EGL_BAD_MATCH)           \
    X_(EGL_BAD_PARAMETER)       \
    X_(EGL_BAD_NATIVE_PIXMAP)   \
    X_(EGL_BAD_NATIVE_WINDOW)   \
    X_(EGL_CONTEXT_LOST)


#define UNKNOWN_ERR_BUF_SIZE 128
    static char unknown_err_buf[128];

    const EGLint error_code = egl->fns.eglGetError();
    switch (error_code) {
#define X_(err_name)            \
        case err_name:          \
            return #err_name;

        EGL_ERROR_LIST_
#undef X_

        default:
            memset(unknown_err_buf, 0, UNKNOWN_ERR_BUF_SIZE);
            snprintf(unknown_err_buf, UNKNOWN_ERR_BUF_SIZE - 1,
                "Unknown EGL error %i", error_code);
            return unknown_err_buf;
    }
}

static i32 egl_get_native_platform_and_display(const struct egl_ctx *egl,
    const struct p_window *win, const enum window_bit supported_windows,
    EGLenum *platform_o, u64 *display_o)
{
    switch (win->type) {
        case WINDOW_TYPE_X11:
            if (!(supported_windows & WINDOW_BIT_X11)) {
                s_log_error(
                    "EGL implementation doesn't support X11/XCB windows!"
                );
                return 1;
            }
            *platform_o = EGL_PLATFORM_XCB_EXT;
            *display_o = (u64)win->x11.conn;
            return 0;
        case WINDOW_TYPE_DRI:
            if (!(supported_windows & WINDOW_BIT_DRI)) {
                s_log_error("EGL implementation doesn't support GBM!");
                return 1;
            }
            *platform_o = EGL_PLATFORM_GBM_KHR;
            *display_o = (u64)win->dri.render.egl.device;
            return 0;
        case WINDOW_TYPE_FBDEV:
            s_log_error("Fbdev windows do not support graphics acceleration!");
            return 1;
        case WINDOW_TYPE_DUMMY:
            s_log_error("Dummy windows do not support graphics acceleration!");
            return 1;
    }

    return 1;
}

static i32 egl_get_native_window(const struct egl_ctx *egl,
    const struct p_window *win, EGLNativeWindowType *o)

{
    switch (win->type) {
        case WINDOW_TYPE_X11:
            /* As it turns out, EGL wants a POINTER to `xcb_window_t` lol */
            *o = (EGLNativeWindowType)&win->x11.win;
            return 0;
        case WINDOW_TYPE_DRI:
            *o = (EGLNativeWindowType)win->dri.render.egl.surface;
            return 0;
        case WINDOW_TYPE_FBDEV:
            s_log_error("Fbdev windows do not support graphics acceleration!");
            return 1;
        case WINDOW_TYPE_DUMMY:
            s_log_error("Dummy windows do not support graphics acceleration!");
            return 1;
    }

    return 1;
}

static i32 load_opengl_functions(struct p_opengl_functions *o,
    void * (*eglGetProcAddress)(const char *procName))
{
    u32 n_loaded = 0;

#define X_(return_type, name, ...)                                  \
    o->name = eglGetProcAddress(#name);                             \
    if (o->name == NULL) {                                          \
        s_log_error("Failed to load OpenGL function %s", #name);    \
        return 1;                                                   \
    }                                                               \
    n_loaded++;

    P_OPENGL_FUNCTIONS_LIST

#undef X_

    s_log_debug("Successfully loaded %u OpenGL function(s).", n_loaded);

    return 0;
}

#undef EGL_FUNCTION_LIST
