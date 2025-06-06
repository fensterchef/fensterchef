#ifndef PARSE__PARSE_H
#define PARSE__PARSE_H

#include "utility/attributes.h"
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
    /* the number of errors the parser had */
    unsigned error_count;
    /* the first line an error occured on */
    unsigned first_error_line;
    /* the specific file the error occured in */
    char *first_error_file;

    /* the path of the file, this is `NULL` if the source is a string */
    utf8_t *file_path;

    /* last read string (word or quoted string) */
    LIST(utf8_t, string);
    /* if this string has quotes */
    bool is_string_quoted;

    /* the current index within the input stream */
    size_t index;
    /* the length of the the input stream */
    size_t length;
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

/* Test the parser functionality.
 *
 * This function prints the kind of error to `stderr`.
 *
 * @return ERROR if a parsing error occured, OK otherwise.
 */
int test_parser(Parser *parser);

/* Parse using given parser object and rull all actions within it.
 *
 * @return ERROR if a parsing error occured, OK otherwise.
 */
int parse_and_run_actions(Parser *parser);

#endif
