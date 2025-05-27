#ifndef PARSE__ACTION_H
#define PARSE__ACTION_H

/**
 * Parse an action.
 *
 * WORD [WORD|VALUE]...
 *
 * For example:
 *     move window by -10 0
 */

#include "core/action.h"
#include "parse/parse.h"
#include "utility/list.h"

/* A list of actions with explicit count and capacity.  It also includes
 * additional parsing information.
 */
struct parse_action_block {
    /* the number of open round brackets '(' */
    unsigned bracket_count;
    /* the current action parsing information */
    struct parse_action_information {
        /* Data for this action. */
        LIST(struct action_data, data);
        /* The offset within the string identifiers of the actions.
         * If this is -1, then the action was disregarded.
         */
        int offset;
    } actions[ACTION_SIMPLE_MAX];
    /* The first and last (exclusive) action still valid.  There might be
     * invalid actions inbetween, check the offset against -1 for that.
     */
    action_type_t first_action, last_action;
    /* the action items being parsed */
    LIST(struct action_block_item, items);
    /* data for the action items being parsed */
    LIST(struct action_data, data);
};

/* Parse an action list in brackets.
 *
 * Expects that an open bracket has been read from the parser stream.
 *
 * @return ERROR if there is no action, OK otherwise.
 */
int continue_parsing_actions_in_brackets(Parser *parser,
        struct parse_action_block *block);

/* Parse an action list.
 *
 * Expects that a string has been read into @parser.
 *
 * @list is the action list to fill with the actions.
 *
 * @return ERROR if there is no action, OK otherwise.
 */
int continue_parsing_actions(Parser *parser, struct parse_action_block *block);

/* Clear all memory within @block. */
void clear_parse_action_block(struct parse_action_block *block);

/* Make a real action list from a parser action list. */
ActionBlock *convert_parse_action_block(struct parse_action_block *block);

#endif
