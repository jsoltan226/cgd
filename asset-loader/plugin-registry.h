#ifndef ASSET_PLUGIN_REGISTRY_H_
#define ASSET_PLUGIN_REGISTRY_H_

#ifndef ASSET_PLUGIN_INTERNAL_GUARD__
#error This header file is internal to the plugin API of the cgd asset-loader module and is not intended to be used elswhere
#endif /* ASSET_PLUGIN_INTERNAL_GUARD__ */

#include "plugin.h"
#include <platform/thread.h>

extern struct asset_plugin plugin_registry[];
extern volatile const u32 plugin_registry_n_plugins;

extern p_mt_mutex_t plugin_registry_mutex;

#endif /* ASSET_PLUGIN_REGISTRY_H_ */
