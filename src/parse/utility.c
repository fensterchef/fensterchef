#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <X11/keysym.h>

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

    while (character = get_stream_character(parser),
            character != ',' && character != '\n' && character != EOF) {
        /* skip over quotes */
        if (character == '\"' || character == '\'') {
            const int quote = character;
            while (parser->index = parser->index,
                    character = get_stream_character(parser),
                    character != quote && character != EOF &&
                    character != '\n') {
                /* nothing */
            }
        }
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
static inline bool is_word_character(int character)
{
    if (iscntrl(character)) {
        return false;
    }

    if (character < 0) {
        return false;
    }

    if (character == ' ' || character == '!' || character == '\"' ||
            character == '\'' || character == ',' || character == ';' ||
            character == '+') {
        return false;
    }

    return true;
}

/* Read a string/word from the active input stream. */
int read_string(Parser *parser)
{
    int character;

    skip_space(parser);

    parser->index = parser->index;
    parser->string_length = 0;
    character = peek_stream_character(parser);
    if (character == '\"' || character == '\'') {
        parser->is_string_quoted = true;

        const int quote = character;

        /* skip over the opening quote */
        (void) get_stream_character(parser);

        while (character = get_stream_character(parser),
                character != quote && character != EOF && character != '\n') {
            /* escape any characters following \\ */
            if (character == '\\') {
                character = get_stream_character(parser);
                LIST_APPEND_VALUE(parser->string, '\\');
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

    /* append a nul-terminator but reduce the size again */
    LIST_APPEND_VALUE(parser->string, '\0');
    parser->string_length--;
    return OK;
}

/* Makes sure that next comes a word. */
void assert_read_string(Parser *parser)
{
    if (read_string(parser) != OK) {
        skip_all_statements(parser);
        emit_parse_error(parser, "unexpected token");
        longjmp(parser->throw_jump, 1);
    }
}

/* Translate the string within @parser to a button index. */
int resolve_button(Parser *parser)
{
    /* conversion from string to button index */
    static const struct button_string {
        /* the string representation of the button */
        const char *name;
        /* the button index */
        button_t button_index;
    } button_strings[] = {
        /* buttons can also be Button<integer> to directly address the index */
        { "LButton", BUTTON_LEFT },
        { "LeftButton", BUTTON_LEFT },

        { "MButton", BUTTON_MIDDLE },
        { "MiddleButton", BUTTON_MIDDLE },

        { "RButton", BUTTON_RIGHT },
        { "RightButton", BUTTON_RIGHT },

        { "ScrollUp", BUTTON_WHEEL_UP },
        { "WheelUp", BUTTON_WHEEL_UP },

        { "ScrollDown", BUTTON_WHEEL_DOWN },
        { "WheelDown", BUTTON_WHEEL_DOWN },

        { "ScrollLeft", BUTTON_WHEEL_LEFT },
        { "WheelLeft", BUTTON_WHEEL_LEFT },

        { "ScrollRight", BUTTON_WHEEL_RIGHT },
        { "WheelRight", BUTTON_WHEEL_RIGHT },
    };

    int index = -1;
    const char *string;

    string = parser->string;
    /* parse strings starting with "X" */
    if (string[0] == 'X') {
        int x_index = 0;

        string++;
        while (isdigit(string[0])) {
            x_index *= 10;
            x_index += string[0] - '0';
            if (x_index >= BUTTON_MAX - BUTTON_X1) {
                emit_parse_error(parser, "X button value is too high");
                x_index = 1 - BUTTON_X1;
                break;
            }
            string++;
        }

        index = BUTTON_X1 + x_index - 1;
    /* parse strings starting with "Button" */
    } else if (strncmp(string, "Button", strlen("Button")) == 0) {
        index = 0;
        string += strlen("Button");
        while (isdigit(string[0])) {
            index *= 10;
            index += string[0] - '0';
            if (index > UINT8_MAX) {
                index = BUTTON_NONE;
                emit_parse_error(parser, "button value is too high");
                break;
            }
            string++;
        }
    } else {
        for (unsigned i = 0; i < SIZE(button_strings); i++) {
            if (strcmp(string, button_strings[i].name) == 0) {
                index = button_strings[i].button_index;
                break;
            }
        }
    }

    if (string[0] != '\0') {
        index = BUTTON_NONE;
    }

    return index;
}

/* Try to resolve `parser->string` as integer.
 *
 * @return ERROR if `parser->string` is not an integer.
 */
int resolve_integer(Parser *parser)
{
    /* a translation table from string to integer */
    static const struct {
        const char *string;
        parse_integer_t integer;
    } string_to_integer[] = {
        { "true", 1 },
        { "false", 0 },
        { "on", 1 },
        { "off", 0 },
        { "yes", 1 },
        { "no", 0 },

        { "None", 0 },
        { "Shift", ShiftMask },
        { "Lock", LockMask },
        { "CapsLock", LockMask },
        { "Control", ControlMask },
        { "Alt", Mod1Mask },
        { "Mod1", Mod1Mask },
        { "Mod2", Mod2Mask },
        { "Mod3", Mod3Mask },
        { "Super", Mod4Mask },
        { "Mod4", Mod4Mask },
        { "Mod5", Mod5Mask },
    };

    char *word;
    int error = ERROR;
    parse_integer_t sign = 1, integer = 0;

    word = parser->string;
    parser->data.flags = 0;
    if ((word[0] == '-' && isdigit(word[1])) || isdigit(word[0])) {
        if (word[0] == '-') {
            sign = -1;
            word++;
        }
        integer = word[0] - '0';
        word++;
        while (isdigit(word[0])) {
            integer *= 10;
            integer += word[0] - '0';
            word++;

            if (integer > PARSE_INTEGER_LIMIT) {
                emit_parse_error(parser, "integer overflows "
                        STRINGIFY(PARSE_INTEGER_LIMIT));
                do {
                    word++;
                } while (isdigit(word[0]));

                break;
            }
        }

        if (word[0] == '%') {
            parser->data.flags |= PARSE_DATA_FLAGS_IS_PERCENT;
            word++;
        }

        if (word[0] == '\0') {
            error = OK;
        }
    } else if (word[0] == '#') {
        int count = 0;

        word++;
        for (; isxdigit(word[0]); word++) {
            count++;
            integer <<= 4;
            integer += isdigit(word[0]) ?
                    word[0] - '0' : tolower(word[0]) + 0xa - 'a';
        }

        if (count == 0) {
            emit_parse_error(parser, "expected hexadecimal digits");
        }

        /* if no alpha channel is specified, make it fully opaque */
        if (count <= 6) {
            integer |= 0xff << 24;
        }

        if (word[0] == '\0') {
            error = OK;
        }
    } else {
        for (unsigned i = 0; i < SIZE(string_to_integer); i++) {
            if (strcmp(string_to_integer[i].string, word) == 0) {
                integer = string_to_integer[i].integer;
                error = OK;
                break;
            }
        }
    }

    parser->data.u.integer = sign * integer;
    return error;
}

/* Translate a string to some extended key symbols. */
KeySym resolve_key_symbol(Parser *parser)
{
    struct {
        const char *name;
        KeySym key_symbol;
    } symbol_table[] = {
        /* since integers are interpreted as keycodes, these are needed to still
         * use the digit keys
         */
        { "zero", XK_0 },
        { "one", XK_1 },
        { "two", XK_2 },
        { "three", XK_3 },
        { "four", XK_4 },
        { "five", XK_5 },
        { "six", XK_6 },
        { "seven", XK_7 },
        { "eight", XK_8 },
        { "nine", XK_9 },
    };

    for (unsigned i = 0; i < SIZE(symbol_table); i++) {
        if (strcmp(parser->string, symbol_table[i].name) == 0) {
            return symbol_table[i].key_symbol;
        }
    }

    return XStringToKeysym(parser->string);
}
