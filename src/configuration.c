#include <errno.h>
#include <unistd.h>

#include "binding.h"
#include "configuration.h"
#include "cursor.h"
#include "fensterchef.h"
#include "font.h"
#include "log.h"
#include "parse/alias.h"
#include "parse/data_type.h"
#include "parse/group.h"
#include "parse/parse.h"
#include "parse/input.h"
#include "window.h"

/* the settings of the default configuration */
const struct configuration default_configuration = {
    .resize_tolerance = 8,

    .first_window_number = 1,

    .overlap = 80,

    .auto_split = false,
    .auto_equalize = true,
    .auto_fill_void = true,
    .auto_remove = false,
    .auto_remove_void = false,

    .notification_duration = 3,

    .text_padding = 6,

    .border_size = 3,
    .border_color = 0xff49494d,
    .border_color_active = 0xff939388,
    .border_color_focus = 0xff7fd0f1,
    .foreground = 0xff7fd0f1,
    .background = 0xff49494d,

    .gaps_inner = { 2, 2, 2, 2 },
    .gaps_outer = { 0, 0, 0, 0 },
};

/* the currently loaded configuration settings */
struct configuration configuration = default_configuration;

/* default mouse bindings */
static const struct default_button_binding {
    /* the binding flags */
    bool is_release;
    /* the modifiers of the button */
    unsigned modifiers;
    /* the button to press */
    int button_index;
    /* the singular action to execute */
    action_type_t type;
} default_button_bindings[] = {
    /* start moving or resizing a window (depends on the mouse position) */
    { false, 0, 1, ACTION_INITIATE_RESIZE },
    /* minimize (hide) a window */
    { true, 0, 2, ACTION_MINIMIZE_WINDOW },
    /* start moving a window */
    { false, 0, 3, ACTION_INITIATE_MOVE },
};

/* default key bindings */
static const struct default_key_binding {
    /* the modifiers of the key */
    unsigned modifiers;
    /* the key symbol */
    KeySym key_symbol;
    /* the type of the action */
    action_type_t action;
    /* optional additional action data */
    const utf8_t *string;
} default_key_bindings[] = {
    /* reload the configuration */
    { ShiftMask, XK_r, .action = ACTION_RELOAD_CONFIGURATION },

    /* move the focus to a child or parent frame */
    { 0, XK_a, .action = ACTION_FOCUS_PARENT },
    { 0, XK_b, .action = ACTION_FOCUS_CHILD },
    { ShiftMask, XK_a, .action = ACTION_FOCUS_ROOT },

    /* make the size of frames equal */
    { 0, XK_equal, .action = ACTION_EQUALIZE },

    /* close the active window */
    { 0, XK_q, .action = ACTION_CLOSE_WINDOW },

    /* minimize the active window */
    { 0, XK_minus, .action = ACTION_MINIMIZE_WINDOW },

    /* go to the next window in the tiling */
    { 0, XK_n, .action = ACTION_SHOW_NEXT_WINDOW },
    { 0, XK_p, .action = ACTION_SHOW_PREVIOUS_WINDOW },

    /* remove the current tiling frame */
    { 0, XK_r, .action = ACTION_REMOVE },

    /* put the stashed frame into the current one */
    { 0, XK_o, .action = ACTION_POP_STASH },

    /* toggle between tiling and the previous mode */
    { ShiftMask, XK_space, .action = ACTION_TOGGLE_TILING },

    /* toggle between fullscreen and the previous mode */
    { 0, XK_f, .action = ACTION_TOGGLE_FULLSCREEN },

    /* focus from tiling to non tiling and vise versa */
    { 0, XK_space, .action = ACTION_TOGGLE_FOCUS },

    /* split a frame */
    { 0, XK_v, .action = ACTION_SPLIT_HORIZONTALLY },
    { 0, XK_s, .action = ACTION_SPLIT_VERTICALLY },

    /* move between frames */
    { 0, XK_k, .action = ACTION_FOCUS_UP },
    { 0, XK_h, .action = ACTION_FOCUS_LEFT },
    { 0, XK_l, .action = ACTION_FOCUS_RIGHT },
    { 0, XK_j, .action = ACTION_FOCUS_DOWN },
    { 0, XK_Up, .action = ACTION_FOCUS_UP },
    { 0, XK_Left, .action = ACTION_FOCUS_LEFT },
    { 0, XK_Right, .action = ACTION_FOCUS_RIGHT },
    { 0, XK_Down, .action = ACTION_FOCUS_DOWN },

    /* exchange frames */
    { ShiftMask, XK_k, .action = ACTION_EXCHANGE_UP },
    { ShiftMask, XK_h, .action = ACTION_EXCHANGE_LEFT },
    { ShiftMask, XK_l, .action = ACTION_EXCHANGE_RIGHT },
    { ShiftMask, XK_j, .action = ACTION_EXCHANGE_DOWN },
    { ShiftMask, XK_Up, .action = ACTION_EXCHANGE_UP },
    { ShiftMask, XK_Left, .action = ACTION_EXCHANGE_LEFT },
    { ShiftMask, XK_Right, .action = ACTION_EXCHANGE_RIGHT },
    { ShiftMask, XK_Down, .action = ACTION_EXCHANGE_DOWN },

    /* show the interactive window list */
    { 0, XK_w, .action = ACTION_SHOW_LIST },

    /* run the terminal or xterm as fall back */
    { 0, XK_Return, ACTION_RUN,
            "[ -n \"$TERMINAL\" ] && exec \"$TERMINAL\" || exec xterm"
    },

    /* quit fensterchef */
    { ControlMask | ShiftMask, XK_e, .action = ACTION_QUIT }
};

