#ifndef PARSE__BINDING_H
#define PARSE__BINDING_H

/**
 * Parse bindings or part of a binding.
 *
 * release? transparent? WHAT_BINDING TOP
 * unbind release? WHAT_BINDING
 * unbind GROUP_NAME
 *
 * Where WHAT_BINDING:
 * MODIFIERS+BUTTON|KEY_SYMBOL
 * MODIFIERS+[KEY_CODE]
 *
 * Where MODIFIERS:
 * [INTEGER [+ INTEGER]...]
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
 *         j move window by 0 10
 *         k move window by 0 -10
 *         l move window by 10 0
 *
 *         q unbind h, unbind l, unbind k, unbind j
 *     )
 *
 * Using a group:
 *     group move (
 *         h move window by -1% 0
 *         j move window by 0 1%
 *         k move window by 0 -1%
 *         l move window by 1% 0
 *
 *         q unbind move
 *     )
 *
 *     Super+Shift+r call move
 *
 * Button binding:
 *     transparent LeftButton focus window
 */

#include "core/binding.h"
#include "parse/action.h"
#include "parse/parse.h"

/* Try to resolve the string within parser to a modifier constant.
 *
 * @return ERROR when the string is not a modifier constant.
 */
int resolve_modifier(Parser *parser, _Out unsigned *modifier);

/* Parse a full binding definition.
 *
 * Expects that a string has been read into @parser.
 *
 * @block will be appended with the bind action.
 */
void continue_parsing_binding(Parser *parser, struct parse_action_block *block);

/* Parse a full key code binding definition.
 *
 * Expects that a '[' has been just consumed.
 *
 * @block will be appended with the bind action.
 */
void continue_parsing_key_code_binding(Parser *parser,
        struct parse_action_block *block);

/* Parse a full unbind statement.
 *
 * Expects that an `unbind` has already been read.
 *
 * @block will be appended with the unbind action.
 */
void continue_parsing_unbind(Parser *parser, struct parse_action_block *block);

#endif
