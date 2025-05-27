#include <stdlib.h>

#include "parse/parse.h"
#include "test.h"
#include "utility/utility.h"

int config_file_parser(void)
{
    Parser *parser;

    parser = create_file_parser("wm");

    if (test_parser(parser) != OK) {
        destroy_parser(parser);
        return 1;
    }

    destroy_parser(parser);
    return 0;
}

int random_file_parser(void)
{
    Parser *parser;

    /* just for fun */
    parser = create_file_parser("src/parse/parse.c");

    if (test_parser(parser) == OK) {
        destroy_parser(parser);
        return 1;
    }

    destroy_parser(parser);
    return 0;
}

int main(void)
{
    add_test(config_file_parser);
    add_test(random_file_parser);
    return run_tests("Parsing test files");
}
