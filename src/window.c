#include <limits.h>
#include <string.h>
#include <time.h>

#include <X11/Xatom.h>

#include "relation.h"
#include "binding.h"
#include "event.h"
#include "frame.h"
#include "log.h"
#include "monitor.h"
#include "parse/parse.h"
#include "window.h"
#include "window_list.h"
#include "x11/display.h"

/* the number of all windows within the linked list */
unsigned Window_count;

/* the window that was created before any other */
SINGLY_LIST(FcWindow, Window_oldest);

/* the window at the bottom/top of the Z stack */
DOUBLY_LIST(FcWindow, Window_bottom, Window_top);

/* the window at the bottom/top of the Z stack on the server */
DOUBLY_LIST(FcWindow, Window_server_bottom, Window_server_top);

/* the first window in the number linked list */
SINGLY_LIST(FcWindow, Window_first);

/* the currently focused window */
FcWindow *Window_focus;

/* what the server thinks is the focused window */
FcWindow *Window_server_focus;

/* the last pressed window */
FcWindow *Window_pressed;

/* the selected window used for actions */
FcWindow *Window_selected;

/*********************
 * Window properties *
 *********************/

/* Add window states to the window properties. */
void add_window_states(FcWindow *window, Atom *states,
        unsigned number_of_states)
{
    unsigned effective_count = 0;

    /* for each state in `states`, either add it or filter it out */
    for (unsigned i = 0, j; i < number_of_states; i++) {
        /* filter out the states already in the window properties */
        if (has_window_state(window, states[i])) {
            continue;
        }

        j = 0;

        /* add the state to the window properties */
        if (window->properties.states != NULL) {
            /* find the number of elements */
            for (; window->properties.states[j] != None; j++) {
                /* nothing */
            }
        }

        REALLOCATE(window->properties.states, j + 2);
        window->properties.states[j] = states[i];
        window->properties.states[j + 1] = None;

        states[effective_count++] = states[i];
    }

    /* check if anything changed */
    if (effective_count == 0) {
        return;
    }

    /* append the properties to the list in the X server */
    XChangeProperty(display, window->reference.id, ATOM(_NET_WM_STATE), XA_ATOM,
            32, PropModeAppend, (unsigned char*) states, effective_count);
}

/* Remove window states from the window properties. */
void remove_window_states(FcWindow *window, Atom *states,
        unsigned number_of_states)
{
    unsigned i;
    unsigned effective_count = 0;

    /* if no states are there, nothing can be removed */
    if (window->properties.states == NULL) {
        return;
    }

    /* filter out all states in the window properties that are in `states` */
    for (i = 0; window->properties.states[i] != None; i++) {
        unsigned j;

        /* check if the state exists in `states`... */
        for (j = 0; j < number_of_states; j++) {
            if (states[j] == window->properties.states[i]) {
                break;
            }
        }

        /* ...if not, add it */
        if (j == number_of_states) {
            window->properties.states[effective_count++] =
                window->properties.states[i];
        }
    }

    /* check if anything changed */
    if (effective_count == i) {
        return;
    }

    /* terminate the end with `None` */
    window->properties.states[effective_count] = None;

    /* replace the atom list on the X server */
    XChangeProperty(display, window->reference.id, ATOM(_NET_WM_STATE), XA_ATOM,
            32, PropModeReplace,
            (unsigned char*) window->properties.states, effective_count);
}

/* Update the property within @window corresponding to given atom. */
bool cache_window_property(FcWindow *window, Atom atom)
{
    if (atom == XA_WM_NAME || atom == ATOM(_NET_WM_NAME)) {
        free(window->properties.name);
        window->properties.name =
            get_window_name_property(window->reference.id);
    } else if (atom == XA_WM_CLASS) {
        XFree(window->properties.class.res_name);
        XFree(window->properties.class.res_class);
        window->properties.class.res_name = NULL;
        window->properties.class.res_class = NULL;
        XGetClassHint(display, window->reference.id, &window->properties.class);
    } else if (atom == XA_WM_NORMAL_HINTS) {
        long supplied;

        XGetWMNormalHints(display, window->reference.id,
                &window->properties.size_hints, &supplied);
        /* clip the window to new potential size hints */
        set_window_size(window, window->x, window->y, window->width,
                window->height);
    } else if (atom == XA_WM_HINTS) {
        XWMHints *wm_hints;

        wm_hints = XGetWMHints(display, window->reference.id);
        if (wm_hints == NULL) {
            window->properties.hints.flags = 0;
        } else {
            window->properties.hints = *wm_hints;
            XFree(wm_hints);
        }
    } else if (atom == ATOM(_NET_WM_STRUT) ||
            atom == ATOM(_NET_WM_STRUT_PARTIAL)) {
        get_strut_property(window->reference.id, &window->properties.strut);
    } else if (atom == XA_WM_TRANSIENT_FOR) {
        XGetTransientForHint(display, window->reference.id,
                &window->properties.transient_for);
    } else if (atom == ATOM(WM_PROTOCOLS)) {
        free(window->properties.protocols);
        window->properties.protocols =
            get_protocols_property(window->reference.id);
    } else if (atom == ATOM(_NET_WM_FULLSCREEN_MONITORS)) {
        get_fullscreen_monitors_property(window->reference.id,
                &window->properties.fullscreen_monitors);
    } else {
        return false;
    }
    return true;
}

