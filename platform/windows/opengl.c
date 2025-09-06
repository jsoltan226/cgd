#define P_OPENGL_LOADER_INTERNAL_GUARD__
#include "../opengl.h"
#undef P_OPENGL_LOADER_INTERNAL_GUARD__
#include "../window.h"
#include "../librtld.h"
#define P_INTERNAL_GUARD__
#include "window-internal.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "error.h"
#undef P_INTERNAL_GUARD__
#include <core/log.h>
#include <core/util.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <winnt.h>
#include <wingdi.h>
#include <windef.h>

#define MODULE_NAME "opengl"

#define WGL_LIB_NAME "opengl32"
#define WGL_FUNCTIONS_LIST                                                  \
    X_(HGLRC, wglCreateContext, HDC dc)                                     \
    X_(BOOL, wglDeleteContext, HGLRC glContext)                             \
    X_(PROC, wglGetProcAddress, LPCSTR procName)                            \
    X_(BOOL, wglMakeCurrent, HDC dc, HGLRC glContext)                       \

#define X_(ret_type, name, ...) union { \
    ret_type (*name)(__VA_ARGS__);      \
    void *_voidp_##name;                \
};

struct wgl_functions {
    WGL_FUNCTIONS_LIST
};

#undef X_

#define X_(ret_type, name, ...) #name,
static const char *const wgl_symnames[] = {
    WGL_FUNCTIONS_LIST
    NULL
};
#undef X_

struct p_opengl_ctx {
    bool initialized_;

    HDC window_dc;

    i32 pixel_format_handle;

    struct p_lib *wgl_lib;
    struct wgl_functions wgl;

    HGLRC wgl_ctx;

    struct p_opengl_functions opengl_functions;

    bool made_current;
};

static i32 set_dc_pixel_format(HDC window_dc, i32 *o_pixel_format);

static i32 load_wgl(struct p_lib **o_wgl_lib, struct wgl_functions *o_wgl);

static void * load_opengl_function(
    struct p_lib *opengl32_lib,
    PROC (*wglGetProcAddress)(LPCSTR procName),
    const char *proc_name
);

static i32 load_all_opengl_functions(
    struct p_opengl_functions *o,
    struct p_lib *opengl32_lib,
    PROC (*wglGetProcAddress)(LPCSTR procName)
);

struct p_opengl_ctx * p_opengl_create_context(struct p_window *win)
{
    u_check_params(win != NULL && atomic_load(&win->init_ok_));
    struct p_window_info win_info;
    p_window_get_info(win, &win_info);
    if (win_info.gpu_acceleration != P_WINDOW_ACCELERATION_OPENGL)
        goto_error("Window acceleration not set to OpenGL!");

    struct p_opengl_ctx *ctx = calloc(1, sizeof(struct p_opengl_ctx));
    s_assert(ctx != NULL, "calloc failed for new opengl context");

    ctx->initialized_ = true;

    ctx->window_dc = GetDC(win->win_handle);
    if (ctx->window_dc == NULL) {
        goto_error("Couldn't get the window's device context: %s",
            get_last_error_msg());
    }

    if (set_dc_pixel_format(ctx->window_dc, &ctx->pixel_format_handle))
        goto_error("Failed to set the window pixel format");

    if (load_wgl(&ctx->wgl_lib, &ctx->wgl))
        goto_error("Failed to load the Window GL library (opengl32)");

    ctx->wgl_ctx = ctx->wgl.wglCreateContext(ctx->window_dc);
    if (ctx->wgl_ctx == NULL)
        goto_error("Failed to create the WGL context: %s",
            get_last_error_msg());

    if (load_all_opengl_functions(&ctx->opengl_functions,
            ctx->wgl_lib, ctx->wgl.wglGetProcAddress)
    ) {
        goto_error("Failed to load the OpenGL functions");
    }

    ctx->made_current = false;
    if (!ctx->wgl.wglMakeCurrent(ctx->window_dc, ctx->wgl_ctx))
        goto_error("Failed to bind the OpenGL context "
            "to the current thread: %s", get_last_error_msg());
    ctx->made_current = true;

    /* Render 1 empty frame */
    ctx->opengl_functions.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ctx->opengl_functions.glClear(GL_COLOR_BUFFER_BIT);
    p_opengl_swap_buffers(ctx, win);
    if (p_window_swap_buffers(win, P_WINDOW_PRESENT_NOW)
         == P_WINDOW_SWAP_BUFFERS_FAIL)
    {
        goto_error("Failed to present the first frame");
    }

    return ctx;

err:
    p_opengl_destroy_context(&ctx, win);
    return NULL;
}

void p_opengl_get_functions(struct p_opengl_ctx *ctx,
    struct p_opengl_functions *o)
{
    u_check_params(ctx != NULL && o != NULL && ctx->initialized_);
    memcpy(o, &ctx->opengl_functions, sizeof(struct p_opengl_functions));
}

void p_opengl_swap_buffers(struct p_opengl_ctx *ctx, struct p_window *win)
{
    (void) win;
    u_check_params(ctx != NULL && ctx->made_current);
    if (!SwapBuffers(ctx->window_dc))
        s_log_error("Failed to swap buffers: %s", get_last_error_msg());
}

void p_opengl_destroy_context(struct p_opengl_ctx **ctx_p,
    struct p_window *win)
{
    if (ctx_p == NULL || *ctx_p == NULL || win == NULL)
        return;

    struct p_opengl_ctx *const ctx = *ctx_p;
    if (!ctx->initialized_)
        return;

