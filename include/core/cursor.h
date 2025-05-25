#ifndef CURSOR_H
#define CURSOR_H

#include <X11/X.h>

#include "utility/attributes.h"

/* expand to all cursors we use */
#define DEFINE_ALL_CURSORS \
    /* the cursor on the root window */ \
    X(ROOT, "left_ptr") \
    /* the cursor when moving a window */ \
    X(MOVING, "fleur") \
    /* the cursor when resizing horizontally */ \
    X(HORIZONTAL, "sb_h_double_arrow") \
    /* the cursor when resizing vertically */ \
    X(VERTICAL, "sb_v_double_arrow") \
    /* the cursor when sizing anything else */ \
    X(SIZING, "sizing")

/* internal cache points for cursor constants */
typedef enum cursor {
#define X(identifier, default_name) \
    CURSOR_##identifier,
    DEFINE_ALL_CURSORS
#undef X
    CURSOR_MAX
} cursor_id_t;

/* Load the cursor with given name using the user's preferred style.
 *
 * @cursor_id is the point where the cursor is cached for repeated calls.
 * @name may be NULL to retrieve a cached cursor.  If none is cached, the
 *       default name is used to cache it.
 */
Cursor load_cursor(cursor_id_t cursor_id, _Nullable const char *name);

/* Clear all cursors within the cursor cache. */
void clear_cursor_cache(void);

#endif
