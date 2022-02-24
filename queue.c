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

//#define DEBUG_PRINT
#ifdef DEBUG_PRINT
/*
 * Print the member value correspoding to member list
 * @node: the member list of element_t (struct list_head *)
 * @head: head of the list
 */
#define print_value(node, head) \
    printf("%s: %s\n", #node,   \
           (node != head) ? list_entry(node, element_t, list)->value : "head")
#endif /* DEBUG_PRINT */

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
void q_reverse(struct list_head *head)
{
    struct list_head *a, *b, *tmp;

    if (!head || list_empty(head) || list_is_singular(head))
        return;

    /* initialize */
    a = head->next;
    b = head->prev;

    do {
        /* swap a, b */
        if (a->next != b) { /* if a is not beside b */
            a->prev->next = b;
            b->prev->next = a;
            tmp = a->prev;
            a->prev = b->prev;
            b->prev = tmp;

            a->next->prev = b;
            b->next->prev = a;
            tmp = a->next;
            a->next = b->next;
            b->next = tmp;
        } else {
            a->prev->next = b;
            b->prev = a->prev;
            b->next->prev = a;
            a->next = b->next;
            a->prev = b;
            b->next = a;
        }

        /* update a, b for next iteration */
        tmp = a;
        a = b->next;
        b = tmp->prev;
    } while (a != b && a->prev != b); /* stop when a is b or a precedes b */
}

static struct list_head *merge(struct list_head *a, struct list_head *b)
{
    /* head initial value is meaningless, just for satisfying cppcheck */
    struct list_head *head = NULL, **tail = &head; /* next's tail */

    /* threading the lists in order */
    for (;;) {
        element_t *elm_a = list_entry(a, element_t, list);
        element_t *elm_b = list_entry(b, element_t, list);
        if (strcmp(elm_a->value, elm_b->value) <= 0) {
            *tail = a;
            tail = &a->next;
            a = a->next;
            if (!a) {
                *tail = b;
                break;
            }
        } else {
            *tail = b;
            tail = &b->next;
            b = b->next;
            if (!b) {
                *tail = a;
                break;
            }
        }
    }

    return head;
}

static void merge_restore(struct list_head *head,
                          struct list_head *a,
                          struct list_head *b)
{
    struct list_head *tail = head;

    /* threading in order and restore the prev pointer */
    for (;;) {
        element_t *elm_a = list_entry(a, element_t, list);
        element_t *elm_b = list_entry(b, element_t, list);
        if (strcmp(elm_a->value, elm_b->value) <= 0) {
            tail->next = a;
            a->prev = tail;
            tail = a;
            a = a->next;
            if (!a)
                break;
        } else {
            tail->next = b;
            b->prev = tail;
            tail = b;
            b = b->next;
            if (!b) {
                b = a;
                break;
            }
        }
    }

    /* splice two lists */
    tail->next = b;

    /* restore the remaining nodes' prev pointers */
    do {
        b->prev = tail;
        tail = b;
        b = b->next;
    } while (b);

    /* restore the cycle of doubly linked list */
    tail->next = head;
    head->prev = tail;
}

/*
 * Sort elements of queue in ascending order
 * No effect if q is NULL or empty. In addition, if q has only one
 * element, do nothing.
 *
 * From Linux: lib/list_sort.c
 * bottom-up merge sort (not fully-eager):
 *
 *       prepare-to-merge(4 + 4 = 8)
 *             |   \   tail(10) <~ tail(1011)    1011 = 8 + 2 + 1
 *        prev |    \ /           /               ^   = (4 + 4) + 2 + 1
 *    NULL <-- o <-- o <-- o <-- o  <~~ pending   |
 *            /     /     /     /                 0 bit on 4 (2^2)
 *           o     o     o    NULL               /
 *     next /     /     /            state: [ 4-4-2-1 > 8-2-1 ]
 *         o     o    NULL           count = 11
 *        /     /              list
 *       o     o                 |
 *      /     /         head --> o --> o --> NULL
 *    NULL  NULL             next
 * (older)  (newer)
 *
 * observation:
 *   count = 8, bin: 1000, [ 4-2-1-1 ] -> [ 4-2-2 ]
 *                      ^ the next carry digit
 *   count = 9, bin: 1001, [ 4-2-2-1 ] -> [ 4-4-1 ]
 *                     ^ the next carry digit
 *   count = 15, bin: 1111, [ 8-4-2-1 ]
 *                   ^ the next carry digit (out of pending, no merge)
 *
 *   count   state          merge-num accum-num-from-new-start
 *   1    1  [ 1 ]
 *   2   10  [ 1-1 > 2 ]         1      (new start)
 *   3   11  [ 2-1 ]                        1
 *   4  100  [ 2-1-1 > 2-2 ]     1*     (new start)
 *   5  101  [ 2-2-1 > 4-1 ]     2          1
 *   6  110  [ 4-1-1 > 4-2 ]     1*         2
 *   7  111  [ 4-2-1 ]                      3 (11 = 100 ^ 111)
 *   8 1000  [ 4-2-1-1 > 4-2-2 ] 1*     (new start)
 *   9 1001  [ 4-2-2-1 > 4-4-1 ] 2*         1
 *  10 1010  [ 4-4-1-1 > 4-4-2 ] 1*         2
 *  11 1011  [ 4-4-2-1 > 8-2-1 ] 4          3
 *  12 1100  [ 8-2-1-1 > 8-2-2 ] 1*         4
 *  13 1101  [ 8-2-2-1 > 8-4-1 ] 2*         5
 *  14 1110  [ 8-4-1-1 > 8-4-2 ] 1*         6
 *  15 1111  [ 8-4-2-1 ]                    7 (111 = 1000 ^ 1111)
 *
 *  * marks the previous merge-num mode, it shows up repeatedly, that's why the
 *  not-fully-eager merge can perfectly match the bits, the binary number,
 *  beautiful
 */
void q_sort(struct list_head *head)
{
    struct list_head *list, *pending;
    unsigned long count = 0;

    if (!head)
        return;

    list = head->next;
    /* if head is empty or singular */
    if (list == head->prev)
        return;

    pending = NULL;
    head->prev->next = NULL; /* break the cycle of doubly linked list */

    /* bottom-up merge sort */
    do {
        int bits;
        struct list_head **tail = &pending; /* pending's tail */

        /* move tail to the next two possible merging pending lists */
        for (bits = count; bits & 1; bits >>= 1)
            tail = &(*tail)->prev;

        /* merge if bits becomes non-zero even */
        if (bits) {
            struct list_head *newer = *tail, *older = newer->prev;

            newer = merge(older, newer);
            newer->prev = older->prev;
            *tail = newer;
        }

        /* add list into pending and update list, count */
        list->prev = pending;
        pending = list;
        list = list->next;
        pending->next = NULL;
        ++count;
    } while (list);

    /* all lists are in pending, merge them */
    list = pending;
    pending = list->prev;
    for (;;) {
        struct list_head *next = pending->prev;

        if (!next)
            break;

        list = merge(pending, list);
        pending = next;
    }

    /* final merge and restore the doubly linked list */
    merge_restore(head, pending, list);
}
