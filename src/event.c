#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include <X11/XKBlib.h>
#include <X11/extensions/Xrandr.h>

#include "binding.h"
#include "event.h"
#include "fensterchef.h"
#include "frame.h"
#include "log.h"
#include "notification.h"
#include "window.h"
#include "window_list.h"
#include "x11/display.h"
#include "x11/ewmh.h"
#include "x11/move_resize.h"

/* signals whether the alarm signal was received */
volatile sig_atomic_t has_timer_expired;

/* Handle an incoming alarm. */
static void alarm_handler(int signal_number)
{
    has_timer_expired = true;
    /* make sure the signal handler sticks around and is not deinstalled */
    signal(signal_number, alarm_handler);
}

/* Create a signal handler for `SIGALRM`. */
void initialize_signal_handlers(void)
{
    signal(SIGALRM, alarm_handler);
}


/* Select will block until data on the file descriptor for the X display
 * arrives.  When a signal is received, `select()` will however also unblock and
 * return -1.
 *
 * @return -1 if a signal disrupted the waiting.
 */
static int wait_for_file_descriptor(void)
{
    int file_descriptor;
    fd_set set;

    FD_ZERO(&set);
    file_descriptor = ConnectionNumber(display);
    FD_SET(file_descriptor, &set);

    return select(file_descriptor + 1, &set, NULL, NULL, NULL);
}

/* Run the next cycle of the event loop. */
int next_cycle(void)
{
    FcWindow *old_focus_window;
    Frame *old_focus_frame;
    XEvent event;

    /* signal to stop running */
    if (!Fensterchef_is_running) {
        return ERROR;
    }

    /* save the old focus for comparison (checking if the focus changed) */
    old_focus_window = Window_focus;
    old_focus_frame = Frame_focus;
    /* make sure the pointers do not get freed */
    if (old_focus_window != NULL) {
        reference_window(old_focus_window);
    }
    if (old_focus_frame != NULL) {
        reference_frame(old_focus_frame);
    }

    /* The additional check for `XPending()` is needed because events might have
     * already been put into the local buffer.  This means there might not be
     * anything in the file descriptor but there still might be events.
     */
    if (XPending(display) || wait_for_file_descriptor() > 0) {
        /* handle all received events */
        while (XPending(display)) {
            XNextEvent(display, &event);

            handle_window_list_event(&event);

            handle_event(&event);
        }

        synchronize_with_server();

        /* show the current frame indicator if needed */
        if (old_focus_frame != Frame_focus ||
                (Frame_focus->window == Window_focus &&
                    old_focus_window != Window_focus)) {
            /* Only indicate the focused frame if one of these is true:
             * - The frame has no inner window
             * - The inner window has no border
             * - The focused window is a non tiling window
             */
            if (Frame_focus->window == NULL ||
                     Frame_focus->window->border_size == 0 ||
                     (Window_focus != NULL &&
                        Window_focus->state.mode != WINDOW_MODE_TILING)) {
                char number[MAXIMUM_DIGITS(Frame_focus->number) + 1];

                snprintf(number, sizeof(number), "%u",
                        Frame_focus->number);
                set_system_notification(Frame_focus->number > 0 ? number :
                            Frame_focus->left == NULL ?  "Current frame" :
                            "Current frames",
                        Frame_focus->x + Frame_focus->width / 2,
                        Frame_focus->y + Frame_focus->height / 2);
            }
            LOG("frame %F was focused\n", Frame_focus);
        }
    }

    if (has_timer_expired && system_notification != NULL) {
        unmap_client(&system_notification->reference);
        has_timer_expired = false;
    }

    /* no longer need them */
    if (old_focus_frame != NULL) {
        dereference_frame(old_focus_frame);
    }
    if (old_focus_window != NULL) {
        dereference_window(old_focus_window);
    }

    /* flush after every series of events so all changes are reflected */
    XFlush(display);

    return OK;
}

