#ifndef PARSE__TOP_H
#define PARSE__TOP_H

/**
 * Parse a top level statement.
 *
 * ACTION
 * ALIAS
 * BINDING
 * GROUP
 * RELATION
 * SOURCE
 * UNBIND
 * ( TOP... )
 *
 * These may be separated by a comma, for example:
 * ACTION, ACTION, SOURCE
 *
 * Or an less abstract example:
 * split vertically, focus right, source map
 */

#include "parse/parse.h"

/* Parse a top level statement.
 *
 * @block is appended with the parsed actions.  Make sure it has valid members.
 */
int parse_top(Parser *parser, struct parse_action_block *block);

#endif
