#include <ctype.h>
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
void emit_parse_error(Parser *parser, const char *message)
{
    unsigned line, column;
    const utf8_t *file;
    const char *string_line;
    unsigned length;

    parser->error_count++;
    get_stream_position(parser, parser->index, &line, &column);
    string_line = get_stream_line(parser, line, &length);

    file = parser->file_path;
    if (file == NULL) {
        file = "<string>";
    }

    LOG_ERROR("%s:%d:%d: %s\n",
            file, line + 1, column + 1, message);
    fprintf(stderr, "%.*s\n", (int) length, string_line);
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

        free(parser->associations);

        free(parser);
    }
}

/* Parse the currently active stream. */
int start_parser(Parser *parser)
{
    int character;

    /* clear an old parser */
    parser->error_count = 0;
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