/* Initialize all properties within @properties. */
static void initialize_window_properties(FcWindow *window)
{
    int atom_count;
    Atom *atoms;
    Atom *states = NULL;
    Atom *types = NULL;
    window_mode_t predicted_mode = WINDOW_MODE_TILING;

    /* get a list of properties currently set on the window */
    atoms = XListProperties(display, window->reference.id, &atom_count);

    /* cache all properties */
    for (int i = 0; i < atom_count; i++) {
        LOG_DEBUG("window has: %a\n",
                atoms[i]);
        if (atoms[i] == ATOM(_NET_WM_STATE)) {
            states = get_atom_list_property(window->reference.id,
                    ATOM(_NET_WM_STATE));
#ifdef DEBUG
            LOG_DEBUG("_NET_WM_STATE: ");
            for (Atom *atom = states; atom[0] != None; atom++) {
                log_formatted("%a",
                        atom[0]);
                if (atom[1] != None) {
                    log_formatted(", ");
                }
            }
            log_formatted("\n");
#endif
        } else if (atoms[i] == ATOM(_NET_WM_WINDOW_TYPE)) {
            types = get_atom_list_property(window->reference.id,
                    ATOM(_NET_WM_WINDOW_TYPE));
#ifdef DEBUG
            LOG_DEBUG("_NET_WM_WINDOW_TYPE: ");
            for (Atom *atom = types; atom[0] != None; atom++) {
                log_formatted("%a",
                        atom[0]);
                if (atom[1] != None) {
                    log_formatted(", ");
                }
            }
            log_formatted("\n");
#endif
        } else {
            cache_window_property(window, atoms[i]);
        }
    }

    if (is_atom_included(types, ATOM(_NET_WM_WINDOW_TYPE_DESKTOP))) {
        predicted_mode = WINDOW_MODE_DESKTOP;
    } else if (is_atom_included(types, ATOM(_NET_WM_WINDOW_TYPE_DOCK)) ||
            !is_strut_empty(&window->properties.strut)) {
        predicted_mode = WINDOW_MODE_DOCK;
    } else if (is_atom_included(states, ATOM(_NET_WM_STATE_FULLSCREEN))) {
        predicted_mode = WINDOW_MODE_FULLSCREEN;
    } else if (is_atom_included(types, ATOM(_NET_WM_WINDOW_TYPE_DIALOG)) ||
            is_atom_included(types, ATOM(_NET_WM_WINDOW_TYPE_SPLASH)) ||
            window->properties.transient_for != 0) {
        predicted_mode = WINDOW_MODE_FLOATING;
    /* floating windows have an equal minimum and maximum size */
    } else if ((window->properties.size_hints.flags & (PMinSize | PMaxSize)) ==
            (PMinSize | PMaxSize) &&
            (window->properties.size_hints.min_width ==
                window->properties.size_hints.max_width ||
                window->properties.size_hints.min_height ==
                    window->properties.size_hints.max_height)) {
        predicted_mode = WINDOW_MODE_FLOATING;
    }

    window->properties.states = states;

    free(types);

    XFree(atoms);

    set_window_mode(window, predicted_mode);
}

/***********************************
 * Window creation and destruction *
 ***********************************/

/* Increment the reference count of the window. */
inline void reference_window(FcWindow *window)
{
    window->reference_count++;
}

/* Decrement the reference count of the window and free @window when it
 * reaches 0.
 */
inline void dereference_window(FcWindow *window)
{
    window->reference_count--;
    if (window->reference_count == 0) {
        free(window);
    }
}

/* Find where in the number linked list a gap is.
 *
 * @return NULL when the window should be inserted before the first window.
 */
static inline FcWindow *find_number_gap(void)
{
    FcWindow *previous;

    /* if the first window has window number greater than the first number that
     * means there is space at the front
     */
    if (Window_first->number > configuration.first_window_number) {
        return NULL;
    }

    previous = Window_first;
    /* find the first window with a higher number than
     * `first_window_number`
     */
    for (; previous->next != NULL; previous = previous->next) {
        if (previous->next->number > configuration.first_window_number) {
            break;
        }
    }
    /* find a gap in the window numbers */
    for (; previous->next != NULL; previous = previous->next) {
        if (previous->number + 1 < previous->next->number) {
            break;
        }
    }
    return previous;
}

/* Find the window after which a new window with given @number should be
 * inserted.
 *
 * @return NULL when the window should be inserted before the first window.
 */
static inline FcWindow *find_window_number(unsigned number)
{
    FcWindow *previous;

    if (Window_first == NULL || Window_first->number > number) {
        return NULL;
    }

    previous = Window_first;
    /* find a gap in the window numbers */
    for (; previous->next != NULL; previous = previous->next) {
        if (previous->next->number > number) {
            break;
        }
    }
    return previous;
}

