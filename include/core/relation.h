#ifndef RELATION_H
#define RELATION_H

/**
 * Relations are actions that run when a window class/instance matches the
 * relation.
 */

#include "bits/action_block.h"
#include "bits/window.h"
#include "utility/attributes.h"
#include "utility/types.h"

/* relation between class/instance and actions */
struct window_relation {
    /* the pattern the instance should match */
    utf8_t *instance_pattern;
    /* the pattern the class should match */
    utf8_t *class_pattern;
    /* the actions to execute */
    ActionBlock *actions;
};

/* Clear the memory occupied by the window relation. */
void clear_window_relation(struct window_relation *relation);

/* Duplicate a relation deeply into itself. */
void duplicate_window_relation(struct window_relation *relation);

/* Signal to the currently running relation that it should remove itself.
 *
 * It is only removed after it finished running.
 */
void signal_window_unrelate(void);

/* Set a window relation from instance/class name to actions.
 *
 * All memory from the given relation is duplicated.
 *
 * Set @relation->actions to NULL to clear a window relation.
 */
void set_window_relation(const struct window_relation *relation);

/* Unset all currently set window relations. */
void unset_window_relations(void);

/* Run the actions related to given window.
 *
 * @return if any relation ran.
 */
bool run_window_relations(FcWindow *window);

#endif
