#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <X11/keysym.h>
#include <X11/Xlib.h>

#include "core/binding.h"
#include "core/log.h"
#include "parse/alias.h"
#include "parse/parse.h"
#include "parse/input.h"
#include "parse/utility.h"

/* Skip to the beginning of the next line. */
void skip_line(Parser *parser)
{
    int character;

    while (character = get_stream_character(parser),
            character != '\n' && character != EOF) {
        /* nothing */
    }
}

/* Skip over any blank (`isblank()`). */
void skip_blanks(Parser *parser)
{
    while (isblank(peek_stream_character(parser))) {
        (void) get_stream_character(parser);
    }
}

/* Skip over any white space (`isspace()`). */
void skip_space(Parser *parser)
{
    while (isspace(peek_stream_character(parser))) {
        (void) get_stream_character(parser);
    }
}

/* Skip to the next statement.
 *
 * This is used when an error occurs but more potential errors can be shown.
 */
void skip_statement(Parser *parser)
{
    int character;
    int brackets = 0;

    while (character = peek_stream_character(parser),
            (brackets > 0 && character != EOF) ||
            (character != ',' && character != '\n' && character != EOF)) {
        /* skip over strings */
        if (character == '\"' || character == '\'') {
            read_string_no_alias(parser);
        } else if (character == '(') {
            (void) get_stream_character(parser);
            brackets++;
        } else if (character == ')') {
            (void) get_stream_character(parser);
            brackets--;
        } else {
            (void) get_stream_character(parser);
        }
    }

    if (character == ',' || character == '\n') {
        (void) get_stream_character(parser);
    }
}

/* Skip all following statements. */
void skip_all_statements(Parser *parser)
{
    do {
        skip_statement(parser);
        if (peek_stream_character(parser) == ',') {
            (void) get_stream_character(parser);
            continue;
        }
    } while (false);
}

/* Check if @character can be part of a word. */
static bool is_word_character(int character)
{
    if (iscntrl(character)) {
        return false;
    }

    if (character < 0) {
        return false;
    }

    if (character == ' ' || character == '\"' ||
            character == '\'' || character == ',' || character == ';' ||
            character == '(' || character == ')' ||
            character == '{' || character == '}' ||
            character == '[' || character == ']' ||
            character == '&' || character == '|' ||
            character == '+' || character == '*' || character == '=') {
        return false;
    }

    return true;
}

/* Read a string/word but do not resolve as an alias. */
int read_string_no_alias(Parser *parser)
{
    int character;

    skip_space(parser);

    parser->start_index = parser->index;
    parser->string_length = 0;
    character = peek_stream_character(parser);
    if (character == '\"' || character == '\'') {
        parser->is_string_quoted = true;

        const int quote = character;

        /* skip over the opening quote */
        (void) get_stream_character(parser);

        while (character = get_stream_character(parser),
                character != quote && character != EOF && character != '\n') {
            /* escape any characters following \ */
            if (character == '\\') {
                character = get_stream_character(parser);
                /* keep the \ for all but the quote used and another \ */
                if (character != quote && character != '\\') {
                    LIST_APPEND_VALUE(parser->string, '\\');
                }
                if (character == EOF || character == '\n') {
                    break;
                }
            }
            LIST_APPEND_VALUE(parser->string, character);
        }

        if (character != quote) {
            emit_parse_error(parser, "missing closing quote character");
        }
    } else {
        parser->is_string_quoted = false;

        while (character = peek_stream_character(parser),
                is_word_character(character)) {
            (void) get_stream_character(parser);

            LIST_APPEND_VALUE(parser->string, character);
        }

        if (parser->string_length == 0) {
            return ERROR;
        }
    }

    LIST_APPEND_VALUE(parser->string, '\0');
    parser->string_length--;

    return OK;
}

/* Read a string/word from the active input stream. */
int read_string(Parser *parser)
{
    if (read_string_no_alias(parser) != OK) {
        return ERROR;
    }

    if (!parser->is_string_quoted) {
        const char *const alias = resolve_alias(parser->string);
        if (alias != NULL) {
            LOG_DEBUG("resolved %s to %s\n",
                    parser->string, alias);

            LIST_SET(parser->string, 0, alias, strlen(alias) + 1);
            parser->string_length--;
        }
    }
    return OK;
}

/* Modifies @parser->string and then splits it in two and stores two allocated
 * strings in @class.
 */
void resolve_class_string(Parser *parser, struct parse_class *class)
{
    utf8_t *next, *separator;
    size_t length;
    unsigned backslash_count;

    /* separate the parser string into instance and class */
    next = parser->string;
    length = parser->string_length;
    /* handle any comma "," that is escaped by a backslash "\" */
    do {
        separator = memchr(next, ',', length);
        if (separator == NULL) {
            break;
        }

        length -= separator - next + 1;
        next = separator + 1;

        backslash_count = 0;
        for (; separator > parser->string; separator--) {
            if (separator[-1] != '\\') {
                break;
            }
            backslash_count++;
        }

        if (backslash_count % 2 == 1) {
            /* remove a backslash \ */
            parser->string_length--;
            MOVE(separator, separator + 1,
                    &parser->string[parser->string_length] - separator);
            continue;
        }
    } while (0);

    if (separator == NULL) {
        class->instance = xstrdup("*");
    } else {
        class->instance = xstrndup(parser->string, separator - parser->string);
    }
    class->class = xstrndup(next, length);
}
