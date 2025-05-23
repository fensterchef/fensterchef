#include <string.h>

#include "core/log.h"
#include "core/window.h"
#include "parse/action.h"
#include "parse/input.h"
#include "parse/top.h"
#include "parse/utility.h"
#include "utility/utility.h"

/* Find a section in the action strings that match the string loaded into
 * @parser.
 *
 * @return ERROR if no action matches.
 */
static int resolve_action_word(Parser *parser, struct parse_action_list *list)
{
    action_type_t count = 0;

    for (action_type_t i = 0; i < ACTION_SIMPLE_MAX; i++) {
        const char *action, *space;
        unsigned skip_length;

        action = get_action_string(i);
        if (action == NULL) {
            /* some actions have special constructs */
            continue;
        }

        /* get the end of the next action word of the action string */
        space = strchr(action, ' ');
        if (space == NULL) {
            skip_length = strlen(action);
            space = action + skip_length;
        } else {
            skip_length = space - action + 1;
        }

        if (parser->string_length == (size_t) (space - action) &&
                memcmp(action, parser->string,
                    parser->string_length) == 0) {
            if (count == 0) {
                list->first_action = i;
            }

            count++;

            list->last_action = i + 1;

            list->actions[i].offset = skip_length;

            /* prepare the data for filling if there was data previously */
            list->actions[i].data_length = 0;
        } else if (count > 0) {
            /* optimization: no more will match because of the alphabetical
             * sorting
             */
            break;
        }
    }

    return count == 0 ? ERROR : OK;
}

/* Read the next string and find the actions where it matches.
 *
 * @return ERROR if no action matches.
 */
static int read_and_resolve_next_action_word(Parser *parser,
        struct parse_action_list *list)
{
    action_type_t count = 0;
    struct parse_data data;

    /* also skip new lines if there is an open bracket */
    if (list->bracket_count > 0) {
        skip_space(parser);
    }

    if (read_string(parser) != OK) {
        return ERROR;
    }

    /* go through all actions that matched previously */
    for (action_type_t i = list->first_action, end = list->last_action;
            i < end;
            i++) {
        const char *action, *space;
        unsigned skip_length;

        if (list->actions[i].offset == -1) {
            continue;
        }

        action = get_action_string(i);
        action = &action[list->actions[i].offset];

        /* get the end of the next action word of the action string */
        space = strchr(action, ' ');
        if (space == NULL) {
            skip_length = strlen(action);
            space = action + skip_length;
        } else {
            skip_length = space - action + 1;
        }

        if (resolve_data(parser, action[0], &data)) {
            if (data.type == PARSE_DATA_TYPE_MAX) {
                list->actions[i].offset = -1;
                continue;
            }
            LIST_APPEND_VALUE(list->actions[i].data, data);
         /* try to match the next word if no data type is required */
        } else if (parser->string_length != (size_t) (space - action) ||
                memcmp(action, parser->string,
                    parser->string_length) != 0) {
            list->actions[i].offset = -1;
            continue;
        }

        /* got a valid action */

        if (count == 0) {
            list->first_action = i;
        }
        count++;

        list->last_action = i + 1;

        list->actions[i].offset += skip_length;
    }

    return count == 0 ? ERROR : OK;
}

/* Compare two words. */
static int compare_words(const void *a, const void *b)
{
    return strcmp(*(const char**) a, *(const char**) b);
}

/* Print a word in the right color and expansion. */
static void print_word(const char *word)
{
    if (word[0] != '\0' && word[1] == '\0') {
        for (parse_data_type_t i = 0; i < PARSE_DATA_TYPE_MAX; i++) {
            if (parse_data_meta_information[i].identifier == word[0]) {
                fprintf(stderr, COLOR(BLUE) "%s",
                        parse_data_meta_information[i].name);
                return;
            }
        }
    }
    fprintf(stderr, COLOR(GREEN) "%s",
            word);
}

