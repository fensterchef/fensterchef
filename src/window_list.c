#include <X11/XKBlib.h>

#include "configuration.h"
#include "font.h"
#include "frame.h"
#include "log.h"
#include "notification.h"
#include "window.h"
#include "window_list.h"
#include "x11/display.h"

/* user window list window */
struct window_list WindowList;

/* Initialize the window list. */
static int initialize_window_list(void)
{
    XSetWindowAttributes attributes;

    if (WindowList.reference.id == None) {
        WindowList.reference.x = -1;
        WindowList.reference.y = -1;
        WindowList.reference.width = 1;
        WindowList.reference.height = 1;
        WindowList.reference.border_width = configuration.border_size;
        WindowList.reference.border = configuration.border_color;
        attributes.border_pixel = WindowList.reference.border;
        attributes.backing_pixel = configuration.background;
        attributes.event_mask = KeyPressMask | FocusChangeMask | ExposureMask;
        /* indicate to not manage the window */
        attributes.override_redirect = True;
        WindowList.reference.id = XCreateWindow(display,
                    DefaultRootWindow(display), WindowList.reference.x,
                    WindowList.reference.y, WindowList.reference.width,
                    WindowList.reference.height,
                    WindowList.reference.border_width, CopyFromParent,
                    InputOutput, (Visual*) CopyFromParent,
                    CWBorderPixel | CWBackPixel | CWOverrideRedirect |
                        CWEventMask,
                    &attributes);

        if (WindowList.reference.id == None) {
            LOG_ERROR("failed creating window list window\n");
            return ERROR;
        }

        const char *const window_list_name = "[fensterchef] window list";
        XStoreName(display, WindowList.reference.id, window_list_name);
    }

    /* create an XftDraw object if not done already */
    if (WindowList.xft_draw == NULL) {
        WindowList.xft_draw = XftDrawCreate(display, WindowList.reference.id,
                DefaultVisual(display, DefaultScreen(display)),
                DefaultColormap(display, DefaultScreen(display)));

        if (WindowList.xft_draw == NULL) {
            LOG_ERROR("could not create drawing context for the window list "
                        "window\n");
            return ERROR;
        }
    }
    return OK;
}

/* Check if @window should appear in the window list. */
static bool is_window_in_window_list(FcWindow *window)
{
    return is_window_focusable(window);
}

/* Get character indicating the window state. */
static inline char get_indicator_character(FcWindow *window)
{
    return window == Window_focus ? '*' :
            !window->state.is_visible ? '-' :
            window->state.mode == WINDOW_MODE_FLOATING ? '=' :
            window->state.mode == WINDOW_MODE_FULLSCREEN ? 'F' : '+';
}

/* Get a string representation of a window. */
static inline int get_window_string(FcWindow *window, utf8_t *buffer,
        size_t buffer_size)
{
    return snprintf(buffer, buffer_size, "%u%c%s",
            window->number, get_indicator_character(window),
            window->properties.name);
}

/* Get the window currently selected in the window list. */
static FcWindow *get_selected_window(void)
{
    unsigned index;

    index = WindowList.selected;
    for (FcWindow *window = Window_first;
            window != NULL;
            window = window->next) {
        if (!is_window_in_window_list(window)) {
            continue;
        }
        if (index == 0) {
            return window;
        }
        index--;
    }
    return NULL;
}

/* Render the window list.
 *
 * @return ERROR if the rendering failed, OK otherwise.
 */
