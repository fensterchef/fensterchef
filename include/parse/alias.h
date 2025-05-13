#ifndef PARSE__ALIAS_H
#define PARSE__ALIAS_H

/**
 * Parse an alias.
 *
 * alias STRING = STRING
 *
 * unalias STRING
 *
 * For example:
 * alias mod = Super
 * unalias mod
 */

#include "parse/parse.h"

#define PARSE_MAX_ALIASES 2048

/* Parse all after the `alias` keyword. */
void continue_parsing_alias(Parser *parser);

/* Parse all after the `unalias` keyword. */
void continue_parsing_unalias(Parser *parser);

/* Check if given string is within the alias table.
 *
 * @return NULL if the alias does not exist, otherwise a null-terminated string
 *              with the value of the alias.
 */
const char *resolve_alias(const char *string);

#endif
