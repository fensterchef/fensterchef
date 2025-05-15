#ifndef PARSE__UTILITY_H
#define PARSE__UTILITY_H

/**
 * Parse very simple constructs like repeated space and resolve integers and
 * strings.
 *
 * This file is meant to be private to the parser.
 */

#include "parse/data_type.h"
#include "parse/parse.h"
#include "utility/utility.h"

/* Skip to the beginning of the next line. */
void skip_line(Parser *parser);

/* Skip over any blank (`isblank()`). */
void skip_blanks(Parser *parser);

/* Skip over any white space (`isspace()`). */
void skip_space(Parser *parser);

/* Skip to the next statement.
 *
 * This is used when an error occurs but more potential errors can be shown.
 */
void skip_statement(Parser *parser);

/* Skip all following statements. */
void skip_all_statements(Parser *parser);

/* Read a string/word from the parser input.
 *
 * @return ERROR if there was no string, OK otherwise.
 *
 * Any alias that matches this string is resolved.
 */
int read_string(Parser *parser);

/* Read a string/word but do not resolve as an alias. */
int read_string_no_alias(Parser *parser);

/* Translate the string within @parser to a button index.
 *
 * @return `BUTTON_NONE` if the button string is invalid.
 */
int resolve_button(Parser *parser);

/* Try to resolve the string within @parser as integer.
 *
 * @flags are special integer flags defined in `parse/data_type.h`.
 *
 * @return ERROR if @parser->string is not an integer.
 */
int resolve_integer(Parser *parser,
        _Out unsigned *flags,
        _Out parse_integer_t *integer);

/* Modifies @parser->string and then splits it in two and stores two allocated
 * strings in @class.
 */
void resolve_class_string(Parser *parser, _Out struct parse_class *class);

/* Resolve the string within parser as a data point.
 *
 * @return false if @identifier is invalid.
 *
 * data->type is set to PARSE_DATA_TYPE_MAX if the string within parser does not
 * match the data type.
 */
bool resolve_data(Parser *parser, char identifier,
        _Out struct parse_data *data);

/* Try to resolve the string within @parser as key symbol.
 *
 * @return NoSymbol if the string is not a key symbol.
 */
KeySym resolve_key_symbol(Parser *parser);

#endif
