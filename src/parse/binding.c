#include "core/x11_synchronize.h"
#include "parse/action.h"
#include "parse/input.h"
#include "parse/parse.h"
#include "parse/utility.h"

/* Parse the next binding definition in @parser. */
void continue_parsing_modifiers_or_binding(Parser *parser)
{
    bool has_anything = false;
    bool is_release = false;
    bool is_transparent = false;
    int character;
    unsigned modifiers = 0;
    button_t button_index;
    unsigned flags;
    parse_integer_t integer;
    KeySym key_symbol = NoSymbol;
    KeyCode key_code = 0;
    /* position for error reporting */
    size_t transparent_position = 0;
    struct parse_action_list list;

    if (strcmp(parser->string, "release") == 0) {
        if (read_string(parser) != OK) {
            emit_parse_error(parser,
                    "expected binding definition after 'release'");
            skip_all_statements(parser);
            return;
        }
        is_release = true;
        has_anything = true;
    }

    if (strcmp(parser->string, "transparent") == 0) {
        if (read_string(parser) != OK) {
            emit_parse_error(parser,
                    "expected binding definition after 'transparent'");
            skip_all_statements(parser);
            return;
        }
        transparent_position = parser->index;
        is_transparent = true;
        has_anything = true;
    }

    /* read any modifiers */
    while (skip_blanks(parser),
            character = peek_stream_character(parser),
            character == '+') {
        /* skip over '+' */
        (void) get_stream_character(parser);

        if (resolve_integer(parser, &flags, &integer) != OK) {
            emit_parse_error(parser, "invalid integer value");
        }
        modifiers |= integer;
        if (read_string(parser) != OK) {
            emit_parse_error(parser,
                    "expected modifier, button or key symbol after '+'");
            skip_all_statements(parser);
            return;
        }
        has_anything = true;
    }

    button_index = resolve_button(parser);
    if (button_index == BUTTON_NONE) {
        if (resolve_integer(parser, &flags, &integer) == OK) {
            int min_key_code, max_key_code;

            /* for testing code, the display is NULL */
            if (UNLIKELY(display != NULL)) {
                XDisplayKeycodes(display, &min_key_code, &max_key_code);
                key_code = integer;
                if (key_code < min_key_code || key_code > max_key_code) {
                    emit_parse_error(parser, "key code is out of range");
                }
            }
        } else {
            key_symbol = resolve_key_symbol(parser);
            if (key_symbol == NoSymbol) {
                if (has_anything) {
                    emit_parse_error(parser,
                            "invalid button, key symbol or key code");
                } else {
                    emit_parse_error(parser, "invalid action, button or key");
                    skip_all_statements(parser);
                    return;
                }
            }
        }
    }

    if (read_string(parser) != OK) {
        emit_parse_error(parser, "expected action list");
        skip_all_statements(parser);
        return;
    }

    if (parser->is_string_quoted) {
        emit_parse_error(parser,
                "expected word and not a string for binding actions");
        skip_all_statements(parser);
        return;
    } else if (continue_parsing_action(parser, &list) != OK) {
        emit_parse_error(parser, "invalid action");
        skip_all_statements(parser);
        return;
    }

    if (button_index != BUTTON_NONE) {
        struct button_binding button;
        struct action_list_item item;
        struct parse_generic_data data;

        button.is_release = is_release;
        button.is_transparent = is_transparent;
        button.modifiers = modifiers;
        button.button = button_index;
        make_real_action_list(&button.actions, &list);

        item.type = ACTION_BUTTON_BINDING;
        item.data_count = 1;
        LIST_APPEND_VALUE(parser->items, item);

        data.flags = 0;
        data.type = PARSE_DATA_TYPE_BUTTON;
        data.u.button = button;
        LIST_APPEND_VALUE(parser->data, data);
    } else {
        if (is_transparent) {
            parser->index = transparent_position;
            emit_parse_error(parser,
                    "key bindings do not support 'transparent'");
        } else {
            struct key_binding key;
            struct action_list_item item;
            struct parse_generic_data data;

            key.is_release = is_release;
            key.modifiers = modifiers;
            key.key_symbol = key_symbol;
            key.key_code = key_code;
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
}
