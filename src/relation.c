#include "relation.h"
#include "log.h"
#include "window.h"
#include "utility/list.h"

/* the window relations */
LIST(struct window_relation, window_relations);

/* These two are needed to add/remove relations while relations are
 * running.  There would otherwise be numerous issues like base pointer changes,
 * index shifts.  These are all cleanly handled by these variables.
 *
 * See below in run_window_relations() on how they are used.
 */

/* the number of relations before starting to match relations */
size_t old_window_relations_length;
/* the index of the currently running relation */
size_t running_relation;

/* Remove the window relation at given index. */
static void remove_window_relation(size_t index)
{
    LOG_DEBUG("removing window relation %s,%s\n",
            window_relations[index].instance_pattern,
            window_relations[index].class_pattern);

    clear_window_relation(&window_relations[index]);
    window_relations_length--;
    MOVE(&window_relations[index], &window_relations[index + 1],
            window_relations_length - index);

    if (index <= running_relation) {
        running_relation--;
    }

    /* tell run_window_relations() to check one less relation */
    old_window_relations_length--;
}

/* Signal to the currently running relation that it should remove itself. */
void signal_window_unrelate(void)
{
    remove_window_relation(running_relation);
}

/* Duplicate a relation deeply into itself. */
void duplicate_window_relation(struct window_relation *relation)
{
    relation->instance_pattern = xstrdup(relation->instance_pattern);
    relation->class_pattern = xstrdup(relation->class_pattern);
    duplicate_action_list(&relation->actions);
}

/* Add a new relation from window instance/class to actions. */
void add_window_relation(const struct window_relation *relation)
{
    struct window_relation new_relation;

    LOG_DEBUG("adding window relation %s,%s\n",
            relation->instance_pattern, relation->class_pattern);

    new_relation = *relation;
    duplicate_window_relation(&new_relation);
    LIST_APPEND_VALUE(window_relations, new_relation);
}

/* Remove the relation matching @instance and @class. */
void remove_matching_window_relation(const utf8_t *instance,
        const utf8_t *class)
{
    struct window_relation *relation;

    for (size_t i = 0; i < window_relations_length; i++) {
        relation = &window_relations[i];
        if (matches_pattern(relation->instance_pattern, instance) &&
                matches_pattern(relation->class_pattern, class)) {
            remove_window_relation(i);
        }
    }
}

/* Remove the relation with pattern @instance_pattern and @class_pattern. */
void remove_exact_window_relation(const utf8_t *instance_pattern,
        const utf8_t *class_pattern)
{
    struct window_relation *relation;

    for (size_t i = 0; i < window_relations_length; i++) {
        relation = &window_relations[i];
        if (strcmp(relation->instance_pattern, instance_pattern) == 0 &&
                strcmp(relation->class_pattern, class_pattern) == 0) {
            remove_window_relation(i);
        }
    }
}

/* Clear the memory occupied by the window relation. */
void clear_window_relation(const struct window_relation *relation)
{
    free(relation->instance_pattern);
    free(relation->class_pattern);
    clear_action_list(&relation->actions);
}

/* Clear all currently set window relations. */
void clear_window_relations(void)
{
    LOG_DEBUG("clearing all window relations\n");

    for (size_t i = 0; i < window_relations_length; i++) {
        clear_window_relation(&window_relations[i]);
    }

    LIST_CLEAR(window_relations);
}

/* Run all actions within an assocation after selecting @window. */
static inline void run_window_relation(FcWindow *window,
        struct window_relation *relation)
{
    LOG_DEBUG("running related actions: %A\n",
            &relation->actions);

    Window_selected = window;
    run_action_list(&relation->actions);
}

/* Run the actions related to given window. */
bool run_window_relations(FcWindow *window)
{
    bool has_match = false;
    struct window_relation *relation;

    old_window_relations_length = window_relations_length;

    /* find all relations that match the window */
    for (size_t i = 0; i < old_window_relations_length; i++) {
        relation = &window_relations[i];
        if (matches_pattern(relation->instance_pattern,
                    window->properties.class.res_name) &&
                matches_pattern(relation->class_pattern,
                        window->properties.class.res_class)) {
            /* set back and forth in case the index changes because an
             * relation was removed
             */
            running_relation = i;
            run_window_relation(window, relation);
            i = running_relation;

            has_match = true;
        }
    }

    if (!has_match) {
        LOG_DEBUG("no relation for %s,%s\n",
                window->properties.class.res_name,
                window->properties.class.res_class);
    }

    return has_match;
}
