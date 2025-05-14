#ifndef ASSOCIATION_H
#define ASSOCIATION_H

/**
 * Associations are actions that run when a window class/instance matches the
 * association.
 */

#include "action.h"
#include "bits/window.h"
#include "utility/attributes.h"
#include "utility/types.h"

/* association between class/instance and actions */
struct window_association {
    /* the pattern the instance should match */
    utf8_t *instance_pattern;
    /* the pattern the class should match */
    utf8_t *class_pattern;
    /* the actions to execute */
    struct action_list actions;
};

/* Signal to the currently running association that it should remove itself.
 *
 * It is only removed after it finished running.
 */
void signal_window_unassociate(void);

/* Duplicate an association deeply into itself. */
void duplicate_window_association(struct window_association *association);

/* Add a new association from window instance/class to actions.
 *
 * All memory from the association is duplicated.
 */
void add_window_association(const struct window_association *association);

/* Clear the memory occupied by the window association. */
void clear_window_association(const struct window_association *association);

/* Clear all currently set window associations. */
void clear_window_associations(void);

/* Run the actions associated to given window.
 *
 * @return true if any association existed, false otherwise.
 */
bool run_window_associations(FcWindow *window);

#endif
