#include "../event.h"
#include "../thread.h"
#include "../window.h"
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WINWIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <winnt.h>
#include <wingdi.h>
#include <winuser.h>
#include <minwindef.h>
#include <processthreadsapi.h>
#define P_INTERNAL_GUARD__
#include "window-internal.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "error.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "global.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-thread.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-present-sw.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "window"

static i32 check_flags(const u32 flags);
static void set_default_flags(u32 *flags);
static i32 init_window_info(struct p_window_info *info,
    const rect_t *area, u32 flags);

static i32 initialize_acceleration(struct p_window *win,
    const enum p_window_flags flags);

struct p_window * p_window_open(const char *title,
    const rect_t *area, u32 flags)
{
    u_check_params(area != NULL && check_flags(flags) == 0);
    set_default_flags(&flags);

    struct p_window *win = calloc(1, sizeof(struct p_window));
    s_assert(win != NULL, "calloc() failed for struct window");

    atomic_store(&win->exists_, true);
    atomic_store(&win->init_ok_, false);

    s_assert(g_instance_handle != NULL,
        "Global instance handle not initialized!");

    /* Populate the info struct */
    if (init_window_info(&win->info, area, flags))
        goto_error("Failed to init the window info struct");

    /* Create the window thread, which will do the rest of the work */
    p_mt_mutex_t init_mutex = p_mt_mutex_create();
    p_mt_cond_t init_cond = p_mt_cond_create();
    struct window_init init;
    p_mt_mutex_lock(&init_mutex);
    {
        /* PARAMS */
        init.in.title = (const char *)title;
        init.in.win_info = &win->info;
        init.in.flags = flags;

        /* Data returned by the thread */
        memset(&init.out, 0, sizeof(init.out));

        /* Temporary thread sync/communication objects */
        init.cond = init_cond,
        init.mutex = init_mutex,

        init.result = -1;
    }
    p_mt_mutex_unlock(&init_mutex);

    /* Create the thread */
    atomic_store(&win->thread_started_, false);
    if (p_mt_thread_create(&win->thread, window_thread_fn, &init)) {
        p_mt_cond_destroy(&init.cond);
        p_mt_mutex_destroy(&init.mutex);
        goto_error("Failed to spawn window thread");
    }
    atomic_store(&win->thread_started_, true);

    /* Wait for the thread to complete window initialization */
    p_mt_mutex_lock(&init_mutex);
    {
        while (init.result == -1)
            p_mt_cond_wait(init_cond, init_mutex);
    }
    p_mt_mutex_unlock(&init.mutex);

    /* Clean up temporary objects */
    p_mt_cond_destroy(&init.cond);
    p_mt_mutex_destroy(&init.mutex);

    /* Check the results */
    if (init.result != 0)
        goto_error("Window init failed.");

    atomic_store(&win->init_ok_, true);

    win->win_handle = init.out.win_handle;
    memcpy(&win->window_rect, &init.out.window_rect, sizeof(RECT));

    /* Initialize anything acceleration-specific */
    if (initialize_acceleration(win, flags))
        goto_error("Failed to initialize window acceleration");

    /* Initialize the event queue */
    p_event_send(&(const struct p_event) { .type = P_EVENT_CTL_INIT_ });

    s_log_verbose("%s() OK; window position: [%i, %i]; window dimensions: %ux%u",
        __func__, rect_arg_expand(win->info.client_area));

    return win;

err:
    p_window_close(&win);
    return NULL;
}

struct pixel_flat_data * p_window_swap_buffers(struct p_window *win,
    const enum p_window_present_mode present_mode)
{
    u_check_params(win != NULL);

    switch (win->info.gpu_acceleration) {
    case P_WINDOW_ACCELERATION_NONE:
        if (present_mode != P_WINDOW_PRESENT_NOW) {
            s_log_error("Software-rendered windows don't yet support VSync");
            return NULL;
        }

        return render_software_swap_buffers_and_request_presentation(
                win->win_handle, &win->render.sw
        );
    case P_WINDOW_ACCELERATION_OPENGL:
    case P_WINDOW_ACCELERATION_VULKAN:
        s_log_error("Vulkan & OpenGL acceleration aren't yet implemented "
            "on the Windows platform.");
        return NULL;
    case P_WINDOW_ACCELERATION_UNSET_:
        s_log_error("Attempt to swap buffers "
            "of a window with unset acceleration");
        return NULL;
    default:
        s_log_fatal("Invalid window acceleration mode (%d)",
            win->info.gpu_acceleration);
    }
}

