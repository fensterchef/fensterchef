/**
 * This is how a binding is parsed:
 *
 * release? transparent? MODIFIERS+BUTTON|KEY_SYMBOL ACTIONS
 * --------------------------------
 *    continue_parsing_modifiers   -----------------
 *                                          |        -------
 *                                          |           |
 *                          continue_pasing_button_or_key_symbol
 *                                                      |
 *                                              finish_parsing_binding
 */

#include <ctype.h>

#include "parse/action.h"
#include "parse/data_type.h"
#include "parse/binding.h"
#include "parse/input.h"
#include "parse/integer.h"
#include "parse/parse.h"
#include "parse/top.h"
#include "parse/utility.h"
#include "x11/display.h"

/* parse information about a binding */
struct parse_binding {
    /* if there were were any modifiers */
    bool has_modifiers;
    /* the position of the transparent keyword if any */
    size_t transparent_position;
    /* if `release` was specified */
    bool is_release;
    /* if `transparent` was specified */
    bool is_transparent;
    /* the specified X key/button modifiers */
    unsigned modifiers;
    /* the X button index */
    button_t button_index;
    /* the key symbol */
    KeySym key_symbol;
    /* the key code */
    KeyCode key_code;
};

/* Try to resolve the string within parser to a modifier constant. */
int resolve_modifier(Parser *parser, unsigned *modifier)
{
    /* string to modifier translation */
    static const struct {
        const char *string;
        unsigned modifier;
    } string_to_modifier[] = {
        { "None", 0 },
        { "Shift", ShiftMask },
        { "Lock", LockMask },
        { "Control", ControlMask },
        { "Mod1", Mod1Mask },
        { "Mod2", Mod2Mask },
        { "Mod3", Mod3Mask },
        { "Mod4", Mod4Mask },
        { "Mod5", Mod5Mask },
    };

    unsigned index;

    if (parser->string_length < 4) {
        return ERROR;
    }

    /* the fourth character is unique among all constants */
    switch (parser->string[3]) {
    case 'e': index = 0; break;
    case 'f': index = 1; break;
    case 'k': index = 2; break;
    case 't': index = 3; break;
    case '1': index = 4; break;
    case '2': index = 5; break;
    case '3': index = 6; break;
    case '4': index = 7; break;
    case '5': index = 8; break;
    default:
        return ERROR;
    }

    if (strcmp(string_to_modifier[index].string, parser->string) == 0) {
        *modifier = string_to_modifier[index].modifier;
        return OK;
    } else {
        return ERROR;
    }
}

/* Translate the string within @parser to a button index.
 *
 * @return BUTTON_NONE when the string is not a button constant.
 */
static int resolve_button(Parser *parser)
{
    /* conversion from string to button index */
    static const struct button_string {
        /* the string representation of the button */
        const char *name;
        /* the button index */
        button_t button_index;
    } button_strings[] = {
        /* buttons can also be Button<integer> to directly address the index */
        { "LButton", BUTTON_LEFT },
        { "LeftButton", BUTTON_LEFT },

        { "MButton", BUTTON_MIDDLE },
        { "MiddleButton", BUTTON_MIDDLE },

        { "RButton", BUTTON_RIGHT },
        { "RightButton", BUTTON_RIGHT },

        { "ScrollUp", BUTTON_WHEEL_UP },
        { "WheelUp", BUTTON_WHEEL_UP },

        { "ScrollDown", BUTTON_WHEEL_DOWN },
        { "WheelDown", BUTTON_WHEEL_DOWN },

        { "ScrollLeft", BUTTON_WHEEL_LEFT },
        { "WheelLeft", BUTTON_WHEEL_LEFT },

        { "ScrollRight", BUTTON_WHEEL_RIGHT },
        { "WheelRight", BUTTON_WHEEL_RIGHT },
    };

    int index = 0;
    const char *string;

    string = parser->string;
    /* parse strings starting with "X" */
    if (string[0] == 'X') {
        int x_index = 0;

        string++;
        while (isdigit(string[0])) {
            x_index *= 10;
            x_index += string[0] - '0';
            if (x_index >= BUTTON_MAX - BUTTON_X1) {
                emit_parse_error(parser, "X button value is too high");
                x_index = 1 - BUTTON_X1;
                break;
            }
            string++;
        }

        index = BUTTON_X1 + x_index - 1;
    /* parse strings starting with "Button" */
    } else if (strncmp(string, "Button", strlen("Button")) == 0) {
        string += strlen("Button");
        while (isdigit(string[0])) {
            index *= 10;
            index += string[0] - '0';
            if (index > UINT8_MAX) {
                index = BUTTON_NONE;
                emit_parse_error(parser, "button value is too high");
                break;
            }
            string++;
        }
    } else {
        for (unsigned i = 0; i < SIZE(button_strings); i++) {
            if (strcmp(string, button_strings[i].name) == 0) {
                return button_strings[i].button_index;
            }
        }
    }

    if (string[0] != '\0') {
        index = BUTTON_NONE;
    }

    return index;
}

