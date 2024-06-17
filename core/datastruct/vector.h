#ifndef VECTOR_H_
#define VECTOR_H_

#ifdef __cplusplus
#error What are you doing using this header? Use std::vector instead!
#endif

/* Prevent internal functions from being called by the user */
#define VECTOR_INTERNAL__

#include "core/int.h"
#include "core/log.h"
#include <stdbool.h>
#include <stdlib.h>

/* Create a new vector of type `T` */
#define vector_new(T) ((T *)vector_init(sizeof(T)))
void * vector_init(u32 item_size);

/* Get the element at `index` from `v` */
/* We are using C, so we can forget about array bounds checking :) */
#define vector_at(v, index) (v[index])

/* Append `item` to `v` */
#define vector_push_back(v, item...) do {                               \
    if (v == NULL)                                                      \
        s_log_fatal("vector", "vector_push_back", "invalid_parameters");\
    v = vector_increase_size(v);                                        \
    v[vector_size(v) - 1] = item;                                       \
} while (0)
#ifdef VECTOR_INTERNAL__
void * vector_increase_size(void *v);
#endif /* VECTOR_INTERNAL__ */

/* Remove the last element from `v` */
#define vector_pop_back(v) do { v = vector_pop_back_p(v); } while (0)
#ifdef VECTOR_INTERNAL__
void * vector_pop_back_p(void *v);
#endif /* VECTOR_INTERNAL__ */

/* Insert `item` to `v` at index `at` */
#define vector_insert(v, at, item...) do {                              \
    if (v == NULL || at < 0 || at > vector_size(v))                     \
        s_log_fatal("vector", "vector_insert", "invalid parameters");   \
    v = vector_increase_size(v);                                        \
    vector_memmove(v, at, at + 1, vector_size(v) - at);                 \
    v[at] = item;                                                       \
} while (0)
#ifdef VECTOR_INTERNAL__
/* Won't do anything if the vector doesn't have enough spare capacity */
void vector_memmove(void *v, u32 src_index, u32 dst_index, u32 nmemb);
#endif

/* Remove element from `v` at index `at` */
#define vector_erase(v, at) do { v = vector_erase_p(v, at); } while (0)
#ifdef VECTOR_INTERNAL__
void * vector_erase_p(void *v, u32 at);
#endif /* VECTOR_INTERNAL__ */

/* Return the pointer to the first element of `v` */
#define vector_begin(v) (v)

/* Return the first element of `v` */
#define vector_front(v) (v ? v[0] : 0)

/* Return the pointer to the element immidiately after the last one in `v` */
void * vector_end(void *v);

/* Return the last element */
#define vector_back(v) (vector_at(v, vector_size(v) - 1))

/* Check whether `v` is empty */
bool vector_empty(void *v);

/* Return the size of `v` (number of elements) */
u32 vector_size(void *v);

/* Return the allocated capacity of `v` */
u32 vector_capacity(void *v);

/* Shrink `capacity` to `size` */
#define vector_shrink_to_fit(v) do { v = vector_shrink_to_fit_p(v); } while(0)
#ifdef VECTOR_INTERNAL__
void * vector_shrink_to_fit_p(void *v);
#endif /* VECTOR_INTERNAL__ */

/* Reset the size of `v` and memset is to 0,
 * but leave the allocated capacity unchanged */
void vector_clear(void *v);

/* Works the same as `realloc()`, but takes into account the vector metadata,
 * although note that `vector_realloc(NULL, new_capacity)` won't do anything! */
/* This function is not intended to be used by the user */
#ifdef VECTOR_INTERNAL__
void * vector_realloc(void *v, u32 new_capacity);
#endif /* VECTOR_INTERNAL__ */

/* Increase the capacity of `v` to `count`
 * (if `count` is greater than the capacity of `v`) */
#define vector_reserve(v, count) do {                                   \
    if (v == NULL)                                                      \
        s_log_fatal("vector", "vector_reserve", "invalid parameters");  \
    if (count > vector_size(v))                                         \
        v = vector_realloc(v, count);                                   \
} while (0)

/* Resize `v` to `new_size`,
 * cutting off any elements at index greater than `new_size` */
#define vector_resize(v, new_size) do { v = vector_resize_p(v, new_size); } while (0);
#ifdef VECTOR_INTERNAL__
void * vector_resize_p(void *v, u32 new_size);
#endif /* VECTOR_INTERNAL__ */

#define vector_copy vector_clone
void * vector_clone(void *v);

/* Destroy `v` */
void vector_destroy(void *v);

#undef VECTOR_INTERNAL__

#endif /* VECTOR_H_ */