static int render_window_list(void)
{
    XftColor background, foreground;
    Monitor *monitor;
    utf8_t buffer[256];
    unsigned item_count = 0;
    FcChar32 *glyphs;
    int glyph_count;
    int y = 0;
    unsigned width = 0, height = 0;
    int selected_y = 0, selected_height = 0;

    if (allocate_xft_color(configuration.background, &background) == ERROR) {
        return ERROR;
    }

    if (allocate_xft_color(configuration.foreground, &foreground) == ERROR) {
        free_xft_color(&background);
        return ERROR;
    }

    /* get the monitor the window list should be on */
    monitor = get_focused_monitor();

    /* put enough text items on the stack */
    Text *texts[Window_count + 1];

    /* measure the maximum needed width/height and count the windows */
    for (FcWindow *window = Window_first;
            window != NULL;
            window = window->next) {
        Text *text;

        if (!is_window_in_window_list(window)) {
            continue;
        }


        const int length = get_window_string(window, buffer, sizeof(buffer));
        glyphs = get_glyphs(buffer, length, &glyph_count);
        text = create_text(glyphs, glyph_count);
        width = MAX(width, text->width);

        /* do not "overflow" the height */
        if (height < monitor->height) {
            height += text->height;
        }

        if (item_count == WindowList.selected) {
            selected_y = y;
            selected_height = text->height;
        }

        y += text->height;

        texts[item_count] = text;
        item_count++;
    }

    /* add a single text item indicating that there are no focusable windows */
    if (item_count == 0) {
        const int length = snprintf(buffer, sizeof(buffer),
                    "There are %u other windows",
                Window_count);
        glyphs = get_glyphs(buffer, length, &glyph_count);
        texts[0] = create_text(glyphs, glyph_count);
        width = texts[0]->width;
        height = texts[0]->height;
        item_count = 1;

        /* reset the selected item */
        WindowList.selected = 0;
        selected_y = 0;
        selected_height = texts[0]->height;
    } else {
        /* if windows were removed, dynamically adjust the selected item so that
         * when it is out of bounds, the last item is selected
         */
        if (WindowList.selected >= item_count) {
            selected_height = texts[item_count - 1]->height;
            selected_y = y - selected_height;
            WindowList.selected = item_count - 1;
        }
    }

    width += configuration.text_padding;
    /* only apply half padding for the top, ignore the bottom */
    height += configuration.text_padding / 2;

    width = MIN(width, monitor->width / 2);
    height = MIN(height, monitor->height);

    /* change border color of the window list window */
    change_client_attributes(&WindowList.reference, configuration.foreground);

    /* set the list position and size so it is in the top center of the
     * focused monitor
     */
    configure_client(&WindowList.reference,
            monitor->x + (monitor->width - width) / 2 -
                configuration.border_size, monitor->y,
            width, height, configuration.border_size);

    selected_y += configuration.text_padding / 2;
    /* special case so that the padding is shown at the top again */
    if (WindowList.selected == 0) {
        WindowList.scrolling = 0;
    } else if (selected_y < WindowList.scrolling) {
        WindowList.scrolling = selected_y;
    } else if (selected_y + selected_height >
            (int) height + WindowList.scrolling) {
        WindowList.scrolling = selected_y + selected_height - height;
    }

    y = configuration.text_padding / 2 - WindowList.scrolling;

    LOG_DEBUG("showing items starting from %d (pixel scroll=%u)\n",
            y, WindowList.scrolling);

    /* render the items showing the window names */
    for (unsigned i = 0; i < item_count; i++) {
        XftColor *background_pointer, *foreground_pointer;
        int rect_y, rect_height;

        Text *const text = texts[i];

        /* add a little extra to the top if this is the first item */
        if (i == 0) {
            rect_y = 0;
            rect_height = text->height + configuration.text_padding / 2;
        } else {
            rect_y = y;
            rect_height = text->height;
        }

        /* only render the item if it is visible */
        if (rect_y + rect_height >= 0) {
            /* use normal or inverted colors */
            if (i != WindowList.selected) {
                foreground_pointer = &foreground;
                background_pointer = &background;
            } else {
                foreground_pointer = &background;
                background_pointer = &foreground;
            }

            LOG_DEBUG("drawing list item %d in rect %R\n",
                    i, 0, rect_y, width, rect_height);

            /* draw background and text */
            XftDrawRect(WindowList.xft_draw, background_pointer,
                    0, rect_y, width, rect_height);
            draw_text(WindowList.xft_draw, foreground_pointer,
                    configuration.text_padding / 2 + text->x,
                    y + text->y, text);
        }

        y += text->height;

        /* stop rendering if there is no more space */
        if (y >= (int) height) {
            break;
        }
    }

    /* clear all text objects */
    for (unsigned i = 0; i < item_count; i++) {
        destroy_text(texts[i]);
    }

    free_xft_color(&foreground);
    free_xft_color(&background);

    return OK;
}

