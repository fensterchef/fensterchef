#ifndef PARSE__BINDING_H
#define PARSE__BINDING_H

/**
 * Parse bindings or part of a binding.
 *
 * A binding has the syntax:
 * release? transparent? MODIFIERS+BUTTON|KEY_SYMBOL ACTIONS
 * --------------------------------
 *    continue_parsing_modifiers   -----------------
 *                                          |        -------
 *                                          |           |
 *                          continue_pasing_button_or_key_symbol                  -------
 *                                                      |
 *                                              finish_parsing_binding
 *
 * But this is also used for clearing a binding:
 * unbind release? transparent? MODIFIERS+BUTTON|KEY_SYMBOL
 *
 * To completely parse a binding, do this (omitting error checking):
 * ```
 * Parser *parser = ...;
 * struct parse_binding binding;
 * 
 * read_string(parser);                                      
 * continue_parsing_modifiers(parser, &binding);            |
 * continue_parsing_button_or_key_symbol(parser, &binding); +------+
 * finish_parsing_binding(parser, &binding);                |      |
 * ```                                                             v
 *                                          same as: continue_parsing_binding
 */

#include "core/binding.h"

/* parse information about a binding */
struct parse_binding {
    /* if there were were any modifiers */
    bool has_anything;
    /* the position of the transparent keyword if any */
    size_t transparent_position;
    /* if `release` was specified */
    bool is_release;
    /* if `transparent` was specified */
    bool is_transparent;
    /* the specified X key/button modifiers */
    unsigned modifiers;
    /* the X button index */
    button_t button_index;
    /* the key symbol */
    KeySym key_symbol;
    /* the key code */
    KeyCode key_code;
};

/* Parse binding modifiers.
 *
 * This includes: release, transparent and the X modifiers.
 *
 * Expects that a string has been read into @parser.
 *
 * @return ERROR if there are any modifiers but they are invalid.
 *
 * If an error occurs, it is printed and all statements are skipped.
 */
int continue_parsing_modifiers(Parser *parser, struct parse_binding *binding);

/* Parse the following button or key symbol. */
int continue_parsing_button_or_key_symbol(Parser *parser,
        struct parse_binding *binding);

/* Complete parsing a binding definition and load the completed binding into the
 * parser.
 */
int finish_parsing_binding(Parser *parser, struct parse_binding *binding);

/* Parse a full binding definition.
 *
 * Expects that a string has been read into @parser.
 */
void continue_parsing_binding(Parser *parser);

/* Parse a full unbind statement.
 *
 * Expects that an `unbind` has already been read.
 */
void continue_parsing_unbind(Parser *parser);

#endif
