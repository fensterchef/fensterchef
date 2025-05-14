#ifndef UTILITY__LINKED_LIST_H
#define UTILITY__LINKED_LIST_H

/**
 * Maintaining a singly or doubly linked list.  This is meant to reduce much
 * code repetition, especially when multiple linked lists traverse through the
 * same object.
 */

#include <stddef.h> /* size_t, NULL, offsetof() */

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
 * void @return expand to the head of the singly linked list.
 */
#define SINGLY_LIST(type, head) \
    type *head

/* Unlink an item from a singly linked list.
 *
 * T    @type of the linked list.
 * T*   @head is the head of the linked list.
 * T*   @item is the item to remove.
 * T*   @next is the member of T that has the next element.
 *
 * Assumes that @item IS part of the list.
 *
 * void @return
 */
#define SINGLY_UNLINK(head, item, next) do { \
    void *const _item = (item); \
    const size_t _next_offset = (char*) &head->next - (char*) head; \
    if (head == _item) { \
        head = *(void**) ((char*) head + _next_offset); \
    } else { \
        void *_previous, *_next; \
\
        _previous = head; \
        while (_next = *(void**) ((char*) _previous + _next_offset), \
                _next != (void*) (item)) { \
            _previous = _next; \
        } \
        *(void**) ((char*) _previous + _next_offset) = \
            *(void**) ((char*) (item) + _next_offset); \
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
 * void @return expand to the head and tail of the doubly linked list.
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
 * ASSUMES that @item IS part of the list.
 *
 * void @return
 */
#define DOUBLY_UNLINK(head, tail, item, previous, next) do { \
    void *const _item = (item); \
    const size_t _previous_offset = (char*) &head->previous - (char*) head; \
    const size_t _next_offset = (char*) &head->next - (char*) head; \
    void *const _previous = *(void**) ((char*) _item + _previous_offset); \
    void *const _next = *(void**) ((char*) _item + _next_offset); \
    if (_previous != NULL) { \
        *(void**) ((char*) _previous + _next_offset) = _next; \
    } else { \
        head = _next; \
    } \
\
    if (_next != NULL) { \
        *(void**) ((char*) _next + _previous_offset) = _previous; \
    } else { \
        tail = _previous; \
    } \
\
    *(void**) ((char*) _item + _previous_offset) = NULL; \
    *(void**) ((char*) _item + _next_offset) = NULL; \
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
 * ASSUMES that @item is NOT part of the list.
 * ASSUMES that @before IS part of the list.
 *
 * void @return
 */
#define DOUBLY_LINK_AFTER(head, tail, item, before, previous, next) do { \
    void *const _item = (item); \
    void *const _before = (before); \
    const size_t _previous_offset = (char*) &head->previous - (char*) head; \
    const size_t _next_offset = (char*) &head->next - (char*) head; \
    if (_before == NULL) { \
        if (head != NULL) { \
            *(void**) ((char*) head + _previous_offset) = _item; \
            *(void**) ((char*) _item + _next_offset) = head; \
        } else { \
            tail = _item; \
        } \
        head = _item; \
    } else { \
        void *const _before_next = *(void**) ((char*) _before + _next_offset); \
        *(void**) ((char*) _item + _previous_offset) = _before; \
        *(void**) ((char*) _item + _next_offset) = _before_next; \
        if (_before_next != NULL) { \
            *(void**) ((char*) _before_next + _previous_offset) = item; \
        } else { \
            tail = _item; \
        } \
        *(void**) ((char*) _before + _next_offset) = _item; \
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
 * ASSUMES that @item AND @before IS part of the list.
 *
 * void @return
 */
#define DOUBLY_RELINK_AFTER(head, tail, item, before, previous, next) do { \
    void *const _relink_item = (item); \
    void *const _relink_before = (before); \
    DOUBLY_UNLINK(head, tail, _relink_item, previous, next); \
    DOUBLY_LINK_AFTER(head, tail, _relink_item, _relink_before, \
            previous, next); \
} while (0)

#endif
