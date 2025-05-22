#include "core/log.h"
#include "parse/action.h"
#include "parse/data_type.h"
#include "parse/integer.h"
#include "parse/utility.h"

/* meta information about a data type */
struct parse_data_meta_information parse_data_meta_information[] = {
#define X(identifier, type_name, short_name) \
    { short_name, STRINGIFY(identifier) },
    DEFINE_ALL_PARSE_DATA_TYPES
#undef X
};

/* Search the parse data meta table for given identifier. */
parse_data_type_t get_parse_data_type_from_identifier(char identifier)
{
    parse_data_type_t type = 0;

    for (; type < PARSE_DATA_TYPE_MAX; type++) {
        if (parse_data_meta_information[type].identifier == identifier) {
            break;
        }
    }

    return type;
}

/* Duplicate a data value into itself. */
void duplicate_parse_data(struct parse_data *data)
{
    switch (data->type) {
    case PARSE_DATA_TYPE_INTEGER:
        /* nothing */
        break;

    case PARSE_DATA_TYPE_STRING:
        data->u.string = xstrdup(data->u.string);
        break;

    case PARSE_DATA_TYPE_RELATION:
        duplicate_window_relation(&data->u.relation);
        break;

    case PARSE_DATA_TYPE_BUTTON:
        duplicate_action_list(&data->u.button.actions);
        break;

    case PARSE_DATA_TYPE_KEY:
        duplicate_action_list(&data->u.key.actions);
        break;

    case PARSE_DATA_TYPE_MAX:
        /* nothing */
        break;
    }
}

/* Clear the data value within @data. */
void clear_parse_data(const struct parse_data *data)
{
    switch (data->type) {
    case PARSE_DATA_TYPE_INTEGER:
        /* nothing */
        break;

    case PARSE_DATA_TYPE_STRING:
        free(data->u.string);
        break;

    case PARSE_DATA_TYPE_RELATION:
        clear_window_relation(&data->u.relation);
        break;

    case PARSE_DATA_TYPE_BUTTON:
        clear_action_list(&data->u.button.actions);
        break;

    case PARSE_DATA_TYPE_KEY:
        clear_action_list(&data->u.key.actions);
        break;

    case PARSE_DATA_TYPE_MAX:
        /* nothing */
        break;
    }
}

/* Resolve the string within parser as a data point. */
bool resolve_data(Parser *parser, char identifier,
        _Out struct parse_data *data)
{
    parse_data_type_t type = 0;

    for (; type < PARSE_DATA_TYPE_MAX; type++) {
        if (identifier == parse_data_meta_information[type].identifier) {
            break;
        }
    }

    if (type == PARSE_DATA_TYPE_MAX) {
        return false;
    }

    data->type = type;

    switch (type) {
    case PARSE_DATA_TYPE_INTEGER:
        if (continue_parsing_integer_expression(parser, &data->flags,
                    &data->u.integer) != OK) {
            data->type = PARSE_DATA_TYPE_MAX;
        }
        break;

    case PARSE_DATA_TYPE_STRING:
        LIST_COPY(parser->string, 0, parser->string_length + 1,
                data->u.string);
        break;

    case PARSE_DATA_TYPE_RELATION:
    case PARSE_DATA_TYPE_BUTTON:
    case PARSE_DATA_TYPE_KEY:
        /* not supported as action parameter */
        break;

    case PARSE_DATA_TYPE_MAX:
        /* nothing */
        break;
    }

    return true;
}
