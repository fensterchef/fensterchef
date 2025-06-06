#include <inttypes.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <X11/XKBlib.h>

#include "action.h"
#include "event.h"
#include "frame.h"
#include "log.h"
#include "notification.h"
#include "window.h"
#include "window_list.h"
#include "x11/display.h"
#include "x11/ewmh.h"

/* the severity of the logging */
log_severity_t log_severity = LOG_SEVERITY_INFO;

/* the path of the log file */
char *log_file_path;

/* the file to log to */
FILE *log_file;

/***********************/
/** String conversion **/

static const char *event_to_string(int type)
{
    const char *event_strings[] = {
        [KeyPress] = "KeyPress",
        [KeyRelease] = "KeyRelease",
        [ButtonPress] = "ButtonPress",
        [ButtonRelease] = "ButtonRelease",
        [MotionNotify] = "MotionNotify",
        [EnterNotify] = "EnterNotify",
        [LeaveNotify] = "LeaveNotify",
        [FocusIn] = "FocusIn",
        [FocusOut] = "FocusOut",
        [KeymapNotify] = "KeymapNotify",
        [Expose] = "Expose",
        [GraphicsExpose] = "GraphicsExpose",
        [NoExpose] = "NoExpose",
        [VisibilityNotify] = "VisibilityNotify",
        [CreateNotify] = "CreateNotify",
        [DestroyNotify] = "DestroyNotify",
        [UnmapNotify] = "UnmapNotify",
        [MapNotify] = "MapNotify",
        [MapRequest] = "MapRequest",
        [ReparentNotify] = "ReparentNotify",
        [ConfigureNotify] = "ConfigureNotify",
        [ConfigureRequest] = "ConfigureRequest",
        [GravityNotify] = "GravityNotify",
        [ResizeRequest] = "ResizeRequest",
        [CirculateNotify] = "CirculateNotify",
        [CirculateRequest] = "CirculateRequest",
        [PropertyNotify] = "PropertyNotify",
        [SelectionClear] = "SelectionClear",
        [SelectionRequest] = "SelectionRequest",
        [SelectionNotify] = "SelectionNotify",
        [ColormapNotify] = "ColormapNotify",
        [ClientMessage] = "ClientMessage",
        [MappingNotify] = "MappingNotify",
    };

    /* types smaller than 0 are reserved for errors and replies */
    if (type < 2 || type > (int) SIZE(event_strings)) {
        return NULL;
    }
    return event_strings[type];
}

static const char *atom_to_string(Atom atom)
{
    const char *atom_strings[] = {
        [None] = "NONE",
        [XA_PRIMARY] = "PRIMARY",
        [XA_SECONDARY] = "SECONDARY",
        [XA_ARC] = "ARC",
        [XA_ATOM] = "ATOM",
        [XA_BITMAP] = "BITMAP",
        [XA_CARDINAL] = "CARDINAL",
        [XA_COLORMAP] = "COLORMAP",
        [XA_CURSOR] = "CURSOR",
        [XA_CUT_BUFFER0] = "CUT_BUFFER0",
        [XA_CUT_BUFFER1] = "CUT_BUFFER1",
        [XA_CUT_BUFFER2] = "CUT_BUFFER2",
        [XA_CUT_BUFFER3] = "CUT_BUFFER3",
        [XA_CUT_BUFFER4] = "CUT_BUFFER4",
        [XA_CUT_BUFFER5] = "CUT_BUFFER5",
        [XA_CUT_BUFFER6] = "CUT_BUFFER6",
        [XA_CUT_BUFFER7] = "CUT_BUFFER7",
        [XA_DRAWABLE] = "DRAWABLE",
        [XA_FONT] = "FONT",
        [XA_INTEGER] = "INTEGER",
        [XA_PIXMAP] = "PIXMAP",
        [XA_POINT] = "POINT",
        [XA_RECTANGLE] = "RECTANGLE",
        [XA_RESOURCE_MANAGER] = "RESOURCE_MANAGER",
        [XA_RGB_COLOR_MAP] = "RGB_COLOR_MAP",
        [XA_RGB_BEST_MAP] = "RGB_BEST_MAP",
        [XA_RGB_BLUE_MAP] = "RGB_BLUE_MAP",
        [XA_RGB_DEFAULT_MAP] = "RGB_DEFAULT_MAP",
        [XA_RGB_GRAY_MAP] = "RGB_GRAY_MAP",
        [XA_RGB_GREEN_MAP] = "RGB_GREEN_MAP",
        [XA_RGB_RED_MAP] = "RGB_RED_MAP",
        [XA_STRING] = "STRING",
        [XA_VISUALID] = "VISUALID",
        [XA_WINDOW] = "WINDOW",
        [XA_WM_COMMAND] = "WM_COMMAND",
        [XA_WM_HINTS] = "WM_HINTS",
        [XA_WM_CLIENT_MACHINE] = "WM_CLIENT_MACHINE",
        [XA_WM_ICON_NAME] = "WM_ICON_NAME",
        [XA_WM_ICON_SIZE] = "WM_ICON_SIZE",
        [XA_WM_NAME] = "WM_NAME",
        [XA_WM_NORMAL_HINTS] = "WM_NORMAL_HINTS",
        [XA_WM_SIZE_HINTS] = "WM_SIZE_HINTS",
        [XA_WM_ZOOM_HINTS] = "WM_ZOOM_HINTS",
        [XA_MIN_SPACE] = "MIN_SPACE",
        [XA_NORM_SPACE] = "NORM_SPACE",
        [XA_MAX_SPACE] = "MAX_SPACE",
        [XA_END_SPACE] = "END_SPACE",
        [XA_SUPERSCRIPT_X] = "SUPERSCRIPT_X",
        [XA_SUPERSCRIPT_Y] = "SUPERSCRIPT_Y",
        [XA_SUBSCRIPT_X] = "SUBSCRIPT_X",
        [XA_SUBSCRIPT_Y] = "SUBSCRIPT_Y",
        [XA_UNDERLINE_POSITION] = "UNDERLINE_POSITION",
        [XA_UNDERLINE_THICKNESS] = "UNDERLINE_THICKNESS",
        [XA_STRIKEOUT_ASCENT] = "STRIKEOUT_ASCENT",
        [XA_STRIKEOUT_DESCENT] = "STRIKEOUT_DESCENT",
        [XA_ITALIC_ANGLE] = "ITALIC_ANGLE",
        [XA_X_HEIGHT] = "X_HEIGHT",
        [XA_QUAD_WIDTH] = "QUAD_WIDTH",
        [XA_WEIGHT] = "WEIGHT",
        [XA_POINT_SIZE] = "POINT_SIZE",
        [XA_RESOLUTION] = "RESOLUTION",
        [XA_COPYRIGHT] = "COPYRIGHT",
        [XA_NOTICE] = "NOTICE",
        [XA_FONT_NAME] = "FONT_NAME",
        [XA_FAMILY_NAME] = "FAMILY_NAME",
        [XA_FULL_NAME] = "FULL_NAME",
        [XA_CAP_HEIGHT] = "CAP_HEIGHT",
        [XA_WM_CLASS] = "WM_CLASS",
        [XA_WM_TRANSIENT_FOR] = "WM_TRANSIENT_FOR",
    };

    if (atom < SIZE(atom_strings)) {
        return atom_strings[atom];
    }
    for (unsigned i = 0; i < ATOM_MAX; i++) {
        if (x_atom_ids[i] == atom) {
            return x_atom_names[i];
        }
    }

    return NULL;
}