/* Run the main event loop that handles all X events. */
void run_event_loop(void)
{
    while (next_cycle() == OK) {
        /* nothing */
    }
    quit_fensterchef(Fensterchef_is_running ? EXIT_FAILURE : EXIT_SUCCESS);
}

/* Key press events are sent when a grabbed key is pressed. */
static void handle_key_press(XKeyPressedEvent *event)
{
    if (system_notification != NULL) {
        alarm(0);
        unmap_client(&system_notification->reference);
    }

    Window_pressed = Window_focus;
    Window_selected = Window_pressed;
    run_key_binding(false, event->state, event->keycode);
}

/* Key release events are sent when a grabbed key is released. */
static void handle_key_release(XKeyReleasedEvent *event)
{
    Window_pressed = Window_focus;
    Window_selected = Window_pressed;
    run_key_binding(true, event->state, event->keycode);
}

/* Button press events are sent when a grabbed button is pressed. */
static void handle_button_press(XButtonPressedEvent *event)
{
    if (cancel_window_move_resize()) {
        /* use this event only for stopping the resize */
        return;
    }

    /* set the pressed window so actions can use it */
    Window_pressed = get_fensterchef_window(event->window);
    Window_selected = Window_pressed;
    run_button_binding(event->time, false, event->state, event->button);
}

/* Button releases are sent when a grabbed button is released. */
static void handle_button_release(XButtonReleasedEvent *event)
{
    if (finish_window_move_resize()) {
        /* use this event only for finishing the resize */
        return;
    }

    /* set the pressed window so actions can use it */
    Window_pressed = get_fensterchef_window(event->window);
    Window_selected = Window_pressed;
    run_button_binding(event->time, true, event->state, event->button);
}

/* Motion notifications (mouse move events) are only sent when we grabbed them.
 * This only happens when a floating window is being moved.
 */
static void handle_motion_notify(XMotionEvent *event)
{
    if (!handle_window_move_resize_motion(event)) {
        LOG_ERROR("receiving motion events without a window to move?\n");
    }
}

/* Unmap notifications are sent after a window decided it wanted to not be seen
 * anymore.
 */
static void handle_unmap_notify(XUnmapEvent *event)
{
    FcWindow *window;

    window = get_fensterchef_window(event->window);
    if (window == NULL) {
        return;
    }

    window->reference.is_mapped = false;

    if (finish_window_move_resize_for(window)) {
        LOG("window that was moved/resized was unmapped\n");
    }

    hide_window(window);
}

/* Map requests are sent when a new window wants to become on screen, this
 * is also where we register new windows and wrap them into the internal
 * `FcWindow` struct.
 */
static void handle_map_request(XMapRequestEvent *event)
{
    FcWindow *window;

    window = get_fensterchef_window(event->window);
    if (window == NULL) {
        window = create_window(event->window);
        if (window == NULL) {
            LOG("not managing %w\n", event->window);
            return;
        }
    }
}

/* Destroy notifications are sent when a window leaves the X server.
 * Good bye to that window!
 */
static void handle_destroy_notify(XDestroyWindowEvent *event)
{
    FcWindow *window;

    window = get_fensterchef_window(event->window);
    if (window != NULL) {
        destroy_window(window);
    }
}

/* Property notifications are sent when a window property changes. */
static void handle_property_notify(XPropertyEvent *event)
{
    FcWindow *window;

    window = get_fensterchef_window(event->window);
    if (window == NULL) {
        return;
    }
    cache_window_property(window, event->atom);
}

/* Configure requests are received when a window wants to choose its own
 * position and size. We allow this for unmanaged windows.
 */
