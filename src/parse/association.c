#include "parse/action.h"
#include "parse/parse.h"
#include "parse/utility.h"

#include "window.h"

/* Parse the next assigment in the active stream. */
void continue_parsing_association(Parser *parser)
{
    utf8_t *next, *separator;
    size_t length;
    int backslash_count;

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
        parser->instance_pattern_length = 0;
    } else {
        LIST_SET(parser->instance_pattern, 0, parser->string,
                separator - parser->string);
        LIST_APPEND_VALUE(parser->instance_pattern, '\0');
    }
    LIST_SET(parser->class_pattern, 0, next, length);
    LIST_APPEND_VALUE(parser->class_pattern, '\0');

    assert_read_string(parser);
    if (parser->is_string_quoted) {
        emit_parse_error(parser,
                "expected word and not a string for association");
        skip_all_statements(parser);
    } else if (continue_parsing_action(parser) == OK) {
        struct window_association association;

        if (parser->instance_pattern_length == 0) {
            association.instance_pattern = NULL;
        } else {
            LIST_COPY_ALL(parser->instance_pattern,
                    association.instance_pattern);
        }
        LIST_COPY_ALL(parser->class_pattern, association.class_pattern);

        LIST_COPY_ALL(parser->action_items, association.actions.items);
        association.actions.number_of_items = parser->action_items_length;
        LIST_COPY_ALL(parser->action_data, association.actions.data);

        /* set all to zero */
        LIST_SET(parser->action_data, 0, NULL, parser->action_data_length);

        LIST_APPEND_VALUE(parser->associations, association);
    } else {
        emit_parse_error(parser, "invalid action word");
        skip_all_statements(parser);
    }
}
