#include "association.h"
#include "log.h"
#include "window.h"
#include "utility/list.h"

/* if an action send an unassociate signal */
static bool is_unassociate_signalled;

/* the window associations */
LIST(struct window_association, window_associations);

/* Signal to the currently running association that it should remove itself. */
void signal_window_unassociate(void)
{
    is_unassociate_signalled = true;
}

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

    for (size_t i = 0; i < window_associations_length; i++) {
        clear_window_association(&window_associations[i]);
    }

    LIST_CLEAR(window_associations);
}

/* Run all actions within an assocation after selecting @window. */
static inline void run_window_association(FcWindow *window,
        struct window_association *association)
{
    LOG_DEBUG("running associated actions: %A\n",
            &association->actions);

    Window_selected = window;
    run_action_list(&association->actions);
}

/* Check if the association matches the given instance and class. */
static inline bool is_association_matching(
        const struct window_association *const association,
        const char *instance, const char *class)
{
    return matches_pattern(association->instance_pattern, instance) &&
        matches_pattern(association->class_pattern, class);
}

/* Run the actions associated to given window. */
bool run_window_associations(FcWindow *window)
{
    size_t length;
    bool has_match = false;

    /* store this so we do not use the global variable which causes unwanted
     * side effects
     */
    length = window_associations_length;

    /* find all associations that match the window */
    for (size_t i = 0; i < length; i++) {
        /* Note for the future: `&window_associations[i]` must be recomputed
         * every time, it is the right way!
         */
        if (is_association_matching(&window_associations[i],
                    window->properties.instance, window->properties.class)) {
            is_unassociate_signalled = false;

            run_window_association(window, &window_associations[i]);

            /* check if an action gave us this signal and the actions were not
             * cleared
             */
            if (is_unassociate_signalled && i < window_associations_length) {
                clear_window_association(&window_associations[i]);
                window_associations_length--;
                MOVE(&window_associations[i],
                        &window_associations[i + 1],
                        window_associations_length - i);
                length--;
                i--;
            }

            has_match = true;
        }
    }

    return has_match;
}
