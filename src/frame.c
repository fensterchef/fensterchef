#include <inttypes.h>

#include "configuration.h"
#include "fensterchef.h"
#include "frame.h"
#include "log.h"
#include "monitor.h"
#include "utility/utility.h"
#include "window.h"

/* the last frame in the frame stashed linked list */
Frame *Frame_last_stashed;

/* the currently selected/focused frame */
Frame *Frame_focus;

/*********************************
 * Frame creation an destruction *
 *********************************/

/* Increment the reference count of the frame. */
inline void reference_frame(Frame *frame)
{
    frame->reference_count++;
}

/* Decrement the reference count of the frame and free @frame when it reaches 0.
 */
inline void dereference_frame(Frame *frame)
{
    frame->reference_count--;
    if (frame->reference_count == 0) {
        free(frame);
    }
}

/* Create a frame object. */
inline Frame *create_frame(void)
{
    Frame *frame;

    ALLOCATE_ZERO(frame, 1);
    frame->reference_count = 1;
    return frame;
}

/* Free the frame object. */
void destroy_frame(Frame *frame)
{
    Frame *previous;

    if (frame->parent != NULL) {
        LOG_ERROR("the frame being destroyed still has a parent\n");
        remove_frame(frame);
        /* execution should be fine after this point but this still only gets
         * triggered by a bug, frames should always be disconnected before
         * destroying them
         */
    }

    if (frame->left != NULL) {
        LOG_ERROR("the frame being destroyed still has children, "
                "this might leak memory\n");
    }

    /* we could also check for a case where the frame is the root frame of a
     * monitor but checking too much is not good; these things should not happen
     * anyway
     */

    if (frame == Frame_focus) {
        LOG_ERROR("the focused frame is being freed :(\n");
        Frame_focus = NULL;
        /* we can only hope the focused frame is set to something sensible after
         * this, we can not set it to something sensible here
         */
    }

    /* remove from the stash linked list if it is contained in it */
    if (Frame_last_stashed == frame) {
        Frame_last_stashed = Frame_last_stashed->previous_stashed;
    } else {
        previous = Frame_last_stashed;
        while (previous != NULL && previous->previous_stashed != frame) {
            previous = previous->previous_stashed;
        }
        if (previous != NULL) {
            previous->previous_stashed = frame->previous_stashed;
        }
    }

    dereference_frame(frame);
}

/*****************
 * Frame utility *
 *****************/

/* Get the frame above the given one that has no parent. */
Frame *get_root_frame(const Frame *frame)
{
    if (frame == NULL) {
        return NULL;
    }
    while (frame->parent != NULL) {
        frame = frame->parent;
    }
    return (Frame*) frame;
}

/* Check if @frame has given number and if not, try to find a child frame with
 * this number.
 */
static Frame *get_frame_by_number_recursively(Frame *frame, unsigned number)
{
    Frame *left;

    if (frame->number == number) {
        return frame;
    }

    if (frame->left == NULL) {
        return NULL;
    }

    left = get_frame_by_number_recursively(frame->left, number);
    if (left != NULL) {
        return left;
    }

    return get_frame_by_number_recursively(frame->right, number);
}

/* Look through all visible frames to find a frame with given @number. */
Frame *get_frame_by_number(unsigned number)
{
    Frame *frame = NULL;

    for (Monitor *monitor = Monitor_first; monitor != NULL;
            monitor = monitor->next) {
        frame = get_frame_by_number_recursively(monitor->frame, number);
        if (frame != NULL) {
            break;
        }
    }
    return frame;
}

/* Check if the given frame has no splits and no window. */
inline bool is_frame_void(const Frame *frame)
{
    return frame->left == NULL && frame->window == NULL;
}

/* Check if the given point is within the given frame. */
bool is_point_in_frame(const Frame *frame, int x, int y)
{
    return x >= frame->x && y >= frame->y &&
        x - (int) frame->width < frame->x &&
        y - (int) frame->height < frame->y;
}

/* Get the frame at given position. */
Frame *get_frame_at_position(int x, int y)
{
    Frame *frame;

    for (Monitor *monitor = Monitor_first; monitor != NULL;
            monitor = monitor->next) {
        frame = monitor->frame;
        if (is_point_in_frame(frame, x, y)) {
            /* recursively move into child frame until we are at a leaf */
            while (frame->left != NULL) {
                if (is_point_in_frame(frame->left, x, y)) {
                    frame = frame->left;
                    continue;
                }
                if (is_point_in_frame(frame->right, x, y)) {
                    frame = frame->right;
                    continue;
                }
                return NULL;
            }
            return frame;
        }
    }
    return NULL;
}

