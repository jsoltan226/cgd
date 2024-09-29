#ifndef ASSET_PLUGIN_REGISTRY_H_
#define ASSET_PLUGIN_REGISTRY_H_

#ifndef ASSET_PLUGIN_INTERNAL_GUARD__
#error This header file is internal to the plugin API of the cgd asset-loader module and is not intended to be used elswhere
#endif /* ASSET_PLUGIN_INTERNAL_GUARD__ */

#include "plugin.h"
#include "img-type.h"
#include "io-PNG.h"

static struct asset_plugin plugin_registry[] = {
    (struct asset_plugin) {
        .name = "libPNG",
        .handles_type = IMG_TYPE_PNG,
        .priority = 0,
        .load_fn = load_libPNG,
        .unload_fn = close_libPNG
    },
};

#endif /* ASSET_PLUGIN_REGISTRY_H_ */
