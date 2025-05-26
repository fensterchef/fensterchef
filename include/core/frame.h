#ifndef FRAME_H
#define FRAME_H

/* Frames are used to partition a monitor into multiple rectangular regions.
 *
 * When a frame has one child, it must have a second one, so either BOTH left
 * AND right are NULL OR neither are NULL.
 * That is why `frame->left != NULL` is always used as a test for checking if a
 * frame has children or not.
 *
 * `parent` is NULL when the frame is a root frame or stashed frame.
 */

#include <stdbool.h>
#include <limits.h>

#include "bits/frame.h"
#include "bits/window.h"
#include "utility/attributes.h"
#include "utility/types.h"

/* The minimum resize width or height of a frame.  Frames are never clipped to
 * this size and can even have a size of 0.  It is only used when resizing
 * frames.
 */
#define FRAME_RESIZE_MINIMUM_SIZE 12

/* an edge of the frame */
typedef enum {
    /* the left edge */
    FRAME_EDGE_LEFT,
    /* the top edge */
    FRAME_EDGE_TOP,
    /* the right edge */
    FRAME_EDGE_RIGHT,
    /* the bottom edge */
    FRAME_EDGE_BOTTOM,
} frame_edge_t;

/* a direction to split a frame in */
typedef enum {
    /* the frame was split horizontally (children are left and right) */
    FRAME_SPLIT_HORIZONTALLY,
    /* the frame was split vertically (children are up and down) */
    FRAME_SPLIT_VERTICALLY,
} frame_split_direction_t;

/* the frame structure */
struct frame {
    /* reference counter to keep the pointer alive for longer */
    unsigned reference_count;

    /* The window inside the frame, may be NULL.  It might become also be
     * destroyed.  To check this use `window->client.id` which should be `None`
     * when the window is destroyed.  This case is important for stashed frames.
     */
    FcWindow *window;

    /* coordinates and size of the frame */
    int x;
    int y;
    unsigned width;
    unsigned height;

    /* ration between the two children */
    Ratio ratio;

    /* the direction the frame was split in */
    frame_split_direction_t split_direction;

    /* if a parent frame is focused, this parent stores from which child it
     * was focused from
     */
    bool moved_from_left;

    /* parent of the frame */
    Frame *parent;
    /* left child and right child of the frame */
    Frame *left;
    Frame *right;

    /* the previous stashed frame in the frame stashed linked list */
    Frame *previous_stashed;

    /* the id of this frame, this is a unique number, the exception is 0 */
    unsigned number;
};

/* the last frame in the frame stashed linked list */
extern Frame *Frame_last_stashed;

/* the currently selected/focused frame */
extern Frame *Frame_focus;

/* Increment the reference count of the frame. */
void reference_frame(Frame *frame);

/* Decrement the reference count of the frame and free @frame when it reaches 0.
 */
void dereference_frame(Frame *frame);

/* Create a frame object. */
Frame *create_frame(void);

/* Free the frame object.
 *
 * @frame must have no parent or children and it shall not be the root frame
 *        of a monitor.
 */
void destroy_frame(Frame *frame);

/* Show a notification on the given frame indicating the number. */
void indicate_frame(Frame *frame);

/* Get the frame above the given one that has no parent.
 *
 * @frame may be NULL, then simply NULL is returned.
 */
Frame *get_root_frame(_Nullable const Frame *frame);

/* Look through all visible frames to find a frame with given @number. */
Frame *get_frame_by_number(unsigned number);

/* Check if the given @frame has no splits and no window. */
bool is_frame_void(const Frame *frame);

/* Check if the given point is within the given frame.
 *
 * @return if the point is inside the frame.
 */
bool is_point_in_frame(const Frame *frame, int x, int y);

/* Get a frame at given position.
 *
 * @return a LEAF frame at given position or NULL when there is none.
 */
Frame *get_frame_at_position(int x, int y);

