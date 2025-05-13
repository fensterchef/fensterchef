#ifndef WINDOW_LIST_H
#define WINDOW_LIST_H

#include <X11/X.h>

#include "font.h"
#include "x11/synchronize.h"

/* user window list window */
extern struct window_list {
    /* the X correspondence */
    XReference reference;
    /* Xft drawing context */
    XftDraw *xft_draw;
    /* the currently selected window index */
    unsigned selected;
    /* the currently scrolled amount in pixels */
    int scrolling;
} WindowList;

/* Handle an incoming X event for the window list. */
void handle_window_list_event(XEvent *event);

/* Show the window list on screen or hide if it is already shown.
 *
 * The user can now select a window and it will be shown on screen.
 *
 * @return ERROR if the window list can not be shown, OK otherwise.
 */
int show_window_list(void);

#endif
