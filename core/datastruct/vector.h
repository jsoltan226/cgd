#ifndef VECTOR_H_
#define VECTOR_H_

#include "core/int.h"
#include <stdbool.h>
#include <stdlib.h>

/* Create a new vector of type `T` */
#define vector_new(T) ((T *)vector_init(sizeof(T)))
void * vector_init(u32 item_size);

/* Get the element at `index` from `v` */
#define vector_at(v, index) \
    (v != NULL && index > 0 && index < vector_size(v) ? v[index] : (typeof(*v))0)

/* Append `item` to `v` */
#define vector_push_back(v, item) do {  \
    typeof(item) _i = item;             \
    v = vector_push_back_p(v, &(_i));   \
} while (0)
void * vector_push_back_p(void *v, void *item);

/* Remove the last element from `v` */
#define vector_pop_back(v) do { v = vector_pop_back_p(v); } while (0)
void * vector_pop_back_p(void *v);

/* Insert `item` to `v` at index `at` */
#define vector_insert(v, at, item) do { \
    typeof(item) _i = item;             \
    v = vector_insert_p(v, at, &(_i));  \
} while (0)
void * vector_insert_p(void *v, u32 at, void *item);

/* Remove element from `v` at index `at` */
#define vector_erase(v, at) do { v = vector_erase_p(v, at); } while (0)
void * vector_erase_p(void *v, u32 at);

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
void * vector_shrink_to_fit_p(void *v);

/* Reset the size of `v` and memset is to 0,
 * but leave the allocated capacity unchanged */
void vector_clear(void *v);

/* Works the same as `realloc()`, but takes into account the vector metadata,
 * although note that `vector_realloc(NULL, new_capacity)` won't do anything! */
/* This function is not intended to be used by the user */
void * vector_realloc(void *v, u32 new_capacity);

/* Increase the capacity of `v` to `count` 
 * (if `count` is greater than the capacity of `v`) */
#define vector_reserve(v, count) do {               \
    if (v != NULL && count > vector_capacity(v))    \
        v = vector_realloc(v, count);               \
} while (0)

/* Resize `v` to `new_size`, 
 * cutting off any elements at index greater than `new_size` */
#define vector_resize(v, new_size) do { v = vector_resize_p(v, new_size); } while (0);
void * vector_resize_p(void *v, u32 new_size);

#define vector_copy vector_clone
void * vector_clone(void *v);

/* Destroy `v` */
void vector_destroy(void *v);

#endif /* VECTOR_H_ */
