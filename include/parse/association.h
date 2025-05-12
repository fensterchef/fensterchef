#ifndef PARSE__ASSOCIATION_H
#define PARSE__ASSOCIATION_H

/**
 * Associations have the syntax:
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
void continue_parsing_association(Parser *parser,
        struct parse_action_list *list);

#endif
