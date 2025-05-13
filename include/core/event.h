#ifndef X11__EVENT_H
#define X11__EVENT_H

/**
 * This handles all kinds of X events.
 *
 * Note the difference between REQUESTS and NOTIFICATIONS.
 * REQUEST: What is requested has not happened yet and will not happen
 * until the window manager does something.
 *
 * NOTIFICATIONS: What is notified ALREADY happened, there is nothing
 * to do now but to take note of it.
 */

#include <X11/Xlib.h>

#include "bits/window.h"
#include "x11/ewmh.h"

/* Create a signal handler for `SIGALRM`. */
void initialize_signal_handlers(void);

/* Runs the next cycle of the event loop. This handles signals and all events
 * that are currently queued.
 *
 * It also delegates events to the window list if it is mapped.
 */
int next_cycle(void);

/* Run the main event loop that handles all X events. */
void run_event_loop(void);

/* Handle the given X event. */
void handle_event(XEvent *event);

#endif
