#include "parse/action.h"
#include "parse/data_type.h"

/* Duplicate a data value into itself. */
void duplicate_generic_data(struct parse_generic_data *data)
{
    switch (data->type) {
    case PARSE_DATA_TYPE_INTEGER:
        /* nothing */
        break;

    case PARSE_DATA_TYPE_STRING:
        data->u.string = xstrdup(data->u.string);
        break;

    case PARSE_DATA_TYPE_BUTTON:
        duplicate_action_list(&data->u.button.actions);
        break;

    case PARSE_DATA_TYPE_KEY:
        duplicate_action_list(&data->u.key.actions);
        break;
    }
}

/* Clear the data value within @data. */
void clear_generic_data(const struct parse_generic_data *data)
{
    switch (data->type) {
    case PARSE_DATA_TYPE_INTEGER:
        /* nothing */
        break;

    case PARSE_DATA_TYPE_STRING:
        free(data->u.string);
        break;

    case PARSE_DATA_TYPE_BUTTON:
        clear_action_list(&data->u.button.actions);
        break;

    case PARSE_DATA_TYPE_KEY:
        clear_action_list(&data->u.key.actions);
        break;
    }
}