/* Create a window struct and add it to the window list. */
FcWindow *create_window(Window id)
{
    XWindowAttributes attributes;
    Window root;
    int x, y;
    unsigned int width, height;
    unsigned int border_width;
    unsigned int depth;
    XSetWindowAttributes set_attributes;
    FcWindow *window;
    FcWindow *previous;
    XWindowChanges changes;

    if (XGetWindowAttributes(display, id, &attributes) == 0) {
        /* the window got invalid because it was abruptly destroyed */
        LOG_DEBUG("window %#lx abruptly disappeared\n",
                id);
        return NULL;
    }

    /* Override redirect is used by windows to indicate that our window manager
     * should not tamper with them.  We also check if the class is InputOnly
     * which is not a case we want to handle (the window has no graphics).
     */
    if (attributes.override_redirect || attributes.class == InputOnly) {
        /* check if the window holds a command */
        utf8_t *const command = get_fensterchef_command_property(id);
        if (command != NULL) {
            Parser *parser;

            LOG("window %#lx has command: %s\n",
                    id, command);

            parser = create_string_parser(command);
            (void) parse_and_run_actions(parser);
            destroy_parser(parser);

            free(command);

            /* signal to the window that we executed its command and it can go
             * now
             */
            XDeleteProperty(display, id, ATOM(FENSTERCHEF_COMMAND));
        }
        return NULL;
    }

    /* set the initial border color */
    set_attributes.border_pixel = configuration.border_color_focus;
    /* we want to know if if any properties change */
    set_attributes.event_mask = PropertyChangeMask;
    XChangeWindowAttributes(display, id, CWBorderPixel | CWEventMask,
            &set_attributes);

    XGetGeometry(display, id, &root, &x, &y, &width, &height, &border_width,
            &depth);

    ALLOCATE_ZERO(window, 1);

    window->reference_count = 1;
    window->reference.id = id;
    window->reference.x = x;
    window->reference.y = x;
    window->reference.width = width;
    window->reference.height = height;
    window->reference.border = configuration.border_color_focus;
    window->reference.border_width = border_width;
    /* check if the window is already mapped on the X server */
    if (attributes.map_state != IsUnmapped) {
        window->reference.is_mapped = true;
    }

    /* start off with an invalid mode, this gets set below */
    window->state.mode = WINDOW_MODE_MAX;
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->border_color = configuration.border_color_focus;
    window->border_size = configuration.border_size;

    /* link into the Z, age and number linked lists */
    if (Window_first == NULL) {
        Window_oldest = window;
        Window_bottom = window;
        Window_top = window;
        Window_server_top = window;
        Window_first = window;
        window->number = configuration.first_window_number;
    } else {
        previous = find_number_gap();
        if (previous == NULL) {
            window->next = Window_first;
            Window_first = window;
            window->number = configuration.first_window_number;
        } else {
            if (previous->number < configuration.first_window_number) {
                window->number = configuration.first_window_number;
            } else {
                window->number = previous->number + 1;
            }
            window->next = previous->next;
            previous->next = window;
        }

        /* put the window at the top of the Z linked list */
        window->below = Window_top;
        Window_top->above = window;
        Window_top = window;

        /* Put the window at the top of the Z server linked list.  That is where
         * the server puts new windows.
         */
        window->server_below = Window_server_top;
        Window_server_top->server_above = window;
        Window_server_top = window;

        /* if the window list is open, put it below it */
        if (WindowList.reference.is_mapped) {
            changes.stack_mode = Below;
            changes.sibling = WindowList.reference.id;
            XConfigureWindow(display, id, CWStackMode | CWSibling, &changes);
        }

        /* put the window into the age linked list */
        previous = Window_oldest;
        while (previous->newer != NULL) {
            previous = previous->newer;
        }
        previous->newer = window;
    }

    /* new window is now in the list */
    Window_count++;

    /* setup window properties and get the initial mode */
    initialize_window_properties(window);

    /* grab the buttons for this window */
    grab_configured_buttons(id);

    LOG("created new window %W\n",
            window);

    if (run_window_relations(window)) {
        /* nothing */
    /* if a window does not start in normal state, do not map it */
    } else if ((window->properties.hints.flags & StateHint) &&
            window->properties.hints.initial_state != NormalState) {
        LOG("window %W starts off as hidden window\n",
                window);
    } else {
        /* run the default behavior */
        show_window(window);
        if (is_window_focusable(window)) {
            set_focus_window_with_frame(window);
        }
    }

    /* put the window on a sensible Z position for its mode */
    update_window_layer(window);

    return window;
}

/* Destroys given window and removes it from the window linked list. */
void destroy_window(FcWindow *window)
{
    Frame *frame;

    /* Really make sure the window is hidden.  Not sure if this case can ever
     * happen because usually a MapUnnotify event hides the window beforehand.
     */
    hide_window_abruptly(window);

    /* exceptional state, this should never happen */
    if (window == Window_focus) {
        Window_focus = NULL;
        LOG_ERROR("destroying window with focus\n");
    }

    if (window == Window_server_focus) {
        Window_server_focus = NULL;
    }

    if (window == Window_pressed) {
        Window_pressed = NULL;
    }

    if (window == Window_selected) {
        Window_selected = NULL;
    }

    /* this should also never happen but we check just in case */
    frame = get_window_frame(window);
    if (frame != NULL) {
        frame->window = NULL;
        LOG_ERROR("window being destroyed is still within a frame\n");
    }

    LOG("destroying window %W\n", window);

    DOUBLY_UNLINK(Window_bottom, Window_top, window, below, above);
    DOUBLY_UNLINK(Window_server_bottom, Window_server_top, window,
            server_below, server_above);
    SINGLY_UNLINK(Window_oldest, window, newer);
    SINGLY_UNLINK(Window_first, window, next);

    /* window is gone from the list now */
    Window_count--;

    /* setting the id to None marks the window as destroyed */
    window->reference.id = None;
    free(window->properties.name);
    XFree(window->properties.class.res_name);
    XFree(window->properties.class.res_class);
    free(window->properties.protocols);
    free(window->properties.states);

    dereference_window(window);
}

