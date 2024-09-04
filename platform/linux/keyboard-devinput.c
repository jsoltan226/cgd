#include <linux/input-event-codes.h>
#define _GNU_SOURCE
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

#define P_INTERNAL_GUARD__
#include "keyboard-devinput.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "keyboard-devinput"

static VECTOR(struct keyboard_devinput_kbdev) find_available_keyboards();

static i32 sysfs_scandir_filter(const struct dirent *dirent);
static i32 is_a_directory(const char *filepath);

static i32 has_keyboard_ev_cap(const char *input_dev_dirent);
static i32 has_keyboard_key_cap(const char *input_dev_dirent);
static inline bool get_bit_from_keys(VECTOR(u64) v, u32 key_no);

i32 devinput_keyboard_init(struct keyboard_devinput *kb)
{
    memset(kb, 0, sizeof(struct keyboard_devinput));
    kb->kbdevs = find_available_keyboards();
    if (kb->kbdevs == NULL)
        goto_error("No available keyboard devices found");

    return 0;

err:
    devinput_keyboard_destroy(kb);
    return 1;
}

void devinput_keyboard_destroy(struct keyboard_devinput *kb)
{
    if (kb == NULL) return;

    if (kb->kbdevs != NULL) vector_destroy(kb->kbdevs);
}

enum p_keyboard_keycode devinput_keyboard_next_key(struct keyboard_devinput *kb)
{
    return -1;
}

static VECTOR(struct keyboard_devinput_kbdev) find_available_keyboards()
{
    VECTOR(struct keyboard_devinput_kbdev) v = NULL;
    struct dirent **namelist = NULL;
    i32 dir_fd = -1;
    i32 n = 0;

    v = vector_new(struct keyboard_devinput_kbdev);

    /* Obtain a directory listing of "/sys/class/input/input*" */
    if (!is_a_directory(SYSFS_INPUT_DIR))
        s_log_error("%s is not a directory!", SYSFS_INPUT_DIR);

    dir_fd = open(SYSFS_INPUT_DIR, O_RDONLY);
    if (dir_fd == -1)
        goto_error("Failed to open %s: %s", SYSFS_INPUT_DIR, strerror(errno));

    n = scandir(SYSFS_INPUT_DIR, &namelist, sysfs_scandir_filter, alphasort);
    if (n == -1) {
        goto_error("Failed to scan dir \"%s\" for input devices: %s",
            SYSFS_INPUT_DIR, strerror(errno));
    } else if (n == 0) {
        goto_error("0 devices were found in \"%s\"", SYSFS_INPUT_DIR);
    }

    for (u32 i = 0; i < n; i++) {
        if (!has_keyboard_ev_cap(namelist[i]->d_name)) continue;
        else if (!has_keyboard_key_cap(namelist[i]->d_name)) continue;
        else {
            struct keyboard_devinput_kbdev tmp = { 0 };
            snprintf(tmp.path, PATH_MAX, SYSFS_INPUT_DIR "/%s", namelist[i]->d_name);
           
            s_log_debug("Found keyboard: %s", tmp.path);
            vector_push_back(v, tmp);
        }
    }

    /* Cleanup */
    for (u32 i = 0; i < n; i++) {
        free(namelist[i]);
    }
    free(namelist);
    close(dir_fd);

    return v;

err:
    if (namelist != NULL && n > -1) {
        for (u32 i = 0; i < n; i++) {
            free(namelist[i]);
        }
        free(namelist);
    }
    if (dir_fd != -1) close(dir_fd);
    if (v != NULL) vector_destroy(v);

    return NULL;
}

static i32 sysfs_scandir_filter(const struct dirent *dirent)
{
    char name[FILENAME_MAX + 1] = { 0 };
    strcpy(name, SYSFS_INPUT_DIR "/");
    strncat(name, dirent->d_name, FILENAME_MAX - u_strlen(SYSFS_INPUT_DIR "/"));

    return is_a_directory(name) &&
        !strncmp(dirent->d_name, "input", u_strlen("input"));
}

static i32 is_a_directory(const char *filepath)
{
    struct stat s;
    if (stat(filepath, &s) == -1 || !S_ISDIR(s.st_mode)) return 0;
    else return 1;
}

static i32 has_keyboard_ev_cap(const char *input_dev_dirent)
{
    char name[FILENAME_MAX + 1] = { 0 };
    snprintf(name, FILENAME_MAX,
        "/sys/class/input/%s/capabilities/ev", input_dev_dirent
    );

    i32 fd = open(name, O_RDONLY);
    if (fd == -1) return 0;

    char buf[SYSFS_INPUT_DEV_EV_MAX_LEN] = { 0 };
    if (read(fd, buf, sizeof(buf) - 1) <= 0) {
        close(fd);
        return 0;
    }

    close(fd);

    return strtoul(buf, NULL, 16) & EV_KEY;
}

static i32 has_keyboard_key_cap(const char *input_dev_dirent)
{
    char name[FILENAME_MAX + 1] = { 0 };
    snprintf(name, FILENAME_MAX,
        "/sys/class/input/%s/capabilities/key", input_dev_dirent
    );

    i32 fd = open(name, O_RDONLY);
    if (fd == -1) return 0;

    char read_buf[SYSFS_INPUT_DEV_KEY_MAX_LEN] = { 0 };
    if (read(fd, read_buf, SYSFS_INPUT_DEV_KEY_MAX_LEN - 1) <= 0) {
        close(fd);
        return 0;
    }

    char *subtoken = NULL, *save_ptr = NULL;
    VECTOR(u64) subkeys = vector_new(u64);

    /* First call to strtok_r must have the original string as the first arg,
     * while all calls after that must be given NULL instead
     */
    subtoken = strtok_r(read_buf, " ", &save_ptr);
    vector_push_back(subkeys, strtoul(subtoken, NULL, 16));

    while (subtoken = strtok_r(NULL, " ", &save_ptr), subtoken != NULL)
        vector_push_back(subkeys, strtoul(subtoken, NULL, 16));

    bool fail = false;
    static const u32 key_checks[] = {
        KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, 
        KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y,
        KEY_SPACE, KEY_ESC, KEY_ENTER,
        KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    };
    for (u32 i = 0; i < u_arr_size(key_checks); i++) {
        if (get_bit_from_keys(subkeys, key_checks[i]) == 0) {
            fail = true;
            break;
        }
    }

    vector_destroy(subkeys);
    close(fd);
    return fail == false;
}

static inline bool get_bit_from_keys(VECTOR(u64) v, u32 key_no)
{
    u32 size = vector_size(v);
    if (key_no >= size * 64) return 0;

    u32 key_index = size - (key_no / 64) - 1;
    u32 bit_index = key_no % 64;

    return v[key_index] & (1 << bit_index);
}
