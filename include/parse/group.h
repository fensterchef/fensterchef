#ifndef PARSE__GROUP_H
#define PARSE__GROUP_H

/* Parse a group declaration.
 *
 * group STRING ACTIONS
 */

#include "core/action.h"
#include "parse/action.h"
#include "parse/parse.h"

/* must be a power of two */
#define PARSE_MAX_GROUPS 1024

/* a parsed group */
struct parse_group {
    /* name of the group */
    char *name;
    /* the actions within the group */
    struct action_list actions;
};

/* Find the group by given name. */
struct parse_group *find_group(const char *name);

/* Run a list of actions to undo all within the group.
 *
 * This includes:
 * - bindings
 * - relations
 */
void undo_group(const struct parse_group *group);

/* Clear all groups the parser set. */
void clear_all_groups(void);

/* Parse all after a `group` keyword. */
void continue_parsing_group(Parser *parser);

/* Parse all after a `ungroup` keyword. */
void continue_parsing_ungroup(Parser *parser, struct parse_action_list *list);

#endif