/******************
 * Window utility *
 ******************/

/* Get the internal window that has the associated X window. */
FcWindow *get_fensterchef_window(Window id)
{
    for (FcWindow *window = Window_first; window != NULL;
            window = window->next) {
        if (window->reference.id == id) {
            return window;
        }
    }
    return NULL;
}

/* Set the number of a window. */
void set_window_number(FcWindow *window, unsigned number)
{
    FcWindow *previous;

    SINGLY_UNLINK(Window_first, window, next);

    previous = find_window_number(number);
    if (previous == NULL) {
        window->next = Window_first;
        Window_first = window;
    } else {
        window->next = previous->next;
        previous->next = window;
    }

    window->number = number;
}

/* Get a window with given @number or NULL if no window has that id. */
FcWindow *get_window_by_number(unsigned number)
{
    FcWindow *window;

    for (window = Window_first; window != NULL; window = window->next) {
        if (window->number == number) {
            break;
        }
    }
    return window;
}

/* Checks if @frame contains @window and checks this for all its children. */
static Frame *find_frame_recursively(Frame *frame, const FcWindow *window)
{
    if (frame->window == window) {
        return frame;
    }

    if (frame->left == NULL) {
        return NULL;
    }

    Frame *const find = find_frame_recursively(frame->left, window);
    if (find != NULL) {
        return find;
    }

    return find_frame_recursively(frame->right, window);
}

/* Get the frame this window is contained in. */
Frame *get_window_frame(const FcWindow *window)
{
    /* shortcut: only tiling windows are within a frame */
    if (window->state.mode != WINDOW_MODE_TILING) {
        return NULL;
    }

    for (Monitor *monitor = Monitor_first; monitor != NULL;
            monitor = monitor->next) {
        Frame *const find = find_frame_recursively(monitor->frame, window);
        if (find != NULL) {
            return find;
        }
    }
    return NULL;
}

/* Check if @window supports @protocol. */
bool supports_window_protocol(FcWindow *window, Atom protocol)
{
    return is_atom_included(window->properties.protocols, protocol);
}

/* Check if @window has @state. */
bool has_window_state(FcWindow *window, Atom state)
{
    return is_atom_included(window->properties.states, state);
}

/* Get the side of a monitor @window would like to attach to. */
int get_window_gravity(FcWindow *window)
{
    if (window->properties.strut.left > 0) {
        return WestGravity;
    }
    if (window->properties.strut.top > 0) {
        return NorthGravity;
    }
    if (window->properties.strut.right > 0) {
        return EastGravity;
    }
    if (window->properties.strut.bottom > 0) {
        return SouthGravity;
    }

    if ((window->properties.size_hints.flags & PWinGravity)) {
        return window->properties.size_hints.win_gravity;
    }
    return StaticGravity;
}

/* Attempt to close a window. */
void close_window(FcWindow *window)
{
    time_t current_time;

    current_time = time(NULL);

    /* if either WM_DELETE_WINDOW is not supported or a close was requested
     * twice in a row, forcefully destroy the window
     */
    if (!supports_window_protocol(window, ATOM(WM_DELETE_WINDOW)) ||
            (window->state.was_close_requested && current_time <=
                window->state.user_request_close_time +
                    REQUEST_CLOSE_MAX_DURATION)) {
        XDestroyWindow(display, window->reference.id);
    } else {
        send_delete_window_message(window->reference.id);

        window->state.was_close_requested = true;
        window->state.user_request_close_time = current_time;
    }
}

/****************************
 * Window moving and sizing *
 ****************************/

/* Get the minimum size the window can have. */
void get_minimum_window_size(const FcWindow *window, Size *size)
{
    unsigned int width = 0, height = 0;

    if (window->state.mode != WINDOW_MODE_TILING) {
        if ((window->properties.size_hints.flags & PMinSize)) {
            width = window->properties.size_hints.min_width;
            height = window->properties.size_hints.min_height;
        }
    }
    size->width = MAX(width, WINDOW_MINIMUM_SIZE);
    size->height = MAX(height, WINDOW_MINIMUM_SIZE);
}

/* Get the maximum size the window can have. */
void get_maximum_window_size(const FcWindow *window, Size *size)
{
    unsigned int width = UINT_MAX, height = UINT_MAX;

    if ((window->properties.size_hints.flags & PMaxSize)) {
        width = window->properties.size_hints.max_width;
        height = window->properties.size_hints.max_height;
    }
    size->width = MIN(width, WINDOW_MAXIMUM_SIZE);
    size->height = MIN(height, WINDOW_MAXIMUM_SIZE);
}

