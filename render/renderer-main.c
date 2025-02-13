#define R_INTERNAL_GUARD__
#include "rctx-internal.h"
#undef R_INTERNAL_GUARD__
#include <platform/thread.h>

void renderer_main(void *arg)
{
    struct r_ctx_thread_info *info = arg;
    p_mt_mutex_lock(&info->mutex);
    p_mt_cond_wait(info->cond, info->mutex);
    p_mt_thread_exit();
}
