#include <X11/Xatom.h>

#include "fensterchef.h"
#include "log.h"
#include "x11/display.h"
#include "x11/ewmh.h"

/* Create the wm check window. */
Window create_ewmh_window(void)
{
    Window window;

    window = XCreateWindow(display, DefaultRootWindow(display),
            -1, -1, 1, 1, 0, CopyFromParent, InputOnly,
            (Visual*) CopyFromParent, 0, NULL);
    /* set the title to the window manager name */
    XStoreName(display, window, FENSTERCHEF_NAME);
    /* set the check window property so it points to itself */
    XChangeProperty(display, window, ATOM(_NET_SUPPORTING_WM_CHECK),
            XA_WINDOW, 32, PropModeReplace,
            (unsigned char*) &window, 1);
    /* map the window so it can receive the fallback focus */
    XMapWindow(display, window);

    return window;
}

/*********************
 * Window properties *
 *********************/

/* Get a long window property. */
long *get_long_property(Window window, Atom property,
        unsigned long expected_item_count)
{
    Atom type;
    int format;
    unsigned long item_count;
    unsigned long bytes_after;
    unsigned char *property_result;

    XGetWindowProperty(display, window, property, 0, expected_item_count, False,
            AnyPropertyType, &type, &format, &item_count, &bytes_after,
            &property_result);
    if (format != 32 || item_count != expected_item_count) {
        if (type != None) {
            LOG("window %w has misformatted property %a\n",
                    window, property);
        }
        XFree(property_result);
        property_result = NULL;
    }
    return (long*) property_result;
}

/* Get a text window property. */
char *get_text_property(Window window, Atom property,
        unsigned long *length)
{
    Atom type;
    int format;
    unsigned long bytes_after;
    unsigned char *property_result;

    XGetWindowProperty(display, window, property, 0, 1024, False,
            AnyPropertyType, &type, &format, length,
            &bytes_after, &property_result);

    if (format != 8) {
        if (type != None) {
            LOG("window %w has misformatted property %a\n",
                    window, property_result);
        }
        XFree(property_result);
        property_result = NULL;
    }

    return (char*) property_result;
}

/* Get a window property as list of atoms. */
Atom *get_atom_list_property(Window window, Atom property)
{
    Atom type;
    int format;
    unsigned long count;
    unsigned long bytes_after;
    unsigned char *raw_property;
    Atom *raw_atoms;
    Atom *atoms;

    /* get up to 32 atoms from this property */
    XGetWindowProperty(display, window, property, 0, 32, False,
            XA_ATOM, &type, &format, &count,
            &bytes_after, &raw_property);
    if (format != 32 || type != XA_ATOM) {
        if (type != None) {
            LOG("window %w has misformatted property %a\n",
                    window, property);
        }
        XFree(raw_property);
        return NULL;
    }

    raw_atoms = (Atom*) raw_property;
    ALLOCATE(atoms, count + 1);
    COPY(atoms, raw_atoms, count);
    atoms[count] = None;

    XFree(raw_property);
    return atoms;
}

/* Check if an atom is within the given list of atoms. */
bool is_atom_included(const Atom *atoms, Atom atom)
{
    if (atoms == NULL) {
        return false;
    }
    for (; atoms[0] != None; atoms++) {
        if (atoms[0] == atom) {
            return true;
        }
    }
    return false;
}

/* Get the _NET_WM_NAME or WM_NAME window property. */
char *get_window_name_property(Window window)
{
    char *text;
    unsigned long length;
    char *name = NULL;

    text = get_text_property(window, ATOM(_NET_WM_NAME), &length);
    /* try to fall back to `WM_NAME` */
    if (text == NULL) {
        text = get_text_property(window, XA_WM_NAME, &length);
    }

    if (text != NULL) {
        name = xstrndup(text, length);
        XFree(text);
    }

    return name;
}

/* Get the _NET_WM_STRUT_PARTIAL or _NET_WM_STRUT window property. */
bool get_strut_property(Window window, wm_strut_partial_t *strut)
{
    long *longs;

    ZERO(strut, 1);

    longs = get_long_property(window, ATOM(_NET_WM_STRUT_PARTIAL),
            sizeof(*strut) / sizeof(strut->left));
    if (longs == NULL) {
        /* _NET_WM_STRUT is older than _NET_WM_STRUT_PARTIAL, fall back to
         * it when there is no strut partial
         */
        longs = get_long_property(window, ATOM(_NET_WM_STRUT), 4);
        if (longs == NULL) {
            return false;
        } else {
            strut->left = longs[0];
            strut->top = longs[1];
            strut->right = longs[2];
            strut->bottom = longs[3];
        }
    } else {
        strut->left = longs[0];
        strut->right = longs[1];
        strut->top = longs[2];
        strut->bottom = longs[3];
        strut->left_start_y = longs[4];
        strut->left_end_y = longs[5];
        strut->right_start_y = longs[6];
        strut->right_end_y = longs[7];
        strut->top_start_x = longs[8];
        strut->top_end_x = longs[9];
        strut->bottom_start_x = longs[10];
        strut->bottom_end_x = longs[11];
    }

    XFree(longs);

    return true;
}

/* Get the _NET_WM_FULLSCREEN_MONITORS window property. */
bool get_fullscreen_monitors_property(Window window,
        Extents *fullscreen_monitors)
{
    long *longs;

    longs = get_long_property(window, ATOM(_NET_WM_FULLSCREEN_MONITORS),
            sizeof(*fullscreen_monitors) / sizeof(fullscreen_monitors->left));
    if (longs == NULL) {
        ZERO(fullscreen_monitors, 1);
        return false;
    } else {
        fullscreen_monitors->left = longs[0];
        fullscreen_monitors->top = longs[1];
        fullscreen_monitors->right = longs[2];
        fullscreen_monitors->bottom = longs[3];
        XFree(longs);
        return true;
    }
}

/* Gets the `FENSTERCHEF_COMMAND` property from @window. */
char *get_fensterchef_command_property(Window window)
{
    char *command_property;
    unsigned long length;
    char *command = NULL;

    command_property = get_text_property(window, ATOM(FENSTERCHEF_COMMAND),
            &length);
    if (command_property != NULL) {
        command = xstrndup(command_property, length);
        XFree(command_property);
    }
    return command;
}

/**************************
 * Window client messages *
 **************************/

/* Send a WM_TAKE_FOCUS client message to given window. */
void send_take_focus_message(Window window)
{
    XEvent event;

    ZERO(&event, 1);
    /* bake an event for running a protocol on the window */
    event.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = ATOM(WM_PROTOCOLS);
    event.xclient.format = 32;
    event.xclient.data.l[0] = ATOM(WM_TAKE_FOCUS);
    event.xclient.data.l[1] = CurrentTime;
    XSendEvent(display, window, false, NoEventMask, &event);
}

/* Send a WM_DELETE_WINDOW client message to given window. */
void send_delete_window_message(Window window)
{
    XEvent event;

    ZERO(&event, 1);

    /* bake an event for running a protocol on the window */
    event.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = ATOM(WM_PROTOCOLS);
    event.xclient.format = 32;
    event.xclient.data.l[0] = ATOM(WM_DELETE_WINDOW);
    XSendEvent(display, window, False, NoEventMask, &event);
}