/* Set the position and size of a window. */
void set_window_size(FcWindow *window, int x, int y, unsigned width,
        unsigned height)
{
    Size minimum, maximum;

    get_minimum_window_size(window, &minimum);
    get_maximum_window_size(window, &maximum);

    /* make sure the window does not become too large or too small */
    width = MIN(width, maximum.width);
    height = MIN(height, maximum.height);
    width = MAX(width, minimum.width);
    height = MAX(height, minimum.height);

    if (window->state.mode == WINDOW_MODE_FLOATING) {
        window->floating.x = x;
        window->floating.y = y;
        window->floating.width = width;
        window->floating.height = height;
    }

    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
}

/* Put windows along a diagonal line, spacing them out a little. */
static inline void move_to_next_available(Monitor *monitor, FcWindow *window,
        int *destination_x, int *destination_y)
{
    FcWindow *other;
    int start_x, start_y;
    int x = 0, y = 0;
    FcWindow *top = NULL;

    start_x = monitor->x + monitor->width / 10;
    start_y = monitor->y + monitor->height / 10;

    /* Check if all windows up to the start position are on a diagonal line.
     * If that is the case, yield the position of the window furthest along this
     * line.
     */
    for (other = Window_top;
            other != NULL && other->state.mode != WINDOW_MODE_TILING;
            other = other->below) {
        if (other == window || !other->state.is_visible) {
            continue;
        }

        const Point difference = {
            other->x - start_x,
            other->y - start_y,
        };

        /* if the window is not on the diagonal line, stop */
        if (difference.x < 0 || difference.x != difference.y ||
                difference.x % 20 != 0) {
            top = NULL;
            break;
        }

        if (top == NULL) {
            top = other;
        } else if (x - 20 != difference.x || y - 20 != difference.y) {
            /* not all windows are on the diagnoal line */
            top = NULL;
            break;
        }

        if (difference.x == 0) {
            /* we found the start of the diagonal line */
            break;
        }

        /* save these for the next iteration */
        x = difference.x;
        y = difference.y;
    }

    if (top == NULL) {
        /* start a fresh diagonal line */
        *destination_x = start_x;
        *destination_y = start_y;
    } else {
        /* append the window to the line */
        *destination_x = top->x + 20;
        *destination_y = top->y + 20;
    }
}

/* Set the window size and position according to the size hints. */
static void configure_floating_size(FcWindow *window)
{
    int x, y;
    unsigned width, height;

    /* if the window never had a floating size, figure it out based off the
     * hints
     */
    if (window->floating.width == 0) {
        Monitor *monitor;

        /* put the window on the monitor that is either on the same monitor as
         * the focused window or the focused frame
         */
        monitor = get_focused_monitor();

        if ((window->properties.size_hints.flags & PSize)) {
            width = window->properties.size_hints.width;
            height = window->properties.size_hints.height;
        } else {
            width = monitor->width * 2 / 3;
            height = monitor->height * 2 / 3;
        }

        if ((window->properties.size_hints.flags & PMinSize)) {
            width = MAX(width,
                    (unsigned) window->properties.size_hints.min_width);
            height = MAX(height,
                    (unsigned) window->properties.size_hints.min_height);
        }

        if ((window->properties.size_hints.flags & PMaxSize)) {
            width = MIN(width,
                    (unsigned) window->properties.size_hints.max_width);
            height = MIN(height,
                    (unsigned) window->properties.size_hints.max_height);
        }

        /* non resizable windows are centered */
        if ((window->properties.size_hints.flags & (PMinSize | PMaxSize)) ==
                    (PMinSize | PMaxSize) &&
                (window->properties.size_hints.min_width ==
                    window->properties.size_hints.max_width ||
                    window->properties.size_hints.min_height ==
                        window->properties.size_hints.max_height)) {
            x = monitor->x + (monitor->width - width) / 2;
            y = monitor->y + (monitor->height - height) / 2;
        } else {
            move_to_next_available(monitor, window, &x, &y);
        }
    } else {
        x = window->floating.x;
        y = window->floating.y;
        width = window->floating.width;
        height = window->floating.height;
    }

    set_window_size(window, x, y, width, height);
}

/* Set the position and size of the window to fullscreen. */
static void configure_fullscreen_size(FcWindow *window)
{
    Monitor *monitor;

    if (window->properties.fullscreen_monitors.top !=
            window->properties.fullscreen_monitors.bottom) {
        set_window_size(window, window->properties.fullscreen_monitors.left,
                window->properties.fullscreen_monitors.top,
                window->properties.fullscreen_monitors.right -
                    window->properties.fullscreen_monitors.left,
                window->properties.fullscreen_monitors.bottom -
                    window->properties.fullscreen_monitors.left);
    } else {
        monitor = get_monitor_containing_window(window);
        set_window_size(window, monitor->x, monitor->y,
                monitor->width, monitor->height);
    }
}

