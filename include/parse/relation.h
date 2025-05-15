#ifndef PARSE__RELATION_H
#define PARSE__RELATION_H

/**
 * Relations have the syntax:
 * QUOTED_STRING TOP
 *
 * For example:
 *     "Firefox" set floating, focus window
 */

#include "parse/action.h"
#include "parse/parse.h"

/* Parse the next assigment in the active stream.
 *
 * This assumes a string has been read into the @parser.
 */
void continue_parsing_relation(Parser *parser,
        struct parse_action_list *list);

#endif
