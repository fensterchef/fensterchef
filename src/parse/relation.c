#include "core/window.h"
#include "parse/action.h"
#include "parse/parse.h"
#include "parse/top.h"
#include "parse/utility.h"

/* Parse the next assigment in the active stream. */
void continue_parsing_relation(Parser *parser,
        struct parse_action_list *list)
{
    struct window_relation relation;
    struct parse_action_list sub_list;
    struct action_list_item item;
    struct parse_class class;
    struct parse_data data;

    resolve_class_string(parser, &class);

    ZERO(&sub_list, 1);
    if (parse_top(parser, &sub_list) != OK) {
        emit_parse_error(parser,
                "expected actions after relation pattern");
        clear_parse_list(&sub_list);
        free(class.instance);
        free(class.class);
        return;
    }

    clear_parse_list_data(&sub_list);

    relation.class_pattern = class.class;
    relation.instance_pattern = class.instance;
    make_real_action_list(&relation.actions, &sub_list);

    item.type = ACTION_RELATION;
    item.data_count = 1;
    LIST_APPEND_VALUE(list->items, item);

    data.flags = 0;
    data.type = PARSE_DATA_TYPE_RELATION;
    data.u.relation = relation;
    LIST_APPEND_VALUE(list->data, data);
}
