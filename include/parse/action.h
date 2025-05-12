#ifndef PARSE__ACTION_H
#define PARSE__ACTION_H

/**
 * Parse an action.
 *
 * WORD [WORD|VALUE]...
 *
 * For example:
 * move window by -10 0
 */

#include "parse/parse.h"
#include "utility/list.h"

/* Parse an action list in brackets.
 *
 * Expects that an open bracket has been read from the parser stream.
 *
 * @return ERROR if there is no action, OK otherwise.
 */
int continue_parsing_actions_in_brackets(Parser *parser,
        struct parse_action_list *list);

/* Parse an action list.
 *
 * Expects that a string has been read into @parser.
 *
 * @list is the action list to fill with the actions.
 *
 * @return ERROR if there is no action, OK otherwise.
 */
int continue_parsing_actions(Parser *parser, struct parse_action_list *list);

/* Clear all memory within @list. */
void clear_parse_list(struct parse_action_list *list);

/* Clear the data within @list that is needed for parsing and the
 * associations.
 *
 * Note that the associations are not needed for `make_real_action_list()`,
 * making it safe to call both.
 */
void clear_parse_list_data(struct parse_action_list *list);

/* Make a real action list from a parser action list.
 *
 * Note that the the data is copied in a shallow way.  Clearing the memory of
 * either list will free the other.
 */
void make_real_action_list(struct action_list *real_list,
        struct parse_action_list *list);

#endif