/* Replace @frame with @with.
 *
 * @frame receives the children or the window within @with and the
 *        number, split direction and ration @with has.
 *        @frame only keeps its size.
 *        @frame should be a void (pass `is_frame_void()`).
 * @with is emptied by this function, only the original size remains.
 */
void replace_frame(Frame *frame, Frame *with);

/* Get the gaps the frame applies to its inner window. */
void get_frame_gaps(const Frame *frame, Extents *gaps);

/* Resizes the inner window to fit within the frame. */
void reload_frame(Frame *frame);

/* Set the frame in focus, this also focuses a related window if possible.
 *
 * The related window is either a window covering the monitor the frame is on
 * or the window within the frame.
 *
 * If you just want to set the focused frame without focusing a window:
 * `Frame_focus = frame` suffices.
 */
void set_focus_frame(Frame *frame);

/* Note for all `get_XXX_frame()` functions that the frame returned usually is a
 * parent frame, meaning it has children, for example:
 * +---+---+---+
 * |   | 2 | 4 |
 * | 1 |   +---+
 * |   +---+ 5 |
 * |   | 3 |   |
 * +---+---+---+
 *
 * (pointers illustrated by above numbers)
 *
 * `get_left_frame(2)` -> 1
 * `get_left_frame(3)` -> 1
 * `get_right_frame(3)` -> 4, 5
 * `get_right_frame(1)` -> 2, 3
 * `get_above_frame(3)` -> 2
 * `get_above_frame(2)` -> NULL
 * `get_below_frame(1)` -> NULL
 * `get_left_frame(4)` -> 2, 3
 *
 * These function do NOT move across monitors.
 */

/* Get the frame on the left of @frame. */
Frame *get_left_frame(Frame *frame);

/* Get the frame above @frame. */
Frame *get_above_frame(Frame *frame);

/* Get the frame on the right of @frame. */
Frame *get_right_frame(Frame *frame);

/* Get the frame below @frame. */
Frame *get_below_frame(Frame *frame);

/* Get a child frame of @frame that is positioned according to @x and @y.
 *
 * Examples:
 * Getting a centered frame:
 *     get_best_leaf_frame(frame,
 *             frame->x + frame->width / 2,
 *             frame->y + frame->height / 2);
 * Getting the most left/right frame:
 *     get_best_leaf_frame(frame,
 *             INT_MIN,
 *             frame->y + frame->height / 2);
 *     get_best_leaf_frame(frame,
 *             INT_MAX,
 *             frame->y + frame->height / 2);
 */
Frame *get_best_leaf_frame(Frame *frame, int x, int y);

/* The `move_frame_XXX()` functions work analogous.  Here is an illustration:
 * +-----+---+---+---+
 * |     |   2   |   |
 * |  1  +---+---+ 5 |
 * |     | 3 | 4 |   |
 * +-----+---+---+---+
 *
 * Cases 1-5 (moving frame N to the left):
 * 1. Nothing happens
 * 2. Remove and split to the right of 1
 * 3. Remove and split to the left of 2,4
 * 4. Remove and split to the left of 3
 * 5. Remove and split to the right of 2
 *
 * These special cases are handled as well:
 * S1. A frame is moved into a void
 * S2. Movement across monitors
 */

/* Move @frame to the left.
 *
 * @return if the frame could be moved.
 */
bool move_frame_left(Frame *frame);

/* Move @frame up.
 *
 * @return if the frame could be moved.
 */
bool move_frame_up(Frame *frame);

/* Move @frame to the right.
 *
 * @return if the frame could be moved.
 */
bool move_frame_right(Frame *frame);

/* Move @frame down.
 *
 * @return if the frame could be moved.
 */
bool move_frame_down(Frame *frame);

/* Exchange @from with @to.
 *
 * The exchange must be well defined for the two frames. These cases lead to
 * undefined behavior:
 * - @to is within @from
 * - @from is within @to
 */
void exchange_frames(Frame *from, Frame *to);

