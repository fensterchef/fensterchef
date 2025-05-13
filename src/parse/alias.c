#include "core/log.h"
#include "parse/alias.h"
#include "parse/input.h"
#include "parse/utility.h"
#include "utility/list.h"

/* Do not put this macro in brackets! */
#define PARSE_MAX_FILL_RATE 4/5

/* map of all set aliases */
struct parse_alias {
    /* the name of the alias */
    char *name;
    /* the value to resolve the alias to */
    char *value;
} alias_table[PARSE_MAX_ALIASES];

/* the number of elements in the alias table */
int alias_table_count;

/* Get the index where the alias with given name is supposed to be. */
static int get_alias_index(const char *name)
{
    const int c1 = 880, c2 = 8998, c3 = 999482, c4 = 1848481, c5 = 848488;
    int hash = 0;
    int probe = 0;
    int index;

    switch (strlen(name)) {
    case 0:
        return -1;

    default:
        hash ^= name[4] * c5;
        /* fall through */
    case 4:
        hash ^= name[3] * c4;
        /* fall through */
    case 3:
        hash ^= name[2] * c3;
        /* fall through */
    case 2:
        hash ^= name[1] * c2;
        /* fall through */
    case 1:
        hash = name[0] * c1;
        break;
    }

    do {
        index = hash + (probe * probe + probe) / 2;
        index %= PARSE_MAX_ALIASES;
        probe++;
    } while (alias_table[index].name != NULL &&
            strcmp(alias_table[index].name, name) != 0);

    return index;
}

/* Parse all after the `alias` keyword. */
void continue_parsing_alias(Parser *parser)
{
    struct parse_alias alias;
    int index;

    if (read_string_no_alias(parser) != OK) {
        emit_parse_error(parser, "expected alias name");
        skip_statement(parser);
        return;
    }

    skip_blanks(parser);
    if (peek_stream_character(parser) != '=') {
        parser->start_index = parser->index;
        emit_parse_error(parser, "expected '=' after alias name");
        skip_statement(parser);
        return;
    }

    /* skip over '=' */
    get_stream_character(parser);

    index = get_alias_index(parser->string);

    alias.name = xstrdup(parser->string);

    if (read_string(parser) != OK) {
        free(alias.name);
        emit_parse_error(parser, "expected alias value");
        skip_statement(parser);
        return;
    }

    if (alias_table[index].name == NULL &&
            alias_table_count + 1 > PARSE_MAX_ALIASES * PARSE_MAX_FILL_RATE) {
        free(alias.name);
        emit_parse_error(parser, "there is no more space for aliases");
        return;
    }

    alias.value = xstrdup(parser->string);

    if (alias_table[index].name != NULL) {
        LOG_DEBUG("overwriting alias %s = %s\n",
                alias.name, alias_table[index].value);
    } else {
        LOG_DEBUG("creating alias %s = %s\n",
                alias.name, alias.value);
    }

    free(alias_table[index].name);
    free(alias_table[index].value);

    alias_table[index] = alias;
}

/* Parse all after the `unalias` keyword. */
void continue_parsing_unalias(Parser *parser)
{
    if (read_string_no_alias(parser) != OK) {
        emit_parse_error(parser, "expected alias name");
        skip_statement(parser);
    } else {
        const int index = get_alias_index(parser->string);
        free(alias_table[index].name);
        free(alias_table[index].value);
        alias_table[index].name = NULL;
        alias_table[index].value = NULL;
    }
}

/* Check if given string is within the alias table. */
const char *resolve_alias(const char *string)
{
    const int index = get_alias_index(string);
    return alias_table[index].value;
}