/* Set the position and size of the window to a dock window. */
static void configure_dock_size(FcWindow *window)
{
    Monitor *monitor;
    int x, y;
    unsigned width, height;

    monitor = get_monitor_containing_window(window);

    if (!is_strut_empty(&window->properties.strut)) {
        x = monitor->x;
        y = monitor->y;
        width = monitor->width;
        height = monitor->height;

        /* do the sizing/position based on the strut the window defines,
         * reasoning is that when the window wants to occupy screen space, then
         * it should be within that occupied space
         */
        if (window->properties.strut.left != 0) {
            width = window->properties.strut.left;
            /* check if the extended strut is set or if it is malformed */
            if (window->properties.strut.left_start_y <
                    window->properties.strut.left_end_y) {
                y = window->properties.strut.left_start_y;
                height = window->properties.strut.left_end_y -
                    window->properties.strut.left_start_y + 1;
            }
        } else if (window->properties.strut.top != 0) {
            height = window->properties.strut.top;
            if (window->properties.strut.top_start_x <
                    window->properties.strut.top_end_x) {
                x = window->properties.strut.top_start_x;
                width = window->properties.strut.top_end_x -
                    window->properties.strut.top_start_x + 1;
            }
        } else if (window->properties.strut.right != 0) {
            x = monitor->x + monitor->width - window->properties.strut.right;
            width = window->properties.strut.right;
            if (window->properties.strut.right_start_y <
                    window->properties.strut.right_end_y) {
                y = window->properties.strut.right_start_y;
                height = window->properties.strut.right_end_y -
                    window->properties.strut.right_start_y + 1;
            }
        } else if (window->properties.strut.bottom != 0) {
            y = monitor->y + monitor->height - window->properties.strut.bottom;
            height = window->properties.strut.bottom;
            if (window->properties.strut.bottom_start_x <
                    window->properties.strut.bottom_end_x) {
                x = window->properties.strut.bottom_start_x;
                width = window->properties.strut.bottom_end_x -
                    window->properties.strut.bottom_start_x + 1;
            }
        }
    } else {
        x = window->x;
        y = window->y;
        width = window->width;
        height = window->height;

        const int gravity = get_window_gravity(window);
        adjust_for_window_gravity(monitor, &x, &y, width, height, gravity);
    }

    set_window_size(window, x, y, width, height);
}

/* Reset the position and size of given window according to its window mode. */
void reset_window_size(FcWindow *window)
{
    switch (window->state.mode) {
    case WINDOW_MODE_TILING:
        /* do nothing, the frame knows better */
        break;

    case WINDOW_MODE_FLOATING:
        configure_floating_size(window);
        break;
    case WINDOW_MODE_FULLSCREEN:
        configure_fullscreen_size(window);
        break;
    case WINDOW_MODE_DOCK:
        configure_dock_size(window);
        break;

    case WINDOW_MODE_DESKTOP:
        /* none of our business */
        break;

    /* not a real window mode */
    case WINDOW_MODE_MAX:
        break;
    }
}

/****************
 * Window state *
 ****************/

/* When a window changes mode or is shown, this is called.
 *
 * This adjusts the window size or puts the window into the tiling layout.
 */
static void update_shown_window(FcWindow *window)
{
    switch (window->state.mode) {
    /* the window has to become part of the tiling layout */
    case WINDOW_MODE_TILING: {
        Frame *frame;

        frame = get_window_frame(window);
        /* we never would want this to happen */
        if (frame != NULL) {
            LOG_ERROR("window %W is already in frame %F\n",
                    window, frame);
            reload_frame(frame);
            break;
        }

        frame = get_frame_by_number(window->number);
        if (frame != NULL) {
            LOG("found frame %F matching the window id\n",
                    frame);
            (void) stash_frame(frame);
            frame->window = window;
            reload_frame(frame);
            break;
        }

        if (configuration.auto_find_void) {
            Monitor *monitor;

            frame = find_frame_void(Frame_focus);
            if (frame == NULL) {
                monitor = get_focused_monitor();
                frame = find_frame_void(monitor->frame);
            }
            if (frame != NULL) {
                LOG("found a void to fill\n");

                frame->window = window;
                reload_frame(frame);
                break;
            }
        }

        if (configuration.auto_split && Frame_focus->window != NULL) {
            Frame *const wrap = create_frame();
            wrap->window = window;
            split_frame(Frame_focus, wrap, false, Frame_focus->split_direction);
            Frame_focus = wrap;
        } else {
            stash_frame(Frame_focus);
            Frame_focus->window = window;
            reload_frame(Frame_focus);
        }
        break;
    }

    /* the window has to show as floating window */
    case WINDOW_MODE_FLOATING:
        configure_floating_size(window);
        break;

    /* the window has to show as fullscreen window */
    case WINDOW_MODE_FULLSCREEN:
        configure_fullscreen_size(window);
        break;

    /* the window has to show as dock window */
    case WINDOW_MODE_DOCK:
        configure_dock_size(window);
        break;

    /* do nothing, the desktop window should know better */
    case WINDOW_MODE_DESKTOP:
        break;

    /* not a real window mode */
    case WINDOW_MODE_MAX:
        break;
    }
}