static const char *notify_detail_to_string(int detail)
{
    switch (detail) {
    case NotifyAncestor:
        return "ancestor";
    case NotifyVirtual:
        return "virtual";
    case NotifyInferior:
        return "inferior";
    case NotifyNonlinear:
        return "nonlinear";
    case NotifyNonlinearVirtual:
        return "nonlinear virtual";
    case NotifyPointer:
        return "pointer";
    case NotifyPointerRoot:
        return "pointer root";
    case NotifyDetailNone:
        return "none";
    default:
        return NULL;
    }
}

static const char *notify_mode_to_string(int mode)
{
    switch (mode) {
    case NotifyNormal:
        return "normal";
    case NotifyGrab:
        return "grab";
    case NotifyUngrab:
        return "ungrab";
    case NotifyWhileGrabbed:
        return "while grabbed";
    default:
        return NULL;
    }
}

static const char *button_to_string(unsigned button)
{
    switch (button) {
    case AnyButton:
        return "any";
    case Button1:
        return "left";
    case Button2:
        return "middle";
    case Button3:
        return "right";
    case Button4:
        return "scroll up";
    case Button5:
        return "scroll down";
    case /* Button6 */ 6:
        return "scroll left";
    case /* Button7 */ 7:
        return "scroll right";
    default:
        return NULL;
    }
}

static const char *direction_to_string(wm_move_resize_direction_t direction)
{
    switch (direction) {
    case _NET_WM_MOVERESIZE_SIZE_TOPLEFT:
        return "top left";
    case _NET_WM_MOVERESIZE_SIZE_TOP:
        return "top";
    case _NET_WM_MOVERESIZE_SIZE_TOPRIGHT:
        return "top right";
    case _NET_WM_MOVERESIZE_SIZE_RIGHT:
        return "right";
    case _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT:
        return "bottom right";
    case _NET_WM_MOVERESIZE_SIZE_BOTTOM:
        return "bottom";
    case _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT:
        return "bottom left";
    case _NET_WM_MOVERESIZE_SIZE_LEFT:
        return "left";
    case _NET_WM_MOVERESIZE_MOVE:
        return "move";
    case _NET_WM_MOVERESIZE_SIZE_KEYBOARD:
        return "size keyboard";
    case _NET_WM_MOVERESIZE_MOVE_KEYBOARD:
        return "move keyboard";
    case _NET_WM_MOVERESIZE_CANCEL:
        return "cancel";
    default:
        return NULL;
    }
}

static const char *gravity_to_string(int gravity)
{
    switch (gravity) {
    case UnmapGravity:
        return "win unmap";
    case NorthWestGravity:
        return "north west";
    case NorthGravity:
        return "north";
    case NorthEastGravity:
        return "north east";
    case WestGravity:
        return "west";
    case CenterGravity:
        return "center";
    case EastGravity:
        return "east";
    case SouthWestGravity:
        return "south west";
    case SouthGravity:
        return "south";
    case SouthEastGravity:
        return "south east";
    case StaticGravity:
        return "static";
    default:
        return NULL;
    }
}

/****************************/
/** Utility log functions **/

static void log_boolean(bool boolean)
{
    fputs(boolean ? COLOR(GREEN) "true" CLEAR_COLOR :
            COLOR(RED) "false" CLEAR_COLOR, log_file);
}

static void log_hexadecimal(unsigned x)
{
    fprintf(log_file, COLOR(GREEN) "%#x" CLEAR_COLOR,
            x);
}

static void log_point(int x, int y)
{
    fprintf(log_file, COLOR(GREEN) "%d+%d" CLEAR_COLOR,
            x, y);
}

static void log_rectangle(int x, int y, unsigned width, unsigned height)
{
    fprintf(log_file, COLOR(GREEN) "%d+%d+%ux%u" CLEAR_COLOR,
            x, y, width, height);
}

static void log_size(unsigned width, unsigned height)
{
    fprintf(log_file, COLOR(GREEN) "%ux%u" CLEAR_COLOR,
            width, height);
}

static void log_integer(int x)
{
    fprintf(log_file, COLOR(GREEN) "%d" CLEAR_COLOR,
            x);
}

static void log_unsigned(unsigned x)
{
    fprintf(log_file, COLOR(GREEN) "%d" CLEAR_COLOR,
            x);
}

static void log_unsigned_pair(unsigned x, unsigned y)
{
    fprintf(log_file, COLOR(GREEN) "%u,%u" CLEAR_COLOR,
            x, y);
}