/* Print all possible actions to stderr. */
static void print_action_possibilities(struct parse_action_list *list)
{
    int actual_count = 0;

    fprintf(stderr, "possible words are: ");

    const int count = list->last_action - list->first_action;
    char *words[count];

    for (action_type_t i = list->first_action; i < list->last_action; i++) {
        const char *action, *space;
        int length;

        if (list->actions[i].offset == -1) {
            continue;
        }

        action = get_action_string(i);
        action = &action[list->actions[i].offset];

        /* get the end of the next action word of the action string */
        space = strchr(action, ' ');
        if (space == NULL) {
            length = strlen(action);
        } else {
            length = space - action;
        }

        words[actual_count] = xstrndup(action, length);
        actual_count++;
    }

    SORT(words, actual_count, compare_words);

    print_word(words[0]);
    for (int i = 1; i < actual_count; i++) {
        /* skip duplicate words */
        if (strcmp(words[i], words[i - 1]) == 0) {
            continue;
        }
        fprintf(stderr, CLEAR_COLOR ", ");
        print_word(words[i]);
    }
    fprintf(stderr, CLEAR_COLOR "\n");

    for (int i = 0; i < actual_count; i++) {
        free(words[i]);
    }
}

/* Parse the next action word or check for an action separator.
 *
 * @list is the place to append the parsed action to.
 */
static void parse_next_action_part(Parser *parser,
        struct parse_action_list *list)
{
    int character;

    skip_blanks(parser);
    character = peek_stream_character(parser);
    if (character == EOF || character == '\n' || character == ',' ||
            character == ')') {
        action_type_t type;

        for (type = list->first_action; type < list->last_action; type++) {
            if (list->actions[type].offset == -1) {
                continue;
            }

            const char *const action = get_action_string(type);
            if (action[list->actions[type].offset] == '\0') {
                break;
            }
        }

        if (type == list->last_action) {
            parser->start_index = parser->index;
            emit_parse_error(parser, "incomplete action");
            print_action_possibilities(list);
        } else {
            LIST_APPEND(list->items, NULL, 1);
            list->items[list->items_length - 1].type = type;
            list->items[list->items_length - 1].data_count =
                list->actions[type].data_length;
            LIST_APPEND(list->data,
                list->actions[type].data,
                list->actions[type].data_length);

            /* reset the count so the memory is not freed */
            list->actions[type].data_length = 0;
        }
    } else {
        if (read_and_resolve_next_action_word(parser, list) == ERROR) {
            emit_parse_error(parser, "invalid action word");
            skip_statement(parser);
        } else {
            parse_next_action_part(parser, list);
        }
    }
}

/* Parse an action list. */
int continue_parsing_actions(Parser *parser, struct parse_action_list *list)
{
    if (resolve_action_word(parser, list) == OK) {
        parse_next_action_part(parser, list);
        return OK;
    } else {
        return ERROR;
    }
}

/* Clear all memory within @list. */
void clear_parse_list(struct parse_action_list *list)
{
    free(list->items);
    for (size_t i = 0; i < list->data_length; i++) {
        clear_parse_data(&list->data[i]);
    }
    free(list->data);
    clear_parse_list_data(list);
}

/* Clear the data within @list that is needed for parsing and the
 * relations.
 */
void clear_parse_list_data(struct parse_action_list *list)
{
    /* free the action parse data */
    for (action_type_t i = 0; i < ACTION_SIMPLE_MAX; i++) {
        for (size_t j = 0; j < list->actions[i].data_length; j++) {
            clear_parse_data(&list->actions[i].data[j]);
        }
        free(list->actions[i].data);
    }
}

/* Make a real action list from a parser action list. */
void make_real_action_list(struct action_list *real_list,
        struct parse_action_list *list)
{
    /* trim the memory for convenience of the caller */
    REALLOCATE(list->items, list->items_length);
    list->items_capacity = list->items_length;
    REALLOCATE(list->data, list->data_length);
    list->data_capacity = list->data_length;

    real_list->items = list->items;
    real_list->number_of_items = list->items_length;
    real_list->data = list->data;
}
