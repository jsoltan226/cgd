#include <GL/gl.h>
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <platform/ptime.h>
#include <platform/event.h>
#include <platform/opengl.h>
#include <platform/window.h>
#include <platform/librtld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#define MODULE_NAME "render-test"

#define WINDOW_TITLE ((const unsigned char *)"render-test")
#define WINDOW_AREA (&(const rect_t) { 0, 0, 500, 500 })
#define WINDOW_FLAGS (P_WINDOW_POS_CENTERED_XY)

static FILE *log_fp = NULL;
static struct p_window *win = NULL;
static struct p_opengl_ctx *gl_ctx = NULL;

static __attribute_maybe_unused__ i32 opengl_test(void);
static i32 software_test(void);

static void cleanup_log(void);

int cgd_main(int argc, char **argv)
{
    //log_fp = fopen("log.txt", "wb");
    log_fp = stdout;
    if (log_fp == NULL)
        return EXIT_FAILURE;

    if (atexit(cleanup_log))
        return EXIT_FAILURE;

    s_configure_log(LOG_DEBUG, log_fp, log_fp);

    win = p_window_open(WINDOW_TITLE, WINDOW_AREA, WINDOW_FLAGS);
    if (win == NULL)
        goto_error("Failed to open the window. Stop.");

    if (software_test()) goto err;
    //if (opengl_test()) goto err;

    s_log_info("Closing the window...");
    p_window_close(&win);

    return EXIT_SUCCESS;
err:
    if (gl_ctx != NULL) p_opengl_destroy_context(&gl_ctx);
    if (win != NULL) p_window_close(&win);
    return EXIT_FAILURE;
}

static i32 opengl_test(void)
{
    s_log_info("Initializing OpenGL...");
    gl_ctx = p_opengl_create_context(win);
    if (gl_ctx == NULL)
        goto_error("Failed to create OpenGL context. Stop.");

    struct p_opengl_functions GL = { 0 };
    if (p_opengl_get_functions(gl_ctx, &GL))
        goto_error("Failed to retrieve OpenGL function pointers. Stop.");

    struct p_event ev;

    GL.glViewport(0, 0, WINDOW_AREA->w, WINDOW_AREA->h);
    GL.glClearColor(0.0f, 0.f, 0.f, 0.f);
    GL.glClear(GL_COLOR_BUFFER_BIT);
    p_opengl_swap_buffers(gl_ctx);
    p_window_render(win, NULL);
    p_time_sleep(1);
    while (p_event_poll(&ev)) {
        if (ev.type == P_EVENT_PAGE_FLIP) {
            s_log_debug("Received page flip event, status: %u",
                ev.info.page_flip_status);
        }
    }

    GL.glViewport(0, 0, WINDOW_AREA->w, WINDOW_AREA->h);
    GL.glClearColor(1.0f, 0.f, 0.f, 0.f);
    GL.glClear(GL_COLOR_BUFFER_BIT);
    p_opengl_swap_buffers(gl_ctx);
    p_window_render(win, NULL);
    p_time_sleep(1);
    while (p_event_poll(&ev)) {
        if (ev.type == P_EVENT_PAGE_FLIP) {
            s_log_debug("Received page flip event, status: %u",
                ev.info.page_flip_status);
        }
    }

    GL.glViewport(0, 0, WINDOW_AREA->w, WINDOW_AREA->h);
    GL.glClearColor(0.0f, 1.f, 0.f, 0.f);
    GL.glClear(GL_COLOR_BUFFER_BIT);
    p_opengl_swap_buffers(gl_ctx);
    p_window_render(win, NULL);
    p_time_sleep(1);
    while (p_event_poll(&ev)) {
        if (ev.type == P_EVENT_PAGE_FLIP) {
            s_log_debug("Received page flip event, status: %u",
                ev.info.page_flip_status);
        }
    }

    s_log_info("Unloading OpenGL...");
    p_opengl_destroy_context(&gl_ctx);

    return 0;

err:
    return 1;
}

static i32 software_test(void)
{
    s_log_info("Initializing the software rendering test...");
    struct pixel_flat_data *back_buf = p_window_swap_buffers(win);
    if (back_buf == NULL)
        goto_error("Failed to swap buffers. Stop.");
    const u32 size = back_buf->w * back_buf->h * sizeof(pixel_t);

    struct p_event ev;

    /* Fill the buffer with white pixels */
    memset(back_buf->buf, (u8)255, size);
    back_buf = p_window_swap_buffers(win);
    if (back_buf == NULL)
        goto_error("Failed to swap buffers. Stop.");
    p_time_sleep(1);
    while (p_event_poll(&ev)) {
        if (ev.type == P_EVENT_PAGE_FLIP) {
            s_log_debug("Received page flip event, status: %u",
                ev.info.page_flip_status);
        }
    }

    /* Fill the buffer with black pixels */
    memset(back_buf->buf, (u8)0, size);
    back_buf = p_window_swap_buffers(win);
    if (back_buf == NULL)
        goto_error("Failed to swap buffers. Stop.");
    p_time_sleep(1);
    while (p_event_poll(&ev)) {
        if (ev.type == P_EVENT_PAGE_FLIP) {
            s_log_debug("Received page flip event, status: %u",
                ev.info.page_flip_status);
        }
    }

    /* Fill just the first half of the buffer with white pixels */
    memset(back_buf->buf, (u8)0, size);
    memset(back_buf->buf, (u8)255, size / 2);
    back_buf = p_window_swap_buffers(win);
    if (back_buf == NULL)
        goto_error("Failed to swap buffers. Stop.");
    p_time_sleep(1);
    while (p_event_poll(&ev)) {
        if (ev.type == P_EVENT_PAGE_FLIP) {
            s_log_debug("Received page flip event, status: %u",
                ev.info.page_flip_status);
        }
    }

    /* Fill the second first half of the buffer with white pixels */
    memset(back_buf->buf, (u8)255, size);
    memset(back_buf->buf, (u8)0, size / 2);
    back_buf = p_window_swap_buffers(win);
    if (back_buf == NULL)
        goto_error("Failed to swap buffers. Stop.");
    p_time_sleep(1);
    while (p_event_poll(&ev)) {
        if (ev.type == P_EVENT_PAGE_FLIP) {
            s_log_debug("Received page flip event, status: %u",
                ev.info.page_flip_status);
        }
    }

    return 0;

err:
    return 1;
}

static void cleanup_log(void)
{
    if (log_fp != NULL) {
        s_set_log_out_filep(stdout);
        s_set_log_err_filep(stderr);
        fclose(log_fp);
        log_fp = NULL;
    }
}
