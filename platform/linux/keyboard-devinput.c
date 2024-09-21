#define _GNU_SOURCE
#include <sys/ioctl.h>
#include "core/pressable-obj.h"
#include <linux/limits.h>
#include <sys/poll.h>
#include "../keyboard.h"
#include "core/log.h"
#include "core/int.h"
#include "core/util.h"
#include "core/datastruct/vector.h"
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#define P_INTERNAL_GUARD__
#include "keyboard-devinput.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "keyboard-devinput"


#define NBITS(x) ((((x) - 1) / (8 * sizeof(long))) + 1)

static VECTOR(struct keyboard_devinput_evdev) find_and_load_available_evdevs(void);
static i32 load_evdev(const char *rel_path, struct keyboard_devinput_evdev *out);
static bool keyboard_supported_keys_check(const u64 bits[NBITS(KEY_MAX)]);

static i32 dev_input_event_scandir_filter(const struct dirent *dirent);

static void read_keyevents_from_evdev(i32 fd,
    pressable_obj_t keys[P_KEYBOARD_N_KEYS],
    bool updated_keys[P_KEYBOARD_N_KEYS]);

i32 devinput_keyboard_init(struct keyboard_devinput *kb)
{
    memset(kb, 0, sizeof(struct keyboard_devinput));
    kb->kbdevs = find_and_load_available_evdevs();
    if (kb->kbdevs == NULL)
        goto err;

    return 0;

err:
    devinput_keyboard_destroy(kb);
    return 1;
}

void devinput_keyboard_destroy(struct keyboard_devinput *kb)
{
    if (kb == NULL) return;

    if (kb->kbdevs != NULL) {
        for (u32 i = 0; i < vector_size(kb->kbdevs); i++) {
            if (kb->kbdevs[i].fd != -1) close(kb->kbdevs[i].fd);
        }
        vector_destroy(kb->kbdevs);
    }
}

void devinput_update_all_keys(struct keyboard_devinput *kb, pressable_obj_t pobjs[P_KEYBOARD_N_KEYS])
{
    u_check_params(kb != NULL && kb->kbdevs != NULL && pobjs != NULL);

    u32 n_poll_fds = vector_size(kb->kbdevs);
    struct pollfd poll_fds[n_poll_fds];
    
    for (u32 i = 0; i < n_poll_fds; i++) {
        poll_fds[i].fd = kb->kbdevs[i].fd;
        poll_fds[i].events = POLLIN;
        poll_fds[i].revents = 0;
    }

    if (poll(poll_fds, n_poll_fds, 0) == -1) {
        s_log_error("Failed to poll() on keyboard event devices: %s",
            strerror(errno));
        return;
    }

    bool updated_keys[P_KEYBOARD_N_KEYS] = { 0 };
    for (u32 i = 0; i < n_poll_fds; i++) {
        if (!(poll_fds[i].revents & POLLIN)) continue;
        read_keyevents_from_evdev(kb->kbdevs[i].fd, pobjs, updated_keys);
    }

    /* When a key is pressed, it gets a "pressed" (1) event
     * and in read_keyevents_from_evdev we update our pressable_objs accordingly.
     *
     * However, when the key is being held, it only starts getting
     * "held" (2) events after some amount of time (which might be several ticks).
     *
     * During that moment, the pressable_obj is not updated in the above loop,
     * and so the key is reported to be "down" for many ticks,
     * which should not happen. 
     *
     * Therefore, we need to update the keys
     * that are held (but not getting any events) ourselves.
     */

    for (u32 i = 0; i < P_KEYBOARD_N_KEYS; i++) {
        if (!updated_keys[i] && (pobjs[i].pressed || pobjs[i].up))
            pressable_obj_update(&pobjs[i], true);
    }
}