/* Replace @frame with @with. */
void replace_frame(Frame *frame, Frame *with)
{
    frame->number = with->number;
    with->number = 0;
    /* reparent the child frames */
    if (with->left != NULL) {
        frame->split_direction = with->split_direction;
        frame->ratio = with->ratio;
        frame->left = with->left;
        frame->right = with->right;
        frame->left->parent = frame;
        frame->right->parent = frame;
        /* one null set for convenience, otherwise the very natural looking
         * swapping in `exchange_frames()` would not work
         */
        frame->window = NULL;

        with->left = NULL;
        with->right = NULL;
    } else {
        frame->window = with->window;
        /* two null sets for convenience */
        frame->left = NULL;
        frame->right = NULL;

        with->window = NULL;
    }

    /* size the children so they fit into their new parent */
    resize_frame_and_ignore_ratio(frame, frame->x, frame->y,
            frame->width, frame->height);
}

/* Get the gaps the frame applies to its inner window. */
void get_frame_gaps(const Frame *frame, Extents *gaps)
{
    Frame *root;

    root = get_root_frame(frame);
    if (root->x == frame->x) {
        gaps->left = configuration.gaps_outer[0];
    } else {
        gaps->left = configuration.gaps_inner[2];
    }

    if (root->y == frame->y) {
        gaps->top = configuration.gaps_outer[1];
    } else {
        gaps->top = configuration.gaps_inner[3];
    }

    if (root->x + root->width == frame->x + frame->width) {
        gaps->right = configuration.gaps_outer[2];
    } else {
        gaps->right = configuration.gaps_inner[0];
    }

    if (root->y + root->height == frame->y + frame->height) {
        gaps->bottom = configuration.gaps_outer[3];
    } else {
        gaps->bottom = configuration.gaps_inner[1];
    }
}

/* Resize the inner window to fit within the frame. */
void reload_frame(Frame *frame)
{
    Extents gaps;

    if (frame->window == NULL) {
        return;
    }

    get_frame_gaps(frame, &gaps);

    gaps.right += gaps.left + configuration.border_size * 2;
    gaps.bottom += gaps.top + configuration.border_size * 2;
    set_window_size(frame->window,
            frame->x + gaps.left,
            frame->y + gaps.top,
            gaps.right > 0 && frame->width < (unsigned) gaps.right ? 0 :
                frame->width - gaps.right,
            gaps.bottom > 0 && frame->height < (unsigned) gaps.bottom ? 0 :
                frame->height - gaps.bottom);
}

/* Set the frame in focus, this also focuses an associated window if
 * possible.
 */
void set_focus_frame(Frame *frame)
{
    Monitor *monitor;
    FcWindow *window;

    /* if the monitor changed, check if a window is covering that monitor and
     * focus it
     */
    if (monitor = get_monitor_containing_frame(frame),
                monitor != get_monitor_containing_frame(Frame_focus)) {
        window = get_window_covering_monitor(monitor);
        if (window != NULL) {
            set_focus_window(window);
        } else {
            set_focus_window(frame->window);
        }
    /* focus the inner window if it exists, there is no focus or a tiling window
     * was previously focused
     */
    } else if (frame->window != NULL || Window_focus == NULL ||
            Window_focus->state.mode == WINDOW_MODE_TILING) {
        set_focus_window(frame->window);
    }

    Frame_focus = frame;
}

/*******************************************
 * Getting from one frame to another frame *
 *******************************************/

/* Get the frame on the left of @frame. */
static Frame *get_left_or_above_frame(Frame *frame,
        frame_split_direction_t direction)
{
    Frame *parent;

    while (frame != NULL) {
        parent = frame->parent;
        if (parent == NULL) {
            frame = NULL;
            break;
        }

        if (parent->split_direction != direction) {
            frame = parent;
        } else if (parent->left == frame) {
            frame = parent;
        } else {
            frame = parent->left;
            while (frame->left != NULL &&
                    frame->split_direction == direction) {
                frame = frame->right;
            }
            break;
        }
    }
    return frame;
}

/* Get the frame on the left of @frame. */
Frame *get_left_frame(Frame *frame)
{
    return get_left_or_above_frame(frame, FRAME_SPLIT_HORIZONTALLY);
}

