#include "hashmap.h"
#include "linked-list.h"
#include "core/int.h"
#include "core/log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static inline u32 hash(const char *key, u32 max);
static struct ll_node * lookup_bucket_list_node(struct hashmap *map, const char *key);

struct hashmap * hashmap_create(u32 initial_length)
{
    struct hashmap *map = malloc(sizeof(struct hashmap));
    if (map == NULL) {
        s_log_error("hashmap", "malloc() for struct hashmap failed!");
        return NULL;
    }

    map->length = initial_length;
    map->n_elements = 0;

    map->bucket_lists = calloc(initial_length, sizeof(struct linked_list *));
    if (map->bucket_lists == NULL) {
        s_log_error("hashmap", "calloc() for map->bucket_lists failed!");
        free(map);
        return NULL;
    }

    return map;
}

i32 hashmap_insert(struct hashmap *map, const char *key, const void *entry)
{
    if (map == NULL) return 1;

    u32 index = hash(key, map->length);

    struct hashmap_record *new_record = malloc(sizeof(struct hashmap_record));
    if (new_record == NULL) {
        s_log_error("hashmap", "hashmap_insert: malloc() for new record failed!");
        return 1;
    }

    new_record->value = (void *)entry;
    strncpy(new_record->key, key, HM_MAX_KEY_LENGTH);
    new_record->key[HM_MAX_KEY_LENGTH - 1] = '\0';

    if (map->bucket_lists[index] == NULL) {
        map->bucket_lists[index] = linked_list_create(new_record);
        if (map->bucket_lists[index] == NULL) {
            s_log_error("hashmap",
                "hashmap_insert: linked_list_create() for bucket list @ index %i returned NULL!",
                index
            );
            free(new_record);
            return 1;
        }
    } else {
        map->bucket_lists[index]->head = linked_list_append(
            map->bucket_lists[index]->head,
            new_record
        );
    }

    map->n_elements++;

    return 0;
}

void * hashmap_lookup_record(struct hashmap *map, const char *key)
{
    struct ll_node *node = lookup_bucket_list_node(map, key);

    if (node == NULL || node->content == NULL)
        return NULL;
    else
        return ((struct hashmap_record *)(node->content))->value;
}

void hashmap_delete_record(struct hashmap *map, const char *key)
{
    struct ll_node *node = lookup_bucket_list_node(map, key);
    free(node->content);
    linked_list_destroy_node(node);
}

void hashmap_destroy(struct hashmap *map)
{
    for (u32 i = 0; i < map->length; i++) {
        if (map->bucket_lists[i] != NULL) linked_list_destroy(map->bucket_lists[i]);
    }
    map->bucket_lists = NULL;

    free(map);
}

static inline u32 hash(const char *key, u32 max)
{
    return ((key[0] % max) * key[strlen(key) - 1]) % max;
}

static struct ll_node * lookup_bucket_list_node(struct hashmap *map, const char *key)
{
    u32 index = hash(key, map->length);

    if (map->bucket_lists[index] == NULL) return NULL;

    struct ll_node *curr_node = (map->bucket_lists[index])->tail;
    while (curr_node != NULL) {
        if (curr_node->content != NULL) {
            /* Return the current node if the keys match */
            if (!strncmp(
                ((struct hashmap_record *)curr_node->content)->key,
                key,
                HM_MAX_KEY_LENGTH
            )) return curr_node;
        }
        curr_node = curr_node->next;
    }

    return NULL;
}
