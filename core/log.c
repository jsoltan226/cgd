#define S_LOG_LEVEL_LIST_DEF__
#include "log.h"
#undef S_LOG_LEVEL_LIST_DEF__
#include "math.h"
#include "spinlock.h"
#include "ansi-esc-sequences.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <string.h>

#define MODULE_NAME "log"

static void write_msg_to_file(FILE *fp,
    const char line_string[S_LOG_PREFIX_SIZE], const char *module_name,
    const char *fmt, va_list vlist);
static void write_msg_to_membuf(
    char *buf, u64 buf_size, _Atomic u64 *write_index,
    const char line_string[S_LOG_PREFIX_SIZE], const char *module_name,
    const char *fmt, va_list vlist);

static noreturn void do_abort_v(const char *module_name,
    const char *function_name, const char *fmt, va_list vlist);

static void read_output_config(struct s_log_output_cfg *o,
    enum s_log_level level);
static i32 try_set_output_config(const struct s_log_output_cfg *i,
    enum s_log_level level, bool force);

static void copy_old_data(FILE *new_fp, char *new_buf, u64 new_buf_size,
    enum s_log_output_type new_type, enum s_log_level level);

static void strip_escape_sequences(char out[S_LOG_PREFIX_SIZE],
    const char in[S_LOG_PREFIX_SIZE]);

#define X_(name) [name] = #name,
static const char *const log_level_strings[S_LOG_N_LEVELS_] = {
    S_LOG_LEVEL_LIST
};
#undef X_

/** Global configuration **/

static _Atomic enum s_log_level g_log_level = ATOMIC_VAR_INIT(S_LOG_TRACE);

static const char g_default_log_prefixes[S_LOG_N_LEVELS_][S_LOG_PREFIX_SIZE] = {
    [S_LOG_TRACE] = es_GRAY es_DIM "T " es_COLOR_RESET,
    [S_LOG_DEBUG] = es_GRAY "D " es_COLOR_RESET,
    [S_LOG_VERBOSE] =  "V ",
    [S_LOG_INFO] = es_BOLD "I " es_COLOR_RESET,
    [S_LOG_WARNING] = es_BOLD es_FG_YELLOW  "W " es_COLOR_RESET,
    [S_LOG_ERROR] = es_UNDERLINE es_BOLD es_FG_RED "E " es_COLOR_RESET,
    [S_LOG_FATAL_ERROR] = "",
};
#define X_(name) [name] = ATOMIC_VAR_INIT(g_default_log_prefixes[name]),
static const char *_Atomic g_log_prefixes[S_LOG_PREFIX_SIZE] = {
    S_LOG_LEVEL_LIST
};
#undef X_

static char g_default_out_membuf[S_LOG_DEFAULT_MEMBUF_SIZE] = { 0 };
static char g_default_err_membuf[S_LOG_DEFAULT_MEMBUF_SIZE] = { 0 };

struct output {
        enum s_log_output_type type;
        spinlock_t cfg_lock;

        FILE *fp;
        const char *filepath;

        char *membuf;
        u64 membuf_size;
        _Atomic u64 membuf_write_index;
        bool membuf_locally_allocated;
} g_output_cfgs[S_LOG_N_LEVELS_] = {
#define default_output_config_template(out_membuf)                          \
    {                                                                       \
        .cfg_lock = SPINLOCK_INIT,                                          \
        .type = S_LOG_OUTPUT_MEMORYBUF,                                     \
        .membuf = out_membuf,                                               \
        .membuf_size = S_LOG_DEFAULT_MEMBUF_SIZE,                           \
        .membuf_write_index = ATOMIC_VAR_INIT(0),                           \
    }

    [S_LOG_TRACE] = default_output_config_template(g_default_out_membuf),
    [S_LOG_DEBUG] = default_output_config_template(g_default_out_membuf),
    [S_LOG_VERBOSE] = default_output_config_template(g_default_out_membuf),
    [S_LOG_INFO] = default_output_config_template(g_default_out_membuf),

    [S_LOG_WARNING] = default_output_config_template(g_default_err_membuf),
    [S_LOG_ERROR] = default_output_config_template(g_default_err_membuf),
    [S_LOG_FATAL_ERROR] = default_output_config_template(g_default_err_membuf),

#undef default_output_config_template
};

