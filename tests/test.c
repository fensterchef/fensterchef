#include "core/log.h"
#include "test.h"
#include "utility/list.h"

LIST(test_t, test_functions);

/* Append a test to the functions to test. */
void add_test(test_t test_function)
{
    LIST_APPEND_VALUE(test_functions, test_function);
}

/* Run all test functions. */
int run_tests(const char *title)
{
    int result = 0;

    PRINT_TEST_TITLE(title);
    for (size_t i = 0; i < test_functions_length; i++) {
        result = test_functions[i]();
        if (result != 0) {
            PRINT_TEST_FAILURE(i + 1, test_functions_length);
            break;
        }
        PRINT_TEST_SUCCESS(i + 1, test_functions_length);
    }
    return result;
}
