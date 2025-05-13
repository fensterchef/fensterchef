#ifndef X11__SYNCHRONIZE_H
#define X11__SYNCHRONIZE_H

/**
 * Synchronization with the X11 server.  Keeping all data up to date.
 *
 * The essence of this window manager is to have an internal state that is very
 * quick and easy to modify.  But this of course needs to be put live which
 * happens on an event cycle.
 */

#include <X11/X.h>

#include "utility/types.h"

#define SYNCHRONIZE_CLIENT_LIST     (1 << 0)
#define SYNCHRONIZE_ROOT_CURSOR     (1 << 1)
#define SYNCHRONIZE_BUTTON_BINDING  (1 << 2)
#define SYNCHRONIZE_KEY_BINDING     (1 << 3)
#define SYNCHRONIZE_ALL            ((1 << 4) - 1)

/* reference to an X window */
typedef struct x_reference {
    /* the id of the window */
    Window id;
    /* if the window is mapped (visible) */
    bool is_mapped;
    /* position and size of the window */
    int x;
    int y;
    unsigned width;
    unsigned height;
    /* the size of the border */
    unsigned int border_width;
    /* the color of the border */
    uint32_t border;
} XReference;

/* More information is synchronized without the use of flags which includes:
 * - Updating the window stack ordering
 * - Window positioning/colors
 */

/* hints set by all parts of the program indicating that a specific part needs
 * to be synchronized
 */
extern unsigned synchronization_flags;

/* Synchronize the local data with the X server.
 *
 * @flags is an OR combination of `SYNCHRONIZE_*` and is combined with
 *        `synchronization_flags` if it is 0.
 */
void synchronize_with_server(unsigned flags);

/* Show the client on the X server. */
void map_client(XReference *reference);

/* Show the client on the X server at the top. */
void map_client_raised(XReference *reference);

/* Hide the client on the X server. */
void unmap_client(XReference *reference);

/* Set the size of a window associated to the X server. */
void configure_client(XReference *reference, int x, int y, unsigned width,
        unsigned height, unsigned border_width);

/* Set the background and border color of @client. */
void change_client_attributes(XReference *reference, uint32_t border_color);

#endif