static void handle_configure_request(XConfigureRequestEvent *event)
{
    XWindowChanges changes;
    FcWindow *window;

    window = get_fensterchef_window(event->window);
    if (window != NULL) {
        XEvent notify_event;

        /* Fake a configure notify.  This is needed for many windows to work,
         * otherwise they get in a bugged state.  This could have various
         * reasons.  All this because we are a *tiling* window manager.
         */
        memset(&notify_event, 0, sizeof(notify_event));
        notify_event.type = ConfigureNotify;
        notify_event.xconfigure.event = window->reference.id;
        notify_event.xconfigure.window = window->reference.id;
        notify_event.xconfigure.x = window->x;
        notify_event.xconfigure.y = window->y;
        notify_event.xconfigure.width = window->width;
        notify_event.xconfigure.height = window->height;
        notify_event.xconfigure.border_width = is_window_borderless(window) ?
            0 : window->border_size;
        LOG("sending fake configure notify event %V to %W\n",
                &notify_event, window);
        XSendEvent(display, window->reference.id, false,
                StructureNotifyMask, &notify_event);
        return;
    }

    if ((event->value_mask & CWX)) {
        changes.x = event->x;
    }
    if ((event->value_mask & CWY)) {
        changes.y = event->y;
    }
    if ((event->value_mask & CWWidth)) {
        changes.width = event->width;
    }
    if ((event->value_mask & CWHeight)) {
        changes.height = event->height;
    }
    if ((event->value_mask & CWBorderWidth)) {
        changes.border_width = event->border_width;
    }
    if ((event->value_mask & CWSibling)) {
        changes.sibling = event->above;
    }
    if ((event->value_mask & CWStackMode)) {
        changes.stack_mode = event->detail;
    }

    LOG("configuring unmanaged window %w\n",
            event->window);
    XConfigureWindow(display, event->window, event->value_mask, &changes);
}

/* Client messages are sent by a client to our window manager to request certain
 * things.
 */
static void handle_client_message(XClientMessageEvent *event)
{
    FcWindow *window;
    int x, y;
    unsigned width, height;

    window = get_fensterchef_window(event->window);
    if (window == NULL) {
        return;
    }

    /* ignore misformatted client messages */
    if (event->format != 32) {
        return;
    }

    /* request to close the window */
    if (event->message_type == ATOM(_NET_CLOSE_WINDOW)) {
        close_window(window);
    /* request to move and resize the window */
    } else if (event->message_type == ATOM(_NET_MOVERESIZE_WINDOW)) {
        x = event->data.l[1];
        y = event->data.l[2];
        width = event->data.l[3];
        height = event->data.l[4];
        adjust_for_window_gravity(
                get_monitor_from_rectangle_or_primary(x, y, width, height),
                &x, &y, width, height, event->data.l[0]);
        set_window_size(window, x, y, width, height);
    /* request to dynamically move and resize the window */
    } else if (event->message_type == ATOM(_NET_WM_MOVERESIZE)) {
        if (event->data.l[2] == _NET_WM_MOVERESIZE_CANCEL) {
            cancel_window_move_resize();
            return;
        }
        initiate_window_move_resize(window, event->data.l[2],
                event->data.l[0], event->data.l[1]);
    /* a window wants to be iconified or put into normal state (shown) */
    /* TODO: make this configurable */
    } else if (event->message_type == ATOM(WM_CHANGE_STATE)) {
        switch (event->data.l[0]) {
        /* hide the window */
        case IconicState:
        case WithdrawnState:
            hide_window(window);
            break;

        /* make the window normal again (show it) */
        case NormalState:
            show_window(window);
            break;
        }
    /* a window wants to change a window state */
    /* TODO: make the removing of states configurable */
    } else if (event->message_type == ATOM(_NET_WM_STATE)) {
        const Atom atom = event->data.l[1];
        if (atom == ATOM(_NET_WM_STATE_ABOVE)) {
            switch (event->data.l[0]) {
            /* only react to adding */
            case _NET_WM_STATE_ADD:
                update_window_layer(window);
                break;
            }
        } else if (atom == ATOM(_NET_WM_STATE_FULLSCREEN) ||
                atom == ATOM(_NET_WM_STATE_MAXIMIZED_HORZ) ||
                atom == ATOM(_NET_WM_STATE_MAXIMIZED_VERT)) {
            switch (event->data.l[0]) {
            /* put the window out of fullscreen */
            case _NET_WM_STATE_REMOVE:
                set_window_mode(window, window->state.previous_mode);
                break;

            /* put the window into fullscreen */
            case _NET_WM_STATE_ADD:
                set_window_mode(window, WINDOW_MODE_FULLSCREEN);
                break;

            /* toggle the fullscreen state */
            case _NET_WM_STATE_TOGGLE:
                set_window_mode(window,
                        window->state.mode == WINDOW_MODE_FULLSCREEN ?
                        (window->state.previous_mode == WINDOW_MODE_FULLSCREEN ?
                            WINDOW_MODE_FLOATING :
                            window->state.previous_mode) :
                        WINDOW_MODE_FULLSCREEN);
                break;
            }
        }
    }
    /* TODO: handle _NET_REQUEST_FRAME_EXTENTS and _NET_RESTACK_WINDOW */
}

