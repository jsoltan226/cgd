#include "core/datastruct/vector.h"
#include "core/log.h"
#include <stdlib.h>
#include <string.h>

struct large_struct {
    u64 arr[16];
    const char *str;
};
static u64 *vector_small = NULL;
static struct large_struct *vector_large = NULL;

#define N_LARGE_STRUCTS 32
static struct large_struct large_items[N_LARGE_STRUCTS];

#define N_SMALL_ITEMS 64
static u64 small_items[N_SMALL_ITEMS];

#define goto_error(msg...) do {     \
    s_log_error("vectortest", msg); \
    goto err;                       \
} while (0)

int main(void)
{
    s_set_log_out_filep(stdout);
    s_set_log_err_filep(stdout);
    s_set_log_level(LOG_DEBUG);

    s_log_debug("vectortest", "Initializing vectors...");
    vector_small = vector_new(u64);
    if (vector_small == NULL)
        goto_error("Failed to initialize vector_small!");

    vector_large = vector_new(struct large_struct);
    if (vector_large == NULL)
        goto_error("Failed to initialize vector_large!");

    s_log_debug("vectortest", "Testing vector_push_back...");

    for (u32 i = 0; i < N_SMALL_ITEMS; i++)
        vector_push_back(vector_small, small_items[i]);

    for (u32 i = 0; i < N_LARGE_STRUCTS; i++)
        vector_push_back(vector_large, large_items[i]);

    if (memcmp(vector_small, small_items, sizeof(small_items)))
        goto_error("`vector_small->items` does not match `small_items`");
    if (memcmp(vector_large, large_items, sizeof(large_items)))
        goto_error("`vector_large->items` does not match `large_items`");

    s_log_debug("vectortest", "Testing vector_pop_back (and vector_back)...");
    vector_pop_back(vector_small);
    vector_pop_back(vector_small);
    if (small_items[N_SMALL_ITEMS - 3] != vector_back(vector_small))
        goto_error("`vector_small->items` does not match `small_items`");

    s_log_debug("vectortest", "Testing vector_insert and vector_erase...");
    vector_insert(vector_large, 3, (struct large_struct){ .str = "inserted item" });
    if (strcmp(vector_large[3].str, "inserted item")) {
        s_log_error("vectortest", "vector_insert test failed; string is \"%s\"", vector_large[3].str);
        goto err;
    }
    
    vector_erase(vector_large, 3);
    if (!strcmp(vector_large[3].str, "inserted item")) {
        s_log_error("vectortest", "vector_erase test failed; string is \"%s\"", vector_large[3].str);
        goto err;
    }

    s_log_debug("vectortest", "Testing vector_begin, vector_front and vector_end...");
    u64 *vec_begin_val = vector_begin(vector_small);
    if (vec_begin_val != vector_small)
        goto_error("vector_begin test failed; expected value %p, got %p",
            vector_small, vec_begin_val);
    
    if (vector_front(vector_small) != vector_small[0])
        goto_error("vector_from test failed; expected value %lu, got %lu",
            vector_small[0], vector_front(vector_small));

    u64 *vector_end_val = vector_end(vector_small);
    vector_push_back(vector_small, 3);
    if (vector_back(vector_small) != *vector_end_val)
        goto_error("vector_end test failed, expected value %lu, got %lu",
            vector_back(vector_small), *vector_end_val);

    s_log_debug("vectortest", "Testing vector_clone, vector_empty and vector_clear...");
    u64 *vector_cloned = vector_clone(vector_small);
    if (
        vector_cloned == NULL || 
        memcmp(vector_cloned, vector_small, vector_size(vector_small) * sizeof(u64))
    ) {
        goto_error("vector_clone test failed");
    }
    
    vector_clear(vector_cloned);

    if (vector_empty(vector_small) || !vector_empty(vector_cloned))
        goto_error("vector_empty test failed");

    s_log_debug("vectortest", "Testing vector_capacity and vector_shrink_to_fit...");
    vector_shrink_to_fit(vector_cloned);
    if (vector_capacity(vector_cloned) != 0)
        goto_error("vector_shrink_to_fit test failed; expected value 0, got %u",
            vector_capacity(vector_cloned));

    vector_destroy(vector_cloned);
    
    s_log_debug("vectortest", "Testing vector_resize and vector_at...");
    vector_resize(vector_small, 2);
    if (vector_back(vector_small) != vector_at(vector_small, 1))
        goto_error("vector_resize test failed; expected value %lu, got %lu",
            vector_at(vector_small, 1), vector_back(vector_small));

    vector_destroy(vector_small);
    vector_destroy(vector_large);

    s_log_info("vectortest", "Test result is OK");

    return EXIT_SUCCESS;

err:
    if (vector_small != NULL) vector_destroy(vector_small);
    if (vector_large != NULL) vector_destroy(vector_large);
    s_log_info("vectortest", "Test result is FAIL");
    return EXIT_FAILURE;
}

static struct large_struct large_items[N_LARGE_STRUCTS] = {
    (struct large_struct) { .arr = { 0 }, .str = "0" },
    (struct large_struct) { .arr = { 1 }, .str = "1" },
    (struct large_struct) { .arr = { 2 }, .str = "2" },
    (struct large_struct) { .arr = { 3 }, .str = "3" },
    (struct large_struct) { .arr = { 4 }, .str = "4" },
    (struct large_struct) { .arr = { 5 }, .str = "5" },
    (struct large_struct) { .arr = { 6 }, .str = "6" },
    (struct large_struct) { .arr = { 7 }, .str = "7" },
    (struct large_struct) { .arr = { 8 }, .str = "8" },
    (struct large_struct) { .arr = { 9 }, .str = "9" },
    (struct large_struct) { .arr = { 10 }, .str = "10" },
    (struct large_struct) { .arr = { 11 }, .str = "11" },
    (struct large_struct) { .arr = { 12 }, .str = "12" },
    (struct large_struct) { .arr = { 13 }, .str = "13" },
    (struct large_struct) { .arr = { 14 }, .str = "14" },
    (struct large_struct) { .arr = { 15 }, .str = "15" },
    (struct large_struct) { .arr = { 16 }, .str = "16" },
    (struct large_struct) { .arr = { 17 }, .str = "17" },
    (struct large_struct) { .arr = { 18 }, .str = "18" },
    (struct large_struct) { .arr = { 19 }, .str = "19" },
    (struct large_struct) { .arr = { 20 }, .str = "20" },
    (struct large_struct) { .arr = { 21 }, .str = "21" },
    (struct large_struct) { .arr = { 22 }, .str = "22" },
    (struct large_struct) { .arr = { 23 }, .str = "23" },
    (struct large_struct) { .arr = { 24 }, .str = "24" },
    (struct large_struct) { .arr = { 25 }, .str = "25" },
    (struct large_struct) { .arr = { 26 }, .str = "26" },
    (struct large_struct) { .arr = { 27 }, .str = "27" },
    (struct large_struct) { .arr = { 28 }, .str = "28" },
    (struct large_struct) { .arr = { 29 }, .str = "29" },
    (struct large_struct) { .arr = { 30 }, .str = "30" },
    (struct large_struct) { .arr = { 31 }, .str = "31" },
};

static u64 small_items[N_SMALL_ITEMS] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, 62, 63
};