static void log_button(unsigned button)
{
    const char *string;

    string = button_to_string(button);
    if (string == NULL) {
        fprintf(log_file, COLOR(CYAN) "X%u" CLEAR_COLOR, button - 7);
    } else {
        fprintf(log_file, COLOR(CYAN) "%s" CLEAR_COLOR, string);
    }
}

static void log_source(int source)
{
    switch (source) {
    case 1:
        fputs(COLOR(CYAN) "client" CLEAR_COLOR, log_file);
        break;
    case 2:
        fputs(COLOR(CYAN) "pager" CLEAR_COLOR, log_file);
        break;
    default:
        fputs(COLOR(CYAN) "legacy" CLEAR_COLOR, log_file);
        break;
    }
}

static void log_gravity(int gravity)
{
    const char *string;

    string = gravity_to_string(gravity);
    if (string == NULL) {
        fprintf(log_file, COLOR(GREEN) "%u" CLEAR_COLOR, gravity);
    } else {
        fprintf(log_file, COLOR(CYAN) "%s" CLEAR_COLOR, string);
    }
}

static void log_direction(wm_move_resize_direction_t direction)
{
    const char *string;

    string = direction_to_string(direction);
    if (string == NULL) {
        fprintf(log_file, COLOR(GREEN) "%u" CLEAR_COLOR, direction);
    } else {
        fprintf(log_file, COLOR(CYAN) "%s" CLEAR_COLOR, string);
    }
}

static void log_modifiers(unsigned mask)
{
    static const char *modifiers[] = {
        "Shift", "Lock", "Ctrl",
        "Mod1", "Mod2", "Mod3", "Mod4", "Mod5",
        "Button1", "Button2", "Button3", "Button4", "Button5"
    };
    int modifier_count = 0;

    fputs(COLOR(MAGENTA), log_file);
    for (const char **modifier = modifiers; mask != 0; mask >>= 1, modifier++) {
        if ((mask & 1)) {
            if (modifier_count > 0) {
                fputs(CLEAR_COLOR "+" COLOR(MAGENTA), log_file);
            }
            fputs(*modifier, log_file);
            modifier_count++;
        }
    }
    fputs(CLEAR_COLOR, log_file);
}

static void log_x_window(Window window)
{
    if (window == None) {
        fputs(COLOR(CYAN) "None" CLEAR_COLOR, log_file);
        return;
    }

    log_hexadecimal(window);
    fputs(COLOR(YELLOW), log_file);
    if (window == ewmh_window) {
        fputs("<check>", log_file);
    } else if (window == WindowList.reference.id) {
        fputs("<window list>", log_file);
    } else if (system_notification != NULL &&
            window == system_notification->reference.id) {
        fputs("<notification>", log_file);
    } else if (error_notification != NULL &&
            window == error_notification->reference.id) {
        fputs("<error>", log_file);
    } else if (window == DefaultRootWindow(display)) {
        fputs("<root>", log_file);
    } else {
        for (FcWindow *fwindow = Window_first;
                fwindow != NULL;
                fwindow = fwindow->next) {
            if (fwindow->reference.id == window) {
                fprintf(log_file, "<%u>", fwindow->number);
                break;
            }
        }
    }
    fputs(CLEAR_COLOR, log_file);
}

static void log_motion(int motion)
{
    fputs(COLOR(CYAN), log_file);
    switch (motion) {
    case NotifyNormal:
        fputs("normal", log_file);
        break;
    case NotifyHint:
        fputs("hint", log_file);
        break;
    default:
        fprintf(log_file, "%u", motion);
    }
    fputs(CLEAR_COLOR, log_file);
}

static void log_notify_detail(int detail)
{
    const char *string;

    fputs(COLOR(CYAN), log_file);
    string = notify_detail_to_string(detail);
    if (string == NULL) {
        fprintf(log_file, "%u", detail);
    } else {
        fputs(string, log_file);
    }
    fputs(CLEAR_COLOR, log_file);
}

static void log_notify_mode(int mode)
{
    const char *string;

    fputs(COLOR(CYAN), log_file);
    string = notify_mode_to_string(mode);
    if (string == NULL) {
        fprintf(log_file, "%u", mode);
    } else {
        fputs(string, log_file);
    }
    fputs(CLEAR_COLOR, log_file);
}

static void log_place(int place)
{
    fputs(COLOR(CYAN), log_file);
    switch (place) {
    case PlaceOnTop:
        fputs("on top", log_file);
        break;
    case PlaceOnBottom:
        fputs("on bottom", log_file);
        break;
    default:
        fprintf(log_file, "%u", place);
    }
    fputs(CLEAR_COLOR, log_file);
}

static void log_property_state(int state)
{
    fputs(COLOR(CYAN), log_file);
    switch (state) {
    case PropertyNewValue:
        fputs("new value", log_file);
        break;
    case PropertyDelete:
        fputs("delete", log_file);
        break;
    default:
        fprintf(log_file, "%d", state);
    }
    fputs(CLEAR_COLOR, log_file);
}

static void log_atom(Atom atom)
{
    const char *atom_string;

    fputs(COLOR(CYAN), log_file);
    atom_string = atom_to_string(atom);
    if (atom_string == NULL) {
#ifdef DEBUG
        char *name;

        name = XGetAtomName(display, atom);
        if (name == NULL) {
            fprintf(log_file, "%ld",
                    atom);
        } else {
            fputs(name, log_file);
            XFree(name);
        }
#endif
        fputs(COLOR(RED) "<not known>", log_file);
    } else {
        fprintf(log_file, "%s",
                atom_string);
    }
    fputs(CLEAR_COLOR, log_file);
}

static void log_visibility(int visibility)
{
    fputs(COLOR(CYAN), log_file);
    switch (visibility) {
    case VisibilityUnobscured:
        fputs("unobscured", log_file);
        break;
    case VisibilityPartiallyObscured:
        fputs("partially obscured", log_file);
        break;
    case VisibilityFullyObscured:
        fputs("fully obscured", log_file);
        break;
    default:
        fprintf(log_file, "%u", visibility);
    }
    fputs(CLEAR_COLOR, log_file);
}