/* Parse binding modifiers. */
static int continue_parsing_modifiers(Parser *parser,
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
        binding->has_modifiers = true;
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
        binding->has_modifiers = true;
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
        binding->has_modifiers = true;
    }

    return OK;
}


/* Parse the following button or key symbol. */
static int continue_parsing_button_or_key_symbol(Parser *parser,
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
            binding->key_symbol = XStringToKeysym(parser->string);
            if (binding->key_symbol == NoSymbol) {
                return ERROR;
            }
        }
    }
    return OK;
}

/* Parse the next binding definition in @parser. */
static int finish_parsing_binding(Parser *parser, struct parse_binding *binding,
        struct parse_action_list *list)
{
    struct parse_action_list sub_list;

    ZERO(&sub_list, 1);
    if (parse_top(parser, &sub_list) != OK) {
        clear_parse_list(&sub_list);
        return ERROR;
    }

    clear_parse_list_data(&sub_list);

    if (binding->button_index != BUTTON_NONE) {
        struct button_binding button;
        struct action_list_item item;
        struct parse_data data;

        button.is_release = binding->is_release;
        button.is_transparent = binding->is_transparent;
        button.modifiers = binding->modifiers;
        button.button = binding->button_index;
        make_real_action_list(&button.actions, &sub_list);

        item.type = ACTION_BUTTON_BINDING;
        item.data_count = 1;
        LIST_APPEND_VALUE(list->items, item);

        data.flags = 0;
        data.type = PARSE_DATA_TYPE_BUTTON;
        data.u.button = button;
        LIST_APPEND_VALUE(list->data, data);
    } else {
        struct key_binding key;
        struct action_list_item item;
        struct parse_data data;

        if (binding->is_transparent) {
            parser->start_index = binding->transparent_position;
            emit_parse_error(parser,
                    "key bindings do not support 'transparent'");
        }

        key.is_release = binding->is_release;
        key.modifiers = binding->modifiers;
        key.key_symbol = binding->key_symbol;
        key.key_code = binding->key_code;
        make_real_action_list(&key.actions, &sub_list);

        item.type = ACTION_KEY_BINDING;
        item.data_count = 1;
        LIST_APPEND_VALUE(list->items, item);

        data.flags = 0;
        data.type = PARSE_DATA_TYPE_KEY;
        data.u.key = key;
        LIST_APPEND_VALUE(list->data, data);
    }

    return OK;
}

/* Parse a full binding definition. */
void continue_parsing_binding(Parser *parser, struct parse_action_list *list)
{
    struct parse_binding binding;

    if (continue_parsing_modifiers(parser, &binding) != OK) {
        return;
    }

    if (continue_parsing_button_or_key_symbol(parser, &binding) != OK) {
        if (binding.has_modifiers) {
            emit_parse_error(parser,
                    "invalid button, key symbol or key code");
            /* we can still continue parsing here */
        } else {
            emit_parse_error(parser, "invalid action, button or key");
            skip_all_statements(parser);
            return;
        }
    }

    finish_parsing_binding(parser, &binding, list);
}

/* Parse a full unbind statement.
 *
 * Expects that an `unbind` has already been read.
 */
void continue_parsing_unbind(Parser *parser, struct parse_action_list *list)
{
    struct parse_binding binding;
    struct action_list_item item;
    struct parse_data data;

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
        if (binding.has_modifiers) {
            emit_parse_error(parser,
                    "invalid button, key symbol or key code");
        } else {
            emit_parse_error(parser, "invalid identifier");
        }
    } else if (binding.button_index != BUTTON_NONE) {
        struct button_binding button;

        button.is_release = binding.is_release;
        button.is_transparent = binding.is_transparent;
        button.modifiers = binding.modifiers;
        button.button = binding.button_index;
        ZERO(&button.actions, 0);

        item.type = ACTION_BUTTON_BINDING;
        item.data_count = 1;
        LIST_APPEND_VALUE(list->items, item);

        data.flags = 0;
        data.type = PARSE_DATA_TYPE_BUTTON;
        data.u.button = button;
        LIST_APPEND_VALUE(list->data, data);
    } else {
        struct key_binding key;

        key.is_release = binding.is_release;
        key.modifiers = binding.modifiers;
        key.key_symbol = binding.key_symbol;
        key.key_code = binding.key_code;
        ZERO(&key.actions, 0);

        item.type = ACTION_KEY_BINDING;
        item.data_count = 1;
        LIST_APPEND_VALUE(list->items, item);

        data.flags = 0;
        data.type = PARSE_DATA_TYPE_KEY;
        data.u.key = key;
        LIST_APPEND_VALUE(list->data, data);
    }
}