void p_window_close(struct p_window **win_p)
{
    if (win_p == NULL || *win_p == NULL || !atomic_load(&(*win_p)->exists_))
        return;

    struct p_window *win = *win_p;

    s_log_verbose("Destroying window...");

    if (atomic_exchange(&win->thread_started_, false) ||
        !atomic_load(&win->init_ok_))
    {
        p_window_set_acceleration(win, P_WINDOW_ACCELERATION_UNSET_);

        if (PostMessage(win->win_handle, CGD_WM_EV_QUIT_, 0, 0) == 0) {
            /* The window is OK, but we could't send it the message
             * to exit gracefully by itself, so we have reach out for
             * more drastic measures (or else the execution will deadlock here)
             */
            s_log_error("Failed to send the quit interrupt event "
                    "to the listener thread: %s", get_last_error_msg());
            p_mt_thread_terminate(&win->thread);
        } else {
            /* The normal case (window is OK and we
             * successfully sent it the termination message) */
            p_mt_thread_wait(&win->thread);
        }

        win->win_handle = NULL;
    } else {
        win->win_handle = NULL;

        /* Window init failed
         * (the thread should have already exited by itself) */
        p_mt_thread_wait(&win->thread);
    }

    s_log_verbose("terminated thread & window");

    /* Destroy the event queue */
    p_event_send(&(const struct p_event) { .type = P_EVENT_CTL_DESTROY_ });

    /* Clean up the rest */
    memset(&win->info, 0, sizeof(struct p_window_info));
    memset(&win->window_rect, 0, sizeof(RECT));
    atomic_store(&win->exists_, false);

}

void p_window_get_info(const struct p_window *win, struct p_window_info *out)
{
    u_check_params(win != NULL && out != NULL && atomic_load(&win->exists_));
    memcpy(out, &win->info, sizeof(struct p_window_info));
}

i32 p_window_set_acceleration(struct p_window *win,
    enum p_window_acceleration new_acceleration_mode)
{
    u_check_params(win != NULL && atomic_load(&win->exists_));
    /* `new_acceleration_mode` checked later */

    const enum p_window_acceleration old_acceleration =
        win->info.gpu_acceleration;
    win->info.gpu_acceleration = P_WINDOW_ACCELERATION_UNSET_;

    switch (old_acceleration) {
    case P_WINDOW_ACCELERATION_UNSET_:
        break;
    case P_WINDOW_ACCELERATION_NONE:
        if (new_acceleration_mode == P_WINDOW_ACCELERATION_NONE) {
            s_log_warn("Attempt to set window acceleration to software, "
                "but it's already enabled. Not doing anything.");
            return 0;
        }
        struct render_destroy_software_req req = { .ctx = &win->render.sw };
        if (window_thread_request_operation_and_wait(win->win_handle,
                REQ_OP_RENDER_DESTROY_SOFTWARE, P_MT_MUTEX_NULL, &req)
        ) {
            s_log_error("The render_destroy_software request failed!");
            return 1;
        }
        break;
    case P_WINDOW_ACCELERATION_OPENGL:
    case P_WINDOW_ACCELERATION_VULKAN:
        s_log_error("Vulkan & OpenGL acceleration aren't yet implemented "
            "on the Windows platform.");
        /* Fall-through */
    default:
        s_log_fatal("Invalid old window acceleration mode (%#x)",
            win->info.gpu_acceleration);
    }

