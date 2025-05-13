#include <X11/Xatom.h>

#include "binding.h"
#include "cursor.h"
#include "frame.h"
#include "log.h"
#include "window.h"
#include "x11/display.h"

/* hints set by all parts of the program indicating that a specific part needs
 * to be synchronized
 */
unsigned synchronization_flags;

/* Set the client list root property. */
static void synchronize_client_list(void)
{
    /* a list of window ids that is synchronized with the actual windows */
    static struct {
        /* the id of the window */
        Window *ids;
        /* the number of allocated ids */
        unsigned length;
    } client_list;

    Window root;
    FcWindow *window;
    unsigned index = 0;

    root = DefaultRootWindow(display);

    /* allocate more ids if needed or trim if there are way too many */
    if (Window_count > client_list.length ||
            MAX(Window_count, 4) * 4 < client_list.length) {
        LOG_DEBUG("resizing client list to %u\n",
                Window_count);
        client_list.length = Window_count;
        REALLOCATE(client_list.ids, client_list.length);
    }

    /* sort the list in order of the Z stacking (bottom to top) */
    for (window = Window_bottom; window != NULL; window = window->above) {
        client_list.ids[index] = window->reference.id;
        index++;
    }
    /* set the `_NET_CLIENT_LIST_STACKING` property */
    XChangeProperty(display, root, ATOM(_NET_CLIENT_LIST_STACKING),
            XA_WINDOW, 32, PropModeReplace,
            (unsigned char*) client_list.ids, Window_count);

    index = 0;
    /* sort the list in order of their age (oldest to newest) */
    for (window = Window_oldest; window != NULL; window = window->newer) {
        client_list.ids[index] = window->reference.id;
        index++;
    }
    /* set the `_NET_CLIENT_LIST` property */
    XChangeProperty(display, root, ATOM(_NET_CLIENT_LIST),
            XA_WINDOW, 32, PropModeReplace,
            (unsigned char*) client_list.ids, Window_count);
}

/* Recursively check if the window is contained within @frame. */
static bool is_window_part_of(FcWindow *window, Frame *frame)
{
    if (frame->left != NULL) {
        return is_window_part_of(window, frame->left) ||
            is_window_part_of(window, frame->right);
    }
    return frame->window == window;
}

/* Synchronize the local data with the X server. */
void synchronize_with_server(unsigned flags)
{
    Atom atoms[2];
    FcWindow *window;

    if (flags == 0) {
        flags = synchronization_flags;
    }

    LOG_DEBUG("doing server synchronization with: %#x\n",
            flags);

    if ((flags & SYNCHRONIZE_ROOT_CURSOR)) {
        XDefineCursor(display, DefaultRootWindow(display),
                load_cursor(CURSOR_ROOT, NULL));
        synchronization_flags &= ~SYNCHRONIZE_ROOT_CURSOR;
    }

    if ((flags & SYNCHRONIZE_BUTTON_BINDING)) {
        for (FcWindow *window = Window_first;
                window != NULL;
                window = window->next) {
            grab_configured_buttons(window->reference.id);
        }
        synchronization_flags &= ~SYNCHRONIZE_BUTTON_BINDING;
    }

    if ((flags & SYNCHRONIZE_KEY_BINDING)) {
        grab_configured_keys();
        synchronization_flags &= ~SYNCHRONIZE_KEY_BINDING;
    }

    /* since the strut of a monitor might have changed because a window with
     * strut got hidden or shown, we need to recompute those
     */
    reconfigure_monitor_frames();

    /* set the borders of the windows and manage the focused state */
    for (window = Window_first; window != NULL; window = window->next) {
        /* remove the focused state from windows that are not focused */
        if (window != Window_focus) {
            atoms[0] = ATOM(_NET_WM_STATE_FOCUSED);
            remove_window_states(window, atoms, 1);
        }

        /* update the border size */
        if (is_window_borderless(window)) {
            window->border_size = 0;
        } else {
            window->border_size = configuration.border_size;
        }

        /* set the color of the focused window */
        if (window == Window_focus) {
            window->border_color = configuration.border_color_focus;
        /* deeply set the colors of all windows within the focused frame */
        } else if ((Window_focus == NULL ||
                        Window_focus->state.mode == WINDOW_MODE_TILING) &&
                window->state.mode == WINDOW_MODE_TILING &&
                is_window_part_of(window, Frame_focus)) {
            window->border_color = configuration.border_color_focus;
        /* if the window is the top window or within the focused frame, give it
         * the active color
         */
        } else if (is_window_part_of(window, Frame_focus) ||
                (window->state.mode == WINDOW_MODE_FLOATING &&
                    window == Window_top)) {
            window->border_color = configuration.border_color_active;
        } else {
            window->border_color = configuration.border_color;
        }
    }

    synchronize_window_stacking_order();

    /* update the client list and stacking order */
    /* TODO: only change if it actually changed */
    synchronize_client_list();

    /* configure all visible windows and map them */
    for (window = Window_top; window != NULL; window = window->below) {
        if (!window->state.is_visible) {
            continue;
        }
        configure_client(&window->reference, window->x, window->y,
                window->width, window->height, window->border_size);
        change_client_attributes(&window->reference, window->border_color);

        atoms[0] = ATOM(_NET_WM_STATE_HIDDEN);
        remove_window_states(window, atoms, 1);

        if (window->properties.wm_state != NormalState) {
            window->properties.wm_state = NormalState;
            atoms[0] = NormalState;
            /* no icon */
            atoms[1] = None;
            XChangeProperty(display, window->reference.id, ATOM(WM_STATE),
                    ATOM(WM_STATE), 32, PropModeReplace,
                    (unsigned char*) atoms, 2);
        }

        map_client(&window->reference);
    }

    /* unmap all invisible windows */
    for (FcWindow *window = Window_bottom;
            window != NULL;
            window = window->above) {
        if (window->state.is_visible) {
            continue;
        }

        atoms[0] = ATOM(_NET_WM_STATE_HIDDEN);
        add_window_states(window, atoms, 1);

        if (window->properties.wm_state != WithdrawnState) {
            window->properties.wm_state = WithdrawnState;
            atoms[0] = WithdrawnState;
            /* no icon */
            atoms[1] = None;
            XChangeProperty(display, window->reference.id, ATOM(WM_STATE),
                    ATOM(WM_STATE), 32, PropModeReplace,
                    (unsigned char*) atoms, 2);
        }

        unmap_client(&window->reference);
    }
}

