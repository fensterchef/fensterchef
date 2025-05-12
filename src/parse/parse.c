#include <stdarg.h>
#include <string.h>

#include "core/log.h"
#include "core/window.h"
#include "parse/action.h"
#include "parse/input.h"
#include "parse/parse.h"
#include "parse/top.h"

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
        free(parser->file_path);
        free(parser->string);
        free(parser);
    }
}

/* Test the parser functionality. */
int test_parser(Parser *parser)
{
    struct parse_action_list list;
    struct action_list actions;

    ZERO(&list, 1);
    while (parse_top(parser, &list) == OK) {
        /* nothing */
    }

    if (parser->error_count == 0) {
        make_real_action_list(&actions, &list);
        LOG_DEBUG("got actions: %A\n",
                &actions);
    }

    clear_parse_list(&list);

    return parser->error_count > 0 ? ERROR : OK;
}

/* Parse the currently active stream and run all actions. */
int parse_and_run_actions(Parser *parser)
{
    struct parse_action_list list;
    struct action_list actions;

    ZERO(&list, 1);
    while (parse_top(parser, &list) == OK) {
        /* nothing */
    }

    if (parser->error_count > 0) {
        /* clear all parsed thus far */
        clear_parse_list(&list);
        return ERROR;
    }

    add_window_associations(list.associations, list.associations_length);
    list.associations_length = 0;

    /* do the startup actions */
    make_real_action_list(&actions, &list);
    LOG_DEBUG("running actions: %A\n",
            &actions);
    run_action_list(&actions);

    clear_parse_list(&list);

    return OK;
}
