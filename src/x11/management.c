#include <X11/Xatom.h>

#include <inttypes.h>
#include <string.h>

#include "event.h"
#include "log.h"
#include "fensterchef.h"
#include "notification.h"
#include "window.h"
#include "window_list.h"
#include "x11/display.h"
#include "x11/ewmh.h"

/* Set the input focus to @window. This window may be `NULL`. */
void set_input_focus(FcWindow *window)
{
    Window focus_id = None;
    Window active_id;
    Atom state_atom;

    if (window == NULL) {
        LOG("removed focus from all windows\n");
        active_id = DefaultRootWindow(display);

        focus_id = ewmh_window;
    } else {
        active_id = window->reference.id;

        state_atom = ATOM(_NET_WM_STATE_FOCUSED);
        add_window_states(window, &state_atom, 1);

        /* if the window wants no focus itself */
        if ((window->properties.hints.flags & InputHint) &&
                window->properties.hints.input == 0) {
            XEvent event;

            memset(&event, 0, sizeof(event));
            /* bake an event for running a protocol on the window */
            event.type = ClientMessage;
            event.xclient.window = window->reference.id;
            event.xclient.message_type = ATOM(WM_PROTOCOLS);
            event.xclient.format = 32;
            event.xclient.data.l[0] = ATOM(WM_TAKE_FOCUS);
            event.xclient.data.l[1] = CurrentTime;
            XSendEvent(display, window->reference.id, false,
                    NoEventMask, &event);

            LOG("focusing client: %w through sending WM_TAKE_FOCUS\n",
                    window->reference.id);
        } else {
            focus_id = window->reference.id;
        }
    }

    if (focus_id != None) {
        LOG("focusing client: %w\n", focus_id);
        XSetInputFocus(display, focus_id, RevertToParent, CurrentTime);
    }

    /* set the active window root property */
    XChangeProperty(display, DefaultRootWindow(display),
            ATOM(_NET_ACTIVE_WINDOW), XA_WINDOW, 32, PropModeReplace,
            (unsigned char*) &active_id, 1);
}
