#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/Xproto.h>

#include "fensterchef.h"
#include "log.h"
#include "window.h"
#include "x11/ewmh.h"

/* connection to the X server */
Display *display;

/* first index of an xkb event */
int xkb_event_base, xkb_error_base;

/* first index of a randr event */
int randr_event_base = -1, randr_error_base = -1;

/* all X atom names */
const char *x_atom_names[ATOM_MAX] = {
#define X(atom) \
    #atom,
    DEFINE_ALL_ATOMS
#undef X
};

/* all X atom ids */
Atom x_atom_ids[ATOM_MAX];

/* supporting wm check window */
Window ewmh_window;

/* Open the X11 display (X server connection). */
void open_connection(void)
{
    int major_version, minor_version;
    int status;

    major_version = XkbMajorVersion;
    minor_version = XkbMinorVersion;
    display = XkbOpenDisplay(NULL, &xkb_event_base, &xkb_error_base,
            &major_version, &minor_version, &status);
    if (display == NULL) {
        LOG_ERROR("could not open display: " COLOR(RED));
        switch (status) {
        case XkbOD_BadLibraryVersion:
            fprintf(stderr, "using a bad XKB library version\n");
            break;

        case XkbOD_ConnectionRefused:
            fprintf(stderr, "could not open connection\n");
            break;

        case XkbOD_BadServerVersion:
            fprintf(stderr, "the server and client XKB versions mismatch\n");
            break;

        case XkbOD_NonXkbServer:
            fprintf(stderr, "the server does not have the XKB extension\n");
            break;
        }
        exit(EXIT_FAILURE);
    }

    /* receive events when the keyboard changes */
    XkbSelectEventDetails(display, XkbUseCoreKbd, XkbNewKeyboardNotify,
            XkbNKN_KeycodesMask,
            XkbNKN_KeycodesMask);
    /* listen for changes to key symbols and the modifier map */
    XkbSelectEventDetails(display, XkbUseCoreKbd, XkbMapNotify,
            XkbAllClientInfoMask,
            XkbAllClientInfoMask);

    LOG("%D\n", display);
}

/* The handler for X errors. */
static int x_error_handler(Display *display, XErrorEvent *error)
{
    char buffer[128];

    if (error->error_code == xkb_error_base) {
        LOG_ERROR("Xkb request failed: %d\n",
                error->request_code);
        return 0;
    }

    if (error->error_code == randr_error_base) {
        LOG_ERROR("XRandr request failed: %d\n",
                error->request_code);
        return 0;
    }

    /* ignore the BadWindow error for non debug builds */
#ifndef DEBUG
    if (error->error_code == BadWindow) {
        return 0;
    }
#endif

    XGetErrorText(display, error->error_code, buffer, sizeof(buffer));

    LOG_ERROR("X error: %s\n", buffer);
    return 0;
}

/* Try to take control of the window manager role. */
void take_control(void)
{
    XSetWindowAttributes attributes;

    /* with this event mask, we get the following events:
     * SubstructureRedirectMask
     *  CirculateRequest
     *  ConfigureRequest
     *  MapRequest
     * SubstructureNotifyMask
     *  CirculateNotify
     * 	ConfigureNotify
     * 	CreateNotify
     * 	DestroyNotify
     * 	GravityNotify
     *  MapNotify
     *  ReparentNotify
     *  UnmapNotify
     */
    attributes.event_mask = SubstructureRedirectMask | SubstructureNotifyMask;
    XChangeWindowAttributes(display, DefaultRootWindow(display), CWEventMask,
            &attributes);

    /* wait until we get a reply to our request, it must succeed */
    XSync(display, False);

    /* set an error handler so that further errors do not abort fensterchef */
    XSetErrorHandler(x_error_handler);

    /* intern all atoms into the X server so we receive special identifiers */
    XInternAtoms(display, (char**) x_atom_names, ATOM_MAX, False, x_atom_ids);

    ewmh_window = create_ewmh_window();

    /* initialize the focus */
    XSetInputFocus(display, ewmh_window, RevertToParent, CurrentTime);

    Fensterchef_is_running = true;
}

