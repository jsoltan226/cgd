#include "vector.h"
#include "core/int.h"
#include "core/log.h"
#include "core/math.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct vector_metadata {
    const u32 item_size;
    u32 n_items;
    u32 capacity;
};

#define VECTOR_DEFAULT_CAPACITY 8

#define get_metadata_ptr(v) \
    ((struct vector_metadata *)(v - sizeof(struct vector_metadata)))

#define element_at(v, at) (v + (at * get_metadata_ptr(v)->item_size))

void * vector_init(u32 item_size)
{
    void *v = malloc(sizeof(struct vector_metadata) + (item_size * VECTOR_DEFAULT_CAPACITY));
    if (v == NULL) {
        s_log_error("vector", "malloc() failed for new vector!");
        return NULL;
    }
    memset(v, 0, sizeof(struct vector_metadata) + (item_size * VECTOR_DEFAULT_CAPACITY));

    struct vector_metadata *metadata_ptr = v;
    *(u32*)(&metadata_ptr->item_size) = item_size; /* Cast away `const` */
    metadata_ptr->n_items = 0;
    metadata_ptr->capacity = VECTOR_DEFAULT_CAPACITY;

    v += sizeof(struct vector_metadata);
    return v;
}

void * vector_push_back_p(void *v, void *item)
{
    if (v == NULL || item == NULL) return NULL;
    
    struct vector_metadata *meta = get_metadata_ptr(v);
    
    if (meta->n_items >= meta->capacity) {
        v = vector_realloc(v, meta->capacity * 2);
        meta = get_metadata_ptr(v); /* `meta` might have been moved by `realloc()` */
        meta->capacity *= 2;
    }

    memcpy(element_at(v, meta->n_items), item, meta->item_size);
    meta->n_items++;
    return v;
}

void * vector_pop_back_p(void *v)
{
    if (v == NULL) return NULL;

    struct vector_metadata *meta = get_metadata_ptr(v);
    
    meta->n_items--;
    memset(element_at(v, meta->n_items), 0, meta->item_size);

    if (meta->n_items <= (meta->capacity / 2)) {
        v = vector_realloc(v, meta->capacity / 2);
        meta = get_metadata_ptr(v);
        meta->capacity /= 2;
    }

    return v;
}

void * vector_insert_p(void *v, u32 at, void *item)
{
    if (v == NULL || item == NULL) return NULL;

    struct vector_metadata *meta = get_metadata_ptr(v);
    
    if (meta->n_items >= meta->capacity) {
        v = vector_realloc(v, meta->capacity * 2);
        meta = get_metadata_ptr(v); /* `meta` might have been moved by `realloc()` */
        meta->capacity *= 2;
    }

    /* Move all items after `at` 1 spot to the right */
    memcpy(
        element_at(v, at + 1),
        element_at(v, at),
        (meta->n_items - at) * meta->item_size
    );

    memmove(element_at(v, at), item, meta->item_size);

    return v;
}

bool vector_empty(void *v)
{
    return v == NULL ? true : get_metadata_ptr(v)->n_items == 0;
}

u32 vector_size(void *v)
{
    return v == NULL ? 0 : get_metadata_ptr(v)->n_items;
}

u32 vector_capacity(void *v)
{
    return v == NULL ? 0 : get_metadata_ptr(v)->capacity;
}

void * vector_end(void * v)
{
    struct vector_metadata *meta = get_metadata_ptr(v);
    return v + (meta->n_items * meta->item_size);
}

void * vector_shrink_to_fit_p(void *v)
{
    if (v == NULL) return NULL;

    struct vector_metadata *meta = get_metadata_ptr(v);
    v = vector_realloc(v, meta->n_items);

    meta = get_metadata_ptr(v);
    meta->capacity = meta->n_items;

    return v;
}

void vector_clear(void *v)
{
    if (v == NULL) return;

    struct vector_metadata *meta = get_metadata_ptr(v);

    memset(v, 0, meta->n_items * meta->item_size);
    meta->n_items = 0;
}

void * vector_erase_p(void *v, u32 index)
{
    struct vector_metadata *meta = get_metadata_ptr(v);

    /* Move all memory right of `index` one spot to the left, and pop the last spot */
    memmove(element_at(v, index), element_at(v, index + 1), meta->item_size);
    return vector_pop_back_p(v);
}

void * vector_realloc(void *v, u32 new_cap)
{
    /* Unfortunately if `v` is NULL we do not know the element size,
     * and so we cannot make it work like realloc(NULL, size) would
     */
    if (v == NULL) return NULL;

    void *new_v = NULL;

    struct vector_metadata *meta_p = get_metadata_ptr(v);
    new_v = realloc(meta_p,
        (new_cap * meta_p->item_size) + sizeof(struct vector_metadata));

    if (new_v == NULL) {
        s_log_error("vector", "realloc() failed!");
        return NULL;
    }

    new_v += sizeof(struct vector_metadata);
    return new_v;
}

void * vector_resize_p(void *v, u32 new_size)
{
    v = vector_realloc(v, new_size);
    if (v == NULL) return NULL;

    struct vector_metadata *meta = get_metadata_ptr(v);
    meta->capacity = new_size;
    meta->n_items = min(new_size, meta->n_items);

    return v;
}

void * vector_clone(void *v)
{
    if (v == NULL) return NULL;

    struct vector_metadata *meta_p = get_metadata_ptr(v);

    void *new_v = vector_init(meta_p->item_size);
    if (new_v == NULL) return NULL;

    new_v = vector_realloc(new_v, meta_p->capacity);

    memcpy(get_metadata_ptr(new_v), meta_p,
        (meta_p->capacity * meta_p->item_size) + sizeof(struct vector_metadata)
    );

    return new_v;
}

void vector_destroy(void *v)
{
    if (v == NULL) return;
    free(get_metadata_ptr(v));
}
