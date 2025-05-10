#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include <X11/Xlib.h>

#include "configuration.h"
#include "cursor.h"
#include "font.h"
#include "log.h"
#include "parse/action.h"
#include "parse/association.h"
#include "parse/binding.h"
#include "parse/input.h"
#include "parse/parse.h"
#include "parse/utility.h"
#include "window.h"
#include "x11_synchronize.h"

/* Emit a parse error. */
void emit_parse_error(Parser *parser, const char *format, ...)
{
    unsigned line, column;
    const utf8_t *file;
    const char *string_line;
    unsigned length;
    va_list list;
    Parser *upper;

    parser->error_count++;
    get_stream_position(parser, parser->start_index, &line, &column);
    string_line = get_stream_line(parser, line, &length);

    file = parser->file_path;
    if (file == NULL) {
        file = "<string>";
    }

    upper = parser->upper_parser;
    if (upper != NULL) {
        unsigned line, column;


        fprintf(stderr, "In file included from " COLOR(GREEN) "%s" CLEAR_COLOR,
                upper->file_path);
        get_stream_position(parser, upper->start_index, &line, &column);
        fprintf(stderr, ":" COLOR(GREEN) "%u" CLEAR_COLOR,
                line + 1);

        upper = upper->upper_parser;
        for (; upper != NULL; upper = upper->upper_parser) {
            fprintf(stderr, ",\n                 from ");
            fprintf(stderr, COLOR(GREEN) "%s" CLEAR_COLOR,
                    upper->file_path);
            get_stream_position(parser, upper->start_index, &line, &column);
            fprintf(stderr, ":" COLOR(GREEN) "%u" CLEAR_COLOR,
                    line + 1);
        }
        fprintf(stderr, ":\n");
    }

    LOG_ERROR("%s:%d:%d: ",
            file, line + 1, column + 1);

    va_start(list, format);
    vfprintf(stderr, format, list);
    va_end(list);
    fprintf(stderr, "\n%.*s\n", (int) length, string_line);
    for (unsigned i = 0; i < column; i++) {
        fputc(' ', stderr);
    }
    fputs("^\n", stderr);
}

/* Initialize a parse object to parse file at given path. */
Parser *create_file_parser(const utf8_t *path)
{
    FILE *file;
    long size;
    Parser *parser;

    file = fopen(path, "r");
    if (file == NULL) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size = ftell(file);

    parser = xmalloc(sizeof(*parser) + size);
    memset(parser, 0, sizeof(*parser));

    parser->file_path = xstrdup(path);

    parser->length = size;

    fseek(file, 0, SEEK_SET);
    fread(parser->input, 1, size, file);

    fclose(file);

    return parser;
}

/* Initialize a parse object to parse a given string. */
Parser *create_string_parser(const utf8_t *string)
{
    size_t size;
    Parser *parser;

    size = strlen(string);

    parser = xmalloc(sizeof(*parser) + size);
    memset(parser, 0, sizeof(*parser));

    parser->length = size;

    COPY(parser->input, string, size);
    return parser;
}

/* Destroy a previously allocated parser object. */
void destroy_parser(Parser *parser)
{
    if (parser != NULL) {
        struct action_list list;

        free(parser->file_path);
        free(parser->string);

        for (action_type_t i = 0; i < ACTION_SIMPLE_MAX; i++) {
            free(parser->actions[i].data);
        }

        free(parser->instance_pattern);
        free(parser->class_pattern);

        list.items = parser->action_items;
        list.number_of_items = parser->action_items_length;
        list.data = parser->action_data;
        clear_action_list(&list);

        list.items = parser->startup_items;
        list.number_of_items = parser->startup_items_length;
        list.data = parser->startup_data;
        clear_action_list(&list);

        for (size_t i = 0; i < parser->associations_length; i++) {
            free(parser->associations[i].class_pattern);
            free(parser->associations[i].instance_pattern);
            clear_action_list(&parser->associations[i].actions);
        }
        free(parser->associations);

        free(parser);
    }
}

