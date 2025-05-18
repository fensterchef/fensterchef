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
#include "utility/attributes.h"

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

/* Modifies @parser->string and then splits it in two and stores two allocated
 * strings in @class.
 */
void resolve_class_string(Parser *parser, _Out struct parse_class *class);

#endif
