#ifndef ASSET_PLUGIN_H_
#define ASSET_PLUGIN_H_

#include "img-type.h"
#include <core/int.h>
#include <stdbool.h>

typedef i32 (*asset_plugin_load_fn_t)();
typedef void (*asset_plugin_unload_fn_t)();

#define ASSET_PLUGIN_MAX_NAME_LEN 64
struct asset_plugin {
    const char name[ASSET_PLUGIN_MAX_NAME_LEN];

    const enum asset_img_type handles_type;
    const u32 priority;

    const asset_plugin_load_fn_t load_fn;
    const asset_plugin_unload_fn_t unload_fn;

    bool is_loaded;
};

/* both return 0 on success and non-zero on failure */
i32 asset_load_plugin_by_type(enum asset_img_type type);
i32 asset_load_plugin_by_name(char name[ASSET_PLUGIN_MAX_NAME_LEN]);

/* returns the number of plugins that failed to load (0 on success, abviously) */
i32 asset_load_all_plugins();

enum asset_plugin_loaded_status {
    PLUGIN_DOES_NOT_EXIST   = -1,
    PLUGIN_NOT_LOADED       = 0,
    PLUGIN_LOADED           = 1,
};
enum asset_plugin_loaded_status
asset_get_plugin_loaded(enum asset_img_type type);

void asset_unload_all_plugins();

#endif /* ASSET_PLUGIN_H_ */
