#ifndef CONFIGURATION_PARSER_DATA_TYPE_H
#define CONFIGURATION_PARSER_DATA_TYPE_H

#include <stdbool.h>
#include <stdint.h>

#include "cursor.h"

/* This had to be moved into bits/ because both action.h and
 * configuration_parser.h need it and there were unresolvable intersections
 * between the header files.
 */

/* data types the parser understands
 *
 * NOTE: After editing a data type, also edit the data_types[] array in
 * configuration_parser.c and implement its parser function. It will then be
 * automatically used in parse_line().
 */
typedef enum parser_data_type {
    /* no data type at all */
    PARSER_DATA_TYPE_VOID,
    /* true or false, in text one of: on yes true off no false */
    PARSER_DATA_TYPE_BOOLEAN,
    /* any text without leading or trailing space */
    PARSER_DATA_TYPE_STRING,
    /* an integer in simple decimal notation */
    PARSER_DATA_TYPE_INTEGER,
    /* a set of 4 integers */
    PARSER_DATA_TYPE_QUAD,
    /* color in the format (X: hexadecimal digit): #XXXXXX */
    PARSER_DATA_TYPE_COLOR,
    /* key modifiers, e.g.: Control+Shift */
    PARSER_DATA_TYPE_MODIFIERS,
    /* Xcursor constant, e.g. left-ptr */
    PARSER_DATA_TYPE_CURSOR,
} parser_data_type_t;

/* the value of a data type */
union parser_data_value {
    /* true or false, in text one of: on yes true off no false */
    bool boolean;
    /* any utf8 text without leading or trailing space */
    uint8_t *string;
    /* an integer in simple decimal notation */
    int32_t integer;
    /* a set of 4 integers */
    int32_t quad[4];
    /* color in the format (X: hexadecimal digit): #XXXXXX */
    uint32_t color;
    /* key modifiers, e.g.: Control+Shift */
    uint16_t modifiers;
    /* cursor constant, e.g. left-ptr */
    core_cursor_t cursor;
};

/*** Implemented in configuration_parser.c ***/

/* Duplicates given @value deeply into itself. */
void duplicate_data_value(parser_data_type_t type,
        union parser_data_value *value);

/* Frees the resources the given data value occupies. */
void clear_data_value(parser_data_type_t type, union parser_data_value *value);

#endif
