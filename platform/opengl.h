#ifndef P_OPENGL_LOAD_H_
#define P_OPENGL_LOAD_H_

#include "window.h"
#include <core/int.h>
#include <GL/gl.h>

#define P_OPENGL_FUNCTIONS_LIST                                             \
    X_(void, glViewport, GLint x, GLint y, GLsizei width, GLsizei height)   \
    X_(void, glClearColor,                                                  \
        GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha         \
    )                                                                       \
    X_(void, glClear, GLbitfield mask)                                      \

struct p_opengl_ctx;

#define X_(return_type, name, ...) \
    union { return_type (*name)(__VA_ARGS__); void *_voidp_##name; };
struct p_opengl_functions {
    P_OPENGL_FUNCTIONS_LIST
};
#undef X_

/* Make these defines only visible to the OpenGL Loader sources */
#ifndef P_OPENGL_LOADER_INTERNAL_GUARD__
#undef P_OPENGL_FUNCTIONS_LIST
#endif /* P_OPENGL_LOADER_INTERNAL_GUARD__ */

/* Create an OpenGL context and bind it to `win`.
 * Returns a handle to a new OpenGL context, or `NULL` on failure.
 *
 * Note: You must first set the window's acceleration
 * to `P_WINDOW_ACCELERATION_OPENGL` (see `p_window_set_acceleration`) */
struct p_opengl_ctx * p_opengl_create_context(struct p_window *win);

/* Retrieve the OpenGL function pointers from `ctx` into `o`. */
void p_opengl_get_functions(struct p_opengl_ctx *ctx,
    struct p_opengl_functions *o);

/* Swap the buffers in `ctx` and perform a page flip on the window `win`.
 * Note that `win` must be the same window with which `ctx` was created. */
void p_opengl_swap_buffers(struct p_opengl_ctx *ctx, struct p_window *win);

/* Destroy (and unbind) the context that `ctx_p` points to,
 * and set the value of `*ctx_p` to NULL. */
void p_opengl_destroy_context(struct p_opengl_ctx **ctx_p,
    struct p_window *win);

#endif /* OPENGL_LOAD_H_ */
