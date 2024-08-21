#include "core/datastruct/hashmap.h"
#include "core/int.h"
#include "core/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "hashmaptest"

static void dump_hashmap(struct hashmap *map, s_log_level log_level);

#define MAP_SIZE 10
static struct hashmap *map = NULL;

static const char * const key_value_pairs[MAP_SIZE][2] = {
    { "key0", "val0" },
    { "key1", "val1" },
    { "key2", "val2" },
    { "key3", "val3" },
    { "key4", "val4" },
    { "key5", "val5" },
    { "key6", "val6" },
    { "key7", "val7" },
    { "key8", "val8" },
    { "key9", "val9" },
};

int main(void)
{
    s_set_log_out_filep(stdout);
    s_set_log_err_filep(stdout);
    s_set_log_level(LOG_DEBUG);

    s_log_debug("Creating hashmap...");

    map = hashmap_create(MAP_SIZE);
    if (map == NULL) {
        s_log_error("hashmap_create() failed! Stop.");
        goto err;
    }
    for (u32 i = 0; i < MAP_SIZE; i++) {
        s_log_debug("hashmap_insert(map, \"%s\", \"%s\")",
            key_value_pairs[i][0], key_value_pairs[i][1]
        );
        if (hashmap_insert(map, key_value_pairs[i][0], key_value_pairs[i][1])) {
            s_log_error(
                "Failed to insert key \"%s\" and value \"%s\". Stop.",
                key_value_pairs[i][0], key_value_pairs[i][1]
            );
            goto err;
        }
    }

    dump_hashmap(map, LOG_INFO);

    for (u32 i = 0; i < MAP_SIZE; i++) {
        s_log_info("hashmap_lookup_record(\"%s\") -> \"%s\"",
            key_value_pairs[i][0],
            hashmap_lookup_record(map, key_value_pairs[i][0])
        );
    }

    s_log_debug("Destroying hashmap...");
    hashmap_destroy(map);

    s_log_info("Test result is OK");
    return EXIT_SUCCESS;

err:
    s_log_info("Test result is FAIL");
    if (map) hashmap_destroy(map);
    return EXIT_FAILURE;
}

void dump_hashmap(struct hashmap *map, s_log_level log_level)
{
    if (map == NULL) return;

    s_log(log_level, "hashmaptest", "===== BEGIN HASHMAP DUMP =====");
    for (u32 i = 0; i < map->length; i++) {
#define BUF_SIZE 512
        char buf[BUF_SIZE] = { 0 };

        snprintf(buf, BUF_SIZE - 1, "list %i >", i);

        if (map->bucket_lists[i] == NULL)
            goto line_end;

        struct ll_node *curr_node = (struct ll_node *)map->bucket_lists[i]->tail;
        while (curr_node != NULL) {
            struct hashmap_record *curr_record = (struct hashmap_record *)curr_node->content;
            if (curr_node->content == NULL)
                goto line_end;

            char tmp_buf[BUF_SIZE] = { 0 };
            snprintf(tmp_buf, BUF_SIZE - 1, " { \"%s\", \"%s\" }%s",
                curr_record->key, (const char *)curr_record->value,
                curr_node->next != NULL ? " ->" : "\n"
            );
            strncat(buf, tmp_buf, BUF_SIZE - strlen(buf) - 1);

            curr_node = curr_node->next;
        }

line_end:
        s_log(log_level, "hashmaptest", "%s", buf);
    }
    s_log(log_level, "hashmaptest", "===== END HASHMAP DUMP =====");
}
