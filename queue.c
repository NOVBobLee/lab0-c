#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */

/*
 * Create empty queue.
 * Return NULL if could not allocate space.
 */
struct list_head *q_new()
{
    struct list_head *queue;

    queue = malloc(sizeof(struct list_head));
    if (queue)
        INIT_LIST_HEAD(queue);

    return queue;
}

/* Free all storage used by queue */
void q_free(struct list_head *l)
{
    element_t *elm, *elm_safe;

    if (!l)
        return;

    /* free the queue elements */
    list_for_each_entry_safe (elm, elm_safe, l, list)
        q_release_element(elm);

    /* free the queue itself, the list head */
    free(l);
}

/*
 * Allocate an element_t which member value has length value_size.
 * @value_length: size of member value (byte)
 *
 * Return non-NULL if successful.
 * Return NULL if failed to allocate the space.
 */
static element_t *element_alloc(size_t value_size)
{
    element_t *elm;
    char *value;

    value = malloc(value_size);
    if (!value)
        goto fail_alloc_value;

    elm = malloc(sizeof(element_t));
    if (!elm)
        goto fail_alloc_elm;

    /* assemble the element_t and value */
    elm->value = value;

    /* Poison the list node to prevent anyone using the unlinked list node */
#ifdef LIST_POISONING
    elm->list->prev = (struct list_head *) (0x00100100);
    elm->list->next = (struct list_head *) (0x00200200);
#endif

    return elm;

fail_alloc_elm:
    free(value);
fail_alloc_value:
    return NULL;
}

/*
 * Attempt to insert element at head of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(struct list_head *head, char *s)
{
    element_t *elm;
    int s_length;

    if (!head)
        return false;

    s_length = strlen(s);
    elm = element_alloc(s_length + 1);
    if (!elm)
        return false;

    /* copy the string content */
    memcpy(elm->value, s, s_length);
    elm->value[s_length] = '\0';

    /* add the list into the head of head */
    list_add(&elm->list, head);

    return true;
}

/*
 * Attempt to insert element at tail of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(struct list_head *head, char *s)
{
    element_t *elm;
    int s_length;

    if (!head)
        return false;

    s_length = strlen(s);
    elm = element_alloc(s_length + 1);
    if (!elm)
        return false;

    /* copy the string content */
    memcpy(elm->value, s, s_length);
    elm->value[s_length] = '\0';

    /* add the list into the tail of head */
    list_add_tail(&elm->list, head);

    return true;
}

/*
 * Attempt to remove element from head of queue.
 * Return target element.
 * Return NULL if queue is NULL or empty.
 * If sp is non-NULL and an element is removed, copy the removed string to *sp
 * (up to a maximum of bufsize-1 characters, plus a null terminator.)
 *
 * NOTE: "remove" is different from "delete"
 * The space used by the list element and the string should not be freed.
 * The only thing "remove" need to do is unlink it.
 *
 * REF:
 * https://english.stackexchange.com/questions/52508/difference-between-delete-and-remove
 */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    element_t *elm;

    if (!head || list_empty(head))
        return NULL;

    elm = list_first_entry(head, element_t, list);

    /* copy the string if sp is non-NULL */
    if (sp) {
        memcpy(sp, elm->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }

    /* remove the list from head */
    list_del(&elm->list);

    return elm;
}

/*
 * Attempt to remove element from tail of queue.
 * Other attribute is as same as q_remove_head.
 */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    element_t *elm;

    if (!head || list_empty(head))
        return NULL;

    elm = list_last_entry(head, element_t, list);

    /* copy the string if sp is non-NULL */
    if (sp) {
        memcpy(sp, elm->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }

    /* remove the list from head */
    list_del(&elm->list);

    return elm;
}

/*
 * WARN: This is for external usage, don't modify it
 * Attempt to release element.
 */
void q_release_element(element_t *e)
{
    free(e->value);
    free(e);
}

/*
 * Return number of elements in queue.
 * Return 0 if q is NULL or empty
 */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;

    int len = 0;
    struct list_head *li;

    list_for_each (li, head)
        ++len;

    return len;
}

/*
 * Delete the middle node in list.
 * The middle node of a linked list of size n is the
 * ⌊n / 2⌋th node from the start using 0-based indexing.
 * If there're six element, the third member should be return.
 * Return true if successful.
 * Return false if list is NULL or empty.
 */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    struct list_head *fast, *slow;

    if (!head || list_empty(head))
        return false;

    /* two traverses with different speeds until fast meets head */
    fast = head->next;
    slow = head->next;
    while (fast != head && fast->next != head) {
        slow = slow->next; /* will stop on the middle node */
        fast = fast->next->next;
    }

    /* remove the middle node from head */
    list_del(slow);

    /* delete the relative element */
    q_release_element(list_entry(slow, element_t, list));

    return true;
}

/*
 * Delete all nodes that have duplicate string,
 * leaving only distinct strings from the original list.
 * Return true if successful.
 * Return false if list is NULL.
 *
 * Note: this function always be called after sorting, in other words,
 * list is guaranteed to be sorted in ascending order.
 */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    element_t *elm, *next_elm;
    int found_dup = 0;

    if (!head)
        return false;

    /* if head is empty or singular, then it's done */
    if (list_empty(head) || list_is_singular(head))
        return true;

    /* traverse the whole head and delete the duplicate nodes one by one */
    list_for_each_entry_safe (elm, next_elm, head, list) {
        int cmp_result =
            (elm->list.next != head) && !strcmp(elm->value, next_elm->value);
        if (cmp_result || found_dup) {
            list_del(&elm->list);
            q_release_element(elm);
            found_dup = cmp_result;
        }
    }

    return true;
}

/*
 * Attempt to swap every two adjacent nodes.
 */
void q_swap(struct list_head *head)
{
    // https://leetcode.com/problems/swap-nodes-in-pairs/
    struct list_head *a, *b;

    if (!head || list_empty(head) || list_is_singular(head))
        return;

    /* initialize */
    a = head->next;
    b = a->next;

    do {
        /* swap a, b */
        a->prev->next = b;
        b->prev = a->prev;
        a->prev = b;
        a->next = b->next;
        b->next->prev = a;
        b->next = a;

        /* update a, b for next iteration */
        a = a->next;
        b = a->next;
    } while (a != head && b != head);
}

/*
 * Reverse elements in queue
 * No effect if q is NULL or empty
 * This function should not allocate or free any list elements
 * (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
 * It should rearrange the existing ones.
 */
void q_reverse(struct list_head *head) {}

/*
 * Sort elements of queue in ascending order
 * No effect if q is NULL or empty. In addition, if q has only one
 * element, do nothing.
 */
void q_sort(struct list_head *head) {}
