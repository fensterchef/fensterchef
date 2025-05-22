#include "core/window.h"
#include "parse/action.h"
#include "parse/parse.h"
#include "parse/top.h"
#include "parse/utility.h"

/* Modifies @parser->string and then splits it in two and stores two allocated
 * strings in @class.
 */
static void resolve_class_string(Parser *parser,
        struct window_relation *relation)
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
        relation->instance_pattern = xstrdup("*");
    } else {
        relation->instance_pattern =
            xstrndup(parser->string, separator - parser->string);
    }
    relation->class_pattern = xstrndup(next, length);
}

/* Parse the next assigment in the active stream. */
void continue_parsing_relation(Parser *parser,
        struct parse_action_list *list)
{
    struct window_relation relation;
    struct parse_action_list sub_list;
    struct action_list_item item;
    struct parse_data data;

    if (read_string(parser) != OK) {
        emit_parse_error(parser,
                "expected instance,class pattern to relate to\n");
        return;
    }

    resolve_class_string(parser, &relation);

    ZERO(&sub_list, 1);
    if (parse_top(parser, &sub_list) != OK) {
        emit_parse_error(parser,
                "expected actions after relation pattern");
        clear_parse_list(&sub_list);
        free(relation.instance_pattern);
        free(relation.class_pattern);
        return;
    }

    clear_parse_list_data(&sub_list);

    make_real_action_list(&relation.actions, &sub_list);

    item.type = ACTION_RELATION;
    item.data_count = 1;
    LIST_APPEND_VALUE(list->items, item);

    data.flags = 0;
    data.type = PARSE_DATA_TYPE_RELATION;
    data.u.relation = relation;
    LIST_APPEND_VALUE(list->data, data);
}

/* Parse all after an `unrelate` keyword. */
void continue_parsing_unrelate(Parser *parser,
        struct parse_action_list *list)
{
    struct action_list_item item;
    struct window_relation relation;
    struct parse_data data;

    if (read_string(parser) != OK) {
        /* `unrelate` without a following string */
        item.type = ACTION_UNRELATE;
        item.data_count = 0;
        LIST_APPEND_VALUE(list->items, item);
    } else {
        resolve_class_string(parser, &relation);

        ZERO(&relation.actions, 1);

        item.type = ACTION_RELATION;
        item.data_count = 1;
        LIST_APPEND_VALUE(list->items, item);

        data.flags = 0;
        data.type = PARSE_DATA_TYPE_RELATION;
        data.u.relation = relation;
        LIST_APPEND_VALUE(list->data, data);
    }
}
