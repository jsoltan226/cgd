#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <platform/ptime.h>
#include <platform/event.h>
#include <platform/opengl.h>
#include <platform/window.h>
#include <platform/librtld.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>

#define MODULE_NAME "render-test"
#include "log-util.h"

#define WINDOW_TITLE ("render-test")
#define WINDOW_AREA (&(const rect_t) { 0, 0, 500, 500 })
#define WINDOW_FLAGS (P_WINDOW_POS_CENTERED_XY | P_WINDOW_NO_ACCELERATION)

static struct p_window *win = NULL;
static struct p_opengl_ctx *gl_ctx = NULL;

static i32 opengl_test(void);
static i32 software_test(void);

static struct pixel_flat_data * software_perform_swap_and_wait(
    struct p_window *win, struct pixel_flat_data *old_back_buf, i32 index
);
static i32 opengl_perform_swap_and_wait(struct p_window *win);

int cgd_main(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    if (test_log_setup())
        return EXIT_FAILURE;

    win = p_window_open(WINDOW_TITLE, WINDOW_AREA, WINDOW_FLAGS);
    if (win == NULL)
        goto_error("Failed to open the window. Stop.");
    p_time_sleep(3);

    if (software_test()) goto err;
    if (opengl_test()) goto err;

    s_log_info("Closing the window...");
    p_window_close(&win);

    return EXIT_SUCCESS;
err:
    if (gl_ctx != NULL) p_opengl_destroy_context(&gl_ctx, win);
    if (win != NULL) p_window_close(&win);
    return EXIT_FAILURE;
}

static i32 opengl_test(void)
{
    s_log_info("Initializing OpenGL...");
    if (p_window_set_acceleration(win, P_WINDOW_ACCELERATION_OPENGL))
        goto_error("Failed to set the window acceleration to OpenGL");

    gl_ctx = p_opengl_create_context(win);
    if (gl_ctx == NULL)
        goto_error("Failed to create the OpenGL context. Stop.");
    p_time_sleep(3);

    struct p_opengl_functions GL = { 0 };
    p_opengl_get_functions(gl_ctx, &GL);

    /* Fill the window with black */
    GL.glViewport(0, 0, WINDOW_AREA->w, WINDOW_AREA->h);
    GL.glClearColor(0.0f, 0.f, 0.f, 0.f);
    GL.glClear(GL_COLOR_BUFFER_BIT);
    (void) opengl_perform_swap_and_wait(win);

    /* Fill the window with red */
    GL.glViewport(0, 0, WINDOW_AREA->w, WINDOW_AREA->h);
    GL.glClearColor(1.0f, 0.f, 0.f, 0.f);
    GL.glClear(GL_COLOR_BUFFER_BIT);
    (void) opengl_perform_swap_and_wait(win);

    /* Fill the window with green */
    GL.glViewport(0, 0, WINDOW_AREA->w, WINDOW_AREA->h);
    GL.glClearColor(0.0f, 1.f, 0.f, 0.f);
    GL.glClear(GL_COLOR_BUFFER_BIT);
    (void) opengl_perform_swap_and_wait(win);

    s_log_info("Unloading OpenGL...");
    p_opengl_destroy_context(&gl_ctx, win);

    return 0;

err:
    return 1;
}

static i32 software_test(void)
{
    s_log_info("Initializing the software rendering test...");
    struct p_window_info win_info = { 0 };
    p_window_get_info(win, &win_info);
    if (win_info.gpu_acceleration != P_WINDOW_ACCELERATION_NONE) {
        s_log_verbose("Setting window acceleration to software...");
        if (p_window_set_acceleration(win, P_WINDOW_ACCELERATION_NONE)) {
            s_log_error("Failed to set the window's acceleration to software");
            return -1;
        }
    } else {
        s_log_verbose("Acceleration already set to software");
    }

    i32 i = 0;
    struct pixel_flat_data *back_buf = NULL, *old_back_buf = NULL;
    u64 size = 0;

    back_buf = software_perform_swap_and_wait(win, old_back_buf, i);
    i++;

    /* Fill the buffer with white pixels */
    if (back_buf) {
        size = back_buf->w * back_buf->h * sizeof(pixel_t);
        memset(back_buf->buf, (u8)255, size);
        old_back_buf = back_buf;
    }
    back_buf = software_perform_swap_and_wait(win, old_back_buf, i);
    i++;

    /* Fill the buffer with black pixels */
    if (back_buf) {
        memset(back_buf->buf, (u8)0, size);
        old_back_buf = back_buf;
    }
    back_buf = software_perform_swap_and_wait(win, old_back_buf, i);
    i++;

    /* Fill just the first half of the buffer with white pixels */
    if (back_buf) {
        memset(back_buf->buf, (u8)0, size);
        memset(back_buf->buf, (u8)255, size / 2);
        old_back_buf = back_buf;
    }
    back_buf = software_perform_swap_and_wait(win, old_back_buf, i);
    i++;

    /* Fill the second first half of the buffer with white pixels */
    if (back_buf) {
        memset(back_buf->buf, (u8)255, size);
        memset(back_buf->buf, (u8)0, size / 2);
        old_back_buf = back_buf;
    }
    back_buf = software_perform_swap_and_wait(win, old_back_buf, i);
    i++;

    if (back_buf == NULL) {
        s_log_error("All buffer swaps failed!");
        return 1;
    }
    return 0;
}

static struct pixel_flat_data * software_perform_swap_and_wait(
    struct p_window *win, struct pixel_flat_data *old_back_buf, i32 index
)
{
    struct pixel_flat_data *ret =
        p_window_swap_buffers(win, P_WINDOW_PRESENT_NOW);
    if (ret == P_WINDOW_SWAP_BUFFERS_FAIL) {
        s_log_error("Failed to swap buffers (%d)", index);
        return old_back_buf;
    }

    /* Wait for the page flip event */
    p_time_sleep(1);
    struct p_event ev = { 0 };
    while (p_event_poll(&ev)) {
        if (ev.type == P_EVENT_PAGE_FLIP) {
            s_log_verbose("Received page flip event, status: %u",
                ev.info.page_flip_status);
        }
    }

    return ret;
}

static i32 opengl_perform_swap_and_wait(struct p_window *win)
{
    p_opengl_swap_buffers(gl_ctx, win);
    struct pixel_flat_data *ret =
        p_window_swap_buffers(win, P_WINDOW_PRESENT_NOW);
    if (ret == P_WINDOW_SWAP_BUFFERS_FAIL) {
        s_log_error("Failed to swap buffers");
        return 1;
    }

    /* Wait for the page flip event */
    p_time_sleep(1);
    struct p_event ev = { 0 };
    while (p_event_poll(&ev)) {
        if (ev.type == P_EVENT_PAGE_FLIP) {
            s_log_verbose("Received page flip event, status: %u",
                ev.info.page_flip_status);
        }
    }

    return 0;
}