static void log_configure_mask(unsigned mask)
{
    static const char *flags[] = {
        "X", "Y", "WIDTH", "HEIGHT",
        "BORDER_WIDTH", "SIBLING", "STACK_MODE"
    };
    int flag_count = 0;

    fputs(COLOR(MAGENTA), log_file);
    for (const char **flag = flags; mask != 0; mask >>= 1, flag++) {
        if ((mask & 1)) {
            if (flag_count > 0) {
                fputs(CLEAR_COLOR "+" COLOR(MAGENTA), log_file);
            }
            fputs(*flag, log_file);
            flag_count++;
        }
    }
    fputs(CLEAR_COLOR, log_file);
}

static void log_mapping(int mapping)
{
    fputs(COLOR(CYAN), log_file);
    switch (mapping) {
    case MappingModifier:
        fputs("modifier", log_file);
        break;
    case MappingKeyboard:
        fputs("keyboard", log_file);
        break;
    case MappingPointer:
        fputs("pointer", log_file);
        break;
    default:
        fprintf(log_file, "%u", mapping);
    }
    fputs(CLEAR_COLOR, log_file);
}

static void log_wm_state(int state)
{
    fputs(COLOR(CYAN), log_file);
    switch (state) {
    case WithdrawnState:
        fputs("withdrawn", log_file);
        break;
    case NormalState:
        fputs("normal", log_file);
        break;
    case IconicState:
        fputs("iconic", log_file);
        break;
    default:
        fprintf(log_file, "%u", state);
    }
    fputs(CLEAR_COLOR, log_file);
}

static void log_window_mode(window_mode_t mode)
{
    fputs(COLOR(CYAN), log_file);
    switch (mode) {
    case WINDOW_MODE_TILING:
        fputs("tiling", log_file);
        break;
    case WINDOW_MODE_FLOATING:
        fputs("floating", log_file);
        break;
    case WINDOW_MODE_DOCK:
        fputs("dock", log_file);
        break;
    case WINDOW_MODE_FULLSCREEN:
        fputs("fullscreen", log_file);
        break;
    case WINDOW_MODE_DESKTOP:
        fputs("desktop", log_file);
        break;
    case WINDOW_MODE_MAX:
        fputs("none", log_file);
        break;
    }
    fputs(CLEAR_COLOR, log_file);
}

/*************************/
/** Event log functions **/

#define V(string) fputs(", " string "=", log_file)

/* Log a RandrScreenChangeNotifyEvent to standard error output. */
static void log_xkb_new_keyboard_notify_event(
        XkbNewKeyboardNotifyEvent *event)
{
    V("device"); log_integer(event->device);
    V("min_key_code"); log_integer(event->min_key_code);
    V("max_key_code"); log_integer(event->max_key_code);
}

/* Log a RandrScreenChangeNotifyEvent to standard error output. */
static void log_xkb_map_notify_event(XkbMapNotifyEvent *event)
{
    V("device"); log_integer(event->device);
    V("change_mask"); log_hexadecimal(event->device);
}

/* Log a XRRScreenChangeNotifyEvent to standard error output. */
static void log_randr_screen_change_notify_event(
        XRRScreenChangeNotifyEvent *event)
{
    V("size"); log_size(event->width, event->height);
    V("millimeter size"); log_size(event->mwidth, event->mheight);
}

/* Log a KeyPressed-/KeyReleasedEvent to standard error output. */
static void log_key_press_event(XKeyPressedEvent *event)
{
    V("keycode"); log_unsigned(event->keycode);
    V("time"); log_unsigned(event->time);
    V("root"); log_x_window(event->root);
    V("window"); log_x_window(event->window);
    V("child"); log_x_window(event->subwindow);
    V("root_position"); log_point(event->x_root, event->x_root);
    V("position"); log_point(event->x, event->y);
    V("modifiers"); log_modifiers(event->state);
    V("same_screen"); log_boolean(event->same_screen);
}

/* Log a ButtonPressed-/ButtonReleasedEvent to standard error output. */
static void log_button_press_event(XButtonPressedEvent *event)
{
    V("button"); log_button(event->button);
    V("time"); log_unsigned(event->time);
    V("root"); log_x_window(event->root);
    V("window"); log_x_window(event->window);
    V("child"); log_x_window(event->subwindow);
    V("position"); log_point(event->x, event->y);
    V("root_position"); log_point(event->x_root, event->x_root);
    V("modifiers"); log_modifiers(event->state);
    V("same_screen"); log_boolean(event->same_screen);
}

/* Log a MotionNotifyEvent to standard error output. */
static void log_motion_notify_event(XMotionEvent *event)
{
    V("motion"); log_motion(event->is_hint);
    V("time"); log_unsigned(event->time);
    V("root"); log_x_window(event->root);
    V("window"); log_x_window(event->window);
    V("child"); log_x_window(event->subwindow);
    V("position"); log_point(event->x, event->y);
    V("root_position"); log_point(event->x_root, event->x_root);
    V("modifiers"); log_modifiers(event->state);
    V("same_screen"); log_boolean(event->same_screen);
}

/* Log a Enter-/LeaveNotifyEvent to standard error output. */
static void log_enter_notify_event(XEnterWindowEvent *event)
{
    V("detail"); log_notify_detail(event->detail);
    V("time"); log_unsigned(event->time);
    V("root"); log_x_window(event->root);
    V("window"); log_x_window(event->window);
    V("child"); log_x_window(event->subwindow);
    V("position"); log_point(event->x, event->y);
    V("root_position"); log_point(event->x_root, event->x_root);
    V("modifiers"); log_modifiers(event->state);
    V("mode"); log_notify_mode(event->mode);
    V("same_screen_focus"); log_boolean(event->same_screen);
}

/* Log a FocusIn-/FocusOutEvent to standard error output. */
static void log_focus_in_event(XFocusInEvent *event)
{
    V("detail"); log_notify_detail(event->detail);
    V("window"); log_x_window(event->window);
    V("mode"); log_notify_mode(event->mode);
}

/* Log a KeymapNotifyEvent to standard error output. */
static void log_keymap_notify_event(XKeymapEvent *event)
{
    (void) event;
    fprintf(log_file, ", keys=...");
}

