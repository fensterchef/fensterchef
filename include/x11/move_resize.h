#ifndef X11__MOVE_RESIZE_H
#define X11__MOVE_RESIZE_H

#include <stdbool.h>

#include "bits/window.h"
#include "x11/ewmh.h"

/**
 * Resize or move a window with the mouse.
 */

/* Start resizing a window using the mouse.
 *
 * @return if the move/resize could not be initialized.
 *
 * This happens for example when there is already a window being resized or the
 * given window is not resizable.
 */
bool initiate_window_move_resize(FcWindow *window,
        wm_move_resize_direction_t direction,
        int start_x, int start_y);

/* Finish the moving/resizing.
 *
 * @return if there was any moving/resizing.
 */
bool finish_window_move_resize(void);

/* Finish moving/resizing if the given window is the one being moved/resized.
 *
 * @return if the window was actually being moved/resized.
 */
bool finish_window_move_resize_for(FcWindow *window);

/* Reset the position of the window being moved/resized.
 *
 * @return if there was any moving/resizing.
 */
bool cancel_window_move_resize(void);

/* Handle a motion notify event from the X server. */
bool handle_window_move_resize_motion(XMotionEvent *event);

#endif
