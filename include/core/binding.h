#ifndef BINDING_H
#define BINDING_H

/**
 * User set button and key bindings.  This also includes caching and
 * synchronization functions to grab the keys.
 */

#include <X11/X.h>

#include <stdbool.h>

#include "bits/action_block.h"

/* Note: The actual values on the X11 server may be even more restricted but
 * these work every time.
 */
/* minimum value a keycode can have */
#define KEYCODE_MIN 8
/* maximum value of a key code (exclusive) */
#define KEYCODE_MAX 256

/* list of all button indexes */
typedef enum {
    BUTTON_NONE,

    BUTTON_MIN,

    BUTTON_LEFT = BUTTON_MIN,
    BUTTON_MIDDLE,
    BUTTON_RIGHT,

    BUTTON_WHEEL_UP,
    BUTTON_WHEEL_DOWN,
    BUTTON_WHEEL_LEFT,
    BUTTON_WHEEL_RIGHT,

    BUTTON_X1,
    BUTTON_X2,
    BUTTON_X3,
    BUTTON_X4,
    BUTTON_X5,
    BUTTON_X6,
    BUTTON_X7,
    BUTTON_X8,

    BUTTON_MAX,
} button_t;

/* a button binding structure to pass to `set_button_binding()` */
struct button_binding {
    /* if this key binding is triggered on a release */
    bool is_release;
    /* if the event should pass through to the window the event happened in */
    bool is_transparent;
    /* the key modifiers */
    unsigned modifiers;
    /* the button index */
    button_t button;
    /* the actions to execute */
    ActionBlock *actions;
};

/* a key binding structure to pass to `set_key_binding()` */
struct key_binding {
    /* if this key binding is triggered on a release */
    bool is_release;
    /* the key modifiers */
    unsigned modifiers;
    /* the key symbol, may be `NoSymbol` */
    KeySym key_symbol;
    /* the key code to use */
    KeyCode key_code;
    /* the actions to execute */
    ActionBlock *actions;
};

/* the default modifiers to ignore */
#define DEFAULT_IGNORE_MODIFIERS (Mod2Mask | LockMask)

/* Set the modifiers to ignore. */
void set_ignored_modifiers(unsigned modifiers);

/* Set the specified button binding. */
void set_button_binding(const struct button_binding *button_binding);

/* Run the specified key binding. */
void run_button_binding(Time event_time, bool is_release,
        unsigned modifiers, button_t button);

/* Unset all configured button bindings. */
void unset_button_bindings(void);

/* Set the specified key binding. */
void set_key_binding(const struct key_binding *key_binding);

/* Run the specified key binding. */
void run_key_binding(bool is_release, unsigned modifiers, KeyCode key_code);

/* Unset all configured key bindings. */
void unset_key_bindings(void);

/* Resolve all key symbols in case the mapping changed. */
void resolve_all_key_symbols(void);

/* Grab the button bindings for a window so we receive MousePress/MouseRelease
 * events for it.
 */
void grab_configured_buttons(Window window);

/* Grab the key bindings so we receive KeyPress/KeyRelease events for them. */
void grab_configured_keys(void);

#endif
