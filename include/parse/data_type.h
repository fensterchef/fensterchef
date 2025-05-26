#ifndef PARSE__DATA_TYPE_H
#define PARSE__DATA_TYPE_H

#include <stddef.h>
#include <inttypes.h>

#include "action.h"
#include "parse/parse.h"

/* Resolve the string within parser as a data point.
 *
 * @return false if @identifier is invalid.
 *
 * data->type is set to PARSE_DATA_TYPE_MAX if the string within parser does not
 * match the data type.
 */
bool resolve_data(Parser *parser, char identifier,
        _Out struct action_data *data);

#endif
