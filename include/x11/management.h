#ifndef X11_MANAGEMENT_H
#define X11_MANAGEMENT_H

#include <stdint.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "bits/window.h"
#include "utility/utility.h"

/* Try to take control of the window manager role.
 *
 * Call this after `initialize_connection()`. This will set
 * `Fensterchef_is_running` to true if it succeeds.
 *
 * If it fails, it aborts fensterchef.
 */
void take_control(void);

/* Go through all already existing windows and manage them.
 *
 * Call this after `initialize_monitors()`.
 */
void query_existing_windows(void);

/* Set the input focus to @window. This window may be `NULL`. */
void set_input_focus(FcWindow *window);

#endif