/* Get the frame above @frame. */
Frame *get_above_frame(Frame *frame)
{
    return get_left_or_above_frame(frame, FRAME_SPLIT_VERTICALLY);
}

/* Get the frame on the left of @frame. */
static Frame *get_right_or_below_frame(Frame *frame,
        frame_split_direction_t direction)
{
    Frame *parent;

    while (frame != NULL) {
        parent = frame->parent;
        if (parent == NULL) {
            frame = NULL;
            break;
        }

        if (parent->split_direction != direction) {
            frame = parent;
        } else if (parent->right == frame) {
            frame = parent;
        } else {
            frame = parent->right;
            while (frame->left != NULL &&
                    frame->split_direction == direction) {
                frame = frame->left;
            }
            break;
        }
    }
    return frame;
}

/* Get the frame on the right of @frame. */
Frame *get_right_frame(Frame *frame)
{
    return get_right_or_below_frame(frame, FRAME_SPLIT_HORIZONTALLY);
}

/* Get the frame below @frame. */
Frame *get_below_frame(Frame *frame)
{
    return get_right_or_below_frame(frame, FRAME_SPLIT_VERTICALLY);
}

/* Get a child frame of @frame that is positioned according to @x and @y. */
Frame *get_best_leaf_frame(Frame *frame, int x, int y)
{
    while (frame->left != NULL) {
        if (frame->split_direction == FRAME_SPLIT_HORIZONTALLY) {
            if (frame->left->x + (int) frame->left->width >= x) {
                frame = frame->left;
            } else {
                frame = frame->right;
            }
        } else {
            if (frame->left->y + (int) frame->left->height >= y) {
                frame = frame->left;
            } else {
                frame = frame->right;
            }
        }
    }
    return frame;
}

/* Move @frame up or to the left.
 *
 * @direction determines if moving up or to the left.
 *
 * The comments are written for left movement but are analogous to up movement.
 * The cases refer to what is described in the include file.
 */
static bool move_frame_up_or_left(Frame *frame,
        frame_split_direction_t direction)
{
    Frame *original;
    bool is_left_split = false;

    original = frame;

    /* get the parent frame as long as we are on the left of a horizontal split
     */
    while (frame->parent != NULL &&
            frame->parent->split_direction == direction &&
            frame->parent->left == frame) {
        frame = frame->parent;
    }

    /* if we are in a parent that is split vertically, move it left of this
     * parent, unwinding from the vertical split
     */
    if (frame->parent != NULL && frame->parent->split_direction != direction) {
        /* case 3 */
        frame = frame->parent;
        is_left_split = true;
    } else {
        if (direction == FRAME_SPLIT_HORIZONTALLY) {
            frame = get_left_frame(frame);
        } else {
            frame = get_above_frame(frame);
        }

        /* if there is no frame, move across monitors */
        if (frame == NULL) {
            /* case 1, S2 */
            Monitor *monitor;

            monitor = get_monitor_containing_frame(original);
            if (direction == FRAME_SPLIT_HORIZONTALLY) {
                monitor = get_left_monitor(monitor);
            } else {
                monitor = get_above_monitor(monitor);
            }
            if (monitor == NULL) {
                frame = NULL;
            } else {
                frame = monitor->frame;
            }
        } else {
            if (frame->left != NULL) {
                /* case 2, 5 */
                if (direction == FRAME_SPLIT_HORIZONTALLY) {
                    frame = get_best_leaf_frame(frame,
                            INT_MAX,
                            original->y + original->height / 2);
                } else {
                    frame = get_best_leaf_frame(frame,
                            original->x + original->width / 2,
                            INT_MAX);
                }
            } else {
                /* case 4 */
                is_left_split = true;
            }
        }
    }

    if (frame == NULL) {
        /* there is nowhere to move the frame to */
        return false;
    }

    resplit_frame(frame, original, is_left_split, direction);
    return true;
}

/* Move @frame to the left. */
bool move_frame_left(Frame *frame)
{
    return move_frame_up_or_left(frame, FRAME_SPLIT_HORIZONTALLY);
}

/* Move @frame up. */
bool move_frame_up(Frame *frame)
{
    return move_frame_up_or_left(frame, FRAME_SPLIT_VERTICALLY);
}

/* Move @frame down or to the right.
 *
 * @direction determines if moving down or to the left.
 *
 * The comments are written for right movement but are analogous to down
 * movement. The cases refer to what is described in the include file.
 */
