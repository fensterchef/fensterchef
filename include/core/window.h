#ifndef WINDOW_H
#define WINDOW_H

/**
 * Windows are referencing a specific X window and contain meta data about this
 * X window.
 *
 * Windows need to be created using `create_window()` and destroyed using
 * `destroy_window()`.
 */

#include <stdint.h>

#include <X11/Xutil.h>

#include "bits/frame.h"
#include "bits/window.h"
#include "configuration.h"
#include "monitor.h"
#include "utility/attributes.h"
#include "utility/linked_list.h"
#include "x11/ewmh.h"
#include "x11/synchronize.h"

/* the maximum width or height of a window */
#define WINDOW_MAXIMUM_SIZE UINT16_MAX

/* the minimum width or height a window can have */
#define WINDOW_MINIMUM_SIZE 4

/* the minimum length of the window that needs to stay visible */
#define WINDOW_MINIMUM_VISIBLE_SIZE 8

/* the first number of a window */
#define WINDOW_FIRST_NUMBER 1

/* the amount of pixels on the edges of windows to count as resizable */
#define WINDOW_RESIZE_TOLERANCE 8

/* the mode of the window */
typedef enum window_mode {
    /* the window is part of the tiling layout (if visible) */
    WINDOW_MODE_TILING,
    /* the window is a floating window */
    WINDOW_MODE_FLOATING,
    /* the window is a fullscreen window */
    WINDOW_MODE_FULLSCREEN,
    /* the window is attached to an edge of the screen, usually not focusable */
    WINDOW_MODE_DOCK,
    /* the window is in the background */
    WINDOW_MODE_DESKTOP,

    /* the maximum value of a window mode */
    WINDOW_MODE_MAX,
} window_mode_t;

/* properties a window can have */
typedef struct window_properties {
    /* window name */
    utf8_t *name;

    /* window instance (resource name), class (resource class) */
    XClassHint class;

    /* X size hints of the window */
    XSizeHints size_hints;

    /* special window manager hints */
    XWMHints hints;

    /* window strut (reserved region on the screen) */
    wm_strut_partial_t strut;

    /* the window this window is transient for */
    Window transient_for;

    /* the protocols the window supports */
    Atom *protocols;

    /* the region the window should appear at as fullscreen window */
    Extents fullscreen_monitors;

    /* the window states containing atoms _NET_WM_STATE_* */
    Atom *states;

    /* the current WM_STATE atom set on the window, either WM_STATE_NORMAL or
     * WM_STATE_WITHDRAWN
     */
    Atom wm_state;
} WindowProperties;

/* The state of the window signals our window manager what kind of window it is
 * and how the window should behave.
 */
typedef struct window_state {
    /* if the window is visible (mapped) */
    bool is_visible;
    /* if the user ever requested to close the window */
    bool was_close_requested;
    /* when the user requested to close the window */
    time_t user_request_close_time;
    /* the current window mode */
    window_mode_t mode;
    /* the previous window mode */
    window_mode_t previous_mode;
} WindowState;

/* A window is a wrapper around an X window, it is always part of a few global
 * linked list and has a unique id (number).
 */
struct fensterchef_window {
    /* Reference counter to keep the pointer around for longer after the window
     * has been destroyed.  A destroyed but still referenced window will have
     * `client.id` set to `None`.  All other struct members are invalid at that
     * point.
     */
    unsigned reference_count;

    /* the server's view of the window (unrelated to reference_count) */
    XReference reference;

    /* the X properties of this window */
    WindowProperties properties;

    /* the window state */
    WindowState state;

    /* current window position and size */
    int x;
    int y;
    unsigned width;
    unsigned height;

    /* size and color of the border */
    unsigned border_size;
    uint32_t border_color;

    /* position/size when the window was in floating mode */
    Rectangle floating;

    /* the number of this window, multiple windows may have the same number */
    unsigned number;

    /* The age linked list stores the windows in creation time order. */
    /* a window newer than this one */
    FcWindow *newer;

    /* All windows are part of the Z ordered linked list even when they are
     * hidden now.
     *
     * The terms Z stack, Z linked list and Z stacking are used interchangeably.
     *
     * There is a second linked list to store the server state.  This is not
     * updated by the window module but the synchronization function.
     */
    /* the window above this window */
    FcWindow *below;
    /* the window below this window */
    FcWindow *above;
    /* the window that is below on the actual server side */
    FcWindow *server_below;
    /* the window that is above on the actual server side */
    FcWindow *server_above;

    /* The number linked list stores the windows sorted by their number. */
    /* the next window in the linked list */
    FcWindow *next;
};

/* The number of all windows within the linked list.  This value is kept up to
 * date through `create_window()` and `destroy_window()`.
 */
extern unsigned Window_count;

/* the window that was created before any other */
extern SINGLY_LIST(FcWindow, Window_oldest);

