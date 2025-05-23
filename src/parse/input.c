#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "parse/parse.h"
#include "utility/utility.h"

/* Get a character from the stream without advancing the cursor. */
static inline int peek_character(Parser *parser)
{
    if (parser->index == parser->length) {
        return EOF;
    }
    return (unsigned char) parser->input[parser->index];
}

/* Get the next character from stream. */
static INLINE int get_or_peek_stream_character(Parser *parser,
        bool should_advance)
{
    int character, other;

    /* join lines when the next one starts with a backslash */
    while (true) {
        character = peek_character(parser);
        if (character == EOF) {
            return EOF;
        }

        /* skip comment when there was a line end before */
        if (character == '#' &&
                (parser->index == 0 ||
                    islineend(parser->input[parser->index - 1]))) {
            /* skip over the end of the line */
            while (character = peek_character(parser),
                    !islineend(character) && character != EOF) {
                parser->index++;
            }

            if (character == EOF) {
                return EOF;
            }
            parser->index++;
        } else {
            if (should_advance) {
                parser->index++;
                if (!islineend(character)) {
                    return character;
                }
            } else {
                if (!islineend(character)) {
                    return character;
                }
                parser->index++;
            }
        }

        other = peek_character(parser);
        /* put \r\n and \n\r together */
        if ((other == '\n' && character == '\r') ||
                (other == '\r' && character == '\n')) {
            parser->index++;
        }

        character = peek_character(parser);
        /* let the above part take care of this */
        if (character == '#') {
            continue;
        }

        const size_t save_index = parser->index - 1;

        /* skip over any leading blanks */
        while (isblank(character)) {
            parser->index++;
            character = peek_character(parser);
        }

        if (character != '\\') {
            if (!should_advance) {
                parser->index = save_index;
            }
            return '\n';
        }

        /* skip over \ */
        parser->index++;
    }
}

/* Get the next character from stream. */
int get_stream_character(Parser *parser)
{
    return get_or_peek_stream_character(parser, true);
}

/* Get the next character from given stream without advancing to the following
 * character.
 */
int peek_stream_character(Parser *parser)
{
    return get_or_peek_stream_character(parser, false);
}

/* Get the column and line of @index within the active stream. */
void get_stream_position(const Parser *parser, size_t index,
        unsigned *line, unsigned *column)
{
    unsigned current_line = 0, current_column = 0;
    int character;
    wchar_t wide_character;

    index = MIN(index, parser->length);
    for (size_t i = 0; i < index; i++) {
        character = (unsigned char) parser->input[i];
        if (isprint(character)) {
            current_column++;
        } else if (islineend(character)) {
            current_column = 0;
            current_line++;
            if (i + 1 < index) {
                const int other = (unsigned char) parser->input[i + 1];
                /* put \r\n and \n\r together */
                if ((other == '\n' && character == '\r') ||
                        (other == '\r' && character == '\n')) {
                    i++;
                }
            }
        } else if (character == '\t') {
            current_column = current_column - current_column % PARSE_TAB_SIZE;
        } else if (character < ' ') {
            /* these characters are printed as ? */
            current_column++;
        } else {
            const int result = mbtowc(&wide_character,
                    &parser->input[i], parser->length - i);
            if (result == -1) {
                /* these characters are printed as ? */
                current_column++;
            } else {
                index += result - 1;
                current_column += wcwidth(wide_character);
            }
        }
    }

    *line = current_line;
    *column = current_column;
}

/* Get the beginning of the line at given line index. */
const char *get_stream_line(const Parser *parser, unsigned line,
        unsigned *length)
{
    size_t line_index = 0, end_line_index;
    size_t index = 0;
    int character;

    while (index < parser->length) {
        end_line_index = index;

        character = (unsigned char) parser->input[index];
        index++;
        if (islineend(character)) {
            if (index + 1 < parser->length) {
                const int other = (unsigned char) parser->input[index + 1];
                /* put \r\n and \n\r together */
                if ((other == '\n' && character == '\r') ||
                        (other == '\r' && character == '\n')) {
                    index++;
                }
            }
            if (line == 0) {
                index = end_line_index;
                break;
            }
            line_index = index;
            line--;
        }
    }

    *length = index - line_index;
    return &parser->input[line_index];
}