static bool move_frame_down_or_right(Frame *frame,
        frame_split_direction_t direction)
{
    Frame *original;
    bool is_left_split = true;

    original = frame;

    /* get the parent frame as long as we are on the right of a horizontal split
     */
    while (frame->parent != NULL &&
            frame->parent->split_direction == direction &&
            frame->parent->right == frame) {
        frame = frame->parent;
    }

    /* if we are in a parent that is split vertically, move it right of this
     * parent, unwinding from the vertical split
     */
    if (frame->parent != NULL && frame->parent->split_direction != direction) {
        /* case 3 */
        frame = frame->parent;
        is_left_split = false;
    } else {
        if (direction == FRAME_SPLIT_HORIZONTALLY) {
            frame = get_right_frame(frame);
        } else {
            frame = get_below_frame(frame);
        }

        if (frame == NULL) {
            /* case 1, S2 */
            Monitor *monitor;

            monitor = get_monitor_containing_frame(original);
            if (direction == FRAME_SPLIT_HORIZONTALLY) {
                monitor = get_right_monitor(monitor);
            } else {
                monitor = get_below_monitor(monitor);
            }
            if (monitor == NULL) {
                frame = NULL;
            } else {
                frame = monitor->frame;
            }
        } else {
            if (frame->left != NULL) {
                /* case 2, 5 */
                if (direction == FRAME_SPLIT_HORIZONTALLY) {
                    frame = get_best_leaf_frame(frame,
                            INT_MAX,
                            original->y + original->height / 2);
                } else {
                    frame = get_best_leaf_frame(frame,
                            original->x + original->width / 2,
                            INT_MAX);
                }
            } else {
                /* case 4 */
                is_left_split = false;
            }
        }
    }

    if (frame == NULL) {
        /* there is nowhere to move the frame to */
        return false;
    }

    resplit_frame(frame, original, is_left_split, direction);
    return true;
}

/* Move @frame to the right. */
bool move_frame_right(Frame *frame)
{
    return move_frame_down_or_right(frame, FRAME_SPLIT_HORIZONTALLY);
}

/* Move @frame down. */
bool move_frame_down(Frame *frame)
{
    return move_frame_down_or_right(frame, FRAME_SPLIT_VERTICALLY);
}

/* Exchange @from with @to.
 *
 * We can assume that both frames are independent.
 */
void exchange_frames(Frame *from, Frame *to)
{
    Frame saved_frame;

    /* swap the focus if one frame has it */
    if (Frame_focus == from) {
        Frame_focus = to;
    } else if (Frame_focus == to) {
        Frame_focus = from;
    }

    saved_frame = *from;
    replace_frame(from, to);
    replace_frame(to, &saved_frame);
}

/***************************
 * Frame moving and sizing *
 ***************************/

/* Apply the auto equalizationg to given frame. */
void apply_auto_equalize(Frame *to, frame_split_direction_t direction)
{
    Frame *start_from;

    start_from = to;
    while (to->parent != NULL) {
        if (to->parent->split_direction == direction) {
            start_from = to->parent;
        }
        to = to->parent;
    }
    equalize_frame(start_from, direction);
}

/* Get the minimum size the given frame should have. */
void get_minimum_frame_size(Frame *frame, Size *size)
{
    Extents gaps;

    if (frame->left != NULL) {
        Size left_size, right_size;

        get_minimum_frame_size(frame->left, &left_size);
        get_minimum_frame_size(frame->right, &right_size);
        if (frame->split_direction == FRAME_SPLIT_VERTICALLY) {
            size->width = MAX(left_size.width, right_size.width);
            size->height = left_size.height + right_size.height;
        } else {
            size->width = left_size.width + right_size.width;
            size->height = MAX(left_size.height, right_size.height);
        }
    } else {
        size->width = FRAME_RESIZE_MINIMUM_SIZE;
        size->height = FRAME_RESIZE_MINIMUM_SIZE;
    }

    get_frame_gaps(frame, &gaps);
    size->width += gaps.left + gaps.right;
    size->height += gaps.top + gaps.bottom;
}

