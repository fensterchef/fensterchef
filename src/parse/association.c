#include "core/window.h"
#include "parse/action.h"
#include "parse/parse.h"
#include "parse/utility.h"

/* Parse the next assigment in the active stream. */
void continue_parsing_association(Parser *parser)
{
    utf8_t *next, *separator;
    size_t length;
    int backslash_count;
    struct window_association association;
    struct parse_action_list list;

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
    } while (backslash_count % 2 == 1);

    if (separator == NULL) {
        association.instance_pattern = NULL;
    } else {
        association.instance_pattern = xstrndup(parser->string,
                separator - parser->string);
    }
    association.class_pattern = xstrndup(next, length);

    if (read_string(parser) != OK) {
        free(association.instance_pattern);
        free(association.class_pattern);
        emit_parse_error(parser, "unexpected token");
        return;
    }

    if (parser->is_string_quoted) {
        free(association.instance_pattern);
        free(association.class_pattern);
        emit_parse_error(parser,
                "expected word and not a string for association");
        skip_all_statements(parser);
    } else if (continue_parsing_action(parser, &list) == OK) {
        make_real_action_list(&association.actions, &list);
        LIST_APPEND_VALUE(parser->associations, association);
    } else {
        free(association.instance_pattern);
        free(association.class_pattern);
        emit_parse_error(parser, "invalid action word");
        skip_all_statements(parser);
    }
}
