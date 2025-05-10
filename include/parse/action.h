#ifndef PARSE__ACTION_H
#define PARSE__ACTION_H

#include "../action.h"
#include "parse/parse.h"

/* Parse an action.
 *
 * Expects that a string has been read into @parser.
 *
 * @return ERROR if there is no action, OK otherwise.
 */
int continue_parsing_action(Parser *parser);

#endif
