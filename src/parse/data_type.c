#include "parse/data_type.h"
#include "parse/integer.h"

/* Resolve the string within parser as a data point. */
bool resolve_data(Parser *parser, char identifier,
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
