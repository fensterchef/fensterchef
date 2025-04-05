#ifndef EVENT_H
#define EVENT_H

#include "bits/window_typedef.h"

#include "x11_management.h"

/* first index of an xkb event/error */
extern int xkb_event_base, xkb_error_base;

/* this is the first index of a randr event/error */
extern int randr_event_base, randr_error_base;

/* if the user requested to reload the configuration */
extern bool is_reload_requested;

/* if the client list has changed (if stacking changed, windows were removed or
 * added)
 */
extern bool has_client_list_changed;

/* Create a signal handler for `SIGALRM`. */
void initialize_signal_handlers(void);

/* Set the client list root property. */
void synchronize_client_list(void);

/* Synchronize the local data with the X server. */
void synchronize_with_server(void);

/* Runs the next cycle of the event loop. This handles signals and all events
 * that are currently queued.
 *
 * It also delegates events to the window list if it is mapped.
 */
int next_cycle(void);

/* Start resizing a window using the mouse. */
void initiate_window_move_resize(FcWindow *window,
        wm_move_resize_direction_t direction,
        int start_x, int start_y);

/* Handle the given X event. */
void handle_event(XEvent *event);

#endif
