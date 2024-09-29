#include <linux/input-event-codes.h>
#define _GNU_SOURCE
#define P_INTERNAL_GUARD__
#include "evdev.h"
#undef P_INTERNAL_GUARD__
#include "core/int.h"
#include "core/datastruct/vector.h"
#include "core/log.h"
#include "core/util.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

#define MODULE_NAME "evdev"

#define DEVINPUT_DIR "/dev/input"

#define NBITS(x) ((((x) - 1) / (8 * sizeof(long))) + 1)

static i32 dev_input_event_scandir_filter(const struct dirent *dirent);

static bool keyboard_check(i32 fd, const char *path);
static bool mouse_check(i32 fd, const char *path);

static bool ev_bit_check(const u64 bits[], u32 n_bits,
    const i32 checks[], u32 n_checks);

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
    for (u32 i = 0; i < n_dirents; i++) {
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
    for (u32 i = 0; i < n_dirents; i++)
        free(namelist[i]);

    free(namelist);
    close(dir_fd);

    return v;

err:
    if (namelist != NULL && n_dirents > -1) {
        for (u32 i = 0; i < n_dirents; i++) {
            free(namelist[i]);
        }
        free(namelist);
    }
    if (dir_fd != -1) close(dir_fd);
    if (v != NULL) {
        for (u32 i = 0; i < vector_size(v); i++) {
            if (v[i].fd != -1) close(v[i].fd);
        }
        vector_destroy(v);
    }

    return NULL;
}

i32 evdev_load(const char *rel_path, struct evdev *out, enum evdev_type type)
{
    bool err_invalid_type = false;
    u_check_params(rel_path != NULL && out != NULL);
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


    /* Silently fail if device is of invalid type */
    switch (type) {
        case EVDEV_KEYBOARD:
            if (!keyboard_check(out->fd, out->path)) {
                err_invalid_type = true;
                goto err;
            }
            break;
        case EVDEV_MOUSE:
            if (!mouse_check(out->fd, out->path)) {
                err_invalid_type = true;
                goto err;
            }
            break;
        default: case EVDEV_UNKNOWN:
            s_log_error("%s: Invalid type parameter: %i\n", __func__, type);
            goto err;
    }

    /* Get device name */
    if (ioctl(out->fd, EVIOCGNAME(MAX_EVDEV_NAME_LEN - 1), out->name) < 0) {
        s_log_warn("Failed to get name for event device %s: %s",
            out->path, strerror(errno));
    }

    return 0;

err:
    if (out->fd != -1) {
        close(out->fd);
        out->fd = -1;
    }

    if (err_invalid_type) return 1;
    else return -1;
}

static i32 dev_input_event_scandir_filter(const struct dirent *dirent)
{
    return !strncmp(dirent->d_name, "event", u_strlen("event"));
}

static bool keyboard_check(i32 fd, const char *evdev_path)
{
    u64 ev_bits[NBITS(EV_MAX)];
    if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
        s_log_error("Failed to get supported events from %s: %s",
            evdev_path, strerror(errno));
        return false;
    }

    /* Fail is device is a mouse (doesn't support KEY events or
     * supports REL events which are typically only found in mice)
     */
    if (!(ev_bits[0] & 1 << EV_KEY) || (ev_bits[0] & 1 << EV_REL))
        return false;

    u64 key_bits[NBITS(KEY_MAX)];
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0) {
        s_log_error("Failed to get key events supported by %s: %s",
            evdev_path, strerror(errno));
        return false;
    }

    static const i32 key_checks[] = {
        KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,
        KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_H, KEY_J, KEY_L, KEY_Z,
        KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
        KEY_SPACE, KEY_ESC, KEY_ENTER, KEY_BACKSPACE, KEY_TAB
    };
    
    return ev_bit_check(key_bits, u_arr_size(key_bits),
            key_checks, u_arr_size(key_checks));
}

static bool mouse_check(i32 fd, const char *evdev_path)
{
    u64 ev_bits[NBITS(EV_MAX)];
    if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
        s_log_error("Failed to get events supported by %s: %s",
            evdev_path, strerror(errno));
        return false;
    }

    /* A mouse must support REL and KEY events */
    if (!(ev_bits[0] & (1 << EV_REL) && ev_bits[0] & (1 << EV_KEY)))
        return false;

    /* A mouse must also support mouse button events */
    u64 key_bits[NBITS(KEY_MAX)];
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0) {
        s_log_error("Failed to get KEY events supported by %s: %s",
            evdev_path, strerror(errno));
        return false;
    }

    static const i32 key_checks[] = {
        BTN_LEFT, BTN_RIGHT, BTN_MIDDLE
    };
    if (!ev_bit_check(key_bits, u_arr_size(key_bits),
            key_checks, u_arr_size(key_checks)))
        return false;

    u64 rel_bits[NBITS(KEY_MAX)];
    if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(rel_bits)), rel_bits) < 0) {
        s_log_error("Failed to get REL events supported by %s: %s",
            evdev_path, strerror(errno));
        return false;
    }

    static const i32 rel_checks[] = {
        REL_X, REL_Y, REL_WHEEL
    };
    if (!ev_bit_check(rel_bits, u_arr_size(rel_bits),
            rel_checks, u_arr_size(rel_checks)))
        return false;

    return true;
}

static bool ev_bit_check(const u64 bits[], u32 n_bits,
    const i32 checks[], u32 n_checks)
{
    for (u32 i = 0; i < n_checks; i++) {
        u32 arr_index = checks[i] / 64;
        if (arr_index >= n_bits) continue;

        u32 bit_index = checks[i] % 64;

        if (!(bits[arr_index] & (1 << bit_index)))
            return false;
    }

    return true;
}
