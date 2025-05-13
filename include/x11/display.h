#ifndef X11__DISPLAY_H
#define X11__DISPLAY_H

/**
 * The connection to X11 and a little bit of utility to get us started.
 */

#include <X11/Xlib.h>

/* connection to the X server */
extern Display *display;

/* first index of an xkb event */
extern int xkb_event_base, xkb_error_base;

/* first index of a randr event */
extern int randr_event_base, randr_error_base;

/* Open the X11 display (X server connection). */
void open_connection(void);

/* Try to take control of the window manager role. */
void take_control(void);

/* Go through all existing windows and manage them. */
void query_existing_windows(void);

/* Set the initial root window properties. */
void initialize_root_properties(void);

#endif
