#include "core/window.h"
#include "parse/action.h"
#include "parse/input.h"
#include "parse/parse.h"
#include "parse/top.h"
#include "parse/utility.h"

/* Read an optional ", CLASS" after reading a string. */
static void read_class_string(Parser *parser,
        struct window_relation *relation)
{
    utf8_t *pattern;

    pattern = xstrdup(parser->string);

    skip_blanks(parser);
    if (peek_stream_character(parser) == ',') {
        /* skip ',' */
        (void) get_stream_character(parser);

        if (read_string(parser) != OK) {
            emit_parse_error(parser,
                    "expected class name");
            relation->class_pattern = NULL;
        } else {
            relation->class_pattern = xstrdup(parser->string);
        }

        relation->instance_pattern = pattern;
    } else {
        relation->instance_pattern = xstrdup("*");
        relation->class_pattern = pattern;
    }
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

    read_class_string(parser, &relation);

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
        read_class_string(parser, &relation);

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
