#include <stddef.h> /* NULL */
#include <stdbool.h> /* bool, true, false */

/* Match a string against a pattern. */
bool matches_pattern(const char *pattern, const char *string)
{
    unsigned char pattern_character, string_character;
    const char *pattern_backtrack = NULL, *string_backtrack;

    /* move through both pattern and string until one end is reached or there is
     * a guaranteed mismatch
     */
    while (true) {
        pattern_character = pattern[0];
        string_character = string[0];
        pattern++;
        string++;

        switch (pattern_character) {
        /* there must be a character in the string */
        case '?':
            if (string_character == '\0') {
                return false;
            }
            break;

        /* any characters may follow */
        case '*':
            /* if this is the end of the pattern, anything would match */
            if (pattern[0] == '\0') {
                return true;
            }

            pattern_backtrack = pattern;
            string_backtrack = string;
            break;

        /* match against ranges of characters if there is an ending ] */
        case '[': {
            int is_inverted = 0;
            unsigned char from, to;
            const char *class;
            int is_matching = 0;

            if (string_character == '\0') {
                return false;
            }

            class = pattern;
            /* match ^ and ! literally if the end is there directly */
            if ((class[0] == '^' || class[0] == '!') && class[1] != ']') {
                is_inverted = 1;
                class++;
            }

            from = class[0];
            class++;
            do {
                if (from == '\0') {
                    is_matching = -1;
                    break;
                }

                to = from;
                if (class[0] == '-' && class[1] != ']') {
                    to = class[1];

                    if (to == '\0') {
                        is_matching = -1;
                        break;
                    }

                    class += 2;
                }

                if (string_character >= from && string_character <= to) {
                    is_matching = 1;
                }
            } while (from = class[0], class++, from != ']');

            if (is_matching != -1) {
                if (is_matching == is_inverted) {
                    goto backtrack;
                }
                pattern = class;
                continue;
            }
        }
            /* fall through */
        default:
            /* escape any of \, ?, * or [ */
            if (pattern_character == '\\' &&
                    (pattern[0] == '\\' || pattern[0] == '?' ||
                        pattern[0] == '*' || pattern[0] == '[')) {
                pattern_character = pattern[0];
                pattern++;
            }

            if (pattern_character != string_character) {
        backtrack:
                if (string_character == '\0' || pattern_backtrack == NULL) {
                    /* either no more characters or nowhere to backtrack to */
                    return false;
                }

                /* backtrack to the previous point (pattern character after *)
                 * and try matching again at the next string character
                 */
                pattern = pattern_backtrack;
                string_backtrack++;
                string = string_backtrack;
            } else if (string_character == '\0') {
                return true;
            }
            break;
        }
    }
}
