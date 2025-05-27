#ifndef PARSE__RELATION_H
#define PARSE__RELATION_H

/**
 * Parse a relation.
 *
 * relate STRING TOP
 * unrelate [STRING]
 *
 * For example:
 *     relate Firefox set floating, focus window, unrelate
 */

#include "parse/action.h"
#include "parse/parse.h"

/* Parse the next assigment in the active stream.
 *
 * This assumes a string has been read into the @parser.
 */
void continue_parsing_relation(Parser *parser,
        struct parse_action_block *block);

/* Parse all after an `unrelate` keyword. */
void continue_parsing_unrelate(Parser *parser,
        struct parse_action_block *block);

#endif
