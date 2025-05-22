#ifndef PARSE__DATA_TYPE_H
#define PARSE__DATA_TYPE_H

#include <stddef.h>
#include <inttypes.h>

#include "bits/binding.h"
#include "core/relation.h"
#include "parse/parse.h"
#include "utility/types.h"

#define DEFINE_ALL_PARSE_DATA_TYPES \
    /* an integer or integer constant */ \
    X(INTEGER, parse_integer_t integer, 'I') \
    /* a string of text, quoted or unquoted */ \
    X(STRING, utf8_t *string, 'S') \
    /* window relation */ \
    X(RELATION, struct window_relation relation, 'R') \
    /* button binding */ \
    X(BUTTON, struct button_binding button, 'B') \
    /* key binding */ \
    X(KEY, struct key_binding key, 'K')

/* integer type the parser should use */
typedef int_fast32_t parse_integer_t;
#define PRIiPARSE_INTEGER PRIiFAST32

/* If the integer is a percentage of something.  For example this might be 20%
 * of the width of a monitor.
 */
#define PARSE_DATA_FLAGS_IS_PERCENT (1 << 0)

/* all data types */
typedef enum parse_data_type {
#define X(identifier, type_name, short_name) \
    PARSE_DATA_TYPE_##identifier,
    DEFINE_ALL_PARSE_DATA_TYPES
#undef X
    PARSE_DATA_TYPE_MAX
} parse_data_type_t;

/* class data type */
struct parse_class {
    /* the instance name */
    char *instance;
    /* the class name */
    char *class;
};

/* meta information about a data type */
extern struct parse_data_meta_information {
    /* single letter identifier */
    char identifier;
    /* string representation the data type */
    const char *name;
} parse_data_meta_information[PARSE_DATA_TYPE_MAX];

/* generic action data */
struct parse_data {
    /* an OR combination of `PARSE_DATA_FLAGS_*` */
    unsigned flags;
    parse_data_type_t type;
    /* the literal value */
    union {
#define X(identifier, type_name, short_name) \
        type_name;
        DEFINE_ALL_PARSE_DATA_TYPES
#undef X
    } u;
};

/* Search the parse data meta table for given identifier.
 *
 * @return the data type with given identifier or PARSE_DATA_TYPE_MAX if none
 *         has that identifier.
 */
parse_data_type_t get_parse_data_type_from_identifier(char identifier);

/* Duplicate a data value into itself. */
void duplicate_parse_data(struct parse_data *data);

/* Clear the data value within @data. */
void clear_parse_data(const struct parse_data *data);

/* Resolve the string within parser as a data point.
 *
 * @return false if @identifier is invalid.
 *
 * data->type is set to PARSE_DATA_TYPE_MAX if the string within parser does not
 * match the data type.
 */
bool resolve_data(Parser *parser, char identifier,
        _Out struct parse_data *data);

#endif