/* Set the size of a frame, this also resizes the inner frames and windows. */
void resize_frame(Frame *frame, int x, int y,
        unsigned width, unsigned height)
{
    Frame *left, *right;
    unsigned left_size;

    frame->x = x;
    frame->y = y;
    frame->width = width;
    frame->height = height;
    reload_frame(frame);

    left = frame->left;
    right = frame->right;

    /* check if the frame has children */
    if (left == NULL) {
        return;
    }

    switch (frame->split_direction) {
    /* left to right split */
    case FRAME_SPLIT_HORIZONTALLY:
        left_size = frame->ratio.denominator == 0 ? width / 2 :
                (uint64_t) width * frame->ratio.numerator /
                    frame->ratio.denominator;
        resize_frame(left, x, y, left_size, height);
        resize_frame(right, x + left_size, y, width - left_size, height);
        break;

    /* top to bottom split */
    case FRAME_SPLIT_VERTICALLY:
        left_size = frame->ratio.denominator == 0 ? height / 2 :
                (uint64_t) height * frame->ratio.numerator /
                    frame->ratio.denominator;
        resize_frame(left, x, y, width, left_size);
        resize_frame(right, x, y + left_size, width, height - left_size);
        break;
    }
}

/* Set the size of a frame, this also resizes the child frames and windows. */
void resize_frame_and_ignore_ratio(Frame *frame, int x, int y,
        unsigned width, unsigned height)
{
    Frame *left, *right;
    unsigned left_size;

    frame->x = x;
    frame->y = y;
    frame->width = width;
    frame->height = height;
    reload_frame(frame);

    left = frame->left;
    right = frame->right;

    /* check if the frame has children */
    if (left == NULL) {
        return;
    }

    switch (frame->split_direction) {
    /* left to right split */
    case FRAME_SPLIT_HORIZONTALLY:
        /* keep ratio when resizing or use the default 1/2 ratio */
        left_size = left->width == 0 || right->width == 0 ? width / 2 :
                (uint64_t) width * left->width / (left->width + right->width);
        resize_frame_and_ignore_ratio(left, x, y, left_size, height);
        resize_frame_and_ignore_ratio(right, x + left_size, y,
                width - left_size, height);
        break;

    /* top to bottom split */
    case FRAME_SPLIT_VERTICALLY:
        /* keep ratio when resizing or use the default 1/2 ratio */
        left_size = left->height == 0 || right->height == 0 ? height / 2 :
              (uint64_t) height * left->height / (left->height + right->height);
        resize_frame_and_ignore_ratio(left, x, y, width, left_size);
        resize_frame_and_ignore_ratio(right, x, y + left_size,
                width, height - left_size);
        break;
    }
}

/* Set the ratio, position and size of all parents of @frame to match their
 * children whenever their split direction matches @direction.
 */
static inline void propagate_size(Frame *frame,
        frame_split_direction_t direction)
{
    while (frame->parent != NULL) {
        frame = frame->parent;
        if (frame->split_direction == direction) {
            if (direction == FRAME_SPLIT_HORIZONTALLY) {
                frame->ratio.numerator = frame->right->width;
                frame->ratio.denominator = frame->left->width +
                    frame->right->width;

                frame->x = frame->left->x;
                frame->width = frame->left->width + frame->right->width;
            } else {
                frame->ratio.numerator = frame->right->height;
                frame->ratio.denominator = frame->left->height +
                    frame->right->height;

                frame->y = frame->left->y;
                frame->height = frame->left->height + frame->right->height;
            }
        }
    }
}

