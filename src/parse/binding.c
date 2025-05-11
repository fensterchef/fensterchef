#include "core/x11_synchronize.h"
#include "parse/action.h"
#include "parse/binding.h"
#include "parse/input.h"
#include "parse/parse.h"
#include "parse/utility.h"

/* Parse binding modifiers. */
int continue_parsing_modifiers(Parser *parser,
        struct parse_binding *binding)
{
    int character;

    ZERO(binding, 1);

    if (strcmp(parser->string, "release") == 0) {
        if (read_string(parser) != OK) {
            emit_parse_error(parser,
                    "expected binding definition after 'release'");
            skip_all_statements(parser);
            return ERROR;
        }
        binding->is_release = true;
        binding->has_anything = true;
    }

    if (strcmp(parser->string, "transparent") == 0) {
        if (read_string(parser) != OK) {
            emit_parse_error(parser,
                    "expected binding definition after 'transparent'");
            skip_all_statements(parser);
            return ERROR;
        }
        binding->transparent_position = parser->index;
        binding->is_transparent = true;
        binding->has_anything = true;
    }

    /* read any modifiers */
    while (skip_blanks(parser),
            character = peek_stream_character(parser),
            character == '+') {
        unsigned flags;
        parse_integer_t integer;

        /* skip over '+' */
        (void) get_stream_character(parser);

        if (resolve_integer(parser, &flags, &integer) != OK) {
            emit_parse_error(parser, "invalid integer value");
        }
        binding->modifiers |= integer;
        if (read_string(parser) != OK) {
            emit_parse_error(parser,
                    "expected modifier, button or key symbol after '+'");
            skip_all_statements(parser);
            return ERROR;
        }
        binding->has_anything = true;
    }

    return OK;
}


/* Parse the following button or key symbol. */
int continue_parsing_button_or_key_symbol(Parser *parser,
        struct parse_binding *binding)
{
    unsigned flags;
    parse_integer_t integer;

    binding->button_index = resolve_button(parser);
    if (binding->button_index == BUTTON_NONE) {
        if (resolve_integer(parser, &flags, &integer) == OK) {
            int min_key_code, max_key_code;

            /* for testing code, the display is NULL */
            if (UNLIKELY(display != NULL)) {
                XDisplayKeycodes(display, &min_key_code, &max_key_code);
                binding->key_code = integer;
                if (binding->key_code < min_key_code ||
                        binding->key_code > max_key_code) {
                    emit_parse_error(parser, "key code is out of range");
                }
            }
        } else {
            binding->key_symbol = resolve_key_symbol(parser);
            if (binding->key_symbol == NoSymbol) {
                return ERROR;
            }
        }
    }
    return OK;
}

/* Parse the next binding definition in @parser. */
int finish_parsing_binding(Parser *parser, struct parse_binding *binding)
{
    struct parse_action_list list;

    if (read_string(parser) != OK) {
        emit_parse_error(parser, "expected action list");
        skip_all_statements(parser);
        return ERROR;
    }

    if (parser->is_string_quoted) {
        emit_parse_error(parser,
                "expected word and not a string for binding actions");
        skip_all_statements(parser);
        return ERROR;
    } else if (continue_parsing_action(parser, &list) != OK) {
        emit_parse_error(parser, "invalid action");
        skip_all_statements(parser);
        return ERROR;
    }

    if (binding->button_index != BUTTON_NONE) {
        struct button_binding button;
        struct action_list_item item;
        struct parse_generic_data data;

        button.is_release = binding->is_release;
        button.is_transparent = binding->is_transparent;
        button.modifiers = binding->modifiers;
        button.button = binding->button_index;
        make_real_action_list(&button.actions, &list);

        item.type = ACTION_BUTTON_BINDING;
        item.data_count = 1;
        LIST_APPEND_VALUE(parser->items, item);

        data.flags = 0;
        data.type = PARSE_DATA_TYPE_BUTTON;
        data.u.button = button;
        LIST_APPEND_VALUE(parser->data, data);
    } else {
        if (binding->is_transparent) {
            parser->index = binding->transparent_position;
            emit_parse_error(parser,
                    "key bindings do not support 'transparent'");
        } else {
            struct key_binding key;
            struct action_list_item item;
            struct parse_generic_data data;

            key.is_release = binding->is_release;
            key.modifiers = binding->modifiers;
            key.key_symbol = binding->key_symbol;
            key.key_code = binding->key_code;
            make_real_action_list(&key.actions, &list);

            item.type = ACTION_KEY_BINDING;
            item.data_count = 1;
            LIST_APPEND_VALUE(parser->items, item);

            data.flags = 0;
            data.type = PARSE_DATA_TYPE_KEY;
            data.u.key = key;
            LIST_APPEND_VALUE(parser->data, data);
        }
    }

    return OK;
}

/* Parse a full binding definition. */
void continue_parsing_binding(Parser *parser)
{
    struct parse_binding binding;

    if (continue_parsing_modifiers(parser, &binding) != OK) {
        return;
    }

    if (continue_parsing_button_or_key_symbol(parser, &binding) != OK) {
        if (binding.has_anything) {
            emit_parse_error(parser,
                    "invalid button, key symbol or key code");
            /* we can still continue parsing here */
        } else {
            emit_parse_error(parser, "invalid action, button or key");
            skip_all_statements(parser);
            return;
        }
    }

    finish_parsing_binding(parser, &binding);
}

/* Parse a full unbind statement.
 *
 * Expects that an `unbind` has already been read.
 */
void continue_parsing_unbind(Parser *parser)
{
    struct parse_binding binding;

    if (read_string(parser) != OK) {
        emit_parse_error(parser,
                "expected button, "
                "modifiers + key symbol or key code to unbind");
        skip_statement(parser);
        return;
    }

    if (continue_parsing_modifiers(parser, &binding) != OK) {
        return;
    }

    if (binding.is_transparent) {
        parser->index = binding.transparent_position;
        emit_parse_error(parser,
                "'transparent' can not be specified in unbind");
    }

    if (continue_parsing_button_or_key_symbol(parser, &binding) != OK) {
        if (binding.has_anything) {
            emit_parse_error(parser,
                    "invalid button, key symbol or key code");
            /* we can still continue parsing here */
        } else {
            emit_parse_error(parser,
                    "need button or key to unbind");
            skip_statement(parser);
            return;
        }
    }

    if (binding.button_index != BUTTON_NONE) {
        struct button_binding button;
        struct action_list_item item;
        struct parse_generic_data data;

        button.is_release = binding.is_release;
        button.is_transparent = binding.is_transparent;
        button.modifiers = binding.modifiers;
        button.button = binding.button_index;
        ZERO(&button.actions, 0);

        item.type = ACTION_CLEAR_BUTTON_BINDING;
        item.data_count = 1;
        LIST_APPEND_VALUE(parser->items, item);

        data.flags = 0;
        data.type = PARSE_DATA_TYPE_BUTTON;
        data.u.button = button;
        LIST_APPEND_VALUE(parser->data, data);
    } else {
        struct key_binding key;
        struct action_list_item item;
        struct parse_generic_data data;

        key.is_release = binding.is_release;
        key.modifiers = binding.modifiers;
        key.key_symbol = binding.key_symbol;
        key.key_code = binding.key_code;
        ZERO(&key.actions, 0);

        item.type = ACTION_CLEAR_KEY_BINDING;
        item.data_count = 1;
        LIST_APPEND_VALUE(parser->items, item);

        data.flags = 0;
        data.type = PARSE_DATA_TYPE_KEY;
        data.u.key = key;
        LIST_APPEND_VALUE(parser->data, data);
    }
}