/* Log a ExposeEvent to standard error output. */
static void log_expose_event(XExposeEvent *event)
{
    V("window"); log_x_window(event->window);
    V("positon"); log_point(event->x, event->y);
    V("size"); log_size(event->width, event->height);
    V("count"); log_unsigned(event->count);
}

/* Log a GraphicsExposureEvent to standard error output. */
static void log_graphics_exposure_event(XGraphicsExposeEvent *event)
{
    V("drawable"); log_hexadecimal(event->drawable);
    V("positon"); log_point(event->x, event->y);
    V("size"); log_size(event->width, event->height);
    V("count"); log_unsigned(event->count);
    V("opcode"); log_unsigned_pair(event->major_code, event->minor_code);
}

/* Log a NoExposureEvent to standard error output. */
static void log_no_exposure_event(XNoExposeEvent *event)
{
    V("drawable"); log_hexadecimal(event->drawable);
    V("opcode"); log_unsigned_pair(event->major_code, event->minor_code);
}

/* Log a VisibilityNotifyEvent to standard error output. */
static void log_visibility_notify_event(XVisibilityEvent *event)
{
    V("window"); log_x_window(event->window);
    V("visibility"); log_visibility(event->state);
}

/* Log a CreateNotifyEvent to standard error output. */
static void log_create_notify_event(XCreateWindowEvent *event)
{
    V("parent"); log_x_window(event->parent);
    V("window"); log_x_window(event->window);
    V("position"); log_point(event->x, event->y);
    V("size"); log_size(event->width, event->height);
    V("border_width"); log_unsigned(event->border_width);
    V("override_redirect"); log_boolean(event->override_redirect);
}

/* Log a DestroyNotifyEvent to standard error output. */
static void log_destroy_notify_event(XDestroyWindowEvent *event)
{
    V("event"); log_x_window(event->event);
    V("window"); log_x_window(event->window);
}

/* Log a UnmapNotifyEvent to standard error output. */
static void log_unmap_notify_event(XUnmapEvent *event)
{
    V("event"); log_x_window(event->event);
    V("window"); log_x_window(event->window);
    V("from_configure"); log_boolean(event->from_configure);
}

/* Log a MapNotifyEvent to standard error output. */
static void log_map_notify_event(XMapEvent *event)
{
    V("event"); log_x_window(event->event);
    V("window"); log_x_window(event->window);
    V("override_redirect"); log_boolean(event->override_redirect);
}

/* Log a MapRequestEvent to standard error output. */
static void log_map_request_event(XMapRequestEvent *event)
{
    V("parent"); log_x_window(event->parent);
    V("window"); log_x_window(event->window);
}

/* Log a ReparentNotifyEvent to standard error output. */
static void log_reparent_notify_event(XReparentEvent *event)
{
    V("event"); log_x_window(event->event);
    V("parent"); log_x_window(event->parent);
    V("window"); log_x_window(event->window);
    V("position"); log_point(event->x, event->y);
    V("override_redirect"); log_boolean(event->override_redirect);
}

/* Log a ConfigureNotifyEvent to standard error output. */
static void log_configure_notify_event(XConfigureEvent *event)
{
    V("event"); log_x_window(event->event);
    V("window"); log_x_window(event->window);
    V("above"); log_x_window(event->above);
    V("position"); log_point(event->x, event->y);
    V("size"); log_size(event->width, event->height);
    V("border_width"); log_unsigned(event->border_width);
    V("override_redirect"); log_boolean(event->override_redirect);
}

/* Log a ConfigureRequestEvent to standard error output. */
static void log_configure_request_event(XConfigureRequestEvent *event)
{
    V("parent"); log_x_window(event->parent);
    V("window"); log_x_window(event->window);
    V("sibling"); log_x_window(event->above);
    V("position"); log_point(event->x, event->y);
    V("size"); log_size(event->width, event->height);
    V("border_width"); log_unsigned(event->border_width);
    V("value_mask"); log_configure_mask(event->value_mask);
}

/* Log a NotifyEventGravity to standard error output. */
static void log_gravity_notify_event(XGravityEvent *event)
{
    V("event"); log_x_window(event->event);
    V("window"); log_x_window(event->window);
    V("position"); log_point(event->x, event->y);
}

/* Log a ResizeRequestEvent to standard error output. */
static void log_resize_request_event(XResizeRequestEvent *event)
{
    V("window"); log_x_window(event->window);
    V("size"); log_size(event->width, event->height);
}

/* Log a CirculateNotify-/CirculateRequestEvent to standard error output. */
static void log_circulate_notify_event(XCirculateEvent *event)
{
    V("event"); log_x_window(event->event);
    V("window"); log_x_window(event->window);
    V("place"); log_place(event->place);
}

/* Log a PropertyNotifyEvent to standard error output. */
static void log_property_notify_event(XPropertyEvent *event)
{
    V("window"); log_x_window(event->window);
    V("atom"); log_atom(event->atom);
    V("time"); log_unsigned(event->time);
    V("state"); log_property_state(event->state);
}

/* Log a SelectionClearEvent to standard error output. */
static void log_selection_clear_event(XSelectionClearEvent *event)
{
    V("window"); log_x_window(event->window);
    V("selection"); log_atom(event->selection);
    V("time"); log_unsigned(event->time);
}

/* Log a SelectionRequestEvent to standard error output. */
static void log_selection_request_event(XSelectionRequestEvent *event)
{
    V("time"); log_unsigned(event->time);
    V("owner"); log_unsigned(event->owner);
    V("requestor"); log_unsigned(event->requestor);
    V("selection"); log_atom(event->selection);
    V("target"); log_atom(event->target);
    V("property"); log_atom(event->property);
}

/* Log a SelectionNotifyEvent to standard error output. */
static void log_selection_notify_event(XSelectionEvent *event)
{
    V("time"); log_unsigned(event->time);
    V("requestor"); log_unsigned(event->requestor);
    V("selection"); log_atom(event->selection);
    V("target"); log_atom(event->target);
    V("property"); log_atom(event->property);
}

