#ifndef PARSE__INTEGER_H
#define PARSE__INTEGER_H

/**
 * Parse an integer expression.
 *
 * INTEGER:
 * DIGIT...
 * #HEX_DIGIT...
 * BOOLEAN
 * MODIFIER
 *
 * INTEGER EXPRESSION:
 * INTEGER [+ INTEGER]...
 */

#include "parse/parse.h"
#include "utility/attributes.h"

/* Try to resolve the string within @parser as integer.
 *
 * @flags are special integer flags defined in `parse/data_type.h`.
 *
 * @return ERROR if @parser->string is not an integer.
 */
int resolve_integer(Parser *parser,
        _Out unsigned *flags,
        _Out action_integer_t *integer);

/* Continue parsing an integer expression.
 *
 * A string must have been read into @parser.
 *
 * @return ERROR if there is no integer.
 */
int continue_parsing_integer_expression(Parser *parser,
        _Out unsigned *flags,
        _Out action_integer_t *integer);

#endif