#undef S_LOG_LEVEL_LIST

void s_log(enum s_log_level level, const char *module_name,
    const char *fmt, ...)
{
    if (!(level >= 0 && level < S_LOG_N_LEVELS_))
        s_log_fatal("Invalid parameters: `level` (%d) "
            "not in range <0, S_LOG_N_LEVELS_ (%d)>",
            level, S_LOG_N_LEVELS_);

    if (module_name == NULL)
        s_log_fatal("Invalid parameters: `module_name` is NULL");

    if (fmt == NULL)
        s_log_fatal("Invalid parameters: `fmt` is NULL");

    if (level < atomic_load(&g_log_level))
        return;

    va_list fmt_list;
    va_start(fmt_list, fmt);

    if (level == S_LOG_FATAL_ERROR)
        do_abort_v(module_name, "(unknown)", fmt, fmt_list);

    struct output *const output = &g_output_cfgs[level];
    const char *line_string = atomic_load(&g_log_prefixes[level]);

    switch (output->type) {
    case S_LOG_OUTPUT_FILE:
    case S_LOG_OUTPUT_FILEPATH:
        write_msg_to_file(output->fp,
            line_string, module_name, fmt, fmt_list);
        break;
    case S_LOG_OUTPUT_MEMORYBUF:
        write_msg_to_membuf(output->membuf,
            output->membuf_size,
            &output->membuf_write_index,
            line_string, module_name, fmt, fmt_list);
        break;
    case S_LOG_OUTPUT_NONE:
        break;
    }

    va_end(fmt_list);
}

noreturn void s_abort(const char *module_name, const char *function_name,
    const char *fmt, ...)
{
    va_list vlist;
    va_start(vlist, fmt);
    do_abort_v(module_name, function_name, fmt, vlist);
    va_end(vlist); /* Technically unnecessary */
}

void s_configure_log_level(enum s_log_level new_log_level)
{
    atomic_store(&g_log_level, new_log_level);
}

enum s_log_level s_get_log_level(void)
{
    return atomic_load(&g_log_level);
}

i32 s_configure_log_output(enum s_log_level level,
    const struct s_log_output_cfg *in_new_cfg,
    struct s_log_output_cfg *out_old_cfg)
{
    if (!(level >= 0 && level < S_LOG_N_LEVELS_))
        s_log_fatal("Invalid parameters: `level` (%d) "
            "not in range <0, S_LOG_N_LEVELS_ (%d)>",
            level, S_LOG_N_LEVELS_);

    i32 ret = 0;
    if (out_old_cfg != NULL) {
        spinlock_acquire(&g_output_cfgs[level].cfg_lock);
        read_output_config(out_old_cfg, level);
        spinlock_release(&g_output_cfgs[level].cfg_lock);
    }

    if (in_new_cfg != NULL) {
        ret = try_set_output_config(in_new_cfg, level, false);
    }

    return ret;
}

i32 s_configure_log_outputs(u32 level_mask, const struct s_log_output_cfg *cfg)
{
    i32 n_failed = 0;
    for (u32 i = 0; i < S_LOG_N_LEVELS_; i++) {
        if (level_mask & (1 << i))
            n_failed += 1 & s_configure_log_output(i, cfg, NULL);
    }
    return n_failed;
}