/* Parse all after a `source` keyword. */
static void continue_parsing_source(Parser *parser)
{
    Parser *upper;
    Parser *sub_parser;

    assert_read_string(parser);

    /* Check for a recursive sourcing.  This simple check always works
     * because sourcing is not conditional, it always happens.
     */
    for (upper = parser; upper != NULL; upper = upper->upper_parser) {
        if (strcmp(upper->file_path, parser->string) == 0) {
            break;
        }
    }
    if (upper != NULL) {
        emit_parse_error(parser, "sourcing file \"%s\" recursively\n",
                parser->string);
        return;
    }

    sub_parser = create_file_parser(parser->string);
    if (sub_parser == NULL) {
        emit_parse_error(parser, "can not source \"%s\": %s\n",
                parser->string, strerror(errno));
        return;
    }
    sub_parser->upper_parser = parser;

    /* if parsing succeeds, append the parsed items */
    if (start_parser(sub_parser) == OK) {
        LIST_APPEND(parser->startup_items,
                sub_parser->startup_items,
                sub_parser->startup_items_length);
        sub_parser->startup_items_length = 0;

        LIST_APPEND(parser->startup_data,
                sub_parser->startup_data,
                sub_parser->startup_data_length);
        sub_parser->startup_data_length = 0;

        LIST_APPEND(parser->associations,
                sub_parser->associations,
                sub_parser->associations_length);
        sub_parser->associations_length = 0;
    }
    destroy_parser(sub_parser);
}

/* Parse the currently active stream. */
int start_parser(Parser *parser)
{
    int character;

    /* Clear an old parser.  This usually does not matter because a parser
     * object is destroyed after parsing.
     */
    parser->startup_items_length = 0;
    parser->startup_data_length = 0;
    parser->associations_length = 0;

    /* make it so `throw_parse_error()` jumps to after this statement */
    (void) setjmp(parser->throw_jump);

    if (parser->error_count >= PARSE_MAX_ERROR_COUNT) {
        emit_parse_error(parser, "parsing stopped: too many errors occured");
        return ERROR;
    }

    while (skip_space(parser), character = peek_stream_character(parser),
            character != EOF) {
        assert_read_string(parser);

        if (parser->is_string_quoted) {
            continue_parsing_association(parser);
        } else if (strcmp(parser->string, "source") == 0) {
            continue_parsing_source(parser);
        } else {
            if (continue_parsing_action(parser) == OK) {
                LIST_APPEND(parser->startup_items,
                        parser->action_items, parser->action_items_length);
                LIST_APPEND(parser->startup_data,
                        parser->action_data, parser->action_data_length);
            } else {
                continue_parsing_modifiers_or_binding(parser);
            }
        }
    }

    return parser->error_count > 0 ? ERROR : OK;
}

/* Parse the currently active stream and run all actions. */
int parse_and_run_actions(Parser *parser)
{
    struct action_list startup;

    if (start_parser(parser) != OK) {
        return ERROR;
    }

    add_window_associations(parser->associations, parser->associations_length);
    ZERO(parser->associations, parser->associations_length);
    parser->associations_length = 0;

    /* do the startup actions */
    startup.items = parser->startup_items;
    startup.number_of_items = parser->startup_items_length;
    startup.data = parser->startup_data;
    LOG_DEBUG("running startup actions: %A\n",
            &startup);
    run_action_list(&startup);

    return OK;
}

/* Parse the currently active stream and use it to override the configuration. */
int parse_and_replace_configuration(Parser *parser)
{
    struct action_list startup;

    if (start_parser(parser) != OK) {
        return ERROR;
    }

    /* clear the old configuration */
    clear_configuration();

    add_window_associations(parser->associations, parser->associations_length);
    ZERO(parser->associations, parser->associations_length);
    parser->associations_length = 0;

    /* do the startup actions */
    startup.items = parser->startup_items;
    startup.number_of_items = parser->startup_items_length;
    startup.data = parser->startup_data;
    LOG_DEBUG("running startup actions: %A\n",
            &startup);
    run_action_list(&startup);

    return OK;
}
