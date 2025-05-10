#ifndef PARSE__INPUT_H
#define PARSE__INPUT_H

/**
 * This abstracts over getting input.  The input may come from a string
 * or a file.  It gives consistent line endings (\n) and joins lines that have
 * special constructs with a new line followed by any amount of blacks and then
 * a backslash \.  It also skips comments which are lines starting with '#'.
 */

#include "utility/utility.h"
#include "utility/types.h"

#include "parse/parse.h"

/* Get the next character from given parser.
 *
 * @return EOF if the end has been reached.
 */
int get_stream_character(Parser *parser);

/* Get the next character from given parser without advancing to the following
 * character.
 *
 * @return EOF if the end has been reached.
 */
int peek_stream_character(Parser *parser);

/**
 * NOTE: The below functions are not efficient and should only be used for error
 * output.
 */

/* Get the column and line of @index within @parser.
 *
 * If @index is out of bounds, @line and @column are set to the last position in
 * the parser.
 */
void get_stream_position(const Parser *parser, size_t index,
        unsigned *line, unsigned *column);

/* Get the line within @parser.
 *
 * @length will hold the length of the line.
 */
const char *get_stream_line(const Parser *parser, unsigned line,
        unsigned *length);

#endif
