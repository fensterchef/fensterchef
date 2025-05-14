#ifndef MONITOR_H
#define MONITOR_H

/**
 * Monitors are physical rectangular regions that usually represent the output
 * devices.
 *
 * This has all needed to manage outputs and lots of helpful utility.
 */

#include "bits/frame.h"
#include "bits/monitor.h"
#include "bits/window.h"
#include "utility/attributes.h"
#include "utility/linked_list.h"
#include "utility/types.h"

/* A monitor is a rectangular region tied to an X screen. */
struct monitor {
    /* name of the monitor, used as key */
    char *name;

    /* region of the monitor to cut off */
    Extents strut;

    /* the position and size of the monitor */
    int x;
    int y;
    unsigned width;
    unsigned height;

    /* root frame */
    Frame *frame;

    /* next monitor in the linked list */
    Monitor *next;
};

/* the first monitor in the monitor linked list */
extern SINGLY_LIST(Monitor, Monitor_first);

/* Try to initialize randr and the internal monitor linked list. */
void initialize_monitors(void);

/* Get a list of monitors that are associated to the screen.
 *
 * @return NULL when randr is not supported or when there are no monitors.
 */
Monitor *query_monitors(void);

/* Get a monitor with given name. */
Monitor *get_monitor_by_name(const char *name);

/* Get the first monitor matching given pattern. */
Monitor *get_monitor_by_pattern(const char *pattern);

/* Get the monitor that overlaps given rectangle the most.
 *
 * @return NULL if no monitor intersects the rectangle at all.
 */
Monitor *get_monitor_from_rectangle(int x, int y,
        unsigned width, unsigned height);

/* Get the monitor that overlaps given rectangle the most or primary if there
 * are there are no intersections.
 */
Monitor *get_monitor_from_rectangle_or_primary(int x, int y,
        unsigned width, unsigned height);

/* The most efficient way to get the monitor containing given frame.
 *
 * @return NULL if the frame is not visible.
 */
Monitor *get_monitor_containing_frame(Frame *frame);

/* Get the monitor the window is on. */
Monitor *get_monitor_containing_window(FcWindow *window);

/* Get a window covering given monitor. */
FcWindow *get_window_covering_monitor(Monitor *monitor);

/* Get the monitor on the left of @monitor.
 *
 * @return NULL if there is no monitor at the left.
 */
Monitor *get_left_monitor(Monitor *monitor);

/* Get the monitor above @monitor.
 *
 * @return NULL if there is no monitor above.
 */
Monitor *get_above_monitor(Monitor *monitor);

/* Get the monitor on the right of @monitor.
 *
 * @return NULL if there is no monitor at the right.
 */
Monitor *get_right_monitor(Monitor *monitor);

/* Get the monitor below of @monitor.
 *
 * @return NULL if there is no monitor below.
 */
Monitor *get_below_monitor(Monitor *monitor);

/* Merge given monitor linked list into the screen's monitor list.
 *
 * @monitors may be NULL to indicate no monitors are there or randr is not
 *           supported.
 */
void merge_monitors(_Nullable Monitor *monitors);

/* Go through all windows to find the total strut and apply it to all monitors.
 *
 * This also adjusts all dock windows so that they do not overlap.
 */
void reconfigure_monitor_frames(void);

/* Adjust given @x and @y such that it follows the gravity.
 *
 * @gravity is an X11 constant ending with Gravity.
 *          See "BitGravity" in "X11/X.h".
 */
void adjust_for_window_gravity(Monitor *monitor, _Inout int *x, _Inout int *y,
        unsigned int width, unsigned int height, int gravity);

#endif