/* Increase the @edge of @frame by @amount. */
int bump_frame_edge(Frame *frame, frame_edge_t edge, int amount)
{
    Frame *parent, *right = NULL;
    Size size;
    int space;
    int self_amount;

    parent = frame->parent;

    if (parent == NULL || amount == 0) {
        return 0;
    }

    switch (edge) {
    /* delegate left movement to right movement */
    case FRAME_EDGE_LEFT:
        frame = get_left_frame(frame);
        if (frame == NULL) {
            return 0;
        }
        return -bump_frame_edge(frame, FRAME_EDGE_RIGHT, -amount);

    /* delegate top movement to bottom movement */
    case FRAME_EDGE_TOP:
        frame = get_above_frame(frame);
        if (frame == NULL) {
            return 0;
        }
        return -bump_frame_edge(frame, FRAME_EDGE_BOTTOM, -amount);

    /* move the frame's right edge */
    case FRAME_EDGE_RIGHT:
        right = get_right_frame(frame);
        if (right == NULL) {
            return 0;
        }
        frame = get_left_frame(right);

        if (amount < 0) {
            get_minimum_frame_size(frame, &size);
            space = size.width - frame->width;
            space = MIN(space, 0);
            self_amount = MAX(amount, space);
            if (space > amount) {
                /* try to get more space by pushing the left edge */
                space -= bump_frame_edge(frame, FRAME_EDGE_LEFT,
                        -amount + space);
                amount = MAX(amount, space);
            } else {
                amount = self_amount;
            }
        } else {
            get_minimum_frame_size(right, &size);
            space = right->width - size.width;
            space = MAX(space, 0);
            self_amount = MIN(amount, space);
            if (space < amount) {
                /* try to get more space by pushing the right edge */
                space += bump_frame_edge(right, FRAME_EDGE_RIGHT,
                        amount - space);
                amount = MIN(amount, space);
            } else {
                amount = self_amount;
            }
        }

        resize_frame_and_ignore_ratio(frame, frame->x, frame->y,
                frame->width + self_amount, frame->height);
        resize_frame_and_ignore_ratio(right, right->x + self_amount, right->y,
                right->width - self_amount, right->height);
        break;

    /* move the frame's bottom edge */
    case FRAME_EDGE_BOTTOM:
        right = get_below_frame(frame);
        if (right == NULL) {
            return 0;
        }
        frame = get_above_frame(right);

        if (amount < 0) {
            get_minimum_frame_size(frame, &size);
            space = size.height - frame->height;
            space = MIN(space, 0);
            self_amount = MAX(amount, space);
            if (space > amount) {
                /* try to get more space by pushing the top edge */
                space -= bump_frame_edge(frame, FRAME_EDGE_TOP,
                        -amount + space);
                amount = MAX(amount, space);
            } else {
                amount = self_amount;
            }
        } else {
            get_minimum_frame_size(right, &size);
            space = right->height - size.height;
            space = MAX(space, 0);
            self_amount = MIN(amount, space);
            if (space < amount) {
                /* try to get more space by pushing the bottom edge */
                space += bump_frame_edge(right, FRAME_EDGE_BOTTOM,
                        amount - space);
                amount = MIN(amount, space);
            } else {
                amount = self_amount;
            }
        }

        resize_frame_and_ignore_ratio(frame, frame->x, frame->y,
                frame->width, frame->height + self_amount);
        resize_frame_and_ignore_ratio(right, right->x, right->y + self_amount,
                right->width, right->height - self_amount);
        break;
    }

    /* make the size change known to all parents */
    const frame_split_direction_t direction = edge == FRAME_EDGE_RIGHT ?
            FRAME_SPLIT_HORIZONTALLY : FRAME_SPLIT_VERTICALLY;
    propagate_size(frame, direction);
    propagate_size(right, direction);

    return amount;
}

/* Count the frames in horizontal direction. */
static unsigned count_horizontal_frames(Frame *frame)
{
    unsigned left_count, right_count;

    if (frame->left == NULL) {
        return 1;
    }

    left_count = count_horizontal_frames(frame->left);
    right_count = count_horizontal_frames(frame->right);
    switch (frame->split_direction) {
    case FRAME_SPLIT_VERTICALLY:
        return MAX(left_count, right_count);

    case FRAME_SPLIT_HORIZONTALLY:
        return left_count + right_count;
    }

    /* this code is never reached */
    return 0;
}

/* Count the frames in vertical direction. */
static unsigned count_vertical_frames(Frame *frame)
{
    unsigned left_count, right_count;

    if (frame->left == NULL) {
        return 1;
    }

    left_count = count_vertical_frames(frame->left);
    right_count = count_vertical_frames(frame->right);
    switch (frame->split_direction) {
    case FRAME_SPLIT_VERTICALLY:
        return left_count + right_count;

    case FRAME_SPLIT_HORIZONTALLY:
        return MAX(left_count, right_count);
    }

    /* this code is never reached */
    return 0;
}

/* Set the size of all children of @frame to be equal within a certain
 * direction.
 */
void equalize_frame(Frame *frame, frame_split_direction_t direction)
{
    unsigned left_count, right_count;

    /* check if the frame has any children */
    if (frame->left == NULL) {
        return;
    }

    if (direction == frame->split_direction) {
        switch (direction) {
        case FRAME_SPLIT_HORIZONTALLY:
            left_count = count_horizontal_frames(frame->left);
            right_count = count_horizontal_frames(frame->right);
            frame->left->width = (uint64_t) frame->width * left_count /
                (left_count + right_count);
            frame->right->x = frame->x + frame->left->width;
            frame->right->width = frame->width - frame->left->width;
            break;

        case FRAME_SPLIT_VERTICALLY:
            left_count = count_vertical_frames(frame->left);
            right_count = count_vertical_frames(frame->right);
            frame->left->height = (uint64_t) frame->height * left_count /
                (left_count + right_count);
            frame->right->y = frame->y + frame->left->height;
            frame->right->height = frame->height - frame->left->height;
            break;
        }
    }

    equalize_frame(frame->left, direction);
    equalize_frame(frame->right, direction);
}

