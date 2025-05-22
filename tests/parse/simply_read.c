#include <string.h>

#include "parse/input.h"
#include "parse/parse.h"
#include "test.h"

static struct test {
    const char *input;
    /* if this is NULL, it means copy the input */
    const char *output;
} test_cases[] = {
    { "Hello there\nWhat is up?", NULL },
    { "Hello there\n  \\What is up?", "Hello thereWhat is up?" },
    { "local i = 0\nwhile i < 10\ni++", NULL },
    { "set string = \"Hey\n# Okay then  \n\\ you\n  \\ over\n  \\ there\"",
        "set string = \"Hey you over there\"" },
    { " # I'm not a comment\n# But I'm a comment\nI'm real",
        " # I'm not a comment\nI'm real" },
    { "# I'm nothing", "" },
    { "# I'm supposed to be nothing but this does not affect the parser in any way\n", "\n" },
};

void compare_output(Parser *parser, const char *expected)
{
    int character;

    while (character = get_stream_character(parser), character != EOF) {
        if ((unsigned char) expected[0] != character) {
            printf("got '%c'... but expected '%s'\n",
                    character, &expected[0]);
            exit(EXIT_FAILURE);
        }
        expected++;
    }

    if (expected[0] != '\0') {
        printf("got EOF but still have '%s'\n", expected);
        exit(EXIT_FAILURE);
    }
}

int main(void)
{
    Parser *parser;

    PRINT_TEST_TITLE("Reading strings to parse");
    for (unsigned i = 0; i < SIZE(test_cases); i++) {
        const char *const input = test_cases[i].input;

        parser = create_string_parser(input);
        compare_output(parser, test_cases[i].output == NULL ? input :
                test_cases[i].output);
        destroy_parser(parser);

        FILE *fp = fopen("/tmp/simply_read_test.txt", "w");
        fwrite(input, 1, strlen(input), fp);
        fclose(fp);

        parser = create_file_parser("/tmp/simply_read_test.txt");
        if (parser == NULL) {
            PRINT_TEST_FAILURE(i + 1, SIZE(test_cases));
            fprintf(stderr, "could not open test temporary file\n");
            return EXIT_FAILURE;
        }
        compare_output(parser, test_cases[i].output == NULL ? input :
                test_cases[i].output);
        destroy_parser(parser);

        PRINT_TEST_SUCCESS(i + 1, SIZE(test_cases));
    }

    return EXIT_SUCCESS;
}
