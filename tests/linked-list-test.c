#include <core/int.h>
#include <core/log.h>
#include <core/datastruct/linked-list.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "linked-list-test"

#define LIST_LENGTH 25

static struct linked_list *ll = NULL;
#define ITEM_STR_LEN 8
static char item_str_table[ITEM_STR_LEN * LIST_LENGTH] = { 0 };

int main(void)
{
    s_configure_log(LOG_DEBUG, stdout, stderr);

    s_log_debug("Creating linked list...");
    ll = linked_list_create("item0");
    if (ll == NULL) {
        s_log_error("linked_list_create() failed! Stop.");
        goto err;
    }

    s_log_debug("Populating list...");
    struct ll_node *curr_node = ll->head;
    for (u32 i = 1; i < LIST_LENGTH; i++) {
        char *curr_str = &item_str_table[i * ITEM_STR_LEN];
        snprintf(curr_str, ITEM_STR_LEN, "item%u", i);

        s_log_info("linked_list_append(ll, \"%s\")", curr_str);
        if (curr_node = linked_list_append(curr_node, (void *)curr_str), curr_node == NULL) {
            s_log_error("Failed to append \"%s\" to the list. Stop.", curr_str);
            goto err;
        }
    }

    s_log_debug("Dumping list...");
    curr_node = ll->head;
#define BUF_SIZE 512
    char buf[BUF_SIZE] = { 0 };
    strcpy(buf, "HEAD -> ");
    while (curr_node != NULL) {
        strncat(buf, (char *)curr_node->content, BUF_SIZE - strlen(buf) - 1);
        strncat(buf, " -> ", BUF_SIZE - strlen(buf) - 1);
        curr_node = curr_node->next;
    }
    strncat(buf, "TAIL", BUF_SIZE - strlen(buf) - 1);

    s_log_info("list: %s", buf);

    s_log_debug("OK, Cleaning up...");
    linked_list_destroy(ll, false);

    s_log_info("Test result is OK");
    return EXIT_SUCCESS;

err:
    s_log_info("Test result is FAIL");
    if (ll) linked_list_destroy(ll, false);
    return EXIT_FAILURE;
}