    if (ctx->made_current) {
        s_assert(ctx->wgl._voidp_wglMakeCurrent != NULL, "impossible outcome");
        if (!ctx->wgl.wglMakeCurrent(NULL, NULL))
            s_log_error("Failed to unbind the WGL context: %s",
                get_last_error_msg());

        ctx->made_current = false;
    }

    memset(&ctx->opengl_functions, 0, sizeof(struct p_opengl_functions));

    if (ctx->wgl_ctx != NULL) {
        s_assert(ctx->wgl._voidp_wglDeleteContext != NULL,
            "impossible outcome");
        if (!ctx->wgl.wglDeleteContext(ctx->wgl_ctx))
            s_log_error("Failed to delete the WGL context: %s",
                get_last_error_msg());

        ctx->wgl_ctx = NULL;
    }

    memset(&ctx->wgl, 0, sizeof(struct wgl_functions));

    if (ctx->wgl_lib != NULL)
        p_librtld_close(&ctx->wgl_lib);

    ctx->pixel_format_handle = 0;

    if (ctx->window_dc != NULL) {
        if (ReleaseDC(win->win_handle, ctx->window_dc) == 0)
            s_log_error("The window DC was not released.");

        ctx->window_dc = NULL;
    }

    ctx->initialized_ = false;
}

static i32 set_dc_pixel_format(HDC window_dc, i32 *o_pixel_format)
{
    const PIXELFORMATDESCRIPTOR pfd = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,

        /* A set of flags that specify the properties of this PFD.
         * We need to 1. Support OpenGL, 2. Be able to render it to a window,
         * and 3. have it double-buffered.
         *
         * The rest of the flags are related to whether or not we specifically
         * want to use the GDI implementation (software) or
         * hardware-accelerated graphics drivers,
         * as well as some other stuff we really don't need or want. */
        .dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,

        /* self-explanatory. the only other alternative value of this field
         * is `PFD_TYPE_COLORINDEX` (for use with color palettes),
         * which we obviously don't want. */
        .iPixelType = PFD_TYPE_RGBA,

        /* For RGBA pixel types (our case), the size of the color buffer */
        .cColorBits = 32,

        /* Number of bits for the depth buffer (Z-buffer) */
        .cDepthBits = 24,

        /* Number of bits for the stencil buffer */
        .cStencilBits = 8,

        /* Specifies the layer type. Unused by modern OpenGL implementations,
         * but it doesn't hurt to set it regardless */
        .iLayerType = PFD_MAIN_PLANE,

        /* All the other fields should be initialized to 0 because
         * 1. They are unused by `ChoosePixelFormat` or
         * 2. They have a description like the following:
         *
         * " `cAlphaBits`
         * Specifies the number of alpha bitplanes in each RGBA color buffer.
         * Alpha bitplanes are not supported.
         * "
         *
         * lol.
         */
        0
    };

    *o_pixel_format = ChoosePixelFormat(window_dc, &pfd);
    if (*o_pixel_format == 0)
        goto_error("ChoosePixelFormat failed: %s", get_last_error_msg());

    if (!SetPixelFormat(window_dc, *o_pixel_format, &pfd))
        goto_error("Couldn't set the pixel format: %s", get_last_error_msg());

    return 0;

err:
    *o_pixel_format = 0;
    return 1;
}

static i32 load_wgl(struct p_lib **o_wgl_lib, struct wgl_functions *o_wgl)
{
    *o_wgl_lib = p_librtld_load(WGL_LIB_NAME, wgl_symnames);
    if (*o_wgl_lib == NULL)
        goto_error("Failed to load the WGL library");

#define X_(ret_type, name, ...) \
    o_wgl->_voidp_##name = p_librtld_load_sym(*o_wgl_lib, #name);   \
    if (o_wgl->_voidp_##name == NULL)                               \
        goto_error("Failed to load WGL function \"%s\"", #name);    \

    WGL_FUNCTIONS_LIST
#undef X_

    return 0;

err:
    return 1;
}

static void * load_opengl_function(
    struct p_lib *opengl32_lib,
    PROC (*wglGetProcAddress)(LPCSTR procName),
    const char *proc_name
)
{
    /* First, try to find the function directly in opengl32.dll */
    union { void *vp; PROC fp; } sym = { .vp = NULL };

    sym.vp = p_librtld_load_sym(opengl32_lib, proc_name);
    if (sym.vp != NULL) {
        s_log_trace("Successfully loaded GL function \"%s\" from opengl32.dll",
            proc_name);
        return sym.vp;
    }

    /* If it's not found there, use `wglGetProcAddress` to get it */
    sym.fp = wglGetProcAddress(proc_name);
    if (sym.vp != NULL) {
        s_log_trace("Successfully loaded GL function \"%s\" "
            "with wglGetProcAddress", proc_name);
        return sym.vp;
    }

    /* Only if that fails, return NULL */
    return NULL;
}

static i32 load_all_opengl_functions(
    struct p_opengl_functions *o,
    struct p_lib *opengl32_lib,
    PROC (*wglGetProcAddress)(LPCSTR procName)
)
{
    u32 n_loaded = 0;

#define X_(return_type, name, ...)                                      \
    o->_voidp_##name =                                                  \
        load_opengl_function(opengl32_lib, wglGetProcAddress, #name);   \
    if (o->_voidp_##name == NULL) {                                     \
        s_log_error("Failed to load OpenGL function %s", #name);        \
        return 1;                                                       \
    }                                                                   \
    n_loaded++;

    P_OPENGL_FUNCTIONS_LIST

#undef X_

    s_log_verbose("Successfully loaded %u OpenGL function(s).", n_loaded);

    return 0;
}

#undef WGL_FUNCTIONS_LIST