/* Synchronize the _NET_WM_ALLOWED_ACTIONS X property. */
static void synchronize_allowed_actions(FcWindow *window)
{
    const Atom atom_lists[WINDOW_MODE_MAX][16] = {
        [WINDOW_MODE_TILING] = {
            ATOM(_NET_WM_ACTION_MOVE),
            ATOM(_NET_WM_ACTION_RESIZE),
            ATOM(_NET_WM_ACTION_MINIMIZE),
            ATOM(_NET_WM_ACTION_FULLSCREEN),
            ATOM(_NET_WM_ACTION_MAXIMIZE_HORZ),
            ATOM(_NET_WM_ACTION_MAXIMIZE_VERT),
            ATOM(_NET_WM_ACTION_CLOSE),
            None,
        },

        [WINDOW_MODE_FLOATING] = {
            ATOM(_NET_WM_ACTION_MOVE),
            ATOM(_NET_WM_ACTION_RESIZE),
            ATOM(_NET_WM_ACTION_MINIMIZE),
            ATOM(_NET_WM_ACTION_FULLSCREEN),
            ATOM(_NET_WM_ACTION_MAXIMIZE_HORZ),
            ATOM(_NET_WM_ACTION_MAXIMIZE_VERT),
            ATOM(_NET_WM_ACTION_CLOSE),
            ATOM(_NET_WM_ACTION_ABOVE),
            None,
        },

        [WINDOW_MODE_FULLSCREEN] = {
            ATOM(_NET_WM_ACTION_MINIMIZE),
            ATOM(_NET_WM_ACTION_CLOSE),
            ATOM(_NET_WM_ACTION_ABOVE),
            None,
        },

        [WINDOW_MODE_DOCK] = {
            None,
        },

        [WINDOW_MODE_DESKTOP] = {
            None,
        },
    };

    const Atom *list;
    unsigned list_length;

    list = atom_lists[window->state.mode];
    for (list_length = 0; list_length < SIZE(atom_lists[0]); list_length++) {
        if (list[list_length] == None) {
            break;
        }
    }

    XChangeProperty(display, window->reference.id, ATOM(_NET_WM_ALLOWED_ACTIONS),
            XA_ATOM, 32, PropModeReplace, (unsigned char*) list, list_length);
}

/* Changes the window state to given value and reconfigures the window only
 * if the mode changed.
 */
void set_window_mode(FcWindow *window, window_mode_t mode)
{
    Atom states[3];

    if (window->state.mode == mode) {
        return;
    }

    LOG("transition window mode of %W from %m to %m\n", window,
            window->state.mode, mode);

    /* this is true if the window is being initialized */
    if (window->state.mode == WINDOW_MODE_MAX) {
        window->state.previous_mode = mode;
    } else {
        window->state.previous_mode = window->state.mode;
    }
    window->state.mode = mode;

    if (window->state.is_visible) {
        /* pop out from tiling layout */
        if (window->state.previous_mode == WINDOW_MODE_TILING) {
            /* make sure no shortcut is taken in `get_window_frame()` */
            window->state.mode = WINDOW_MODE_TILING;
            Frame *const frame = get_window_frame(window);
            if (frame == NULL) {
                /* code is broken */
                LOG_DEBUG("this code path should not have been reached\n");
                return;
            }
            window->state.mode = mode;

            frame->window = NULL;
            if (configuration.auto_remove ||
                    configuration.auto_remove_void) {
                /* do not remove a root */
                if (frame->parent != NULL) {
                    remove_frame(frame);
                    destroy_frame(frame);
                }
            } else if (configuration.auto_fill_void) {
                fill_void_with_stash(frame);
            }
        }

        update_shown_window(window);
    }

    /* update the window states */
    if (mode == WINDOW_MODE_FULLSCREEN) {
        states[0] = ATOM(_NET_WM_STATE_FULLSCREEN);
        states[1] = ATOM(_NET_WM_STATE_MAXIMIZED_HORZ);
        states[2] = ATOM(_NET_WM_STATE_MAXIMIZED_VERT);
        add_window_states(window, states, SIZE(states));
    } else if (window->state.previous_mode == WINDOW_MODE_FULLSCREEN) {
        states[0] = ATOM(_NET_WM_STATE_FULLSCREEN);
        states[1] = ATOM(_NET_WM_STATE_MAXIMIZED_HORZ);
        states[2] = ATOM(_NET_WM_STATE_MAXIMIZED_VERT);
        remove_window_states(window, states, SIZE(states));
    }

    update_window_layer(window);

    synchronize_allowed_actions(window);
}

/* Show the window by mapping it to the X server. */
void show_window(FcWindow *window)
{
    if (window->state.is_visible) {
        return;
    }

    update_shown_window(window);

    window->state.is_visible = true;
}

