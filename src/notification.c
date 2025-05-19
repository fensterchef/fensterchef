#include <unistd.h>

#include "configuration.h"
#include "log.h"
#include "notification.h"
#include "x11/display.h"
#include "x11/synchronize.h"

/* notification window */
Notification *system_notification;

/* Initialize the notification window. */
static int initialize_notification(Notification *notification)
{
    XSetWindowAttributes attributes;

    notification->reference.x = -1;
    notification->reference.y = -1;
    notification->reference.width = 1;
    notification->reference.height = 1;
    notification->reference.border_width = configuration.border_size;
    notification->reference.border = configuration.foreground;
    notification->foreground = configuration.foreground;
    attributes.border_pixel = notification->reference.border;
    attributes.backing_pixel = configuration.background;
    /* indicate to not manage the window */
    attributes.override_redirect = True;
    notification->reference.id = XCreateWindow(display,
                DefaultRootWindow(display), notification->reference.x,
                notification->reference.y, notification->reference.width,
                notification->reference.height,
                notification->reference.border_width, CopyFromParent,
                InputOutput, (Visual*) CopyFromParent,
                CWBorderPixel | CWBackPixel | CWOverrideRedirect,
                &attributes);

    if (notification->reference.id == None) {
        LOG_ERROR("failed creating notification window\n");
        return ERROR;
    }

    const char *const notification_name = "[fensterchef] notification";
    XStoreName(display, notification->reference.id, notification_name);

    /* create an XftDraw object if not done already */
    notification->xft_draw = XftDrawCreate(display, notification->reference.id,
            DefaultVisual(display, DefaultScreen(display)),
            DefaultColormap(display, DefaultScreen(display)));

    if (notification->xft_draw == NULL) {
        LOG_ERROR("could not create XftDraw for the notification window\n");
        XDestroyWindow(display, notification->reference.id);
        return ERROR;
    }
    return OK;
}

/* Create a notification window for showing text. */
Notification *create_notification(void)
{
    Notification *notification;

    ALLOCATE_ZERO(notification, 1);

    if (initialize_notification(notification) == ERROR) {
        free(notification);
        return NULL;
    }

    return notification;
}

/* Show a notification window and render text inside of it. */
static int render_notification(Notification *notification,
        const utf8_t *message, int x, int y)
{
    XftColor text_color, background_color;
    FcChar32 *glyphs;
    int glyph_count;
    Text text;

    if (allocate_xft_color(notification->foreground, &text_color) == ERROR) {
        return ERROR;
    }

    if (allocate_xft_color(notification->background,
                &background_color) == ERROR) {
        free_xft_color(&text_color);
        return ERROR;
    }

    glyphs = get_glyphs(message, -1, &glyph_count);

    initialize_text(&text, glyphs, glyph_count);

    /* add the padding */
    text.x += configuration.text_padding / 2;
    text.y += configuration.text_padding / 2;
    text.width += configuration.text_padding;
    text.height += configuration.text_padding;

    /* center the text window */
    x -= text.width / 2;
    y -= text.height / 2;

    /* attempt to put the window fully in bounds */
    const unsigned
        display_width = DisplayWidth(display, DefaultScreen(display)),
        display_height = DisplayHeight(display, DefaultScreen(display));
    if (x < 0) {
        x = 0;
    } else if ((unsigned) x + text.width + configuration.border_size * 2 >=
            display_width) {
        x = display_width - text.width - configuration.border_size * 2;
    }
    if (y < 0) {
        y = 0;
    } else if ((unsigned) y + text.height + configuration.border_size * 2 >=
            display_height) {
        y = display_height - text.height - configuration.border_size * 2;
    }

    /* set the window size, position and set it above */
    configure_client(&notification->reference,
            x, y, text.width, text.height,
            notification->reference.border_width);

    /* show the window */
    map_client_raised(&notification->reference);

    /* draw background and text */
    XftDrawRect(notification->xft_draw, &background_color,
            0, 0, text.width, text.height);
    draw_text(notification->xft_draw, &text_color,
            text.x, text.y, &text);

    clear_text(&text);

    LOG_DEBUG("showed notification: %s at %R with offset %P\n",
            message,
            notification->reference.x, notification->reference.y,
            notification->reference.width, notification->reference.height,
            text.x, text.y);

    free_xft_color(&background_color);
    free_xft_color(&text_color);

    return OK;
}

/* Show the notification window with given message at given coordinates for
 * a duration in seconds specified in the configuration.
 */
void set_system_notification(const utf8_t *message, int x, int y)
{
    if (configuration.notification_duration == 0) {
        return;
    }

    /* initialize the notification window if not done already */
    if (system_notification == NULL) {
        system_notification = create_notification();
        if (system_notification == NULL) {
            return;
        }
    }

    /* change border color and size of the notification window */
    change_client_attributes(&system_notification->reference,
            configuration.foreground);
    system_notification->background = configuration.background;

    if (render_notification(system_notification, message, x, y) == ERROR) {
        return;
    }

    /* set an alarm to trigger after the specified seconds */
    alarm(configuration.notification_duration);
}