/* Log a ColormapNotifyEvent to standard error output. */
static void log_colormap_notify_event(XColormapEvent *event)
{
    V("window"); log_x_window(event->window);
    V("colormap"); log_hexadecimal(event->colormap);
    V("state"); log_unsigned(event->state);
}

/* Log a ClientMessageEvent to standard error output. */
static void log_client_message_event(XClientMessageEvent *event)
{
    const char *state_strings[] = {
        [_NET_WM_STATE_REMOVE] = "remove",
        [_NET_WM_STATE_ADD] = "add",
        [_NET_WM_STATE_TOGGLE] = "toggle",
    };

    V("format"); log_unsigned(event->format);
    V("window"); log_x_window(event->window);
    V("message_type"); log_atom(event->message_type);
    if (event->message_type == ATOM(_NET_CLOSE_WINDOW)) {
        V("time"); log_unsigned(event->data.l[0]);
        V("source"); log_source(event->data.l[1]);
    } else if (event->message_type == ATOM(_NET_MOVERESIZE_WINDOW)) {
        V("gravity"); log_gravity((event->data.l[0] & 0xff));
        V("flags"); log_configure_mask(((event->data.l[0] << 8) & 0x7));
        V("source"); log_source(event->data.l[0] << 12);
        V("geometry"); log_rectangle(event->data.l[1],
                event->data.l[2], event->data.l[3],
                event->data.l[4]);
    } else if (event->message_type == ATOM(_NET_WM_MOVERESIZE)) {
        V("root_position"); log_point(event->data.l[0],
                event->data.l[1]);
        V("direction"); log_direction(event->data.l[2]);
        V("button"); log_button(event->data.l[3]);
        V("source"); log_source(event->data.l[4]);
    } else if (event->message_type == ATOM(_NET_RESTACK_WINDOW)) {
        V("source"); log_source(event->data.l[0]);
        V("sibling"); log_x_window(event->data.l[1]);
        V("detail"); log_notify_detail(event->data.l[2]);
    } else if (event->message_type == ATOM(_NET_REQUEST_FRAME_EXTENTS)) {
        /* no data */
    } else if (event->message_type == ATOM(_NET_WM_DESKTOP)) {
        V("new_desktop"); log_unsigned(event->data.l[0]);
        V("source"); log_source(event->data.l[1]);
    } else if (event->message_type == ATOM(_NET_ACTIVE_WINDOW)) {
        V("source"); log_unsigned(event->data.l[0]);
        V("window"); log_x_window(event->data.l[1]);
    } else if (event->message_type == ATOM(WM_CHANGE_STATE)) {
        V("state"); log_wm_state(event->data.l[0]);
    } else if (event->message_type == ATOM(_NET_WM_STATE)) {
        V("data");
        if (event->data.l[0] >= (long) SIZE(state_strings)) {
            fprintf(log_file, COLOR(RED) "<misformatted>");
        } else {
            fprintf(log_file, COLOR(CYAN) "%s",
                    state_strings[event->data.l[0]]);
        }
        fputc(' ', log_file);
        log_atom(event->data.l[1]);
    } else {
        V("l");
        fprintf(log_file, COLOR(GREEN) "%ld %ld %ld %ld %ld" CLEAR_COLOR,
                event->data.l[0], event->data.l[1], event->data.l[2],
                event->data.l[3], event->data.l[4]);
    }
}

/* Log a MappingNotifyEvent to standard error output. */
static void log_mapping_notify_event(XMappingEvent *event)
{
    V("request"); log_mapping(event->request);
    V("first_keycode"); log_integer(event->first_keycode);
    V("count"); log_integer(event->count);
}

/* Log an event to standard error output. */
static void log_event(XEvent *event)
{
    fprintf(log_file, "[");

    if (event->type == xkb_event_base) {
        switch (((XkbAnyEvent*) event)->xkb_type) {
        case XkbNewKeyboardNotify:
            fputs("XkbNewKeyboardNotify", log_file);
            log_xkb_new_keyboard_notify_event(
                    (XkbNewKeyboardNotifyEvent*) event);
            break;

        case XkbMapNotify:
            fputs("XkbMapNotify", log_file);
            log_xkb_map_notify_event((XkbMapNotifyEvent*) event);
            break;

        default:
            fprintf(log_file, "UNKNOWN_XKB_EVENT(%d)",
                    ((XkbAnyEvent*) event)->xkb_type);
        }
        fputs("]", log_file);
        return;
    }

    if (event->type == randr_event_base) {
        fputs("RandrScreenChangeNotify", log_file);
        log_randr_screen_change_notify_event(
                (XRRScreenChangeNotifyEvent*) event);
        fputs("]", log_file);
        return;
    }

    if (event_to_string(event->type) == NULL) {
        fprintf(log_file, "UNKNOWN_EVENT(%d)",
                event->type);
    } else {
        fputs(event_to_string(event->type), log_file);
    }

    switch (event->type) {
    case KeyPress:
    case KeyRelease:
        log_key_press_event((XKeyPressedEvent*) event);
        break;

    case ButtonPress:
    case ButtonRelease:
        log_button_press_event((XButtonPressedEvent*) event);
        break;

    case MotionNotify:
        log_motion_notify_event((XMotionEvent*) event);
        break;

    case EnterNotify:
    case LeaveNotify:
        log_enter_notify_event((XEnterWindowEvent*) event);
        break;

    case FocusIn:
    case FocusOut:
        log_focus_in_event((XFocusInEvent*) event);
        break;

    case KeymapNotify:
        log_keymap_notify_event((XKeymapEvent*) event);
        break;

    case Expose:
        log_expose_event((XExposeEvent*) event);
        break;

    case GraphicsExpose:
        log_graphics_exposure_event((XGraphicsExposeEvent*) event);
        break;

    case NoExpose:
        log_no_exposure_event((XNoExposeEvent*) event);
        break;

    case VisibilityNotify:
        log_visibility_notify_event((XVisibilityEvent*) event);
        break;

    case CreateNotify:
        log_create_notify_event((XCreateWindowEvent*) event);
        break;

    case DestroyNotify:
        log_destroy_notify_event((XDestroyWindowEvent*) event);
        break;

    case UnmapNotify:
        log_unmap_notify_event((XUnmapEvent*) event);
        break;

    case MapNotify:
        log_map_notify_event((XMapEvent*) event);
        break;

    case MapRequest:
        log_map_request_event((XMapRequestEvent*) event);
        break;

    case ReparentNotify:
        log_reparent_notify_event((XReparentEvent*) event);
        break;

    case ConfigureNotify:
        log_configure_notify_event((XConfigureEvent*) event);
        break;

    case ConfigureRequest:
        log_configure_request_event((XConfigureRequestEvent*) event);
        break;

    case GravityNotify:
        log_gravity_notify_event((XGravityEvent*) event);
        break;

    case ResizeRequest:
        log_resize_request_event((XResizeRequestEvent*) event);
        break;

    case CirculateNotify:
    case CirculateRequest:
        log_circulate_notify_event((XCirculateEvent*) event);
        break;

    case SelectionClear:
        log_selection_clear_event((XSelectionClearEvent*) event);
        break;

    case SelectionRequest:
        log_selection_request_event((XSelectionRequestEvent*) event);
        break;

    case SelectionNotify:
        log_selection_notify_event((XSelectionEvent*) event);
        break;

    case ColormapNotify:
        log_colormap_notify_event((XColormapEvent*) event);
        break;

    case PropertyNotify:
        log_property_notify_event((XPropertyEvent*) event);
        break;

    case ClientMessage:
        log_client_message_event((XClientMessageEvent*) event);
        break;

    case MappingNotify:
        log_mapping_notify_event((XMappingEvent*) event);
        break;
    }
    fputs("]", log_file);
}