/* the window at the bottom/top of the Z stack */
extern DOUBLY_LIST(FcWindow, Window_bottom, Window_top);

/* the window at the bottom/top of the Z stack on the server */
extern DOUBLY_LIST(FcWindow, Window_server_bottom, Window_server_top);

/* the first window in the number linked list */
extern SINGLY_LIST(FcWindow, Window_first);

/* the currently focused window */
extern FcWindow *Window_focus;

/* what the server thinks is the focused window */
extern FcWindow *Window_server_focus;

/* The last pressed window.  This only gets set when a window is pressed by a
 * grabbed button or when a relation runs.
 */
extern FcWindow *Window_pressed;

/* the selected window used for actions */
extern FcWindow *Window_selected;

/* Add window states to the window's properties. */
void add_window_states(FcWindow *window, Atom *states,
        unsigned number_of_states);

/* Remove window states from the window's properties. */
void remove_window_states(FcWindow *window, Atom *states,
        unsigned number_of_states);

/* Update the property within @window corresponding to given @atom. */
bool cache_window_property(FcWindow *window, Atom atom);

/* Increment the reference count of the window. */
void reference_window(FcWindow *window);

/* Decrement the reference count of the window and free @window when it
 * reaches 0.
 */
void dereference_window(FcWindow *window);

/* Create a window object and add it to all window lists.
 *
 * This also runs any associated actions or does the default behavior of showing
 * the window.
 */
FcWindow *create_window(Window id);

/* Destroy given window and removes it from the window linked list.
 * This does NOT destroy the underlying X window.
 */
void destroy_window(FcWindow *window);

/* Get the internal window that has the associated X window.
 *
 * @return NULL when none has this X window.
 */
FcWindow *get_fensterchef_window(Window id);

/* Set the number of a window.
 *
 * This also moves the window in the linked list so it is sorted again.
 */
void set_window_number(FcWindow *window, unsigned number);

/* Get a window with given @id or NULL if no window has that number. */
FcWindow *get_window_by_number(unsigned number);

/* Get the frame this window is contained in.
 *
 * @return NULL when the window is not in any frame.
 */
Frame *get_window_frame(const FcWindow *window);

/* Check if @window supports @protocol. */
bool supports_window_protocol(FcWindow *window, Atom protocol);

/* Check if @window has @state. */
bool has_window_state(FcWindow *window, Atom state);

/* Get the side of a monitor @window would like to attach to.
 *
 * This is based on the window strut if it is set, otherwise the gravity in the
 * size hints is used.
 *
 * @return StaticGravity if there is no preference.
 */
int get_window_gravity(FcWindow *window);

/* Check if @window should have a border. */
bool is_window_borderless(FcWindow *window);

/* time in seconds to wait for a second close */
#define REQUEST_CLOSE_MAX_DURATION 2

/* Attempt to close a window.
 *
 * If it is the first time, use a friendly method by sending a close request to
 * the window.  Call this function again within `REQUEST_CLOSE_MAX_DURATION` to
 * forcefully kill it.
 */
void close_window(FcWindow *window);

/* Get the minimum size the window should have. */
void get_minimum_window_size(const FcWindow *window, Size *size);

/* Get the maximum size the window should have. */
void get_maximum_window_size(const FcWindow *window, Size *size);

/* Set the position and size of a window.
 *
 * Note that this function clips the parameters using `get_minimum_size()` and
 * `get_maximum_size()`.
 */
void set_window_size(FcWindow *window, int x, int y, unsigned width,
        unsigned height);

/* Reset the position and size of given window according to its window mode. */
void reset_window_size(FcWindow *window);

/* Change the mode to given value and reconfigures the window if it is visible.
 *
 * @force_mode is used to force the change of the window mode.
 */
void set_window_mode(FcWindow *window, window_mode_t mode);

/* Show the window by positioning it and mapping it to the X server.
 *
 * Note that this removes the given window from the taken window list.
 */
void show_window(FcWindow *window);

/* Hide @window and adjust the tiling and focus.
 *
 * When the window is a tiling window, this places the next available window in
 * the formed gap.
 *
 * The next window is focused.
 */
void hide_window(FcWindow *window);

/* Hide the window without touching the tiling or focus.
 *
 * Note: The focus however is removed if @window is the focus.
 *
 * To abrubtly show a window, simply do: `window->state.is_visible = true`.
 */
void hide_window_abruptly(FcWindow *window);

/* Put all windows above @window that are transient for it. */
void raise_windows_transient_for(FcWindow *window);

/* Put the window on the best suited Z stack position. */
void update_window_layer(FcWindow *window);

/* Check if the window accepts input focus. */
bool is_window_focusable(FcWindow *window);

/* Set the window that is in focus.
 *
 * @window may be NULL, then the focus is set to no window.
 */
void set_focus_window(_Nullable FcWindow *window);

/* Focus @window and the frame it is contained in if any. */
void set_focus_window_with_frame(FcWindow *window);

#endif
