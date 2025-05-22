#ifndef TESTS__TEST_H
#define TESTS__TEST_H

#include "core/log.h"

/* test function */
typedef int (*test_t)(void);

/* Append a test to the functions to test. */
void add_test(test_t test_function);

/* Run all test functions.
 *
 * @return 0 if all succeeded, otherwise the error code returned by the test
 *         function.
 */
int run_tests(const char *title);

/* Print the title of a test. */
#define PRINT_TEST_TITLE(title) \
    fprintf(stderr, "\n%s\n", (title))

/* Print a message indicating that a test succeeded. */
#define PRINT_TEST_SUCCESS(index, count) \
    fprintf(stderr, "[%u/%u] " COLOR(GREEN) "OK" CLEAR_COLOR "\n", \
           (unsigned) (index), (unsigned) (count))

/* Print a message indicating that a test failed. */
#define PRINT_TEST_FAILURE(index, count) \
    fprintf(stderr, "[%u/%u] " COLOR(RED) "ERROR" CLEAR_COLOR "\n", \
           (unsigned) (index), (unsigned) (count))

#endif