/* Show @window and focus it.
 *
 * @shift controls how the window appears. Either it is put directly into the
 *        current frame or simply shown which allows auto splitting to occur.
 */
static inline void focus_and_let_window_appear(FcWindow *window, bool shift)
{
    /* if shift is down, show the window in the current frame */
    if (shift && !window->state.is_visible) {
        stash_frame(Frame_focus);
        set_window_mode(window, WINDOW_MODE_TILING);
    } else {
        /* put floating windows on the top */
        update_window_layer(window);
    }
    show_window(window);
    set_focus_window_with_frame(window);
}

/* Handle a key press for the window list window. */
static void handle_key_press(XKeyPressedEvent *event)
{
    KeySym key_symbol;

    if (event->window != WindowList.reference.id) {
        return;
    }

    key_symbol = XkbKeycodeToKeysym(display, event->keycode, 0, 0);
    switch (key_symbol) {
    /* cancel selection */
    case XK_q:
    case XK_n:
    case XK_Escape:
        unmap_client(&WindowList.reference);
        break;

    /* confirm selection */
    case XK_y:
    case XK_Return: {
        FcWindow *const selected = get_selected_window();
        if (selected != NULL && selected != Window_focus) {
            focus_and_let_window_appear(selected, !!(event->state & ShiftMask));
        }
        unmap_client(&WindowList.reference);
        break;
    }

    /* go to the first item */
    case XK_Home:
        WindowList.selected = 0;
        break;

    /* go to the last item */
    case XK_End:
        WindowList.selected = INT_MAX;
        break;

    /* go to the previous item */
    case XK_h:
    case XK_k:
    case XK_Left:
    case XK_Up:
        if (WindowList.selected > 0) {
            WindowList.selected--;
        }
        break;

    /* go to the next item */
    case XK_l:
    case XK_j:
    case XK_Right:
    case XK_Down:
        WindowList.selected++;
        break;
    }
}

/* Handle an incoming X event for the window list. */
void handle_window_list_event(XEvent *event)
{
    if (!WindowList.reference.is_mapped) {
        return;
    }

    switch (event->type) {
    /* a key was pressed */
    case KeyPress:
        handle_key_press(&event->xkey);
        render_window_list();
        break;

    /* regain focus if it was taken away */
    case FocusOut: {
        XFocusOutEvent *focus;

        focus = (XFocusOutEvent*) event;
        if (focus->window != WindowList.reference.id) {
            break;
        }

        /* the mode must be NotifyNormal and not grab */
        if (focus->mode != NotifyNormal || focus->detail != NotifyNonlinear) {
            break;
        }

        XSetInputFocus(display, WindowList.reference.id,
                RevertToParent, CurrentTime);
        break;
    }

    /* re-render after a few chosen events */
    case KeyRelease:
    case ButtonPress:
    case ButtonRelease:
    case Expose:
    case MapNotify:
    case UnmapNotify:
        render_window_list();
        break;
    }
}

/* Show the window list on screen. */
int show_window_list(void)
{
    FcWindow *selected;
    unsigned index = 0;

    if (initialize_window_list() != OK) {
        return ERROR;
    }

    /* if the window list is already shown, toggle the visibity */
    if (WindowList.reference.is_mapped) {
        unmap_client(&WindowList.reference);
        return OK;
    }

    /* get the initially selected window */
    if (Window_focus != NULL) {
        for (selected = Window_first;
                selected != Window_focus;
                selected = selected->next) {
            index++;
        }
    } else {
        for (selected = Window_first;
                selected != NULL;
                selected = selected->next) {
            if (is_window_in_window_list(selected)) {
                break;
            }
        }
    }

    WindowList.selected = index;

    /* do an initial rendering, this also sizes the window */
    if (render_window_list() == ERROR) {
        LOG_ERROR("could not render window list\n");
        return ERROR;
    }

    /* show the window list window on screen */
    map_client_raised(&WindowList.reference);

    /* focus the window list */
    XSetInputFocus(display, WindowList.reference.id,
            RevertToParent, CurrentTime);

    Window_server_focus = NULL;
    return OK;
}
