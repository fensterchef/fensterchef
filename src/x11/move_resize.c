#include "cursor.h"
#include "frame.h"
#include "log.h"
#include "window.h"
#include "x11/display.h"

/* this is used for moving/resizing a floating window */
static struct {
    /* the window that is being moved */
    FcWindow *window;
    /* how to move or resize the window */
    wm_move_resize_direction_t direction;
    /* initial position and size of the window */
    Rectangle initial_geometry;
    /* the initial position of the mouse */
    Point start;
} move_resize;

/* Start moving/resizing given window. */
bool initiate_window_move_resize(FcWindow *window,
        wm_move_resize_direction_t direction,
        int start_x, int start_y)
{
    Window root;
    Cursor cursor;

    /* check if no window is already being moved/resized */
    if (move_resize.window != NULL) {
        return false;
    }

    /* only allow sizing/moving of floating and tiling windows */
    if (window->state.mode != WINDOW_MODE_FLOATING &&
            window->state.mode != WINDOW_MODE_TILING) {
        return false;
    }

    LOG("starting to move/resize %W\n", window);

    root = DefaultRootWindow(display);

    /* get the mouse position if the caller does not supply it */
    if (start_x < 0) {
        Window other_root;
        Window child;
        int window_x;
        int window_y;
        unsigned int mask;

        XQueryPointer(display, root, &other_root, &child,
                &start_x, &start_y, /* root_x, root_y */
                &window_x, &window_y, &mask);
    }

    /* figure out a good direction if the caller does not supply one */
    if (direction == _NET_WM_MOVERESIZE_AUTO) {
        const int resize_tolerance = MAX(WINDOW_RESIZE_TOLERANCE,
                window->border_size);
        /* check if the mouse is at the top */
        if (start_y < window->y + resize_tolerance) {
            if (start_x < window->x + resize_tolerance) {
                direction = _NET_WM_MOVERESIZE_SIZE_TOPLEFT;
            } else if (start_x - (int) window->width >=
                    window->x - resize_tolerance) {
                direction = _NET_WM_MOVERESIZE_SIZE_TOPRIGHT;
            } else {
                direction = _NET_WM_MOVERESIZE_SIZE_TOP;
            }
        /* check if the mouse is at the bottom */
        } else if (start_y - (int) window->height >=
                window->y - resize_tolerance) {
            if (start_x < window->x + resize_tolerance) {
                direction = _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT;
            } else if (start_x - (int) window->width >=
                    window->x - resize_tolerance) {
                direction = _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT;
            } else {
                direction = _NET_WM_MOVERESIZE_SIZE_BOTTOM;
            }
        /* check if the mouse is left */
        } else if (start_x < window->x + resize_tolerance) {
            direction = _NET_WM_MOVERESIZE_SIZE_LEFT;
        /* check if the mouse is right */
        } else if (start_x - (int) window->width >=
                window->x - resize_tolerance) {
            direction = _NET_WM_MOVERESIZE_SIZE_RIGHT;
        /* fall back to simply moving (the mouse is within the window) */
        } else {
            direction = _NET_WM_MOVERESIZE_MOVE;
        }
    }

    move_resize.window = window;
    move_resize.direction = direction;
    move_resize.initial_geometry.x = window->x;
    move_resize.initial_geometry.y = window->y;
    move_resize.initial_geometry.width = window->width;
    move_resize.initial_geometry.height = window->height;
    move_resize.start.x = start_x;
    move_resize.start.y = start_y;

    /* determine a fitting cursor */
    switch (direction) {
    case _NET_WM_MOVERESIZE_MOVE:
        cursor = load_cursor(CURSOR_MOVING, NULL);
        break;

    case _NET_WM_MOVERESIZE_SIZE_LEFT:
    case _NET_WM_MOVERESIZE_SIZE_RIGHT:
        cursor = load_cursor(CURSOR_HORIZONTAL, NULL);
        break;

    case _NET_WM_MOVERESIZE_SIZE_TOP:
    case _NET_WM_MOVERESIZE_SIZE_BOTTOM:
        cursor = load_cursor(CURSOR_VERTICAL, NULL);
        break;

    default:
        cursor = load_cursor(CURSOR_SIZING, NULL);
    }

    /* grab mouse events, we will then receive all mouse events */
    XGrabPointer(display, root, False,
            ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
            GrabModeAsync, GrabModeAsync, root, cursor, CurrentTime);

    return true;
}

/* Finish a window moving/resizing. */
bool finish_window_move_resize(void)
{
    if (move_resize.window == NULL) {
        return false;
    }

    /* release mouse events back to the applications */
    XUngrabPointer(display, CurrentTime);
    move_resize.window = NULL;

    return true;
}

/* Finish moving/resizing if the given window is the one being moved/resized. */
bool finish_window_move_resize_for(FcWindow *window)
{
    if (window == move_resize.window) {
        /* release mouse events back to the applications */
        XUngrabPointer(display, CurrentTime);

        move_resize.window = NULL;
        return true;
    } else {
        return false;
    }
}

