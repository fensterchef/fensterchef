#include <ctype.h>

#include "parse/binding.h"
#include "parse/input.h"
#include "parse/integer.h"
#include "parse/utility.h"

/* Try to resolve the string as boolean constant. */
static int resolve_boolean(Parser *parser, _Out bool *boolean)
{
    /* a translation table from string to boolean */
    static const struct {
        const char *string;
        bool boolean;
    } string_to_boolean[] = {
        { "true", true },
        { "false", false },
        { "on", true },
        { "off", false },
        { "yes", true },
        { "no", false },
    };

    unsigned index;

    if (parser->string_length < 2) {
        return ERROR;
    }

    /* the second character is unique among all constants */
    switch (parser->string[1]) {
    case 'r': index = 0; break;
    case 'a': index = 1; break;
    case 'n': index = 2; break;
    case 'f': index = 3; break;
    case 'e': index = 4; break;
    case 'o': index = 5; break;
    default:
        return ERROR;
    }

    if (strcmp(string_to_boolean[index].string, parser->string) == 0) {
        *boolean = string_to_boolean[index].boolean;
        return OK;
    } else {
        return ERROR;
    }
}

/* Try to resolve the string within @parser as integer. */
int resolve_integer(Parser *parser,
        unsigned *flags,
        parse_integer_t *output_integer)
{
    char *word;
    int error = ERROR;
    parse_integer_t sign = 1, integer = 0;
    bool boolean;
    unsigned modifier;

    if (parser->is_string_quoted) {
        return ERROR;
    }

    word = parser->string;
    *flags = 0;
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
            *flags |= PARSE_DATA_FLAGS_IS_PERCENT;
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
    } else if (resolve_boolean(parser, &boolean) == OK) {
        integer = boolean;
        error = OK;
    } else if (resolve_modifier(parser, &modifier) == OK) {
        integer = modifier;
        error = OK;
    }

    *output_integer = sign * integer;
    return error;
}

/* Continue parsing an integer expression. */
int continue_parsing_integer_expression(Parser *parser,
        _Out unsigned *output_flags,
        _Out parse_integer_t *output_integer)
{
    int character;
    bool has_anything = false;
    unsigned flags = 0;
    parse_integer_t integer = 0;
    unsigned sub_flags;
    parse_integer_t sub_integer;

    /* check for any integer operators */
    while (skip_blanks(parser),
            character = peek_stream_character(parser),
            character == '+') {
        has_anything = true;

        /* skip over '+' */
        (void) get_stream_character(parser);

        if (resolve_integer(parser, &sub_flags, &sub_integer) != OK) {
            emit_parse_error(parser, "invalid integer value");
        }

        flags |= sub_flags;
        integer += sub_integer;

        if (read_string(parser) != OK) {
            emit_parse_error(parser, "expected integer value after '+'");
            skip_all_statements(parser);
            break;
        }
    }

    /* read the last value that might had a leading plus + */
    if (resolve_integer(parser, &sub_flags, &sub_integer) != OK) {
        if (!has_anything) {
            return ERROR;
        }
    }

    flags |= sub_flags;
    integer += sub_integer;

    *output_flags = flags;
    *output_integer = integer;

    return OK;
}