    switch (new_acceleration_mode) {
    case P_WINDOW_ACCELERATION_UNSET_:
        break;
    case P_WINDOW_ACCELERATION_NONE:
        ;
        struct render_init_software_req req = {
            .win_info = &win->info,
            .ctx = &win->render.sw
        };
        if (window_thread_request_operation_and_wait(win->win_handle,
                REQ_OP_RENDER_INIT_SOFTWARE, P_MT_MUTEX_NULL, &req)
        ) {
            s_log_error("Failed to initialize software rendering");
            return 1;
        }
        break;
    case P_WINDOW_ACCELERATION_OPENGL:
    case P_WINDOW_ACCELERATION_VULKAN:
        s_log_error("Vulkan & OpenGL acceleration aren't yet implemented "
            "on the Windows platform.");
        return 1;
    default:
        s_log_fatal("Invalid new acceleration mode (%#x)",
            new_acceleration_mode);
    }

    win->info.gpu_acceleration = new_acceleration_mode;

    return 0;
}


static void set_default_flags(u32 *flags)
{
    const u32 orig_flags = *flags;
#define not_set(flag) (!(orig_flags & flag))

    if (not_set(P_WINDOW_TYPE_NORMAL) && not_set(P_WINDOW_TYPE_DUMMY))
        *flags |= P_WINDOW_TYPE_NORMAL;

    if (not_set(P_WINDOW_VSYNC_SUPPORT_REQUIRED) &&
        not_set(P_WINDOW_VSYNC_SUPPORT_OPTIONAL))
    {
        *flags |= P_WINDOW_VSYNC_SUPPORT_OPTIONAL;
    }

    if (not_set(P_WINDOW_PREFER_ACCELERATED) &&
        not_set(P_WINDOW_REQUIRE_ACCELERATED) &&
        not_set(P_WINDOW_REQUIRE_OPENGL) &&
        not_set(P_WINDOW_REQUIRE_VULKAN) &&
        not_set(P_WINDOW_NO_ACCELERATION))
    {
        *flags |= P_WINDOW_PREFER_ACCELERATED;
    }

#undef not_set

}

static i32 check_flags(const u32 flags)
{
    static const enum p_window_flags contradicting_pairs[][2] = {
        { P_WINDOW_TYPE_NORMAL,  P_WINDOW_TYPE_DUMMY },
        { P_WINDOW_TYPE_DUMMY, P_WINDOW_REQUIRE_ACCELERATED },
        { P_WINDOW_TYPE_DUMMY, P_WINDOW_REQUIRE_VULKAN },
        { P_WINDOW_TYPE_DUMMY, P_WINDOW_REQUIRE_OPENGL },
        { P_WINDOW_PREFER_ACCELERATED, P_WINDOW_REQUIRE_ACCELERATED },
        { P_WINDOW_PREFER_ACCELERATED, P_WINDOW_REQUIRE_OPENGL },
        { P_WINDOW_PREFER_ACCELERATED, P_WINDOW_REQUIRE_VULKAN },
        { P_WINDOW_REQUIRE_ACCELERATED, P_WINDOW_REQUIRE_VULKAN },
        { P_WINDOW_REQUIRE_ACCELERATED, P_WINDOW_REQUIRE_OPENGL },
        { P_WINDOW_REQUIRE_VULKAN, P_WINDOW_REQUIRE_OPENGL },
        { P_WINDOW_VSYNC_SUPPORT_REQUIRED, P_WINDOW_VSYNC_SUPPORT_OPTIONAL },
    };
    i32 ret = 0;
    for (u32 i = 0; i < u_arr_size(contradicting_pairs); i++) {
        const u32 flag_a = contradicting_pairs[i][0];
        const u32 flag_b = contradicting_pairs[i][1];
        if (flags & flag_a && flags & flag_b) {
            i32 index_a = 0;
            i32 index_b = 0;
            while (flag_a != 1U << index_a) index_a++;
            while (flag_b != 1U << index_b) index_b++;

            s_log_error("Mutually exclusive flags: \"%s\" and \"%s\"",
                p_window_flag_strings[index_a], p_window_flag_strings[index_b]);
            ret++;
        }
    }

    return ret;

}