/* Apply the auto equalizationg to given frame.
 *
 * This moves up while the split direction matches @direction.
 */
void apply_auto_equalize(Frame *to, frame_split_direction_t direction);

/* Get the minimum size the given frame should have. */
void get_minimum_frame_size(Frame *frame, Size *size);

/* Set the size of a frame, this also resize the child frames and windows. */
void resize_frame(Frame *frame, int x, int y, unsigned width, unsigned height);

/* Set the size of a frame, this also resizes the child frames and windows.
 *
 * This function ignores the @frame->ratio and instead uses the existing ratio
 * between the windows to size them.
 *
 * This function is good for reloading child frames if the parent resized.
 */
void resize_frame_and_ignore_ratio(Frame *frame, int x, int y,
        unsigned width, unsigned height);

/* Increases the @edge of @frame by @amount. */
int bump_frame_edge(Frame *frame, frame_edge_t edge, int amount);

/* Set the size of all children within @frame to be equal within a certain
 * direction.
 */
void equalize_frame(Frame *frame, frame_split_direction_t direction);

/* Move a frame to another position in the tree.
 *
 * @original is the frame that should be moved.
 * @frame is the frame to split off from.
 */
void resplit_frame(Frame *frame, Frame *original, bool is_left_split,
        frame_split_direction_t direction);

/* Split a frame horizontally or vertically.
 *
 * @split_from is the frame to split a frame off of.
 * @other is put into the the slot created by the split. It may be NULL, then
 *        one is allocated by this function and filled using the stash if
 *        auto-fill-void is configured.
 * @is_left_split controls where @other goes together with @direction,
 *                either on the top/left or bottom/right.
 */
void split_frame(Frame *split_from, Frame *other, bool is_left_split,
        frame_split_direction_t direction);

/* Remove a frame from the screen.
 *
 * This keeps the children within @frame in tact meaning they are not destroyed
 * or stashed. This needs to be done externally.
 * Stashing the frame before removing it is a good approach.
 *
 * @frame shall not be a root frame.
 */
void remove_frame(Frame *frame);

/* Try to find any empty frame on the monitor @frame is on.
 *
 * @return NULL if there is no empty frame.
 */
Frame *find_frame_void(Frame *frame);

/* Take frame away from the screen, this leaves a singular empty frame.
 *
 * @frame is made into a completely empty frame as all children and windows are
 *        taken out.
 *
 * Consider using `link_into_stash()` after calling this.
 *
 * @return may be NULL if the frame is not worth stashing.
 */
Frame *stash_frame_later(Frame *frame);

/* Links a frame into the stash linked list.
 *
 * The stash object also gets linked into the frame number list as frame object.
 *
 * @frame may be NULL, then nothing happens.
 *
 * Use this on frames returned by `stash_frame_later()`.
 */
void link_frame_into_stash(Frame *frame);

/* Take @frame away from the screen, hiding all inner windows and leave a
 * singular empty frame behind.
 *
 * This is a simple wrapper around `stash_frame_later()` that calls
 * `link_into_stash()` immediately.
 *
 * @return the stashed frame, there is actually no reason to do anything with
 *         this beside checking if it's NULL.
 */
Frame *stash_frame(Frame *frame);

/* Unlinks given @frame from the stash linked list.
 *
 * This allows to pop arbitrary frames from the stash and not only the last
 * stashed frame.
 *
 * @frame must have been stashed by a previous call to `stash_frame_later()`.
 */
void unlink_frame_from_stash(Frame *frame);

/* Pop a frame from the stashed frame list.
 *
 * The caller may use `replace_frame()` with this frame and then destroy it.
 *
 * @return NULL when there are no stashed frames.
 */
Frame *pop_stashed_frame(void);

/* Puts a frame from the stash into given @frame.
 *
 * @frame must be empty with no windows or children.
 */
void fill_void_with_stash(Frame *frame);

#endif
