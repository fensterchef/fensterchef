#ifndef X11__EWMH_H
#define X11__EWMH_H

/**
 * Implementation of the EWMH specification.
 *
 * This also includes additional atoms and properties as well as utility to get
 * those properties.
 *
 * See: https://specifications.freedesktop.org/wm-spec/latest/index.html
 */

#include <X11/X.h>

#include <stdbool.h>

#include "bits/atoms.h"
#include "utility/attributes.h"
#include "utility/types.h"

/* get the atom identifier from an atom constant */
#define ATOM(id) (x_atom_ids[id])

/* constant list expansion of all atoms */
enum {
#define X(atom) atom,
    DEFINE_ALL_ATOMS
#undef X
    ATOM_MAX
};

/* all X atom names */
extern const char *x_atom_names[ATOM_MAX];

/* all X atom ids */
extern Atom x_atom_ids[ATOM_MAX];

/* the window for _NET_SUPPORTING_WM_CHECK */
extern Window ewmh_window;

/* needed for _NET_WM_STRUT_PARTIAL/_NET_WM_STRUT */
typedef struct wm_strut_partial {
    /* reserved space on left border */
    int left;
    /* reserved space on right border */
    int right;
    /* reserved space on top border */
    int top;
    /* reserved space on bottom border */
    int bottom;
    /* beginning y coordinate of the left strut */
    int left_start_y;
    /* ending y coordinate of the left strut */
    int left_end_y;
    /* beginning y coordinate of the right strut */
    int right_start_y;
    /* ending y coordinate of the right strut */
    int right_end_y;
    /* beginning x coordinate of the top strut */
    int top_start_x;
    /* ending x coordinate of the top strut */
    int top_end_x;
    /* beginning x coordinate of the bottom strut */
    int bottom_start_x;
    /* ending x coordinate of the bottom strut */
    int bottom_end_x;
} wm_strut_partial_t;

/* _NET_WM_MOVERESIZE window movement or resizing */
typedef enum {
    /* resizing applied on the top left edge */
    _NET_WM_MOVERESIZE_SIZE_TOPLEFT = 0,
    /* resizing applied on the top edge */
    _NET_WM_MOVERESIZE_SIZE_TOP = 1,
    /* resizing applied on the top right edge */
    _NET_WM_MOVERESIZE_SIZE_TOPRIGHT = 2,
    /* resizing applied on the right edge */
    _NET_WM_MOVERESIZE_SIZE_RIGHT = 3,
    /* resizing applied on the bottom right edge */
    _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT = 4,
    /* resizing applied on the bottom edge */
    _NET_WM_MOVERESIZE_SIZE_BOTTOM = 5,
    /* resizing applied on the bottom left edge */
    _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT = 6,
    /* resizing applied on the left edge */
    _NET_WM_MOVERESIZE_SIZE_LEFT = 7,
    /* movement only */
    _NET_WM_MOVERESIZE_MOVE = 8,
    /* size via keyboard */
    _NET_WM_MOVERESIZE_SIZE_KEYBOARD = 9,
    /* move via keyboard */
    _NET_WM_MOVERESIZE_MOVE_KEYBOARD = 10,
    /* cancel operation */
    _NET_WM_MOVERESIZE_CANCEL = 11,
    /* automatically figure out a good direction */
    _NET_WM_MOVERESIZE_AUTO,
} wm_move_resize_direction_t;

/* _NET_WM_STATE state change */
/* a state should be removed */
#define _NET_WM_STATE_REMOVE 0
/* a state should be added */
#define _NET_WM_STATE_ADD 1
/* a state should be toggled (removed if it exists and added otherwise) */
#define _NET_WM_STATE_TOGGLE 2

#define MOTIF_WM_HINTS_FLAGS_DECORATIONS (1 << 1)
#define MOTIF_WM_HINTS_DECORATIONS_ALL ((1 << 0))
#define MOTIF_WM_HINTS_DECORATIONS_BORDER ((1 << 2) | (1 << 3))

/* hints of the motif window manager which many applications still use */
struct motif_wm_hints {
    /* what fields below are available */
    int flags;
    /* IGNORED */
    int functions;
    /* border/frame decoration flags */
    int decorations;
    /* IGNORED */
    int input_mode;
    /* IGNORED */
    int status;
};

/* Create the wm check window.
 *
 * This can be used by other applications to identify our window manager.
 * We also use it as fallback focus for convenience.
 */
Window create_ewmh_window(void);

/* Check if given strut has any reserved space. */
static inline bool is_strut_empty(wm_strut_partial_t *strut)
{
    return strut->left == 0 && strut->top == 0 &&
        strut->right == 0 && strut->bottom == 0;
}

/* Get a long window property. */
long *get_long_property(Window window, Atom property,
        unsigned long expected_item_count);

/* Get a text window property.
 *
 * @return a string thay may not be nul-terminated.
 */
char *get_text_property(Window window, Atom property,
        _Out unsigned long *length);

/* Get a window property as list of atoms.
 *
 * @return NULL if the property does not exist on given window or
 *         an atom list terminated by a None atom.
 */
Atom *get_atom_list_property(Window window, Atom property);

/* Check if an atom is within the given list of atoms. */
bool is_atom_included(_Nullable const Atom *atoms, Atom atom);

/* Get the _NET_WM_NAME or WM_NAME window property.
 *
 * @return NULL if the window has no name property,
 *         otherwise the allocated name.
 */
char *get_window_name_property(Window window);

/* Get the WM_PROTOCOLS window property.
 *
 * Window @window
 *
 * Atom* @return NULL or an atom list.
 */
#define get_protocols_property(window) \
    get_atom_list_property(window, ATOM(WM_PROTOCOLS))

/* Get the _NET_WM_FULLSCREEN_MONITORS window property.
 *
 * @return if the window has this property.
 */
bool get_fullscreen_monitors_property(Window window,
        _Out Extents *fullscreen_monitors);

/* Get the _NET_WM_STRUT_PARTIAL or _NET_WM_STRUT window property.
 *
 * @return if the window has any strut properties.
 */
bool get_strut_property(Window window, _Out wm_strut_partial_t *strut);

/* Get the _MOTIF_WM_HINTS window property.
 *
 * @return if the window has this property.
 */
bool get_motif_wm_hints_property(Window window,
        _Out struct motif_wm_hints *hints);

/* Get the FENSTERCHEF_COMMAND window property.
 *
 * @return NULL if the property is not set on given window.
 */
char *get_fensterchef_command_property(Window window);

/* Send a WM_TAKE_FOCUS client message to given window.
 *
 * The caller should have made sure that the window supports this protocol,
 * otherwise nothing happens.
 */
void send_take_focus_message(Window window);

/* Send a WM_DELETE_WINDOW client message to given window.
 *
 * The caller should have made sure that the window supports this protocol,
 * otherwise nothing happens.
 */
void send_delete_window_message(Window window);

#endif
