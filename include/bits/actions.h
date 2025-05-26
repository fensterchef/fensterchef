#ifndef BITS__ACTIONS_H
#define BITS__ACTIONS_H

/* This expands to all actions.  Action strings with equal prefix should come
 * after each other for higher efficiency while parsing.
 *
 * Action strings are formatted like this:
 * - The action string consists of words separated by a single space
 * - I is an integer
 * - S is a string
 */
#define DEFINE_ALL_ACTIONS \
    /* no operation */ \
    X(NOP, "nop") \
    /* assign a number to a frame */ \
    X(ASSIGN, "assign I") \
    /* assign a number to a window */ \
    X(ASSIGN_WINDOW, "assign window I") \
    /* automatically equalize the frames when a frame is split or removed */ \
    X(AUTO_EQUALIZE, "auto equalize I") \
    /* automatic filling of voids */ \
    X(AUTO_FILL_VOID, "auto fill void I") \
    /* automatic finding of voids to fill */ \
    X(AUTO_FIND_VOID, "auto find void I") \
    /* automatic removal of windows (implies remove void) */ \
    X(AUTO_REMOVE, "auto remove I") \
    /* automatic removal of voids */ \
    X(AUTO_REMOVE_VOID, "auto remove void I") \
    /* automatic splitting */ \
    X(AUTO_SPLIT, "auto split I") \
    /* the background color of the fensterchef windows */ \
    X(BACKGROUND, "background I") \
    /* the border color of "active" windows */ \
    X(BORDER_COLOR_ACTIVE, "border color active I") \
    /* set the border color of the current window */ \
    X(BORDER_COLOR_CURRENT, "border color current I") \
    /* set the border size of the current window */ \
    X(BORDER_SIZE_CURRENT, "border size current I") \
    /* the border color of focused windows */ \
    X(BORDER_COLOR_FOCUS, "border color focus I") \
    /* the border color of all windows */ \
    X(BORDER_COLOR, "border color I") \
    /* the border size of all windows */ \
    X(BORDER_SIZE, "border size I") \
    /* call an action group by name */ \
    X(CALL, "call S") \
    /* center the window to the monitor it is on */ \
    X(CENTER_WINDOW, "center window") \
    /* center a window to given monitor (glob pattern) */ \
    X(CENTER_WINDOW_TO, "center window to S") \
    /* closes the currently active window */ \
    X(CLOSE_WINDOW, "close window") \
    /* closes the window with given number */ \
    X(CLOSE_WINDOW_I, "close window I") \
    /* set the cursor for horizontal sizing */ \
    X(CURSOR_HORIZONTAL, "cursor horizontal S") \
    /* set the cursor for movement */ \
    X(CURSOR_MOVING, "cursor moving S") \
    /* set the root cursor */ \
    X(CURSOR_ROOT, "cursor root S") \
    /* set the cursor for sizing a corner */ \
    X(CURSOR_SIZING, "cursor sizing S") \
    /* set the cursor for vertical sizing */ \
    X(CURSOR_VERTICAL, "cursor vertical S") \
    /* write all fensterchef information to a file */ \
    X(DUMP_LAYOUT, "dump layout S") \
    /* remove all within a frame but not the frame itself */ \
    X(EMPTY, "empty") \
    /* equalize the size of the child frames within the current frame */ \
    X(EQUALIZE, "equalize") \
    /* exchange the current frame with the below one */ \
    X(EXCHANGE_DOWN, "exchange down") \
    /* exchange the current frame with the left one */ \
    X(EXCHANGE_LEFT, "exchange left") \
    /* exchange the current frame with the right one */ \
    X(EXCHANGE_RIGHT, "exchange right") \
    /* exchange the current frame with the above one */ \
    X(EXCHANGE_UP, "exchange up") \
    /* focus the child of the current frame */ \
    X(FOCUS_CHILD, "focus child") \
    /* focus the ith child of the current frame */ \
    X(FOCUS_CHILD_I, "focus child I") \
    /* focus the frame below */ \
    X(FOCUS_DOWN, "focus down") \
    /* focus the window within the current frame */ \
    X(FOCUS, "focus") \
    /* focus a frame with given number */ \
    X(FOCUS_I, "focus I") \
    /* move the focus to the leaf frame */ \
    X(FOCUS_LEAF, "focus leaf") \
    /* move the focus to the left frame */ \
    X(FOCUS_LEFT, "focus left") \
    /* focus given monitor by name */ \
    X(FOCUS_MONITOR, "focus monitor S") \
    /* move the focus to the parent frame */ \
    X(FOCUS_PARENT, "focus parent") \
    /* move the focus to the ith parent frame */ \
    X(FOCUS_PARENT_I, "focus parent I") \
    /* move the focus to the right frame */ \
    X(FOCUS_RIGHT, "focus right") \
    /* move the focus to the root frame */ \
    X(FOCUS_ROOT, "focus root") \
    /* move the focus to the root frame of given monitor */ \
    X(FOCUS_ROOT_S, "focus root S") \
    /* move the focus to the above frame */ \
    X(FOCUS_UP, "focus up") \
    /* refocus the current window */ \
    X(FOCUS_WINDOW, "focus window") \
    /* focus the window with given number */ \
    X(FOCUS_WINDOW_I, "focus window I") \
    /* the font used for rendering */ \
    X(FONT, "font S") \
    /* the foreground color of the fensterchef windows */ \
    X(FOREGROUND, "foreground I") \
    /* the inner gaps between frames and windows */ \
    X(GAPS_INNER, "gaps inner I") \
    /* set the horizontal and vertical inner gaps */ \
    X(GAPS_INNER_I_I, "gaps inner I I") \
    /* set the left, right, top and bottom inner gaps */ \
    X(GAPS_INNER_I_I_I_I, "gaps inner I I I I") \
    /* the outer gaps between frames and monitors */ \
    X(GAPS_OUTER, "gaps outer I") \
    /* set the horizontal and vertical outer gaps */ \
    X(GAPS_OUTER_I_I, "gaps outer I I") \
    /* set the left, right, top and bottom outer gaps */ \
    X(GAPS_OUTER_I_I_I_I, "gaps outer I I I I") \
    /* hint that the current frame should split horizontally */ \
    X(HINT_SPLIT_HORIZONTALLY, "hint split horizontally") \
    /* hint that the current frame should split vertically */ \
    X(HINT_SPLIT_VERTICALLY, "hint split vertically") \
    /* show an indication on the current frame */ \
    X(INDICATE, "indicate") \
    /* start moving a window with the mouse */ \
    X(INITIATE_MOVE, "initiate move") \
    /* start resizing a window with the mouse */ \
    X(INITIATE_RESIZE, "initiate resize") \
    /* hide currently active window */ \
    X(MINIMIZE_WINDOW, "minimize window") \
    /* hide the window with given number */ \
    X(MINIMIZE_WINDOW_I, "minimize window I") \
    /* the modifiers to ignore */ \
    X(MODIFIERS_IGNORE, "modifiers ignore I") \
    /* move the current frame down */ \
    X(MOVE_DOWN, "move down") \
    /* move the current frame to the left */ \
    X(MOVE_LEFT, "move left") \
    /* move the current frame to the right */ \
    X(MOVE_RIGHT, "move right") \
    /* move the current frame up */ \
    X(MOVE_UP, "move up") \
    /* move the current window by given amount */ \
    X(MOVE_WINDOW_BY, "move window by I I") \
    /* move the position of the current window to given position */ \
    X(MOVE_WINDOW_TO, "move window to I I") \
    /* the duration the notification window stays for */ \
    X(NOTIFICATION_DURATION, "notification duration I") \
    /* the value at which a window should be counted as overlapping a monitor */ \
    X(OVERLAP, "overlap I") \
    /* replace the current frame with a frame from the stash */ \
    X(POP_STASH, "pop stash") \
    /* quit fensterchef */ \
    X(QUIT, "quit") \
    /* reload the configuration file */ \
    X(RELOAD_CONFIGURATION, "reload configuration") \
    /* remove the current frame */ \
    X(REMOVE, "remove") \
    /* remove frame with given number */ \
    X(REMOVE_I, "remove I") \
    /* resize the current window by given values */ \
    X(RESIZE_WINDOW_BY, "resize window by I I") \
    /* resize the current window to given values */ \
    X(RESIZE_WINDOW_TO, "resize window to I I") \
    /* run a shell program */ \
    X(RUN, "run S") \
    /* select the focused window */ \
    X(SELECT_FOCUS, "select focus") \
    /* select the pressed window */ \
    X(SELECT_PRESSED, "select pressed") \
    /* select the window with given number */ \
    X(SELECT_WINDOW, "select window I") \
    /* set the default settings */ \
    X(SET_DEFAULTS, "set defaults") \
    /* set the mode of the current window to floating */ \
    X(SET_FLOATING, "set floating") \
    /* set the mode of the current window to fullscreen */ \
    X(SET_FULLSCREEN, "set fullscreen") \
    /* set the mode of the current window to tiling */ \
    X(SET_TILING, "set tiling") \
    /* show the interactive window list */ \
    X(SHOW_LIST, "show list") \
    /* show a notification with a string message */ \
    X(SHOW_MESSAGE, "show message S") \
    /* go to the next window in the window list */ \
    X(SHOW_NEXT_WINDOW, "show next window") \
    /* go to the ith next window in the window list */ \
    X(SHOW_NEXT_WINDOW_I, "show next window I") \
    /* go to the previous window in the window list */ \
    X(SHOW_PREVIOUS_WINDOW, "show previous window") \
    /* go to the previous window in the window list */ \
    X(SHOW_PREVIOUS_WINDOW_I, "show previous window I") \
    /* show a notification with a message extracted from a shell program */ \
    X(SHOW_RUN, "show run S") \
    /* show the currently active widnow */ \
    X(SHOW_WINDOW, "show window") \
    /* show the window with given number */ \
    X(SHOW_WINDOW_I, "show window I") \
    /* split the current frame horizontally */ \
    X(SPLIT_HORIZONTALLY, "split horizontally") \
    /* split the current frame left horizontally */ \
    X(SPLIT_LEFT_HORIZONTALLY, "split left horizontally") \
    /* split the current frame left vertically */ \
    X(SPLIT_LEFT_VERTICALLY, "split left vertically") \
    /* split the current frame vertically */ \
    X(SPLIT_VERTICALLY, "split vertically") \
    /* the text padding within the fensterchef windows */ \
    X(TEXT_PADDING, "text padding I") \
    /* change the focus from tiling to non tiling or vise versa */ \
    X(TOGGLE_FOCUS, "toggle focus") \
    /* toggles the fullscreen state of the currently focused window */ \
    X(TOGGLE_FULLSCREEN, "toggle fullscreen") \
    /* changes a non tiling window to a tiling window and vise versa */ \
    X(TOGGLE_TILING, "toggle tiling") \
\
    /* Separator action.  The parser has special handling for the below actions.
     * The problem is that big backtracking would be required as all actions
     * start similar.  These here are just declared for covenience when logging.
     */ \
    X(SIMPLE_MAX, "nop") \
    /* a window relation */ \
    X(RELATION, "relate R") \
    /* remove the currently running relation */ \
    X(UNRELATE, "unrelate") \
    /* a button binding */ \
    X(BUTTON_BINDING, "bind B") \
    /* a key binding */ \
    X(KEY_BINDING, "bind K") \
    /* undo a group */ \
    X(UNGROUP, "ungroup S")

#endif