/* Show the client on the X server. */
void map_client(XReference *reference)
{
    if (reference->is_mapped) {
        return;
    }

    LOG("showing client %w\n", reference->id);

    reference->is_mapped = true;

    XMapWindow(display, reference->id);
}

/* Show the client on the X server at the top. */
void map_client_raised(XReference *reference)
{
    if (reference->is_mapped) {
        return;
    }

    LOG("showing client %w raised\n", reference->id);

    reference->is_mapped = true;

    XMapRaised(display, reference->id);
}

/* Hide the client on the X server. */
void unmap_client(XReference *reference)
{
    if (!reference->is_mapped) {
        return;
    }

    LOG("hiding client %w\n", reference->id);

    reference->is_mapped = false;

    XUnmapWindow(display, reference->id);
}

/* Set the size of a window associated to the X server. */
void configure_client(XReference *reference, int x, int y, unsigned width,
        unsigned height, unsigned border_width)
{
    XWindowChanges changes;
    unsigned mask = 0;

    if (reference->x != x) {
        reference->x = x;
        changes.x = x;
        mask |= CWX;
    }

    if (reference->y != y) {
        reference->y = y;
        changes.y = y;
        mask |= CWY;
    }

    if (reference->width != width) {
        reference->width = width;
        changes.width = width;
        mask |= CWWidth;
    }

    if (reference->height != height) {
        reference->height = height;
        changes.height = height;
        mask |= CWHeight;
    }

    if (reference->border_width != border_width) {
        reference->border_width = border_width;
        changes.border_width = border_width;
        mask |= CWBorderWidth;
    }

    if (mask != 0) {
        LOG("configuring client %w to %R %u\n",
                reference->id, x, y, width, height, border_width);
        XConfigureWindow(display, reference->id, mask, &changes);
    }
}

/* Set the client border color. */
void change_client_attributes(XReference *reference, unsigned border_color)
{
    XSetWindowAttributes attributes;
    unsigned mask = 0;

    if (reference->border != border_color) {
        reference->border = border_color;
        attributes.border_pixel = border_color;
        mask |= CWBorderPixel;
    }

    if (mask != 0) {
        LOG("changing attributes of client %w to " COLOR(GREEN) "#%08x\n",
                reference->id, border_color);
        XChangeWindowAttributes(display, reference->id, mask, &attributes);
    }
}
