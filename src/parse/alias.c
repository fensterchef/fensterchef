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
    unsigned hash = 1731;
    unsigned probe = 0;
    unsigned index;

    for (char *i = name; i[0] != '\0'; i++) {
        hash = hash * 407 + (unsigned char) i[0];

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

    if (parser->is_string_quoted) {
        emit_parse_error(parser, "alias name can not be quoted");
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
        LOG("overwriting alias %s = %s\n",
                alias.name, alias_table[index].value);
        free(alias_table[index].name);
        free(alias_table[index].value);
    } else {
        LOG("creating alias %s = %s\n",
                alias.name, alias.value);
        alias_table_count++;
    }

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
        if (alias_table[index].name != NULL) {
            free(alias_table[index].name);
            free(alias_table[index].value);
            alias_table[index].name = NULL;
            alias_table_count--;
        }
    }
}

/* Check if given string is within the alias table. */
const char *resolve_alias(const char *string)
{
    const int index = get_alias_index(string);
    if (alias_table[index].name == NULL) {
        return NULL;
    } else {
        return alias_table[index].value;
    }
}

/* Clear all aliases the parser set. */
void clear_all_aliases(void)
{
    for (unsigned i = 0; i < PARSE_MAX_ALIASES; i++) {
        if (alias_table_count == 0) {
            break;
        }

        if (alias_table[i].name != NULL) {
            free(alias_table[i].name);
            free(alias_table[i].value);
            alias_table[i].name = NULL;
            alias_table_count--;
        }
    }
}
