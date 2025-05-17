#ifndef RELATION_H
#define RELATION_H

/**
 * Relations are actions that run when a window class/instance matches the
 * relation.
 */

#include "action.h"
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
    struct action_list actions;
};

/* Signal to the currently running relation that it should remove itself.
 *
 * It is only removed after it finished running.
 */
void signal_window_unrelate(void);

/* Duplicate a relation deeply into itself. */
void duplicate_window_relation(struct window_relation *relation);

/* Add a new relation from window instance/class to actions.
 *
 * All memory from the relation is duplicated.
 */
void add_window_relation(const struct window_relation *relation);

/* Remove the relation matching @instance and @class. */
void remove_matching_window_relation(const utf8_t *instance,
        const utf8_t *class);

/* Remove the relation with pattern @instance_pattern and @class_pattern. */
void remove_exact_window_relation(const utf8_t *instance_pattern,
        const utf8_t *class_pattern);

/* Clear the memory occupied by the window relation. */
void clear_window_relation(const struct window_relation *relation);

/* Clear all currently set window relations. */
void clear_window_relations(void);

/* Run the actions related to given window.
 *
 * @return if any relation ran.
 */
bool run_window_relations(FcWindow *window);

#endif
