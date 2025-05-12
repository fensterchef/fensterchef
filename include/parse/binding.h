#ifndef PARSE__BINDING_H
#define PARSE__BINDING_H

/**
 * Parse bindings or part of a binding.
 *
 * release? transparent? MODIFIERS+BUTTON|KEY_SYMBOL TOP
 * unbind release? transparent? MODIFIERS+BUTTON|KEY_SYMBOL
 *
 * Where MODIFIERS:
 * INTEGER [+ INTEGER]...
 *
 * For example:
 *     Super+Shift+l move right
 *
 * Example with more actions:
 *     Super+Shift+v (
 *         split vertically
 *         focus right
 *         run $TERMINAL
 *     )
 *
 * They can be separated by a comma as well.
 *
 * Binding within a binding:
 *     Super+Shift+r (
 *         h move window by -10 0
 *         l move window by 10 0
 *         k move window by 0 10
 *         j move window by 0 -10
 *
 *         q unbind h, unbind l, unbind k, unbind j
 *     )
 *
 * Button binding:
 *     transparent LeftButton focus window
 */

#include "core/binding.h"
#include "parse/parse.h"

/* Parse a full binding definition.
 *
 * Expects that a string has been read into @parser.
 *
 * @list will be appended with the bind action.
 */
void continue_parsing_binding(Parser *parser, struct parse_action_list *list);

/* Parse a full unbind statement.
 *
 * Expects that an `unbind` has already been read.
 *
 * @list will be appended with the unbind action.
 */
void continue_parsing_unbind(Parser *parser, struct parse_action_list *list);

#endif
