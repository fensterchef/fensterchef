#ifndef ACTION_H
#define ACTION_H

/**
 * Actions expose internal functionality to the user.
 *
 * The user can invoke any actions in any order at any time.
 */

#include <stddef.h>

#include "binding.h"
#include "bits/actions.h"
#include "bits/action_block.h"
#include "utility/types.h"
#include "relation.h"

/* integer type the parser should use */
typedef int_fast32_t action_integer_t;
#define PRIiACTION_INTEGER PRIiFAST32

/* If the integer is a percentage of something.  For example this might be 20%
 * of the width of a monitor.
 */
#define ACTION_DATA_FLAGS_IS_PERCENT (1 << 0)

/* expand to all action data types */
#define DEFINE_ALL_ACTION_DATA_TYPES \
    /* an integer or integer constant */ \
    X(INTEGER, action_integer_t integer, 'I') \
    /* a string of text, quoted or unquoted */ \
    X(STRING, utf8_t *string, 'S') \
    /* window relation */ \
    X(RELATION, struct window_relation relation, 'R') \
    /* button binding */ \
    X(BUTTON, struct button_binding button, 'B') \
    /* key binding */ \
    X(KEY, struct key_binding key, 'K')

/* action codes */
typedef enum {
#define X(identifier, string) \
    ACTION_##identifier,
    DEFINE_ALL_ACTIONS
#undef X
    /* not a real action */
    ACTION_MAX,
} action_type_t;

/* all data types */
typedef enum action_data_type {
#define X(identifier, type_name, short_name) \
    ACTION_DATA_TYPE_##identifier,
    DEFINE_ALL_ACTION_DATA_TYPES
#undef X
    ACTION_DATA_TYPE_MAX
} action_data_type_t;

/* data for a specific type */
struct action_data {
    /* an OR combination of `ACTION_DATA_FLAGS_*` */
    unsigned flags;
    action_data_type_t type;
    /* the literal value */
    union {
#define X(identifier, type_name, short_name) \
        type_name;
        DEFINE_ALL_ACTION_DATA_TYPES
#undef X
    } u;
};

/* A block of actions. */
struct action_block {
    /* the number of references to this block */
    unsigned reference_count;
    /* all items within the block */
    struct action_block_item {
        /* the type of this actions */
        action_type_t type;
        /* the number of data points in `data` */
        unsigned data_count;
    } *items;
    /* the number of items in `items` */
    size_t number_of_items;
    /* the data associated to the actions */
    struct action_data data[];
};

/* Search the parse data meta table for given identifier.
 *
 * @return the data type with given identifier or ACTION_DATA_TYPE_MAX if none
 *         has that identifier.
 */
action_data_type_t get_action_data_type_from_identifier(char identifier);

/* Get a string representation of a data type. */
const char *get_action_data_type_name(action_data_type_t type);

/* Clear a single action data point. */
void clear_action_data(struct action_data *data);

/* Duplicate a single action data point into itself. */
void duplicate_action_data(struct action_data *data);

/* Get the action string of given action type. */
const char *get_action_string(action_type_t type);

/* Increment the reference counter of an action block. */
void reference_action_block(_Nullable ActionBlock *block);

/* Decrement the reference counter of an action block.
 *
 * If the reference count reaches 0, it is freed.
 */
void dereference_action_block(_Nullable ActionBlock *block);

/* Create a block of actions with specified number of items and data
 * preallocated.
 */
ActionBlock *create_empty_action_block(size_t number_of_items,
        size_t number_of_data_points);

/* Do all actions within @block. */
void run_action_block(ActionBlock *block);

/* Do the given action using given @data. */
void do_action(action_type_t type, const struct action_data *data);

#endif