#define S_LOG_PREFIX_SIZE 32
void s_configure_log_prefix(enum s_log_level level,
    char in_new_prefix[S_LOG_PREFIX_SIZE],
    char out_old_prefix[S_LOG_PREFIX_SIZE],
    bool strip_esc_sequences)
{
    if (!(level >= 0 && level < S_LOG_N_LEVELS_))
        s_log_fatal("Invalid parameters: `level` (%d) "
            "not in range <0, S_LOG_N_LEVELS_ (%d)>",
            level, S_LOG_N_LEVELS_);

    if (out_old_prefix != NULL) {
        if (strip_esc_sequences) {
            char tmp[S_LOG_PREFIX_SIZE] = { 0 };
            memcpy(tmp, atomic_load(&g_log_prefixes[level]),
                S_LOG_PREFIX_SIZE);
            strip_escape_sequences(out_old_prefix, tmp);
        } else {
            memcpy(out_old_prefix, atomic_load(&g_log_prefixes[level]),
                S_LOG_PREFIX_SIZE);
        }
    }

    if (in_new_prefix != NULL) {
        if (strip_esc_sequences) {
            char tmp[S_LOG_PREFIX_SIZE] = { 0 };
            strip_escape_sequences(tmp, in_new_prefix);
            memcpy(in_new_prefix, tmp, S_LOG_PREFIX_SIZE);
        }
        atomic_store(&g_log_prefixes[level], in_new_prefix);
    }
}

void s_log_cleanup_all(void)
{
    /* Close all the open log file streams */
    const struct s_log_output_cfg close_cfg = { .type = S_LOG_OUTPUT_NONE };
    for (u32 i = 0; i < S_LOG_N_LEVELS_; i++)
        (void) try_set_output_config(&close_cfg, i, true);
}

static void write_msg_to_file(FILE *fp,
    const char line_string[S_LOG_PREFIX_SIZE], const char *module_name,
    const char *fmt, va_list vlist)
{
    fprintf(fp, "%s [%s] ", line_string, module_name);
    vfprintf(fp, fmt, vlist);
    fputc('\n', fp);
}

static void write_msg_to_membuf(
    char *buf, u64 buf_size, _Atomic u64 *write_index,
    const char line_string[S_LOG_PREFIX_SIZE], const char *module_name,
    const char *fmt, va_list vlist)
{
    if (buf_size < S_LOG_MINIMAL_MEMBUF_SIZE) {
        s_log_fatal("membuf size %lu is too small (the minimum is %lu",
            buf_size, S_LOG_MINIMAL_MEMBUF_SIZE);
    }

    /* Determine the length of the string */
    i32 premsg_len = snprintf(NULL, 0, "%s [%s] ",
        line_string, module_name);
    if (premsg_len < 0) {
        /* Failure, skip */
        premsg_len = 0;
    }

    /* Write the string to the buffer and update the write index */
    u64 write_index_value = atomic_load(write_index);
    if (write_index_value + premsg_len + 1/*'\0'*/ > buf_size) {
        atomic_store(write_index, 0);
    } else {
        atomic_store(write_index, write_index_value + premsg_len);
    }
    (void) snprintf(buf + write_index_value, buf_size - write_index_value,
        "%s [%s] ", line_string, module_name);


    /* va_lists can't be used more than once,
     * but we need one for the string length checking
     * and also one for the actual write */
    va_list vcopy;
    va_copy(vcopy, vlist);

    /* Determine the length of the string */
    i32 msg_len = vsnprintf(NULL, 0, fmt, vlist);
    if (msg_len < 0) {
        /* Failure, skip */
        msg_len = 0;
    } else {
        msg_len++; /* The newline char */
    }

    /* Write the string to the buffer and update the write index */
    write_index_value = atomic_load(write_index);
    if (write_index_value + msg_len + 1/*'\0'*/ > buf_size) {
        atomic_store(write_index, 0);
    } else {
        atomic_store(write_index, write_index_value + msg_len);
    }
    (void) vsnprintf(buf + write_index_value, buf_size - write_index_value,
        fmt, vcopy);
    buf[write_index_value + msg_len - 1] = '\n';

    va_end(vcopy);
}