/*******************
 * Frame splitting *
 *******************/

/* Move a frame to another position in the tree. */
void resplit_frame(Frame *frame, Frame *original, bool is_left_split,
        frame_split_direction_t direction)
{
    /* If they have the same parent, then `remove_frame()` would
     * invalidate the `frame` pointer.  We would need to split off
     * the parent.
     */
    if (frame->parent != NULL && frame->parent == original->parent) {
        frame = frame->parent;
    }

    if (is_frame_void(frame)) {
        LOG_DEBUG("splitting off a void\n");

        /* case S1 */
        if (Frame_focus == original) {
            Frame_focus = frame;
        }
        replace_frame(frame, original);
        /* do not destroy root frames */
        if (original->parent != NULL) {
            remove_frame(original);
            destroy_frame(original);
        }
    } else {
        LOG_DEBUG("splitting off a normal frame\n");

        const bool refocus = Frame_focus == original;
        if (original->parent == NULL) {
            /* make a wrapper around the root */
            Frame *const new = create_frame();
            replace_frame(new, original);
            original = new;
        } else {
            /* disconnect the frame from the layout */
            remove_frame(original);
        }
        split_frame(frame, original, is_left_split, direction);
        if (refocus) {
            Frame_focus = is_left_split ? frame->left : frame->right;
        }
    }
}

/* Split a frame horizontally or vertically. */
void split_frame(Frame *split_from, Frame *other, bool is_left_split,
        frame_split_direction_t direction)
{
    Frame *new;

    new = create_frame();
    if (other == NULL) {
        other = create_frame();
        if (configuration.auto_fill_void) {
            fill_void_with_stash(other);
        }
    }

    new->number = split_from->number;
    split_from->number = 0;
    /* let `new` take the children or window */
    if (split_from->left != NULL) {
        new->split_direction = split_from->split_direction;
        new->ratio = split_from->ratio;
        new->left = split_from->left;
        new->right = split_from->right;
        new->left->parent = new;
        new->right->parent = new;
    } else {
        new->window = split_from->window;
        split_from->window = NULL;
    }

    split_from->split_direction = direction;
    /* ratio of 1/2 */
    split_from->ratio.numerator = 1;
    split_from->ratio.denominator = 2;
    if (is_left_split) {
        split_from->left = other;
        split_from->right = new;
    } else {
        split_from->left = new;
        split_from->right = other;
    }
    new->parent = split_from;
    other->parent = split_from;

    if (split_from == Frame_focus) {
        Frame_focus = new;
    }

    /* size the child frames */
    resize_frame(split_from, split_from->x, split_from->y,
            split_from->width, split_from->height);
    if (configuration.auto_equalize) {
        apply_auto_equalize(split_from, direction);
    }

    LOG("split %F(%F, %F)\n", split_from, new, other);
}

/* Remove a frame from the screen. */
void remove_frame(Frame *frame)
{
    Frame *parent, *other;
    frame_split_direction_t direction;

    if (frame->parent == NULL) {
        /* this case should always be handled by the caller */
        LOG_ERROR("can not remove the root frame %F\n", frame);
        return;
    }

    parent = frame->parent;
    direction = parent->split_direction;
    /* disconnect the frame */
    frame->parent = NULL;

    if (frame == parent->left) {
        other = parent->right;
    } else {
        other = parent->left;
    }

    parent->number = other->number;
    other->number = 0;
    parent->left = other->left;
    parent->right = other->right;
    if (other->left != NULL) {
        /* pass through the children */
        parent->split_direction = other->split_direction;
        parent->ratio = other->ratio;
        parent->left->parent = parent;
        parent->right->parent = parent;

        other->left = NULL;
        other->right = NULL;
    } else {
        parent->window = other->window;

        other->window = NULL;
    }
    /* disconnect `other`, it will be destroyed later */
    other->parent = NULL;

    /* reload all child frames */
    resize_frame(parent, parent->x, parent->y, parent->width, parent->height);

    LOG("frame %F was removed\n", frame);

    /* do not leave behind broken focus */
    if (Frame_focus == frame || Frame_focus == other) {
        Frame_focus = get_best_leaf_frame(parent,
                parent->x + parent->width / 2,
                parent->y + parent->height / 2);
    }

    if (configuration.auto_equalize) {
        apply_auto_equalize(parent, direction);
    }

    destroy_frame(other);
}