/* Puts the button bindings of the default configuration into the current
 * configuration.
 */
static void set_default_button_bindings(void)
{
    struct action_list_item item;
    struct button_binding binding;

    binding.actions.items = &item;
    item.data_count = 0;
    binding.actions.number_of_items = 1;
    binding.actions.data = NULL;

    /* overwrite bindings with the default button bindings */
    for (unsigned i = 0; i < SIZE(default_button_bindings); i++) {
        /* bake a button binding from the default button bindings struct */
        binding.is_transparent = false;
        binding.is_release = default_button_bindings[i].is_release;
        binding.modifiers = Mod4Mask | default_button_bindings[i].modifiers;
        binding.button = default_button_bindings[i].button_index;
        item.type = default_button_bindings[i].type;

        set_button_binding(&binding);
    }
}

/* Puts the key bindings of the default configuration into the current
 * configuration.
 */
static void set_default_key_bindings(void)
{
    struct action_list_item item;
    struct parse_data data;
    struct key_binding binding;

    binding.is_release = false;
    binding.key_code = 0;
    binding.actions.items = &item;
    item.data_count = 0;
    binding.actions.number_of_items = 1;
    binding.actions.data = &data;
    data.flags = 0;

    /* overwrite bindings with the default button bindings */
    for (unsigned i = 0; i < SIZE(default_key_bindings); i++) {
        /* bake a key binding from the bindings array */
        binding.modifiers = Mod4Mask | default_key_bindings[i].modifiers;
        binding.key_symbol = default_key_bindings[i].key_symbol;
        item.type = default_key_bindings[i].action;
        if (default_key_bindings[i].string != NULL) {
            item.data_count = 1;
            data.type = PARSE_DATA_TYPE_STRING;
            data.u.string = (utf8_t*) default_key_bindings[i].string;
        } else {
            item.data_count = 0;
        }

        set_key_binding(&binding);
    }
}

/* Set the current configuration to the default configuration. */
void set_default_configuration(void)
{
    clear_configuration();

    configuration = default_configuration;

    set_ignored_modifiers(DEFAULT_IGNORE_MODIFIERS);
    set_default_button_bindings();
    set_default_key_bindings();

    set_font(DEFAULT_FONT);
}

/* Expand given @path.
 *
 * @path becomes invalid after this call, use the return value for the new
 *       value.
 */
static char *expand_path(char *path)
{
    char *expanded;

    if (path[0] == '~' && path[1] == '/') {
        expanded = xasprintf("%s%s",
                Fensterchef_home, &path[1]);
        free(path);
    } else {
        expanded = path;
    }
    return expanded;
}

/* Expand the path and check if it is readable. */
static bool is_readable(char *path)
{
    if (access(path, R_OK) != 0) {
        if (errno != ENOENT) {
            LOG_ERROR("could not open %s: %s\n",
                    path, strerror(errno));
        }
        return false;
    }
    return true;
}

/* Get the configuration file for fensterchef. */
const char *get_configuration_file(void)
{
    static char *cached_path;

    const char *xdg_config_home, *xdg_config_dirs;
    char *path;
    const char *colon, *next;
    size_t length;

    if (Fensterchef_configuration != NULL) {
        return Fensterchef_configuration;
    }

    xdg_config_home = getenv("XDG_CONFIG_HOME");
    if (xdg_config_home == NULL) {
        xdg_config_home = "~/.config";
    }

    path = xasprintf("%s/" FENSTERCHEF_CONFIGURATION,
            xdg_config_home);
    path = expand_path(path);
    LOG_DEBUG("trying configuration path: %s\n",
            path);
    if (is_readable(path)) {
        free(cached_path);
        cached_path = path;
        return path;
    }
    free(path);

    xdg_config_dirs = getenv("XDG_CONFIG_DIRS");
    if (xdg_config_dirs == NULL) {
        xdg_config_dirs = "/usr/local/share:/usr/share";
    }

    next = xdg_config_dirs;
    do {
        colon = strchr(next, ':');
        if (colon != NULL) {
            length = colon - next;
            colon++;
        } else {
            length = strlen(next);
        }

        path = xasprintf("%.*s/" FENSTERCHEF_CONFIGURATION,
                length, next);
        path = expand_path(path);
        LOG_DEBUG("trying configuration path: %s\n",
                path);
        if (is_readable(path)) {
            free(cached_path);
            cached_path = path;
            break;
        }
        free(path);
        path = NULL;

        next = colon;
    } while (next != NULL);

    return path;
}

/* Clear everything that is currently loaded in the configuration. */
void clear_configuration(void)
{
    clear_cursor_cache();
    clear_button_bindings();
    clear_key_bindings();
    clear_window_relations();
}

/* Reload the fensterchef configuration. */
void reload_configuration(void)
{
    Parser *parser = NULL;

    const char *const configuration = get_configuration_file();

    clear_all_aliases();
    clear_all_groups();

    clear_configuration();

    if (configuration == NULL ||
            (parser = create_file_parser(configuration),
                parser == NULL)) {
        if (configuration != NULL) {
            LOG("could not open %s: %s\n",
                    configuration, strerror(errno));
        }
        set_default_configuration();
    } else if (parse_and_run_actions(parser) != OK) {
        set_default_configuration();
    }
    destroy_parser(parser);
}
