#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include "font.h"
#include "utility/types.h"
#include "x11/synchronize.h"

/* notification window */
typedef struct notification {
    /* the X correspondence */
    XReference reference;
    /* Xft drawing context */
    XftDraw *xft_draw;
    /* text color */
    uint32_t foreground;
    /* background color */
    uint32_t background;
} Notification;

/* notification window for fensterchef windows */
extern Notification *system_notification;

/* Create a notification window for showing text. */
Notification *create_notification(void);

/* Show the notification window with given message at given coordinates for
 * a duration in seconds specified in the configuration.
 *
 * @message UTF-8 string.
 * @x Center x position.
 * @y Center y position.
 */
void set_system_notification(const utf8_t *message, int x, int y);

#endif
