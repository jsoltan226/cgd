#define ASSET_PLUGIN_INTERNAL_GUARD__
#include "plugin-registry.h"
#undef ASSET_PLUGIN_INTERNAL_GUARD__
#include "io-PNG.h"
#include <core/util.h>
#include <platform/thread.h>

struct asset_plugin plugin_registry[] = {
    {
        .name = "libPNG",
        .handles_type = IMG_TYPE_PNG,
        .priority = 0,
        .load_fn = &load_libPNG,
        .unload_fn = &close_libPNG
    },
};

volatile const u32 plugin_registry_n_plugins = u_arr_size(plugin_registry);

p_mt_mutex_t plugin_registry_mutex = P_MT_MUTEX_INITIALIZER;
