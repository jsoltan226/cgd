#define _GNU_SOURCE
#include "../keyboard.h"
#include "core/log.h"
#include "core/int.h"
#include "core/util.h"
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

static i32 determine_keyboard_dev_path(char out[MAX_KEYBOARD_DEV_FILEPATH_LEN]);
static i32 sysfs_scandir_filter(const struct dirent *dirent);
static i32 is_a_directory(const char *filepath);
static i32 compare_and_write_devinput_key(i32 fd);

i32 devinput_keyboard_init(struct keyboard_devinput *kb)
{
    memset(kb, 0, sizeof(struct keyboard_devinput));
    determine_keyboard_dev_path(kb->path);
    return 1;
}

void devinput_keyboard_destroy(struct keyboard_devinput *kb)
{
}

enum p_keyboard_keycode devinput_keyboard_next_key(struct keyboard_devinput *kb)
{
    return -1;
}

static i32 determine_keyboard_dev_path(char out[MAX_KEYBOARD_DEV_FILEPATH_LEN])
{
    struct dirent **namelist = NULL;
    i32 dir_fd = -1;
    i32 n = 0;

    /* Obtain a listing of /sys/class/input/input* */
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

#ifndef CGD_BUILDTYPE_RELEASE
    s_log_debug("===== BEGIN directory listing of %s =====", SYSFS_INPUT_DIR);
    for (u32 i = 0; i < n; i++) {
        s_log_debug(namelist[i]->d_name);
    }
    s_log_debug("===== END directory listing of %s =====", SYSFS_INPUT_DIR);
#endif /* CGD_BUILDTYPE_RELEASE */

    for (u32 i = 0; i < n; i++) {
        char name[FILENAME_MAX + 1] = { 0 };
        strncpy(name, namelist[i]->d_name, FILENAME_MAX);
        strncat(name, "/" SYSFS_INPUT_DEV_KEY_FILE, FILENAME_MAX - strlen(name));

        i32 fd = openat(dir_fd, name, O_RDONLY);
        if (compare_and_write_devinput_key(fd))
            s_log_debug("found %s/%s", SYSFS_INPUT_DIR, name);
        close(fd);
    }

    /* Cleanup */
    for (u32 i = 0; i < n; i++) {
        free(namelist[i]);
    }
    free(namelist);
    close(dir_fd);

    return 0;

err:
    if (namelist != NULL && n > -1) {
        for (u32 i = 0; i < n; i++) {
            free(namelist[i]);
        }
        free(namelist);
    }

    return 1;
}

static i32 sysfs_scandir_filter(const struct dirent *dirent)
{
    char name[FILENAME_MAX + 1] = { 0 };
    strcpy(name, SYSFS_INPUT_DIR "/");
    strncat(name, dirent->d_name, FILENAME_MAX - u_strlen(SYSFS_INPUT_DIR "/"));

    return is_a_directory(name) && !strncmp(dirent->d_name, SYSFS_INPUT_DEV_DIR_PREFIX,
        u_strlen(SYSFS_INPUT_DEV_DIR_PREFIX));
}

static i32 is_a_directory(const char *filepath)
{
    struct stat s;
    if (stat(filepath, &s) == -1 || !S_ISDIR(s.st_mode)) return 0;
    else return 1;
}

static i32 compare_and_write_devinput_key(i32 fd)
{
    if (fd == -1) return 0;

    char tmp_buf[SYSFS_INPUT_DEV_KEY_MAX_LEN] = { 0 };
    if (read(fd, tmp_buf, SYSFS_INPUT_DEV_KEY_MAX_LEN - 1) <= 0)
        return 0;

    char *last_key_p = strrchr(tmp_buf, ' ');
    if (last_key_p == NULL) return 0;
    last_key_p++;

    char *last_key_newline_p = strchr(last_key_p, '\n');
    if (last_key_newline_p == NULL) return 0;
    *last_key_newline_p = '\0';

    u64 result = strtoul(last_key_p, NULL, 16);

    s_log_debug("%s(): key = \"%s\", result = %lx", __func__, last_key_p, result);
    if (result == 0xfffffffffffffffe) return 1;
    else return 0;
}
