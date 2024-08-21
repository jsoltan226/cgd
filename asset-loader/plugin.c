#include "plugin.h"
#include "core/int.h"
#include "core/log.h"
#include "img-type.h"
#include "core/util.h"
#include <stdbool.h>
#include <string.h>

#define ASSET_PLUGIN_INTERNAL_GUARD__
#include "plugin_registry.h"
#undef ASSET_PLUGIN_INTERNAL_GUARD__

#define MODULE_NAME "asset-plugin"

static i32 load_plugin(struct asset_plugin *p);
static void unload_plugin(struct asset_plugin *p);
static struct asset_plugin * lookup_by_type(enum asset_img_type type);
static struct asset_plugin * lookup_by_name(char name[ASSET_PLUGIN_MAX_NAME_LEN]);

i32 asset_load_plugin_by_type(enum asset_img_type type)
{
    struct asset_plugin *p = lookup_by_type(type);
    if (p == NULL) {
        s_log_error("%s: No plugin exists for type \"%s\"",
            __func__, asset_get_type_name(type));
        return 1;
    }

    return load_plugin(p);
}

i32 asset_load_plugin_by_name(char name[ASSET_PLUGIN_MAX_NAME_LEN])
{
    struct asset_plugin *p = lookup_by_name(name);
    if (p == NULL) {
        s_log_error("%s: No plugin named \"%s\" exists", __func__, name);
        return 1;
    }

    return load_plugin(p);
}

i32 asset_load_all_plugins()
{
    u32 n_failed = 0;
    for (u32 i = 0; i < u_arr_size(plugin_registry); i++) {
        if (plugin_registry[i].is_loaded)
            continue;
        
        n_failed += 0x1 & load_plugin(&plugin_registry[i]);
    }

    s_log_debug("Loaded all plugins, %u failed", n_failed);

    return n_failed;
}

enum asset_plugin_loaded_status asset_get_plugin_loaded(enum asset_img_type type)
{
    struct asset_plugin *p = lookup_by_type(type);
    if (p == NULL) return PLUGIN_DOES_NOT_EXIST;
    else return p->is_loaded;
}

void asset_unload_all_plugins()
{
#ifndef CGD_BUILDTYPE_RELEASE
    u32 n_unloaded = 0;
#endif /* CGD_BUILDTYPE_RELEASE */
    for (u32 i = 0; i < u_arr_size(plugin_registry); i++) {
        if (plugin_registry[i].is_loaded) {
            unload_plugin(&plugin_registry[i]);
#ifndef CGD_BUILDTYPE_RELEASE
            n_unloaded++;
#endif /* CGD_BUILDTYPE_RELEASE */
        }
    }
    s_log_debug("Unloaded all (%u) plugins", n_unloaded);
}

static i32 load_plugin(struct asset_plugin *p)
{
    if (p->is_loaded) {
        s_log_warn("Plugin \"%s\" already loaded!", p->name);
        return 0;
    }

    s_log_debug("Loading plugin \"%s\"...", p->name);
    if (p->load_fn()) {
        s_log_error("Failed to load plugin \"%s\".", p->name);
        return 1;
    }
    
    p->is_loaded = true;

    return 0;
}

static void unload_plugin(struct asset_plugin *p)
{
    p->unload_fn();
    p->is_loaded = false;
}

static struct asset_plugin * lookup_by_type(enum asset_img_type type)
{
    struct asset_plugin *match = NULL, *current = NULL;

    for (u32 i = 0; i < u_arr_size(plugin_registry); i++) {
        current = &plugin_registry[i];
        if (type == current->handles_type && 
            (match == NULL || current->priority >= match->priority)
        ) {
            match = current;
        }
    }

    return match;
}

static struct asset_plugin * lookup_by_name(char name[ASSET_PLUGIN_MAX_NAME_LEN])
{
    for (u32 i = 0; i < u_arr_size(plugin_registry); i++) {
        if (!strncmp(name, plugin_registry[i].name, ASSET_PLUGIN_MAX_NAME_LEN))
            return &plugin_registry[i];
    }

    return NULL;
}