static i32 init_window_info(struct p_window_info *info,
    const rect_t *area, u32 flags)
{
    /* Get screen resolution */
    info->display_rect.w = GetSystemMetrics(SM_CXSCREEN);
    if (info->display_rect.w == 0) {
        s_log_error("Failed to get screen width: %s", get_last_error_msg());
        return 1;
    }

    info->display_rect.h = GetSystemMetrics(SM_CYSCREEN);
    if (info->display_rect.h == 0) {
        s_log_error("Failed to get screen height: %s", get_last_error_msg());
        return 1;
    }

    /* Handle P_WINDOW_POS_CENTERED flags */
    info->client_area.x = area->x;
    if (flags & P_WINDOW_POS_CENTERED_X)
        info->client_area.x = (info->display_rect.w - area->w) / 2;

    info->client_area.y = area->y;
    if (flags & P_WINDOW_POS_CENTERED_Y)
        info->client_area.y = (info->display_rect.h - area->h) / 2;

    info->client_area.w = area->w;
    info->client_area.h = area->h;

    /* Initialize the rest of the info struct */
    info->display_color_format = BGRX32;
    info->gpu_acceleration = P_WINDOW_ACCELERATION_NONE;
    info->vsync_supported = false;

    return 0;
}

static i32 initialize_acceleration(struct p_window *win,
    const enum p_window_flags flags)
{
    /* Decide which acceleration modes to try
     * and which are required to succeed */
    const bool try_vulkan = (flags & P_WINDOW_PREFER_ACCELERATED)
        || (flags & P_WINDOW_REQUIRE_ACCELERATED)
        || (flags & P_WINDOW_REQUIRE_VULKAN);
    const bool warn_vulkan = (flags & P_WINDOW_PREFER_ACCELERATED)
        || (flags & P_WINDOW_REQUIRE_ACCELERATED);
    const bool require_vulkan = (flags & P_WINDOW_REQUIRE_VULKAN) || 0;

    const bool try_opengl = (flags & P_WINDOW_PREFER_ACCELERATED)
        || (flags & P_WINDOW_REQUIRE_ACCELERATED)
        || (flags & P_WINDOW_REQUIRE_OPENGL);
    const bool warn_opengl = (flags & P_WINDOW_PREFER_ACCELERATED) || 0;
    const bool require_opengl = (flags & P_WINDOW_REQUIRE_OPENGL)
        || (flags & P_WINDOW_REQUIRE_ACCELERATED);

    const bool try_software = (flags & P_WINDOW_PREFER_ACCELERATED)
        || (flags & P_WINDOW_NO_ACCELERATION);

    win->info.gpu_acceleration = P_WINDOW_ACCELERATION_UNSET_;
    if (try_vulkan) {
        if (p_window_set_acceleration(win, P_WINDOW_ACCELERATION_VULKAN)) {
            if (require_vulkan) {
                s_log_error("Failed to initialize Vulkan.");
                return 1;
            } else if (warn_vulkan && try_opengl) {
                s_log_warn("Failed to initialize Vulkan. "
                    "Falling back to OpenGL.");
            } else if (warn_vulkan && try_software) {
                s_log_warn("Failed to initialize Vulkan. "
                    "Falling back to software rendering.");
            } else if (warn_vulkan) {
                s_log_warn("Failed to initialize Vulkan.");
            }
        } else {
            s_log_verbose("OK initializing Vulkan acceleration.");
            return 0;
        }
    }

    if (try_opengl) {
        if (p_window_set_acceleration(win, P_WINDOW_ACCELERATION_OPENGL)) {
            if (require_opengl) {
                s_log_error("Failed to initialize OpenGL.");
                return 1;
            } else if (warn_opengl && try_software) {
                s_log_warn("Failed to initialize OpenGL. "
                    "Falling back to software.");
            } else if (warn_opengl) {
                s_log_warn("Failed to initialize OpenGL.");
            }
        } else {
            s_log_verbose("OK initializing OpenGL acceleration.");
            return 0;
        }
    }

    if (try_software) {
        if (p_window_set_acceleration(win, P_WINDOW_ACCELERATION_NONE)) {
            s_log_error("Failed to initialize software rendering.");
            return 1;
        } else {
            s_log_verbose("OK initializing software rendering.");
            return 0;
        }
    }

    s_log_error("No GPU acceleration mode could be initialized.");
    return 1;
}
