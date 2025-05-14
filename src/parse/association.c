#include "core/window.h"
#include "parse/action.h"
#include "parse/parse.h"
#include "parse/top.h"
#include "parse/utility.h"

/* Parse the next assigment in the active stream. */
void continue_parsing_association(Parser *parser,
        struct parse_action_list *list)
{
    utf8_t *next, *separator;
    size_t length;
    unsigned backslash_count;
    struct window_association association;
    struct parse_action_list sub_list;
    struct action_list_item item;
    struct parse_generic_data data;

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
    } while (backslash_count % 2 == 1);

    if (separator == NULL) {
        association.instance_pattern = NULL;
    } else {
        association.instance_pattern = xstrndup(parser->string,
                separator - parser->string);
    }
    association.class_pattern = xstrndup(next, length);

    ZERO(&sub_list, 1);
    if (parse_top(parser, &sub_list) != OK) {
        emit_parse_error(parser,
                "expected actions after association pattern");
        clear_parse_list(&sub_list);
        free(association.instance_pattern);
        free(association.class_pattern);
        return;
    }

    clear_parse_list_data(&sub_list);

    make_real_action_list(&association.actions, &sub_list);

    item.type = ACTION_ASSOCIATION;
    item.data_count = 1;
    LIST_APPEND_VALUE(list->items, item);

    data.flags = 0;
    data.type = PARSE_DATA_TYPE_ASSOCIATION;
    data.u.association = association;
    LIST_APPEND_VALUE(list->data, data);
}