/* Go through all existing windows and manage them. */
void query_existing_windows(void)
{
    Window root;
    Window parent;
    Window *children;
    unsigned number_of_children;

    /* get a list of child windows of the root in bottom-to-top stacking order
     */
    XQueryTree(display, DefaultRootWindow(display), &root, &parent, &children,
            &number_of_children);

    for (unsigned i = 0; i < number_of_children; i++) {
        (void) create_window(children[i]);
    }

    XFree(children);
}

/* Set the initial root window properties. */
void initialize_root_properties(void)
{
    Window root;

    /* set the supported ewmh atoms of our window manager */
    const Atom supported_atoms[] = {
        ATOM(_NET_SUPPORTED),

        ATOM(_NET_CLIENT_LIST),
        ATOM(_NET_CLIENT_LIST_STACKING),

        ATOM(_NET_ACTIVE_WINDOW),

        ATOM(_NET_SUPPORTING_WM_CHECK),

        ATOM(_NET_CLOSE_WINDOW),

        ATOM(_NET_MOVERESIZE_WINDOW),

        ATOM(_NET_WM_MOVERESIZE),

        ATOM(_NET_RESTACK_WINDOW),

        ATOM(_NET_REQUEST_FRAME_EXTENTS),

        ATOM(_NET_WM_NAME),

        ATOM(_NET_WM_DESKTOP),

        ATOM(_NET_WM_WINDOW_TYPE),
        ATOM(_NET_WM_WINDOW_TYPE_DESKTOP),
        ATOM(_NET_WM_WINDOW_TYPE_DOCK),
        ATOM(_NET_WM_WINDOW_TYPE_TOOLBAR),
        ATOM(_NET_WM_WINDOW_TYPE_MENU),
        ATOM(_NET_WM_WINDOW_TYPE_UTILITY),
        ATOM(_NET_WM_WINDOW_TYPE_SPLASH),
        ATOM(_NET_WM_WINDOW_TYPE_DIALOG),
        ATOM(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU),
        ATOM(_NET_WM_WINDOW_TYPE_POPUP_MENU),
        ATOM(_NET_WM_WINDOW_TYPE_TOOLTIP),
        ATOM(_NET_WM_WINDOW_TYPE_NOTIFICATION),
        ATOM(_NET_WM_WINDOW_TYPE_COMBO),
        ATOM(_NET_WM_WINDOW_TYPE_DND),
        ATOM(_NET_WM_WINDOW_TYPE_NORMAL),

        ATOM(_NET_WM_STATE),
        ATOM(_NET_WM_STATE_MAXIMIZED_VERT),
        ATOM(_NET_WM_STATE_MAXIMIZED_HORZ),
        ATOM(_NET_WM_STATE_FULLSCREEN),
        ATOM(_NET_WM_STATE_HIDDEN),
        ATOM(_NET_WM_STATE_FOCUSED),

        ATOM(_NET_WM_STRUT),
        ATOM(_NET_WM_STRUT_PARTIAL),

        ATOM(_NET_FRAME_EXTENTS),

        ATOM(_NET_WM_FULLSCREEN_MONITORS),
    };
    root = DefaultRootWindow(display);
    XChangeProperty(display, root, ATOM(_NET_SUPPORTED), XA_ATOM, 32,
            PropModeReplace, (unsigned char*) supported_atoms,
            SIZE(supported_atoms));

    /* the wm check window */
    XChangeProperty(display, root, ATOM(_NET_SUPPORTING_WM_CHECK), XA_WINDOW,
            32, PropModeReplace, (unsigned char*) &ewmh_window, 1);


    /* set the active window */
    XChangeProperty(display, root, ATOM(_NET_ACTIVE_WINDOW), XA_WINDOW, 32,
            PropModeReplace, (unsigned char*) &root, 1);
}
