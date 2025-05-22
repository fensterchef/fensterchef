#include <stdlib.h>

#include "core/binding.h"
#include "core/log.h"
#include "parse/action.h"
#include "parse/group.h"
#include "parse/top.h"
#include "parse/utility.h"

/* Do not put this macro in brackets! */
#define PARSE_MAX_FILL_RATE 4/5

/* map of all set groups */
struct parse_group group_table[PARSE_MAX_GROUPS];

/* the number of elements in the group table */
unsigned group_table_count;

/* Get the index where the group with given name is supposed to be. */
static unsigned get_group_index(const char *name)
{
    unsigned hash = 1731;
    unsigned probe = 0;
    unsigned index;

    for (const char *i = name; i[0] != '\0'; i++) {
        hash = hash * 407 + (unsigned char) i[0];
    }

    do {
        index = hash + (probe * probe + probe) / 2;
        index %= PARSE_MAX_GROUPS;
        probe++;
    } while (group_table[index].name != NULL &&
            strcmp(group_table[index].name, name) != 0);

    return index;
}

/* Find the group by given name. */
struct parse_group *find_group(const char *name)
{
    unsigned index;

    index = get_group_index(name);
    if (group_table[index].name == NULL) {
        return NULL;
    } else {
        return &group_table[index];
    }
}

/* Run counter actions that undo anything that the group did. */
void undo_group(const struct parse_group *group)
{
    const struct action_list *actions;
    const struct parse_data *data;
    struct button_binding button;
    struct key_binding key;
    struct window_relation relation;

    actions = &group->actions;
    data = actions->data;
    /* find all binding actions and make an unbind action to counter it */
    for (size_t i = 0; i < actions->number_of_items; i++) {
        if (actions->items[i].type == ACTION_BUTTON_BINDING &&
                data->u.button.actions.number_of_items > 0) {
            button = data->u.button;
            ZERO(&button.actions, 1);
            set_button_binding(&button);
        } else if (actions->items[i].type == ACTION_KEY_BINDING &&
                data->u.key.actions.number_of_items > 0) {
            key = data->u.key;
            ZERO(&key.actions, 1);
            set_key_binding(&key);
        } else if (actions->items[i].type == ACTION_RELATION) {
            relation = data->u.relation;
            ZERO(&relation.actions, 1);
            set_window_relation(&relation);
        }
        data += actions->items[i].data_count;
    }
}

/* Clear all groups the parser set. */
void clear_all_groups(void)
{
    for (unsigned i = 0; i < PARSE_MAX_GROUPS; i++) {
        if (group_table_count == 0) {
            break;
        }

        if (group_table[i].name != NULL) {
            free(group_table[i].name);
            clear_action_list(&group_table[i].actions);
            group_table[i].name = NULL;
            group_table_count--;
        }
    }
}

/* Parse all after a `group` keyword. */
void continue_parsing_group(Parser *parser)
{
    struct parse_action_list sub_list;
    unsigned index;
    struct parse_group group;

    if (read_string(parser) != OK) {
        emit_parse_error(parser, "expected name after group keyword\n");
        return;
    }

    group.name = xstrdup(parser->string);

    ZERO(&sub_list, 1);
    if (parse_top(parser, &sub_list) != OK) {
        free(group.name);
        clear_parse_list(&sub_list);
        return;
    }

    index = get_group_index(parser->string);

    if (group_table[index].name == NULL &&
            group_table_count + 1 > PARSE_MAX_GROUPS * PARSE_MAX_FILL_RATE) {
        clear_parse_list(&sub_list);
        free(group.name);
        emit_parse_error(parser, "there is no more space for groups");
        return;
    }

    clear_parse_list_data(&sub_list);

    make_real_action_list(&group.actions, &sub_list);

    if (group_table[index].name != NULL) {
        LOG("overwriting group %s\n",
                group.name);
        free(group_table[index].name);
        clear_action_list(&group_table[index].actions);
    } else {
        LOG("creating group %s\n",
                group.name);
        group_table_count++;
    }

    group_table[index] = group;
}

/* Parse all after a `ungroup` keyword. */
void continue_parsing_ungroup(Parser *parser, struct parse_action_list *list)
{
    struct action_list_item item;
    struct parse_data data;

    if (read_string(parser) != OK) {
        emit_parse_error(parser,
                "expected group name after 'ungroup'");
        return;
    }

    item.type = ACTION_UNGROUP;
    item.data_count = 1;
    LIST_APPEND_VALUE(list->items, item);

    data.flags = 0;
    data.type = PARSE_DATA_TYPE_STRING;
    data.u.string = xstrdup(parser->string);
    LIST_APPEND_VALUE(list->data, data);
}
