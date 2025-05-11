#ifndef PARSE__PARSE_H
#define PARSE__PARSE_H

#include <setjmp.h>

#include "core/action.h"
#include "parse/data_type.h"
#include "utility/list.h"
#include "utility/types.h"

/* maximum value for a literal integer */
#define PARSE_INTEGER_LIMIT 1000000

/* the assumed width of a tab character \t */
#define PARSE_TAB_SIZE 8

/* The maximum number of errors that can occur before the parser stops.
 * Outputting many errors is good as it helps with fixing files.  But when the
 * user sources an invalid file, the user should not be be flooded with errors.
 * We stop prematurely because of that.
 */
#define PARSE_MAX_ERROR_COUNT 30

/* the parser object */
typedef struct parser {
    /* the upper parser in the parsing process */
    struct parser *upper_parser;

    /* the start index of the last item */
    size_t start_index;
    /* if the parser had any error */
    int error_count;
    /* the point to jump to */
    jmp_buf throw_jump;

    /* the path of the file, this is `NULL` if the source is a string */
    utf8_t *file_path;
    /* the current index within the string */
    size_t index;
    /* the length of the the string */
    size_t length;

    /* last read string */
    LIST(utf8_t, string);
    /* if this string has quotes */
    bool is_string_quoted;

    /* the current action parsing information */
    struct parse_action_information {
        /* Data for this action. */
        LIST(struct parse_generic_data, data);
        /* The offset within the string identifiers of the actions.
         * If this is -1, then the action was disregarded.
         */
        int offset;
    } actions[ACTION_SIMPLE_MAX];
    /* the first/last (exclusive) action still valid within @offsets */
    action_type_t first_action, last_action;

    /* last read data */
    struct parse_generic_data data;

    /* the instance pattern of an association being parsed */
    LIST(utf8_t, instance_pattern);
    /* the class pattern of an association being parsed */
    LIST(utf8_t, class_pattern);

    /* the action items being parsed */
    LIST(struct action_list_item, action_items);
    /* data for the action items being parsed */
    LIST(struct parse_generic_data, action_data);

    /* the startup items being parsed */
    LIST(struct action_list_item, startup_items);
    /* data for the startup action list */
    LIST(struct parse_generic_data, startup_data);

    /* list of associations */
    LIST(struct window_association, associations);

    /* the text input stream */
    utf8_t input[];
} Parser;

/* Emit a parse error. */
void emit_parse_error(Parser *parser, const char *format, ...);

/* Initialize the internal parse object to parse file at given path.
 *
 * @return NULL if the file could not be opened, otherwise the parse object.
 */
Parser *create_file_parser(const utf8_t *path);

/* Initialize the internal parse object to parse a given string.
 *
 * @return the allocated parse object.
 */
Parser *create_string_parser(const utf8_t *string);

/* Destroy a previously allocated input parse object. */
void destroy_parser(_Nullable Parser *parser);

/* Start parsing the input within the parser object.
 *
 * The stream is activated using `create_file_stream()` or
 * `create_string_stream()`.
 *
 * This function prints the kind of error to `stderr`.
 *
 * @return ERROR if a parsing error occured, OK otherwise.
 */
int start_parser(Parser *parser);

/* Parse using given parser object and use it to override the configuration.
 *
 * All parsed actions, bindings etc. are put into the configuration if this
 * function succeeds.
 *
 * @return ERROR if a parsing error occured, OK otherwise.
 */
int parse_and_replace_configuration(Parser *parser);

/* Parse using given parser object and rull all actions within it.
 *
 * @return ERROR if a parsing error occured, OK otherwise.
 */
int parse_and_run_actions(Parser *parser);

#endif
