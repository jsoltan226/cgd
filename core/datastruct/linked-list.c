#include "linked-list.h"
#include "core/log.h"
#include <stdlib.h>

struct linked_list * linked_list_create(void *head_content)
{
    struct linked_list *ll = malloc(sizeof(struct linked_list));
    if (ll == NULL) {
        s_log_error("linked-list", "malloc() for struct linked_list failed!");
        return NULL;
    }

    struct ll_node *first_node = linked_list_create_node(head_content);
    if (first_node == NULL) {
        s_log_error("linked-list", "linked_list_create_node() returned NULL!");
        free(ll);
        return NULL;
    }

    ll->head = first_node;
    ll->tail = first_node;
    return ll;
}

struct ll_node * linked_list_append(struct ll_node *at, void *content)
{
    struct ll_node *new_node = calloc(1, sizeof(struct ll_node));
    if (new_node == NULL) {
        s_log_error("linked-list", "linked_list_append: calloc() for new_node failed!");
        return NULL;
    }

    if (at != NULL) {
        new_node->next = at->next;
        new_node->prev = at;
        if (at->next != NULL) at->next->prev= new_node;
        at->next = new_node;
    }

    new_node->content = content;
    return new_node;
}

struct ll_node * linked_list_prepend(struct ll_node *at, void *content)
{
    struct ll_node *new_node = calloc(1, sizeof(struct ll_node));
    if (new_node == NULL) {
        s_log_error("linked-list", "linked_list_prepend: calloc() for new_node failed!");
        return NULL;
    }

    if (at != NULL) {
        new_node->prev = at->prev;
        new_node->next = at;
        if (at->prev != NULL) at->prev->next = new_node;
        at->prev = new_node;
    }

    new_node->content = content;
    return new_node;
}

void linked_list_destroy_node(struct ll_node *node)
{
    if (node->prev != NULL) node->prev->next = node->next;
    if (node->next != NULL) node->next->prev = node->prev;
    free(node);
}

void linked_list_destroy(struct linked_list *list)
{
    if (list == NULL) return;

    linked_list_recursive_destroy_nodes(list->head);
    free(list);
}

void linked_list_recursive_destroy_nodes(struct ll_node *head)
{
    struct ll_node *curr_node = head;
    while (curr_node != NULL) {
        struct ll_node *next_node = curr_node->next;
        free(curr_node);
        curr_node = next_node;
    };
}
