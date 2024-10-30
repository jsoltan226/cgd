#include <core/log.h>
#include <core/util.h>
#include <core/vector.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define MODULE_NAME "vectortest"

struct large_struct {
    u64 arr[16];
    const char *str;
};

#define N_LARGE_STRUCTS 32
static const struct large_struct large_items[N_LARGE_STRUCTS];

#define N_SMALL_ITEMS 64
static const u64 small_items[N_SMALL_ITEMS];

static bool ERROR = false;
static i32 test();
static u32 random_u32();

#define N_ITERATIONS 20000

int cgd_main(int argc, char **argv)
{
    s_configure_log(LOG_DEBUG, stdout, stderr);

    struct timeval start = { 0 }, stop = { 0 };
    gettimeofday(&start, NULL);

    s_log_debug("Running %u iterations...", N_ITERATIONS);
    for (u32 i = 0; i < N_ITERATIONS; i++) {
        s_log_info("Iteration %u", i);
        if (test()) {
            s_log_info("Test result is FAIL");
            return EXIT_FAILURE;
        }
    }

    gettimeofday(&stop, NULL);
    i64 deltatime_microseconds = ((stop.tv_sec - start.tv_sec)*1000000 + (stop.tv_usec - start.tv_usec));
    s_log_info("[PROFILING]: Total n iterations: %u", N_ITERATIONS);
    s_log_info("[PROFILING]: Time (s): %lf",
        (f64)deltatime_microseconds/1000000.f
    );
    s_log_info("[PROFILING]: Iterations/s: %lf",
        (f64)(N_ITERATIONS*1000000.f) / (f64)deltatime_microseconds
    );

    s_log_info("Test result is OK");

    return EXIT_SUCCESS;
}

#undef MODULE_NAME
#define MODULE_NAME "vectortest:test"
static i32 test()
{
    u64 *vector_small = NULL;
    struct large_struct *vector_large = NULL;
    u64 *vector_cloned = NULL;

    s_log_debug("Initializing vectors...");
    vector_small = vector_new(u64);
    if (vector_small == NULL)
        goto_error("Failed to initialize vector_small!");

    vector_large = vector_new(struct large_struct);
    if (vector_large == NULL)
        goto_error("Failed to initialize vector_large!");

    s_log_debug("Testing vector_push_back...");

    for (u32 i = 0; i < N_SMALL_ITEMS; i++)
        vector_push_back(vector_small, small_items[i]);

    for (u32 i = 0; i < N_LARGE_STRUCTS; i++)
        vector_push_back(vector_large, large_items[i]);

    if (memcmp(vector_small, small_items, sizeof(small_items)))
        goto_error("`vector_small->items` does not match `small_items`");
    if (memcmp(vector_large, large_items, sizeof(large_items)))
        goto_error("`vector_large->items` does not match `large_items`");

    u32 vsmall_n_popped_back = random_u32() % ((N_SMALL_ITEMS / 2) - 1) + 1;
    s_log_debug("Testing vector_pop_back (and vector_back) with %u items...",
        vsmall_n_popped_back);
    for (u32 i = 0; i < vsmall_n_popped_back; i++)
        vector_pop_back(vector_small);

    if (small_items[N_SMALL_ITEMS - vsmall_n_popped_back - 1] != vector_back(vector_small))
        goto_error("`vector_small->items` does not match `small_items`");

    u32 vlarge_insert_index = random_u32() % (N_LARGE_STRUCTS - 1);
    s_log_debug("Testing vector_insert (index %u, max %u) and vector_erase...",
        vlarge_insert_index, N_LARGE_STRUCTS - 1);
    vector_insert(vector_large, vlarge_insert_index,
        (struct large_struct){ .arr = { 0 }, .str = "inserted item" }
    );
    if (strcmp(vector_large[vlarge_insert_index].str, "inserted item"))
        goto_error("vector_insert test failed; index is %u, string is \"%s\"",
            vlarge_insert_index, vector_large[vlarge_insert_index].str);

    vector_erase(vector_large, vlarge_insert_index);
    if (!strcmp(vector_large[vlarge_insert_index].str, "inserted item"))
        goto_error("vector_erase test failed; string is \"%s\"",
            vector_large[vlarge_insert_index].str);

    u64 *vsmall_begin_val = vector_begin(vector_small);
    s_log_debug("Testing vector_begin, vector_front and vector_end...",
        vsmall_begin_val
    );
    if (vsmall_begin_val != vector_small)
        goto_error("vector_begin test failed; expected value %p, got %p",
            vector_small, vsmall_begin_val);

    if (vector_front(vector_small) != vector_small[0])
        goto_error("vector_from test failed; expected value %lu, got %lu",
            vector_small[0], vector_front(vector_small));

    u64 *vsmall_end_val_p = vector_end(vector_small);
    u64 vsmall_pushed_val = (u64)random_u32();
    s_log_debug("vector_end() returned %p -> %lu, pushed value is %lu",
        vsmall_end_val_p, *vsmall_end_val_p, vsmall_pushed_val);
    vector_push_back(vector_small, vsmall_pushed_val);
    if (vector_back(vector_small) != *vsmall_end_val_p || *vsmall_end_val_p != vsmall_pushed_val)
        goto_error("vector_end test failed, expected value %lu, got %lu",
            vsmall_pushed_val, *vsmall_end_val_p);

    s_log_debug("Testing vector_clone, vector_empty and vector_clear...");
    vector_cloned = vector_clone(vector_small);
    if (
        vector_cloned == NULL ||
        memcmp(vector_cloned, vector_small, vector_size(vector_small) * sizeof(u64))
    ) {
        goto_error("vector_clone test failed; the cloned arrays are not identical");
    }

    vector_clear(vector_cloned);

    if (vector_empty(vector_small) || !vector_empty(vector_cloned))
        goto_error("vector_empty test failed");

    s_log_debug("Testing vector_capacity and vector_shrink_to_fit...");
    vector_shrink_to_fit(vector_cloned);
    if (vector_capacity(vector_cloned) != 0)
        goto_error("vector_shrink_to_fit test failed; expected value 0, got %u",
            vector_capacity(vector_cloned));

    u32 vsmall_new_size = 1 + random_u32() % (N_SMALL_ITEMS - vsmall_n_popped_back - 1);
    s_log_debug("Testing vector_resize -> new_size %u and vector_at...",
        vsmall_new_size);
    vector_resize(vector_small, vsmall_new_size);
    if (vector_back(vector_small) != vector_at(vector_small, vsmall_new_size - 1))
        goto_error("vector_resize test failed; expected value %lu, got %lu",
            vector_at(vector_small, vsmall_new_size), vector_back(vector_small));

err:
    if (vector_small != NULL) vector_destroy(&vector_small);
    if (vector_large != NULL) vector_destroy(&vector_large);
    if (vector_cloned != NULL) vector_destroy(&vector_cloned);
    return ERROR ? EXIT_FAILURE : EXIT_SUCCESS;
}

static u32 random_u32()
{
    struct timeval time = { 0 };
    gettimeofday(&time, NULL);
    srand(time.tv_usec + rand());
    return rand();
}

static const struct large_struct large_items[N_LARGE_STRUCTS] = {
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

static const u64 small_items[N_SMALL_ITEMS] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, 62, 63
};