/******************
 * Frame stashing *
 ******************/

/* Hide all windows in @frame and child frames. */
static void hide_and_reference_inner_windows(Frame *frame)
{
    if (frame->left != NULL) {
        hide_and_reference_inner_windows(frame->left);
        hide_and_reference_inner_windows(frame->right);
    } else if (frame->window != NULL) {
        hide_window_abruptly(frame->window);
        /* make sure the pointer sticks around */
        reference_window(frame->window);
    }
}

/* Show all windows in @frame and child frames. */
static void show_and_dereference_inner_windows(Frame *frame)
{
    if (frame->left != NULL) {
        show_and_dereference_inner_windows(frame->left);
        show_and_dereference_inner_windows(frame->right);
    } else if (frame->window != NULL) {
        dereference_window(frame->window);
        reload_frame(frame);
        frame->window->state.is_visible = true;
    }
}

/* Make sure all window pointers are still pointing to an existing invisible
 * window.
 *
 * @return the number of valid windows.
 */
static unsigned validate_inner_windows(Frame *frame)
{
    if (frame->left != NULL) {
        return validate_inner_windows(frame->left) +
            validate_inner_windows(frame->right);
    } else if (frame->window != NULL) {
        if (frame->window->reference.id == None ||
                frame->window->state.is_visible) {
            dereference_window(frame->window);
            frame->window = NULL;
            return 0;
        }
        return 1;
    }
    return 0;
}

/* Take @frame away from the screen, this leaves a singular empty frame. */
Frame *stash_frame_later(Frame *frame)
{
    /* check if it is worth saving this frame */
    if (is_frame_void(frame) && frame->number == 0) {
        return NULL;
    }

    Frame *const stash = create_frame();
    replace_frame(stash, frame);
    hide_and_reference_inner_windows(stash);
    return stash;
}

/* Links a frame into the stash linked list. */
void link_frame_into_stash(Frame *frame)
{
    if (frame == NULL) {
        return;
    }
    frame->previous_stashed = Frame_last_stashed;
    Frame_last_stashed = frame;
}

/* Take @frame away from the screen, this leaves a singular empty frame. */
Frame *stash_frame(Frame *frame)
{
    Frame *const stash = stash_frame_later(frame);
    link_frame_into_stash(stash);
    return stash;
}

/* Unlink given @frame from the stash linked list. */
void unlink_frame_from_stash(Frame *frame)
{
    Frame *previous;

    if (Frame_last_stashed == frame) {
        Frame_last_stashed = Frame_last_stashed->previous_stashed;
    } else {
        previous = Frame_last_stashed;
        while (previous->previous_stashed != frame) {
            previous = previous->previous_stashed;
        }
        previous->previous_stashed = frame->previous_stashed;
    }

    (void) validate_inner_windows(frame);
    show_and_dereference_inner_windows(frame);
}

/* Free @frame and all child frames. */
static void free_frame_recursively(Frame *frame)
{
    if (frame->left != NULL) {
        free_frame_recursively(frame->left);
        free_frame_recursively(frame->right);
        frame->left = NULL;
        frame->right = NULL;
    }
    frame->parent = NULL;
    destroy_frame(frame);
}

/* Pop a frame from the stashed frame list. */
Frame *pop_stashed_frame(void)
{
    Frame *pop = NULL;

    /* Find the first valid frame in the pop list.  It might be that a stashed
     * frame got invalidated because it lost all inner windows and is now
     * completely empty.
     */
    while (Frame_last_stashed != NULL) {
        if (validate_inner_windows(Frame_last_stashed) > 0 ||
                Frame_last_stashed->number > 0) {
            break;
        }

        Frame *const free_me = Frame_last_stashed;
        Frame_last_stashed = Frame_last_stashed->previous_stashed;
        free_frame_recursively(free_me);
    }

    if (Frame_last_stashed != NULL) {
        pop = Frame_last_stashed;
        Frame_last_stashed = Frame_last_stashed->previous_stashed;
        show_and_dereference_inner_windows(pop);
    }

    return pop;
}

/* Put a frame from the stash into given @frame. */
void fill_void_with_stash(Frame *frame)
{
    Frame *pop;

    pop = pop_stashed_frame();
    if (pop == NULL) {
        return;
    }
    replace_frame(frame, pop);
    destroy_frame(pop);
}
