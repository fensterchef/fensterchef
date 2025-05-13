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

/* Synchronize the local data with the X server. */
void synchronize_with_server(void);

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
