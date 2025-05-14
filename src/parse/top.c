#include <errno.h>

#include "core/log.h"
#include "core/window.h"
#include "parse/action.h"
#include "parse/alias.h"
#include "parse/association.h"
#include "parse/binding.h"
#include "parse/input.h"
#include "parse/top.h"
#include "parse/utility.h"

/* Parse all after a `source` keyword. */
static void continue_parsing_source(Parser *parser,
        struct parse_action_list *list)
{
    Parser *upper;
    Parser *sub_parser;
    struct parse_action_list sub_list;

    if (read_string(parser) != OK) {
        emit_parse_error(parser, "expected file string");
        return;
    }

    /* Check for a recursive sourcing.  This simple check always works
     * because sourcing is not conditional, it always happens.
     */
    for (upper = parser; upper != NULL; upper = upper->upper_parser) {
        if (upper->file_path != NULL &&
                strcmp(upper->file_path, parser->string) == 0) {
            break;
        }
    }
    if (upper != NULL) {
        emit_parse_error(parser, "sourcing file \"%s\" recursively",
                parser->string);
        return;
    }

    sub_parser = create_file_parser(parser->string);
    if (sub_parser == NULL) {
        emit_parse_error(parser, "can not source \"%s\": %s",
                parser->string, strerror(errno));
        return;
    }
    sub_parser->upper_parser = parser;

    ZERO(&sub_list, 1);
    while (parse_top(sub_parser, &sub_list) == OK) {
        /* nothing */
    }

    /* if parsing succeeds, append the parsed items */
    if (sub_parser->error_count == 0) {
        LIST_APPEND(list->items,
                sub_list.items,
                sub_list.items_length);

        LIST_APPEND(list->data,
                sub_list.data,
                sub_list.data_length);
    }

    destroy_parser(sub_parser);
}

/* Parse a top level statement. */
int parse_top(Parser *parser, struct parse_action_list *list)
{
    int character;

    if (parser->error_count >= PARSE_MAX_ERROR_COUNT) {
        LOG_ERROR("parsing stopped: too many errors occured\n");
        return ERROR;
    }

    skip_space(parser);
    character = peek_stream_character(parser);
    if (character == EOF) {
        return ERROR;
    }

    /* set this in case an error is printed */
    parser->start_index = parser->index;

    if (character == '(') {
        unsigned bracket_count;

        /* skip over opening '(' */
        (void) get_stream_character(parser);

        list->bracket_count++;
        bracket_count = list->bracket_count;
        while (parse_top(parser, list) == OK) {
            /* nothing */
        }
        /* the inner function should have decremented this again */
        if (bracket_count == list->bracket_count) {
            emit_parse_error(parser, "missing closing bracket ')'");
        }

        return OK;
    } else if (character == ')') {
        /* skip over closing ')' */
        (void) get_stream_character(parser);

        if (list->bracket_count == 0) {
            emit_parse_error(parser, "no matching opening ')'");
        } else {
            list->bracket_count--;
        }

        return ERROR;
    }

    if (read_string(parser) != OK) {
        emit_parse_error(parser,
                "expected association, binding or action");
        /* skip the erroneous character */
        (void) get_stream_character(parser);
        /* start from the top */
        return parse_top(parser, list);
    }

    if (parser->is_string_quoted) {
        continue_parsing_association(parser, list);
    } else if (strcmp(parser->string, "alias") == 0) {
        continue_parsing_alias(parser);
    } else if (strcmp(parser->string, "unalias") == 0) {
        continue_parsing_unalias(parser);
    } else if (strcmp(parser->string, "source") == 0) {
        continue_parsing_source(parser, list);
    } else if (strcmp(parser->string, "unbind") == 0) {
        continue_parsing_unbind(parser, list);
    } else {
        if (continue_parsing_actions(parser, list) != OK) {
            continue_parsing_binding(parser, list);
        }
    }

    character = peek_stream_character(parser);
    if (character == ',') {
        /* skip ',' and parse from top */
        (void) get_stream_character(parser);
        /* parse the next statement as well */
        return parse_top(parser, list);
    }

    /* even if an error occured, we at least got something */
    return OK;
}
