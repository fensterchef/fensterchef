#ifndef ACTION_H
#define ACTION_H

/**
 * Actions expose internal functionality to the user.
 *
 * The user can invoke any actions in any order at any time.
 */

#include <stddef.h>

#include "bits/actions.h"

/* action codes */
typedef enum {
#define X(identifier, string) \
    ACTION_##identifier,
    DEFINE_ALL_PARSE_ACTIONS
#undef X
    /* not a real action */
    ACTION_MAX,
} action_type_t;

/* forward declaration... */
struct parse_data;

/* A list of actions. */
struct action_list {
    /* all items within the list */
    struct action_list_item {
        /* the type of this actions */
        action_type_t type;
        /* the number of data points in `data` */
        unsigned data_count;
    } *items;
    /* the number of items in `items` */
    size_t number_of_items;
    /* the data associated to the actions */
    struct parse_data *data;
};

/* Get the action string of given action type. */
const char *get_action_string(action_type_t type);

/* Do all actions within @list. */
void run_action_list(const struct action_list *list);

/* Make a deep copy of @list and put it into itself. */
void duplicate_action_list(struct action_list *list);

/* Free ALL memory associated to the action list. */
void clear_action_list(const struct action_list *list);

/* Do the given action using given @data. */
void do_action(action_type_t type, const struct parse_data *data);

#endif