/* Log a window to standard error output. */
static void log_window(const FcWindow *window)
{
    log_hexadecimal(window->reference.id);
    fprintf(log_file, COLOR(YELLOW) "<%u>" CLEAR_COLOR,
            window->number);
}

/* Log a window to standard error output. */
static void log_frame(const Frame *frame)
{
    fputs(COLOR(MAGENTA) "[", log_file);
    log_rectangle(frame->x, frame->y, frame->width, frame->height);
    fputs(COLOR(MAGENTA) "]" CLEAR_COLOR, log_file);
    if (frame->number > 0) {
        fprintf(log_file, COLOR(YELLOW) "<%u>" CLEAR_COLOR,
                frame->number);
    }
}

/* Log given data point to log_file. */
static void log_action_data(const struct action_data *data)
{
    switch (data->type) {
    case ACTION_DATA_TYPE_INTEGER:
        fprintf(log_file, COLOR(GREEN) "%" PRIiACTION_INTEGER,
                data->u.integer);
        if ((data->flags & ACTION_DATA_FLAGS_IS_PERCENT)) {
            fputc('%', log_file);
        }
        fputs(CLEAR_COLOR, log_file);
        break;

    case ACTION_DATA_TYPE_STRING:
        log_formatted("%s",
                data->u.string);
        break;

    case ACTION_DATA_TYPE_RELATION: {
        const struct window_relation *const relation =
            &data->u.relation;
        log_formatted("%s,%s %A",
                relation->instance_pattern,
                relation->class_pattern,
                relation->actions);
        break;
    }

    case ACTION_DATA_TYPE_BUTTON: {
        const struct button_binding *const binding = &data->u.button;
        if (binding->is_release) {
            fprintf(log_file, COLOR(YELLOW) "release" CLEAR_COLOR " ");
        }
        if (binding->is_transparent) {
            fprintf(log_file, COLOR(YELLOW) "transparent" CLEAR_COLOR " ");
        }
        if (binding->modifiers != 0) {
            log_formatted("%u+",
                    binding->modifiers);
        }
        log_formatted("%u %A",
                binding->modifiers, binding->actions);
        break;
    }

    case ACTION_DATA_TYPE_KEY: {
        const struct key_binding *const binding = &data->u.key;
        if (binding->is_release) {
            fprintf(log_file, COLOR(YELLOW) "release" CLEAR_COLOR " ");
        }
        if (binding->modifiers != 0) {
            fprintf(log_file, COLOR(GREEN) "%u" CLEAR_COLOR "+",
                    binding->modifiers);
        }

        if (binding->key_symbol == NoSymbol) {
            log_formatted("[%d]",
                    (int) binding->key_code);
        } else {
            fprintf(log_file, COLOR(CYAN) "%s" CLEAR_COLOR,
                    XKeysymToString(binding->key_symbol));
        }

        log_formatted(" %A",
                binding->actions);
        break;
    }

    case ACTION_DATA_TYPE_MAX:
        /* nothing */
        break;
    }
}

/* Log a block of actions to log_file. */
static void log_action_block(const ActionBlock *block)
{
    const struct action_block_item *item;
    const struct action_data *data, *previous_data;

    if (block == NULL) {
        return;
    }

    fputc('(', log_file);

    data = block->data;
    for (size_t i = 0; i < block->number_of_items; i++) {
        const char *action, *space;
        int length;

        if (i > 0) {
            fputs(CLEAR_COLOR ", ", log_file);
        }

        item = &block->items[i];
        action = get_action_string(item->type);
        previous_data = data;
        while (action != NULL) {
            space = strchr(action, ' ');
            if (space != NULL) {
                length = space - action;
                space++;
            } else {
                length = strlen(action);
            }

            /* check if this is after the first part of the action */
            if (action != get_action_string(item->type)) {
                fputs(" ", log_file);
            }

            if (length == 1 &&
                    get_action_data_type_from_identifier(action[0]) !=
                    ACTION_DATA_TYPE_MAX) {
                log_action_data(data);
                data++;
            } else {
                fprintf(log_file, COLOR(YELLOW) "%.*s" CLEAR_COLOR,
                        length, action);
            }

            action = space;
        }
        data = previous_data + item->data_count;
    }

    fputs(CLEAR_COLOR ")", log_file);
}

