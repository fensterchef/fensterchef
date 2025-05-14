#include "association.h"
#include "log.h"
#include "window.h"
#include "utility/list.h"

/* the window associations */
LIST(struct window_association, window_associations);

/* Duplicate an association deeply into itself. */
void duplicate_window_association(struct window_association *association)
{
    association->instance_pattern = xstrdup(association->instance_pattern);
    association->class_pattern = xstrdup(association->class_pattern);
    duplicate_action_list(&association->actions);
}

/* Add a new association from window instance/class to actions. */
void add_window_association(const struct window_association *association)
{
    struct window_association new_association;

    LOG_DEBUG("adding window association %s,%s\n",
            association->instance_pattern, association->class_pattern);

    new_association = *association;
    duplicate_window_association(&new_association);
    LIST_APPEND_VALUE(window_associations, new_association);
}

/* Clear the memory occupied by the window association. */
void clear_window_association(const struct window_association *association)
{
    free(association->instance_pattern);
    free(association->class_pattern);
    clear_action_list(&association->actions);
}

/* Clear all currently set window associations. */
void clear_window_associations(void)
{
    LOG_DEBUG("clearing all window associations\n");

    for (unsigned i = 0; i < window_associations_length; i++) {
        clear_window_association(&window_associations[i]);
    }

    LIST_CLEAR(window_associations);
}

/* Run the actions associated to given window. */
bool run_window_associations(FcWindow *window)
{
    bool has_match = false;

    if (window->properties.instance == NULL &&
            window->properties.class == NULL) {
        return false;
    }

    /* find all associations that match the window */
    for (unsigned i = 0; i < window_associations_length; i++) {
        struct window_association *const association = &window_associations[i];
        if ((association->instance_pattern == NULL ||
                    matches_pattern(association->instance_pattern,
                        window->properties.instance)) &&
                matches_pattern(association->class_pattern,
                    window->properties.class)) {
            LOG_DEBUG("running associated actions: %A\n",
                    &association->actions);

            Window_selected = window;
            run_action_list(&association->actions);
            has_match = true;
        }
    }

    return has_match;
}
