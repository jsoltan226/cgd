#define P_WINDOW_FLAG_LIST_DEF__
#include <platform/window.h>
#undef P_WINDOW_FLAG_LIST_DEF__
#define P_INTERNAL_GUARD__
#include "util-window.h"
#undef P_INTERNAL_GUARD__
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>

#define MODULE_NAME "window-common"

i32 pc_window_check_flags(u32 flags)
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

#define X_(name, id) #name,
            static const char *const flag_strings[P_WINDOW_MAX_N_FLAGS_] = {
                P_WINDOW_FLAG_LIST
            };
#undef X_
            s_log_error("Mutually exclusive flags: \"%s\" and \"%s\"",
                flag_strings[index_a], flag_strings[index_b]);
            ret++;
        }
    }

    return ret;
}

void pc_window_set_default_flags(u32 *flags)
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

i32 pc_window_initialize_acceleration_from_flags(
    pc_window_set_acceleration_fn_t set_acceleration_fn,
    pc_window_handle_t win, u32 flags
)
{
    const union {
        i32 (*fn)(pc_window_handle_t, enum p_window_acceleration);
        void *voidp;
    } set_acceleration_fn_ = {
        .fn = set_acceleration_fn
    };
    u_check_params(set_acceleration_fn_.voidp != NULL && win != NULL);

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

    if (try_vulkan) {
        if (set_acceleration_fn(win, P_WINDOW_ACCELERATION_VULKAN)) {
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
        if (set_acceleration_fn(win, P_WINDOW_ACCELERATION_OPENGL)) {
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
        if (set_acceleration_fn(win, P_WINDOW_ACCELERATION_NONE)) {
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