static VECTOR(struct keyboard_devinput_evdev) find_and_load_available_evdevs(void)
{
    VECTOR(struct keyboard_devinput_evdev) v = NULL;
    struct dirent **namelist = NULL;
    i32 dir_fd = -1;
    i32 n_dirents = 0;

    v = vector_new(struct keyboard_devinput_evdev);

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
        struct keyboard_devinput_evdev tmp;
        i32 r = load_evdev(namelist[i]->d_name, &tmp);
        if (r < 0) { /* Opening failed */
            n_failed++;
            continue;
        } else if (r > 0) { /* Opening succeeded but tests failed */
            continue;
        }
        
        s_log_debug("Found keyboard: %s (%s)", tmp.path, tmp.name);
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

static void read_keyevents_from_evdev(i32 fd,
    pressable_obj_t keys[P_KEYBOARD_N_KEYS],
    bool updated_keys[P_KEYBOARD_N_KEYS])
{
    struct input_event ev = { 0 };
    i32 n_bytes_read = 0;
    while (n_bytes_read = read(fd, &ev, sizeof(struct input_event)),
        n_bytes_read > 0
    ) {
        if (n_bytes_read <= 0) { /* Either no data was read or some other error */
            return;
        } else if (n_bytes_read != sizeof(struct input_event)) {
            s_log_fatal(MODULE_NAME, __func__, 
                    "Read %i bytes from event device, expected size is %i. "
                    "The linux input driver is probably broken...",
                    n_bytes_read, sizeof(struct input_event));
        }

        if (ev.type != EV_KEY) return;

        i32 i = 0;
        enum p_keyboard_keycode p_kb_keycode = -1;
        do {
            if (linux_input_code_2_kb_keycode_map[i][1] == ev.code) {
                p_kb_keycode = linux_input_code_2_kb_keycode_map[i][0];
                break;
            }
        } while (i++ < P_KEYBOARD_N_KEYS);

        if (p_kb_keycode == -1) return; /* Unsupported key press */
        
        /* ev.value = 0: key released
         * ev.value = 1: key pressed
         * ev.value = 2: key held
         */
        pressable_obj_update(&keys[p_kb_keycode], ev.value > 0);
        updated_keys[p_kb_keycode] = true;

        /*
         * s_log_debug("Key event %i for keycode %s",
         * ev.value, p_keyboard_keycode_strings[p_kb_keycode]);
         */
    }
}


static i32 dev_input_event_scandir_filter(const struct dirent *dirent)
{
    return !strncmp(dirent->d_name, "event", u_strlen("event"));
}

static i32 load_evdev(const char *rel_path, struct keyboard_devinput_evdev *out)
{
    bool err_not_keyboard = false;
    u_check_params(rel_path != NULL && out != NULL);
    memset(out, 0, sizeof(struct keyboard_devinput_evdev));

    strncpy(out->path, DEVINPUT_DIR "/", u_FILEPATH_MAX);
    strncat(out->path, rel_path,
        u_FILEPATH_MAX - strlen(out->path) - 1);

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

    u64 ev_bits[NBITS(EV_MAX)];
    if (ioctl(out->fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
        goto_error("Failed to get supported event types by %s: %s",
            out->path, strerror(errno));
    }

    /* Silently fail if device is a mouse
     * (doesn't support KEY events or
     * supports REL events which are typically only found in mice)
     */
    if (!(ev_bits[0] & 1 << EV_KEY) || (ev_bits[0] & 1 << EV_REL)) {
        err_not_keyboard = true;
        goto err;
    }

    u64 key_bits[NBITS(KEY_MAX)];
    if (ioctl(out->fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0) {
        goto_error("Failed to get key events supported by %s: %s",
            out->path, strerror(errno));
    }

    /* Silently fail if device doesn't support keys
     * typically present in keyboards */
    if (!keyboard_supported_keys_check(key_bits)) {
        err_not_keyboard = true;
        goto err;
    }

    if (ioctl(out->fd, EVIOCGNAME(MAX_KEYBOARD_EVDEV_NAME_LEN), out->name) < 0) {
        s_log_warn("Failed to get name for event device %s: %s",
            out->path, strerror(errno));
    }

    return 0;

err:
    if (out->fd != -1) close(out->fd);

    if (err_not_keyboard) return 1;
    else return -1;
}

static bool keyboard_supported_keys_check(const u64 bits[NBITS(KEY_MAX)])
{
    static const i32 key_checks[] = {
        KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,
        KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_H, KEY_J, KEY_L, KEY_Z,
        KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
        KEY_SPACE, KEY_ESC, KEY_ENTER, KEY_BACKSPACE, KEY_TAB
    };

    for (u32 i = 0; i < u_arr_size(key_checks); i++) {
        u32 arr_index = key_checks[i] / 64;
        u32 bit_index = key_checks[i] % 64;

        if (!(bits[arr_index] & (1 << bit_index)))
            return false;
    }

    return true;
}