/* Hide @window and adjust the tiling and focus. */
void hide_window(FcWindow *window)
{
    Frame *frame, *stash;

    if (!window->state.is_visible) {
        return;
    }

    switch (window->state.mode) {
    /* the window is replaced by another window in the tiling layout */
    case WINDOW_MODE_TILING: {
        Frame *pop;

        frame = get_window_frame(window);
        if (frame == NULL) {
            /* code is broken */
            LOG_DEBUG("this code path should not have been reached\n");
            break;
        }

        pop = pop_stashed_frame();

        stash = stash_frame_later(frame);
        if (configuration.auto_remove) {
            /* if the frame is not a root frame, remove it, otherwise
             * auto-fill-void is checked
             */
            if (frame->parent != NULL) {
                remove_frame(frame);
                destroy_frame(frame);
            } else if (configuration.auto_fill_void) {
                if (pop != NULL) {
                    replace_frame(frame, pop);
                }
            }
        } else if (configuration.auto_remove_void) {
            /* this option takes precedence */
            if (configuration.auto_fill_void) {
                if (pop != NULL) {
                    replace_frame(frame, pop);
                }
                /* if the frame is no root and kept on being a void, remove it
                 */
                if (frame->parent != NULL && is_frame_void(frame)) {
                    remove_frame(frame);
                    destroy_frame(frame);
                }
            } else if (frame->parent != NULL) {
                remove_frame(frame);
                destroy_frame(frame);
            }
        } else if (configuration.auto_fill_void) {
            if (pop != NULL) {
                replace_frame(frame, pop);
            }
        }

        /* put `pop` back onto the stack, if it was used it will be empty and
         * therefore not be put onto the stack again but it will in any case be
         * destroyed
         */
        if (pop != NULL) {
            stash_frame(pop);
            destroy_frame(pop);
        }

        link_frame_into_stash(stash);

        /* if nothing is focused, focus the focused frame window */
        if (Window_focus == NULL) {
            set_focus_window(Frame_focus->window);
        }
        break;
    }

    /* need to just focus a different window */
    case WINDOW_MODE_FLOATING:
    case WINDOW_MODE_FULLSCREEN:
    case WINDOW_MODE_DOCK:
    case WINDOW_MODE_DESKTOP:
        set_focus_window(Frame_focus->window);
        break;

    /* not a real window mode */
    case WINDOW_MODE_MAX:
        break;
    }

    window->state.is_visible = false;
}

/* Hide the window without touching the tiling or focus. */
void hide_window_abruptly(FcWindow *window)
{
    /* do nothing if the window is already hidden */
    if (!window->state.is_visible) {
        return;
    }

    window->state.is_visible = false;

    /* make sure there is no invalid focus window */
    if (window == Window_focus) {
        set_focus_window(NULL);
    }
}

/*******************
 * Window stacking *
 *******************/

/* Put all windows above @window that are transient for it. */
void raise_windows_transient_for(FcWindow *window)
{
    FcWindow *other, *next_below;

    for (other = window->below; other != NULL; other = next_below) {
        next_below = other->below;
        if (other->properties.transient_for == window->reference.id) {
            DOUBLY_RELINK_AFTER(Window_bottom, Window_top,
                    other, window, below, above);
        }
    }
}

/* Put the window on the best suited Z stack position. */
void update_window_layer(FcWindow *window)
{
    FcWindow *below = NULL;

    DOUBLY_UNLINK(Window_bottom, Window_top, window, below, above);

    switch (window->state.mode) {
    /* If there are desktop windows, put the window on top of all desktop
     * windows.  Otherwise put it at the bottom.
     */
    case WINDOW_MODE_TILING:
        if (Window_bottom != NULL &&
                Window_bottom->state.mode == WINDOW_MODE_DESKTOP) {
            below = Window_bottom;
            while (below->above != NULL &&
                    below->state.mode == WINDOW_MODE_DESKTOP) {
                below = below->above;
            }
        }
        break;

    /* put the window at the top */
    case WINDOW_MODE_FLOATING:
    case WINDOW_MODE_FULLSCREEN:
    case WINDOW_MODE_DOCK:
        below = Window_top;
        break;

    /* put the window at the bottom */
    case WINDOW_MODE_DESKTOP:
        /* below = NULL */
        break;

    /* not a real window mode */
    case WINDOW_MODE_MAX:
        return;
    }

    DOUBLY_LINK_AFTER(Window_bottom, Window_top, window, below, below, above);

    raise_windows_transient_for(window);
}

/*******************
 * Window focusing *
 *******************/

/* Check if @window accepts input focus. */
bool is_window_focusable(FcWindow *window)
{
    /* if this protocol is supported, we can make use of it */
    if (supports_window_protocol(window, ATOM(WM_TAKE_FOCUS))) {
        return true;
    }

    /* if the client explicitly says it can (or can not) receive focus */
    if ((window->properties.hints.flags & InputHint)) {
        return window->properties.hints.input != 0;
    }

    /* now we enter a weird area where we really can not be sure if this client
     * can handle focus input, we just check for some window modes and otherwise
     * assume it does accept focus
     */

    if (window->state.mode == WINDOW_MODE_DOCK ||
            window->state.mode == WINDOW_MODE_DESKTOP) {
        return false;
    }

    return true;
}

/* Set the window that is in focus to @window. */
void set_focus_window(FcWindow *window)
{
    if (window != NULL) {
        if (!window->state.is_visible) {
            LOG_ERROR("can not focus an invisible window\n");
            window = NULL;
        } else {
            LOG("focusing window %W\n", window);

            if (!is_window_focusable(window)) {
                LOG_ERROR("the window can not be focused\n");
                window = NULL;
            } else if (window == Window_focus) {
                LOG("the window is already focused\n");
            }
        }
    }

    Window_focus = window;
}

/* Focus @window and the frame it is contained in if any. */
void set_focus_window_with_frame(FcWindow *window)
{
    Frame *frame;

    set_focus_window(window);

    frame = get_window_frame(window);
    if (frame != NULL) {
        Frame_focus = frame;
    }
}
