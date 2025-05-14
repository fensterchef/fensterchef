#ifndef UTILITY__LINKED_LIST_H
#define UTILITY__LINKED_LIST_H

/**
 * Maintaining a singly or doubly linked list.
 */

/**********************
 * Singly linked list *
 **********************/

/* Define a singly linked list.
 *
 * The head has to be defined separately.
 *
 * T    @type is the type of the list.
 * T*   @head is the name of the list.
 *
 * void @return expand to the base of the singly linked list.
 */
#define SINGLY_LIST(type, head) \
    type *head

/* Unlink an item from a singly linked list.
 *
 * T    @type of the linked list.
 * T*   @base is the head of the linked list.
 * T*   @item is the item to remove.
 * T*   @next is the member of T that has the next element.
 *
 * void @return
 */
#define SINGLY_UNLINK(type, base, item, next) do { \
    if (base == item) { \
        base = base->next; \
    } else { \
        type *_previous = base; \
        while (_previous->next != item) { \
            _previous = _previous->next; \
        } \
        _previous->next = item->next; \
    } \
} while (0)

/**********************
 * Doubly linked list *
 **********************/

/* Define a doubly linked list.
 *
 * The head has to be defined separately.
 *
 * T    @type is the type of the list.
 * T*   @head is the name of the head element.
 * T*   @tail is the name of the tail element.
 *
 * void @return expand to the base of the doubly linked list.
 */
#define DOUBLY_LIST(type, head, tail) \
    type *head, *tail

/* Unlink an item from the doubly linked list.
 *
 * T*   @head is the head of the linked list.
 * T*   @tail is the tail of the linked list.
 * T*   @item is the item to unlink.
 * T*   @previous is the name of the member leading to the previous element.
 * T*   @next is the name of the member leading to the next element.
 *
 * void @return
 */
#define DOUBLY_UNLINK(head, tail, item, previous, next) do { \
    if (item->previous != NULL) { \
        item->previous->next = item->next; \
    } else { \
        head = window->next; \
    } \
\
    if (item->next != NULL) { \
        item->next->previous = item->below; \
    } else { \
        tail = window->previous; \
    } \
\
    item->next = NULL; \
    item->previous = NULL; \
} while (0)


/* Link an item into after an item in a doubly linked list.
 *
 * T*   @head is the head of the linked list.
 * T*   @tail is the tail of the linked list.
 * T*   @item is the item to unlink.
 * T*   @before is the element that is supposed to be before @item.
 * T*   @previous is the name of the member leading to the previous element.
 * T*   @next is the name of the member leading to the next element.
 *
 * void @return
 */
#define DOUBLY_LINK_AFTER(head, tail, item, before, previous, next) do { \
    if (before == NULL) { \
        if (head != NULL) { \
            head->previous = item; \
            item->next = head; \
        } else { \
            tail = item; \
        } \
        head = item; \
    } else { \
        item->previous = before; \
        item->next = before->next; \
        if (before->next != NULL) { \
            before->next->previous = item; \
        } else { \
            tail = item; \
        } \
        before->next = item; \
    } \
} while (0)


/* Unlink an item from a doubly linked list and relink it after another.
 *
 * T*   @head is the head of the linked list.
 * T*   @tail is the tail of the linked list.
 * T*   @item is the item to unlink.
 * T*   @before is the element that is supposed to be before @item.
 * T*   @previous is the name of the member leading to the previous element.
 * T*   @next is the name of the member leading to the next element.
 *
 * void @return
 */
#define DOUBLY_RELINK_AFTER(head, tail, item, before, previous, next) do { \
    DOUBLY_UNLINK(head, tail, item, previous, next); \
    DOUBLY_LINK_AFTER(head, tail, item, before, previous, next); \
} while (0)

#endif