/* Reset the position of the window being moved/resized. */
bool cancel_window_move_resize(void)
{
    Frame *frame;

    /* make sure a window is currently being moved/resized */
    if (move_resize.window == NULL) {
        return false;
    }

    LOG("cancelling move/resize for %W\n", move_resize.window);

    /* restore the old position and size as good as we can */
    frame = get_window_frame(move_resize.window);
    if (frame != NULL) {
        bump_frame_edge(frame, FRAME_EDGE_LEFT,
                move_resize.window->x - move_resize.initial_geometry.x);
        bump_frame_edge(frame, FRAME_EDGE_TOP,
                move_resize.window->y - move_resize.initial_geometry.y);
        bump_frame_edge(frame, FRAME_EDGE_RIGHT,
                (move_resize.initial_geometry.x +
                 move_resize.initial_geometry.width) -
                    (move_resize.window->x + move_resize.window->width));
        bump_frame_edge(frame, FRAME_EDGE_BOTTOM,
                (move_resize.initial_geometry.y +
                 move_resize.initial_geometry.height) -
                    (move_resize.window->y + move_resize.window->height));
    } else {
        set_window_size(move_resize.window,
                move_resize.initial_geometry.x,
                move_resize.initial_geometry.y,
                move_resize.initial_geometry.width,
                move_resize.initial_geometry.height);
    }

    /* release mouse events back to the applications */
    XUngrabPointer(display, CurrentTime);

    move_resize.window = NULL;

    return true;
}

/* Handle a motion notify event from the X server. */
bool handle_window_move_resize_motion(XMotionEvent *event)
{
    Rectangle new_geometry;
    Size minimum, maximum;
    int delta_x, delta_y;
    int left_delta, top_delta, right_delta, bottom_delta;
    Frame *frame;

    if (move_resize.window == NULL) {
        return false;
    }

    new_geometry = move_resize.initial_geometry;

    get_minimum_window_size(move_resize.window, &minimum);
    get_maximum_window_size(move_resize.window, &maximum);

    delta_x = move_resize.start.x - event->x_root;
    delta_y = move_resize.start.y - event->y_root;

    /* prevent overflows and clip so that moving an edge when no more size is
     * available does not move the window
     */
    if (delta_x < 0) {
        left_delta = -MIN((unsigned) -delta_x,
                new_geometry.width - minimum.width);
    } else {
        left_delta = MIN((unsigned) delta_x,
                maximum.width - new_geometry.width);
    }
    if (delta_y < 0) {
        top_delta = -MIN((unsigned) -delta_y,
                new_geometry.height - minimum.height);
    } else {
        top_delta = MIN((unsigned) delta_y,
                maximum.height - new_geometry.height);
    }

    /* prevent overflows */
    if (delta_x > 0) {
        right_delta = MIN((unsigned) delta_x, new_geometry.width);
    } else {
        right_delta = delta_x;
    }
    if (delta_y > 0) {
        bottom_delta = MIN((unsigned) delta_y, new_geometry.height);
    } else {
        bottom_delta = delta_y;
    }

    switch (move_resize.direction) {
    /* the top left corner of the window is being moved */
    case _NET_WM_MOVERESIZE_SIZE_TOPLEFT:
        new_geometry.x -= left_delta;
        new_geometry.width += left_delta;
        new_geometry.y -= top_delta;
        new_geometry.height += top_delta;
        break;

    /* the top size of the window is being moved */
    case _NET_WM_MOVERESIZE_SIZE_TOP:
        new_geometry.y -= top_delta;
        new_geometry.height += top_delta;
        break;

    /* the top right corner of the window is being moved */
    case _NET_WM_MOVERESIZE_SIZE_TOPRIGHT:
        new_geometry.width -= right_delta;
        new_geometry.y -= top_delta;
        new_geometry.height += top_delta;
        break;

    /* the right side of the window is being moved */
    case _NET_WM_MOVERESIZE_SIZE_RIGHT:
        new_geometry.width -= right_delta;
        break;

    /* the bottom right corner of the window is being moved */
    case _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT:
        new_geometry.width -= right_delta;
        new_geometry.height -= bottom_delta;
        break;

    /* the bottom side of the window is being moved */
    case _NET_WM_MOVERESIZE_SIZE_BOTTOM:
        new_geometry.height -= bottom_delta;
        break;

    /* the bottom left corner of the window is being moved */
    case _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT:
        new_geometry.x -= left_delta;
        new_geometry.width += left_delta;
        new_geometry.height -= bottom_delta;
        break;

    /* the bottom left corner of the window is being moved */
    case _NET_WM_MOVERESIZE_SIZE_LEFT:
        new_geometry.x -= left_delta;
        new_geometry.width += left_delta;
        break;

    /* the entire window is being moved */
    case _NET_WM_MOVERESIZE_MOVE:
        new_geometry.x -= delta_x;
        new_geometry.y -= delta_y;
        break;

    /* these are not relevant for mouse moving/resizing */
    case _NET_WM_MOVERESIZE_SIZE_KEYBOARD:
    case _NET_WM_MOVERESIZE_MOVE_KEYBOARD:
    case _NET_WM_MOVERESIZE_CANCEL:
    case _NET_WM_MOVERESIZE_AUTO:
        break;
    }

    frame = get_window_frame(move_resize.window);
    if (frame != NULL) {
        bump_frame_edge(frame, FRAME_EDGE_LEFT,
                move_resize.window->x - new_geometry.x);
        bump_frame_edge(frame, FRAME_EDGE_TOP,
                move_resize.window->y - new_geometry.y);
        bump_frame_edge(frame, FRAME_EDGE_RIGHT,
                (new_geometry.x + new_geometry.width) -
                (move_resize.window->x + move_resize.window->width));
        bump_frame_edge(frame, FRAME_EDGE_BOTTOM,
                (new_geometry.y + new_geometry.height) -
                (move_resize.window->y + move_resize.window->height));
    } else {
        set_window_size(move_resize.window,
                new_geometry.x,
                new_geometry.y,
                new_geometry.width,
                new_geometry.height);
    }

    return true;
}