/* Handle the given X event.
 *
 * Descriptions for all events are above each handler.
 */
void handle_event(XEvent *event)
{
    if (event->type == xkb_event_base) {
        LOG("%V\n", event);
        switch (((XkbAnyEvent*) event)->xkb_type) {
        /* the keyboard mapping changed */
        case XkbNewKeyboardNotify:
        case XkbMapNotify:
            XkbRefreshKeyboardMapping((XkbMapNotifyEvent*) event);
            resolve_all_key_symbols();
            break;
        }
        return;
    }

    if (event->type == randr_event_base) {
        LOG("%V\n", event);
        /* Screen change notifications are sent when the screen configuration is
         * changed.  This includes position, size etc.
         */
        XRRUpdateConfiguration(event);
        merge_monitors(query_monitors());
        return;
    }

    /* log these events as verbose because they are not helpful */
    if (event->type == MotionNotify || (event->type == ClientMessage &&
                event->xclient.message_type == ATOM(_NET_WM_USER_TIME))) {
        LOG_VERBOSE("%V\n", event);
    } else {
        LOG("%V\n", event);
    }

    switch (event->type) {
    /* a key was pressed */
    case KeyPress:
        handle_key_press(&event->xkey);
        break;

    /* a key was released */
    case KeyRelease:
        handle_key_release(&event->xkey);
        break;

    /* a mouse button was pressed */
    case ButtonPress:
        handle_button_press(&event->xbutton);
        /* Continue processing pointer events normally.  We need to do this
         * because we use SYNC when grabbing buttons  so that we can handle
         * the events ourself.  We can also decide to replay it to the client it
         * was actually meant for.  The replaying is done within the button
         * binding run function.
         */
        XAllowEvents(display, AsyncPointer, event->xbutton.time);
        break;

    /* a mouse button was released */
    case ButtonRelease:
        handle_button_release(&event->xbutton);
        /* continue processing pointer events normally */
        XAllowEvents(display, AsyncPointer, event->xbutton.time);
        break;

    /* the mouse was moved */
    case MotionNotify:
        handle_motion_notify(&event->xmotion);
        break;

    /* a window was destroyed */
    case DestroyNotify:
        handle_destroy_notify(&event->xdestroywindow);
        break;

    /* a window was removed from the screen */
    case UnmapNotify:
        handle_unmap_notify(&event->xunmap);
        break;

    /* a window wants to appear on the screen */
    case MapRequest:
        handle_map_request(&event->xmaprequest);
        break;

    /* a window wants to configure itself */
    case ConfigureRequest:
        handle_configure_request(&event->xconfigurerequest);
        break;

    /* a window changed a property */
    case PropertyNotify:
        handle_property_notify(&event->xproperty);
        break;

    /* a client sent us a message */
    case ClientMessage:
        handle_client_message(&event->xclient);
        break;
    }
}
