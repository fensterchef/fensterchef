#ifndef PARSE__ACTION_H
#define PARSE__ACTION_H

#include "core/action.h"
#include "parse/parse.h"
#include "utility/list.h"

/* a list of actions but with explicit count and capacity */
struct parse_action_list {
    /* the current action parsing information */
    struct parse_action_information {
        /* Data for this action. */
        LIST(struct parse_generic_data, data);
        /* The offset within the string identifiers of the actions.
         * If this is -1, then the action was disregarded.
         */
        int offset;
    } actions[ACTION_SIMPLE_MAX];
    /* the first/last (exclusive) action still valid within @offsets */
    action_type_t first_action, last_action;
    /* the action items being parsed */
    LIST(struct action_list_item, items);
    /* data for the action items being parsed */
    LIST(struct parse_generic_data, data);
};

/* Parse an action.
 *
 * Expects that a string has been read into @parser.
 *
 * @list is the action list to fill with the actions.
 *
 * @return ERROR if there is no action, OK otherwise.
 */
int continue_parsing_action(Parser *parser, struct parse_action_list *list);

/* Make a real action list from a parser action list.
 *
 * Note that the the data is copied in a shallow way.  Clearing the memory of
 * either list will free the other.
 */
void make_real_action_list(struct action_list *real_list,
        struct parse_action_list *list);

#endif
