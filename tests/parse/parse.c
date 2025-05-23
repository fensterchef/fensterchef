#include <stdlib.h>

#include "parse/parse.h"
#include "test.h"
#include "utility/utility.h"

static struct test {
    const char *input;
    bool is_parseable;
} test_cases[] = {
    { "auto equalize 1", true },
    { "auto equalize on", true },
    { "auto remove off", true },
    { "auto equalize true", true },
    { "auto fill void false", true },
    { "auto split cool", false },
    { "equalize", true },
    { "focus down", true },
    { "focus", true },
    { "focus leaf", true },
    { "focus left", true },
    { "show list", true },
    { "set tiling", true },
    { "set fullscreen", true },
    { "quit", true },
    { "pop stash", true },
    { "split left horizontally", true },
    { "split left vertically", true },
    { "split  vertically", true },
    { "show message hello!", true },
    { "relate discord set floating", true },
    { "whattt 4, see 8", false },
    { "can not believe", false },
    { "'what'", false },
    { "( set floating\ncenter window)", true },
    { "'firefox' set boiling, set floating, yeeting", false },
    { "a set floating", true },
    { "b 'floating'", false },
    { "Shift+LOL set floating, quazzle, doo", false },
    { "relate 'mewindow' set floating", true },
    { "relate 'mewindow' set floating, minimize window", true },
    { "gaps inner", false },
    { "center window", true },
    { "center window to hey", true },
    { "move window by -80 0", true },
    { "unbind a", true },
    { "unbind Shift + a", true },
    { "unbind my lord", false },
    { "unbind,", false },
    { "\"asso\" \"assoinner\" \"assomoreinner\" a ( gaps inner )", false },
    { "a ( a ( focus left\n focus up ) )", true },
    { "  )", false },
    { "              (  ", false },
    { "     (  ", false },
    { "alias hello = focus", true },
    { "hello window", true },
    { "hello 8", true },
    { "unalias hello", true },
    { "hello 7", false },
    { "move window by 80 + 80 23 + 88", true },
    { "move window by 80 + 80 + 44 on", true },
    { "move window by Shift + 80 + 44 on", true },
    { "[88] unbind [88]", true },
    { "[ 24  ] run st", true },
    { "24] run st", false },
    { "[24 run st", false },
    { "[] run st", false },
    { "24 run", false },
    { "a", false },

    /* random symbol tests */
    { "@#!#!@#@#,2,,2,", false },
    { "relate \"discord\" @ xD", false },
    { "{ what }, can you offer?", false },
    { "#okay\r\nsee that", false },
    { "\v\f\x01\x02\x03", false },
};

int main(void)
{
    Parser *parser;

    PRINT_TEST_TITLE("Running the parser (includes negative tests)");
    for (unsigned i = 0; i < SIZE(test_cases); i++) {
        parser = create_string_parser(test_cases[i].input);
        if ((test_parser(parser) != OK) == test_cases[i].is_parseable) {
            PRINT_TEST_FAILURE(i + 1, SIZE(test_cases));
            fprintf(stderr, "failed at: %.*s\n",
                    (int) (parser->length - parser->index),
                    &parser->input[parser->index]);
            destroy_parser(parser);
            return EXIT_FAILURE;
        }
        destroy_parser(parser);
        PRINT_TEST_SUCCESS(i + 1, SIZE(test_cases));
    }

    return EXIT_SUCCESS;
}