static noreturn void do_abort_v(const char *module_name,
    const char *function_name, const char *fmt, va_list vlist)
{
    FILE *err_fp = NULL;
    switch (g_output_cfgs[S_LOG_FATAL_ERROR].type) {
    case S_LOG_OUTPUT_FILE:
    case S_LOG_OUTPUT_FILEPATH:
        err_fp = g_output_cfgs[S_LOG_FATAL_ERROR].fp;
        break;
    case S_LOG_OUTPUT_MEMORYBUF:
    case S_LOG_OUTPUT_NONE:
        err_fp = stderr;
        break;
    }

    fprintf(err_fp, "[%s] FATAL ERROR: %s: ", module_name, function_name);

    vfprintf(err_fp, fmt, vlist);

    fprintf(err_fp, "\nFatal error encountered. Calling abort().\n");

    s_log_cleanup_all();

    abort();
}

static void read_output_config(struct s_log_output_cfg *o,
    enum s_log_level level)
{
    memset(o, 0, sizeof(struct s_log_output_cfg));

    o->type = g_output_cfgs[level].type;
    const struct output *const cfg = &g_output_cfgs[level];
    switch (o->type) {
        case S_LOG_OUTPUT_FILE:
            o->out.file = cfg->fp;
            break;
        case S_LOG_OUTPUT_FILEPATH:
            o->out.filepath = cfg->filepath;
            break;
        case S_LOG_OUTPUT_MEMORYBUF:
            o->out.membuf.buf = cfg->membuf;
            o->out.membuf.buf_size = cfg->membuf_size;
            break;
        case S_LOG_OUTPUT_NONE:
            break;
    }

    o->flag_copy = o->flag_append = false;
}

static i32 try_set_output_config(const struct s_log_output_cfg *i,
    enum s_log_level level, bool force)
{
    struct output *const cfg = &g_output_cfgs[level];
    if (!force)
        spinlock_acquire(&cfg->cfg_lock);

    FILE *new_fp = NULL;
    char *new_buf = NULL;
    bool new_buf_locally_allocated = false;

    /* Initialize the new output */
    switch (i->type) {
    case S_LOG_OUTPUT_FILE:
        if (i->out.file == NULL) {
            s_log_error("Invalid parameters: new log file handle "
                "(for level %s) is NULL", log_level_strings[level]);
            goto err;
        }
        new_fp = i->out.file;
        break;
    case S_LOG_OUTPUT_FILEPATH:
        if (i->out.filepath == NULL) {
            s_log_error("Invalid parameters: new log file path "
                "(for level %s) is NULL", log_level_strings[level]);
            goto err;
        }
        new_fp = fopen(i->out.filepath, i->flag_append ? "ab" : "wb");
        if (new_fp == NULL) {
            s_log_error("Failed to open new log file \"%s\" "
                "(for level %s): %s",
                log_level_strings[level], strerror(errno)
            );
            goto err;
        }
        break;
    case S_LOG_OUTPUT_MEMORYBUF:
        if (i->out.membuf.buf_size < S_LOG_MINIMAL_MEMBUF_SIZE) {
            s_log_error("Invalid parameters: new log buffer size "
                "(for level %s) is too small (%lu - the minimum is %lu)",
                log_level_strings[level],
                i->out.membuf.buf_size, S_LOG_MINIMAL_MEMBUF_SIZE
            );
            goto err;
        }
        if (i->out.membuf.buf == NULL) {
            new_buf = calloc(i->out.membuf.buf_size, 1);
            if (new_buf == NULL) {
                s_log_error("Failed to allocate new log buffer of size %lu "
                    "(for level %s)", i->out.membuf.buf_size,
                    log_level_strings[level]);
                goto err;
            }
            new_buf_locally_allocated = true;
        } else {
            new_buf = i->out.membuf.buf;
        }
        break;
    case S_LOG_OUTPUT_NONE:
        break;
    }

    /* Close the old output */
    switch (cfg->type) {
    case S_LOG_OUTPUT_FILEPATH:
        fclose(cfg->fp);
        break;
    case S_LOG_OUTPUT_FILE:
        break;
    case S_LOG_OUTPUT_MEMORYBUF:
        /* Handle the "copy" flag */
        if (i->flag_copy) {
            copy_old_data(new_fp, new_buf, i->out.membuf.buf_size,
                i->type, level);
        }

        if (cfg->membuf_locally_allocated)
            free(cfg->membuf);
        break;
    case S_LOG_OUTPUT_NONE:
        break;
    }
    cfg->filepath = NULL;
    cfg->fp = NULL;
    cfg->membuf = NULL;
    cfg->membuf_size = 0;
    cfg->membuf_write_index = 0;

