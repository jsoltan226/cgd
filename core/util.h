#ifndef UTIL_H_
#define UTIL_H_

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "shapes.h"
#include "log.h"

#define u_BUF_SIZE  1024
#define u_PATH_FROM_BIN_TO_ASSETS "../assets/"

#define u_FILEPATH_MAX 256
typedef const char filepath_t[u_FILEPATH_MAX];


#define goto_error(...) do {    \
    s_log_error(__VA_ARGS__);   \
    goto err;                   \
} while (0);

#define u_check_params(expr) s_assert(expr, "invalid parameters");

#define u_color_arg_expand(color) color.r, color.g, color.b, color.a

#define u_arr_size(arr) (sizeof(arr) / sizeof(*arr))

#define u_strlen(str_literal) (sizeof(str_literal) - 1)

#endif /* UTIL_H_ */
