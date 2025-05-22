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

    skip_blanks(parser);

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
