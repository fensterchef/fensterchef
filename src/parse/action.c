#include <string.h>

#include "core/log.h"
#include "core/window.h"
#include "parse/action.h"
#include "parse/input.h"
#include "parse/integer.h"
#include "parse/top.h"
#include "parse/utility.h"
#include "utility/utility.h"

/* Find a section in the action strings that match the string loaded into
 * @parser.
 *
 * @return ERROR if no action matches.
 */
static int resolve_action_word(Parser *parser, struct parse_action_block *block)
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
                block->first_action = i;
            }

            count++;

            block->last_action = i + 1;

            block->actions[i].offset = skip_length;

            /* prepare the data for filling if there was data previously */
            block->actions[i].data_length = 0;
        } else if (count > 0) {
            /* optimization: no more will match because of the alphabetical
             * sorting
             */
            break;
        }
    }

    return count == 0 ? ERROR : OK;
}

/* Resolve the string within parser as a data point.
 *
 * @return false if @identifier is invalid.
 *
 * data->type is set to PARSE_DATA_TYPE_MAX if the string within parser does not
 * match the data type.
 */
static bool resolve_data(Parser *parser, char identifier,
        _Out struct action_data *data)
{
    action_data_type_t type;

    type = get_action_data_type_from_identifier(identifier);
    if (type == ACTION_DATA_TYPE_MAX) {
        return false;
    }

    data->type = type;

    switch (type) {
    case ACTION_DATA_TYPE_INTEGER:
        if (continue_parsing_integer_expression(parser, &data->flags,
                    &data->u.integer) != OK) {
            data->type = ACTION_DATA_TYPE_MAX;
        }
        break;

    case ACTION_DATA_TYPE_STRING:
        LIST_COPY(parser->string, 0, parser->string_length + 1,
                data->u.string);
        break;

    case ACTION_DATA_TYPE_RELATION:
    case ACTION_DATA_TYPE_BUTTON:
    case ACTION_DATA_TYPE_KEY:
        /* not supported as action parameter */
        break;

    case ACTION_DATA_TYPE_MAX:
        /* nothing */
        break;
    }

    return true;
}

/* Read the next string and find the actions where it matches.
 *
 * @return ERROR if no action matches.
 */
static int read_and_resolve_next_action_word(Parser *parser,
        struct parse_action_block *block)
{
    action_type_t count = 0;
    struct action_data data;

    /* also skip new lines if there is an open bracket */
    if (block->bracket_count > 0) {
        skip_space(parser);
    }

    if (read_string(parser) != OK) {
        return ERROR;
    }

    /* go through all actions that matched previously */
    for (action_type_t i = block->first_action, end = block->last_action;
            i < end;
            i++) {
        const char *action, *space;
        unsigned skip_length;

        if (block->actions[i].offset == -1) {
            continue;
        }

        action = get_action_string(i);
        action = &action[block->actions[i].offset];

        /* get the end of the next action word of the action string */
        space = strchr(action, ' ');
        if (space == NULL) {
            skip_length = strlen(action);
            space = action + skip_length;
        } else {
            skip_length = space - action + 1;
        }

        if (resolve_data(parser, action[0], &data)) {
            if (data.type == ACTION_DATA_TYPE_MAX) {
                block->actions[i].offset = -1;
                continue;
            }
            LIST_APPEND_VALUE(block->actions[i].data, data);
         /* try to match the next word if no data type is required */
        } else if (parser->string_length != (size_t) (space - action) ||
                memcmp(action, parser->string,
                    parser->string_length) != 0) {
            block->actions[i].offset = -1;
            continue;
        }

        /* got a valid action */

        if (count == 0) {
            block->first_action = i;
        }
        count++;

        block->last_action = i + 1;

        block->actions[i].offset += skip_length;
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
        action_data_type_t type;

        type = get_action_data_type_from_identifier(word[0]);
        if (type != ACTION_DATA_TYPE_MAX) {
            fprintf(stderr, COLOR(BLUE) "%s",
                    get_action_data_type_name(type));
            return;
        }
    }
    fprintf(stderr, COLOR(GREEN) "%s",
            word);
}

/* Print all possible actions to stderr. */
static void print_action_possibilities(struct parse_action_block *block)
{
    int actual_count = 0;

    fprintf(stderr, "possible words are: ");

    const int count = block->last_action - block->first_action;
    char *words[count];

    for (action_type_t i = block->first_action; i < block->last_action; i++) {
        const char *action, *space;
        int length;

        if (block->actions[i].offset == -1) {
            continue;
        }

        action = get_action_string(i);
        action = &action[block->actions[i].offset];

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
 * @block is the place to append the parsed action to.
 */
static void parse_next_action_part(Parser *parser,
        struct parse_action_block *block)
{
    int character;

    skip_blanks(parser);
    character = peek_stream_character(parser);
    if (character == EOF || character == '\n' || character == ',' ||
            character == ')') {
        action_type_t type;

        for (type = block->first_action; type < block->last_action; type++) {
            if (block->actions[type].offset == -1) {
                continue;
            }

            const char *const action = get_action_string(type);
            if (action[block->actions[type].offset] == '\0') {
                break;
            }
        }

        if (type == block->last_action) {
            parser->start_index = parser->index;
            emit_parse_error(parser, "incomplete action");
            print_action_possibilities(block);
        } else {
            LIST_APPEND(block->items, NULL, 1);
            block->items[block->items_length - 1].type = type;
            block->items[block->items_length - 1].data_count =
                block->actions[type].data_length;
            LIST_APPEND(block->data,
                block->actions[type].data,
                block->actions[type].data_length);

            /* reset the count so the memory is not freed */
            block->actions[type].data_length = 0;
        }
    } else {
        if (read_and_resolve_next_action_word(parser, block) == ERROR) {
            emit_parse_error(parser, "invalid action word");
            skip_statement(parser);
        } else {
            parse_next_action_part(parser, block);
        }
    }
}

/* Parse an action block. */
int continue_parsing_actions(Parser *parser, struct parse_action_block *block)
{
    if (resolve_action_word(parser, block) == OK) {
        parse_next_action_part(parser, block);
        return OK;
    } else {
        return ERROR;
    }
}

/* Clear all memory within @block. */
void clear_parse_action_block(struct parse_action_block *block)
{
    free(block->items);
    for (size_t i = 0; i < block->data_length; i++) {
        clear_action_data(&block->data[i]);
    }
    free(block->data);

    /* free the action parse data */
    for (action_type_t i = 0; i < ACTION_SIMPLE_MAX; i++) {
        for (size_t j = 0; j < block->actions[i].data_length; j++) {
            clear_action_data(&block->actions[i].data[j]);
        }
        free(block->actions[i].data);
    }
}

/* Make a real action block from a parser action block. */
ActionBlock *convert_parse_action_block(struct parse_action_block *parse)
{
    ActionBlock *block;

    block = create_empty_action_block(parse->items_length, parse->data_length);

    COPY(block->items, parse->items, parse->items_length);
    COPY(block->data, parse->data, parse->data_length);

    LIST_CLEAR(parse->items);
    LIST_CLEAR(parse->data);

    return block;
}