    /* Store the new output */
    switch (i->type) {
    case S_LOG_OUTPUT_FILE:
        cfg->fp = i->out.file;
        break;
    case S_LOG_OUTPUT_FILEPATH:
        cfg->filepath = i->out.filepath;
        cfg->fp = new_fp;
        break;
    case S_LOG_OUTPUT_MEMORYBUF:
        cfg->membuf = new_buf;
        cfg->membuf_locally_allocated = new_buf_locally_allocated;
        cfg->membuf_size = i->out.membuf.buf_size;
        cfg->membuf_write_index = 0;
        break;
    case S_LOG_OUTPUT_NONE:
        break;
    }
    cfg->type = i->type;

    if (!force)
        spinlock_release(&cfg->cfg_lock);
    return 0;

err:
    if (!force)
        spinlock_release(&cfg->cfg_lock);
    return 1;
}

static void copy_old_data(FILE *new_fp, char *new_buf, u64 new_buf_size,
    enum s_log_output_type new_type, enum s_log_level level)
{
    struct output *const cfg = &g_output_cfgs[level];
    char *const c_p = memchr(cfg->membuf,
        '\0', cfg->membuf_size);
    const u64 n_bytes = c_p ?
        (u64)(c_p - cfg->membuf) :
        cfg->membuf_size;

    switch (new_type) {
    case S_LOG_OUTPUT_FILE:
    case S_LOG_OUTPUT_FILEPATH:
        (void) new_buf;
        (void) new_buf_size;

        (void) fwrite(cfg->membuf, 1, n_bytes,
            new_fp);
        if (ferror(new_fp)) {
            s_log_error("Failed to copy over data from old membuf "
                "(for level %s): %s", log_level_strings[level],
                strerror(errno));
        }
        break;
    case S_LOG_OUTPUT_MEMORYBUF:
        (void) new_fp;
        memcpy(new_buf, cfg->membuf,
            u_min(cfg->membuf_size, new_buf_size)
        );
        break;
    case S_LOG_OUTPUT_NONE:
        (void) new_fp;
        (void) new_buf;
        (void) new_buf_size;
        break;
    }
}

static void strip_escape_sequences(char out[S_LOG_PREFIX_SIZE],
    const char in[S_LOG_PREFIX_SIZE])
{
    memset(out, 0, S_LOG_PREFIX_SIZE);

    bool esc = false, csi = false, csi_done_parameter = false;
    u32 j = 0;
    for (u32 i = 0; i < S_LOG_PREFIX_SIZE - 1; i++) {
        /* C0 control codes */
        if (in[i] == es_ESC_chr) {
            esc = true;
            continue;
        } else if (es_is_C0_control_code(in[i])) {
            esc = false;
            continue;
        }

        /* Fe escape sequences */
        if (esc && in[i] == '[') {
            esc = false;
            csi = true;
            continue;
        } else if (esc && es_is_Fe_code(in[i])) {
            esc = false;
            continue;
        }

        /* CSI commands */
        if (csi && es_is_CSI_terminator(in[i])) {
            csi = false;
            csi_done_parameter = false;
            continue;
        } else if (csi && es_is_CSI_parameter(in[i]) && !csi_done_parameter) {
            continue;
        } else if (csi && es_is_CSI_intermediate(in[i]) && !csi_done_parameter) {
            csi_done_parameter = true;
            continue;
        } else if ((csi && es_is_CSI_parameter(in[i]) && csi_done_parameter) ||
                    (csi && !es_is_CSI_parameter(in[i])
                     && !es_is_CSI_intermediate(in[i])
                    )
        ) {
            /* Malformed escape sequence */
            esc = false;
            csi = false;
            csi_done_parameter = false;
            continue;
        }

        out[j++] = in[i];
    }
}