/* Log the display information to standard error output. */
static void log_display(Display *display)
{
    int connection_number;
    int screen, screen_count;
    unsigned long black_pixel, white_pixel;
    Colormap colormap;
    int depth;
    Window root;
    int width;
    int height;
    int millimeter_width;
    int millimeter_height;

    connection_number = ConnectionNumber(display);
    screen = DefaultScreen(display);
    screen_count = ScreenCount(display);
    black_pixel = BlackPixel(display, screen);
    white_pixel = WhitePixel(display, screen);
    colormap = DefaultColormap(display, screen);
    depth = DefaultDepth(display, screen);
    root = DefaultRootWindow(display);
    width = DisplayWidth(display, screen);
    height = DisplayHeight(display, screen);
    millimeter_width = DisplayWidthMM(display, screen);
    millimeter_height = DisplayHeightMM(display, screen);

    fprintf(log_file,
            "Display[connection_number="); log_integer(connection_number);
    V("default_screen"); log_integer(screen);
    V("screen_count"); log_integer(screen_count);
    V("white_pixel"); log_hexadecimal(white_pixel);
    V("black_pixel"); log_hexadecimal(black_pixel);
    V("default_colormap"); log_hexadecimal(colormap);
    V("default_depth"); log_integer(depth);
    V("default_root"); log_hexadecimal(root);
    V("size"); log_size(width, height);
    V("millimeter_size"); log_size(millimeter_width, millimeter_height);
    fputs("]", log_file);
}

/* Log to log_file and use the variable argument list. */
void _log_va_formatted(const char *format, va_list list)
{
    char buffer[64];

    for (; format[0] != '\0'; format++) {
        if (format[0] == '%') {
            switch (format[1]) {
            case '%':
                format++;
                /* fall through */
            case '\0':
                fputc('%', log_file);
                continue;

            /* print a point */
            case 'P': {
                const int x = va_arg(list, int);
                const int y = va_arg(list, int);
                log_point(x, y);
                break;
            }

            /* print a size */
            case 'S': {
                const unsigned width = va_arg(list, unsigned);
                const unsigned height = va_arg(list, unsigned);
                log_size(width, height);
                break;
            }

            /* print a rectangle */
            case 'R': {
                const int x = va_arg(list, int);
                const int y = va_arg(list, int);
                const unsigned width = va_arg(list, unsigned);
                const unsigned height = va_arg(list, unsigned);
                log_rectangle(x, y, width, height);
                break;
            }

            /* print a boolean */
            case 'b':
                log_boolean(va_arg(list, int));
                break;

            /* print an X window */
            case 'w':
                log_x_window(va_arg(list, Window));
                break;

            /* print a window */
            case 'W':
                log_window(va_arg(list, FcWindow*));
                break;

            /* print a window mode */
            case 'm':
                log_window_mode(va_arg(list, window_mode_t));
                break;

            /* print a frame */
            case 'F':
                log_frame(va_arg(list, Frame*));
                break;

            /* print an X event */
            case 'V':
                log_event(va_arg(list, XEvent*));
                break;

            /* print a block of actions */
            case 'A':
                log_action_block(va_arg(list, const ActionBlock*));
                break;

            /* print a parse data point */
            case 'T':
                log_action_data(va_arg(list, const struct action_data*));
                break;

            /* print an X atom */
            case 'a':
                log_atom(va_arg(list, Atom));
                break;

            /* print display information */
            case 'D':
                log_display(va_arg(list, Display*));
                break;

            /* standard print format specifiers */
            default: {
                unsigned i = 0;

                fputs(COLOR(GREEN), log_file);

                /* move a segment of `format` into `buffer` and feed it into the
                 * real `printf()`
                 */
                do {
                    buffer[i] = format[i];
                    if (strchr(PRINTF_FORMAT_SPECIFIERS, format[i]) != NULL) {
                        buffer[i + 1] = '\0';
                        vfprintf(log_file, buffer, list);
                        fputs(CLEAR_COLOR, log_file);
                        format += i - 1;
                        /* all clean */
                        i = 0;
                        break;
                    }
                    i++;
                } while (format[i] != '\0' && i < sizeof(buffer) - 2);

                /* if anything is weird print the rest of the format string */
                if (i > 0) {
                    fputs(format, log_file);
                    while (format[2] != '\0') {
                        format++;
                    }
                }
                break;
            }
            }
            format++;
        } else {
            fputc(format[0], log_file);
        }
    }
}

/* Print a formatted string to standard error output. */
#ifdef DEBUG
void _log_formatted(log_severity_t severity, const char *file, int line,
        const char *format, ...)
{
    va_list list;
    char buffer[64];
    time_t current_time;
    struct tm *tm;

    /* omit logging if not severe enough */
    if (log_severity > severity) {
        return;
    }

    /* print the time and file with line number at the front */
    current_time = time(NULL);
    tm = localtime(&current_time);
    strftime(buffer, sizeof(buffer),
            severity == LOG_SEVERITY_ERROR ? COLOR(RED) "{%F %T} " :
                COLOR(GREEN) "[%F %T] ", tm);
    fprintf(log_file, "%s" COLOR(YELLOW) "(%s:%d) " CLEAR_COLOR,
            buffer, file, line);

    /* parse the format string */
    va_start(list, format);
    _log_va_formatted(format, list);
    va_end(list);
}
#else
void _log_formatted(log_severity_t severity, const char *format, ...)
{
    va_list list;
    char buffer[64];
    time_t current_time;
    struct tm *tm;

    /* omit logging if not severe enough */
    if (log_severity > severity) {
        return;
    }

    /* print the time and file with line number at the front */
    current_time = time(NULL);
    tm = localtime(&current_time);
    strftime(buffer, sizeof(buffer),
            severity == LOG_SEVERITY_ERROR ? COLOR(RED) "{%F %T} " :
                COLOR(GREEN) "[%F %T] ", tm);
    fputs(buffer, log_file);

    /* parse the format string */
    va_start(list, format);
    _log_va_formatted(format, list);
    va_end(list);
}
#endif

/* Same as above but write no prolog. */
void log_formatted(const char *format, ...)
{
    va_list list;

    va_start(list, format);
    _log_va_formatted(format, list);
    va_end(list);
}
