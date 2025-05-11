#ifndef PARSE__BINDING_H
#define PARSE__BINDING_H

#include "core/binding.h"

/* Parse the next binding definition in @parser.
 *
 * Expects that a string has been read into @parser.
 */
void continue_parsing_modifiers_or_binding(Parser *parser);

#endif
