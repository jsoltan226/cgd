#define _GNU_SOURCE
#define P_INTERNAL_GUARD__
#include "evdev.h"
#undef P_INTERNAL_GUARD__
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <core/math.h>
#include <core/vector.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>

#define MODULE_NAME "evdev"

#define DEVINPUT_DIR "/dev/input"

static i32 dev_input_event_scandir_filter(const struct dirent *dirent);

static i32 ev_cap_check(i32 fd, const char *path, enum evdev_type type);
static i32 ev_bit_check(const u64 bits[], u32 n_bits, const i32 *checks);

VECTOR(struct evdev) evdev_find_and_load_devices(enum evdev_type type)
{
    VECTOR(struct evdev) v = NULL;
    struct dirent **namelist = NULL;
    i32 dir_fd = -1;
    i32 n_dirents = 0;

    v = vector_new(struct evdev);

    /* Obtain a directory listing of "/dev/input/event*" */
    dir_fd = open(DEVINPUT_DIR, O_RDONLY);
    if (dir_fd == -1)
        goto_error("Failed to open %s: %s", DEVINPUT_DIR, strerror(errno));

    n_dirents = scandir(DEVINPUT_DIR, &namelist,
        dev_input_event_scandir_filter, alphasort);
    if (n_dirents == -1) {
        goto_error("Failed to scan dir \"%s\" for input devices: %s",
            DEVINPUT_DIR, strerror(errno));
    } else if (n_dirents == 0) {
        goto_error("0 devices were found in \"%s\"", DEVINPUT_DIR);
    }

    u32 n_failed = 0;
    for (i32 i = 0; i < n_dirents; i++) {
        struct evdev tmp;
        i32 r = evdev_load(namelist[i]->d_name, &tmp, type);

        if (r < 0) { /* Opening failed */
            n_failed++;
            continue;
        } else if (r > 0) { /* Opening succeeded but type checks failed */
            continue;
        }

        s_log_debug("Found %s: %s (%s)",
            evdev_type_strings[type], tmp.path, tmp.name
        );
        vector_push_back(v, tmp);
    }

    const u32 fail_threshhold = n_dirents * MINIMAL_SUCCESSFUL_EVDEVS_LOADED;

    /* If half of the devices failed to load, something is probably wrong */
    if (n_failed > fail_threshhold) {
        s_log_warn("Too many event devices failed to load (%u/%u)"
            ", max # of fails was %u",
            n_failed, n_dirents, fail_threshhold
        );
        goto err;
    }

    /* Cleanup */
    for (i32 i = 0; i < n_dirents; i++)
        u_nfree(&namelist[i]);

    u_nfree(&namelist);
    close(dir_fd);

    return v;

err:
    if (namelist != NULL && n_dirents > -1) {
        for (i32 i = 0; i < n_dirents; i++) {
            u_nfree(&namelist[i]);
        }
        u_nfree(&namelist);
    }
    if (dir_fd != -1) close(dir_fd);
    if (v != NULL) {
        for (u32 i = 0; i < vector_size(v); i++) {
            if (v[i].fd != -1) close(v[i].fd);
            /* Resetting the fd doesn't make sense here,
             * since we are calling `vector_destroy`
             * which will zero out the entire thing anyway
             */
        }
        vector_destroy(&v);
    }

    return NULL;
}

i32 evdev_load(const char *rel_path, struct evdev *out, enum evdev_type type)
{
    u_check_params(rel_path != NULL && out != NULL);
    u_check_params(type > 0 && type < EVDEV_N_TYPES);
    memset(out, 0, sizeof(struct evdev));

    strncpy(out->path, DEVINPUT_DIR "/", u_FILEPATH_MAX);
    strncat(out->path, rel_path,
        u_FILEPATH_MAX - strlen(out->path) - 1);

    /* Open the device */
    out->fd = open(out->path, O_RDONLY | O_NONBLOCK);
    if (out->fd == -1) {
        /* Don't spam the user with 'Permission denied' errors
         *
         * Not having permission to read /dev/input/eventXX is the usual case,
         * but the user might think something is wrong when they get
         * a full screen of error messages
         */
        if (errno == EACCES || errno == EPERM) {
            goto err;
        }
        goto_error("%s: Failed to open %s: %s",
            __func__, out->path, strerror(errno));
    }

    /* Get device name */
    if (ioctl(out->fd, EVIOCGNAME(MAX_EVDEV_NAME_LEN - 1), out->name) < 0) {
        s_log_warn("Failed to get name for event device %s: %s",
            out->path, strerror(errno));
    }
    //s_log_debug("Trying \"%s\" (%s)...", out->name, out->path);

    /* Silently fail if device is of invalid type */
    if (ev_cap_check(out->fd, out->path, type))
        goto err;

    return 0;

err:
    if (out->fd != -1) {
        close(out->fd);
        out->fd = -1;
    }

    return 1;
}

static i32 dev_input_event_scandir_filter(const struct dirent *dirent)
{
    return !strncmp(dirent->d_name, "event", u_strlen("event"));
}

static i32 ev_cap_check(i32 fd, const char *path, enum evdev_type type)
{
    u64 ev_bits[u_nbits(EV_MAX)];
    if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
        s_log_error("Failed to get supported events from %s: %s",
            path, strerror(errno));
        return false;
    }

    u32 i = 0;
    const i32 *ev_checks = evdev_type_checks[type][0];
    while (i < EV_max_n_checks_ && ev_checks[i] != EV_check_end_) {
        const i32 curr_ev_bit = ev_checks[i];
        s_assert(curr_ev_bit > 0 && curr_ev_bit < EV_CNT,
            "Invalid EV_* value (%i)", curr_ev_bit);
        const u32 curr_max_val = ev_max_vals[i];

        u64 bits[EV_bits_max_size_u64_];
        if (ioctl(fd, EVIOCGBIT(curr_ev_bit, curr_max_val), bits) < 0) {
            s_log_error("Failed to get event bits from %s: %s",
                path, strerror(errno));
            return 1;
        }

        const i32 *curr_checks = evdev_type_checks[type][curr_ev_bit];
        if (ev_bit_check(bits, curr_max_val, curr_checks)) {
            //s_log_error("EV bit %i check failed", curr_ev_bit);
            return 1;
        }

        i++;
    }

    return 0;
}

static i32 ev_bit_check(const u64 bits[], u32 n_bits, const i32 *checks)
{
    i32 ret = 0;

    u32 i = 0;
    while (i < EV_max_n_checks_ && checks[i] != EV_check_end_) {
        const u32 arr_index = checks[i] / 64;
        if (arr_index >= n_bits) continue;

        const u64 mask = 1ULL << (u64)(checks[i] % 64);

        if (!(bits[arr_index] & mask)) {
            //s_log_error("Bit 0x%x not present", checks[i]);
            ret++;
        }

        i++;
    }

    if (i >= EV_max_n_checks_ &&
        checks[EV_max_n_checks_ - 1] != EV_check_end_)
    {
        s_log_fatal(MODULE_NAME, __func__,
            "No terminator at the end of an event check array!");
    }

    return ret;
}
