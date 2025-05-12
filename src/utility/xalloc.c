#include <errno.h> /* errno */
#include <stdarg.h> /* va_list, va_start(), va_end() */
#include <string.h> /* strerror(), vsnprintf(), vsprintf(), memcpy() */

#include "utility/utility.h" /* ASSERT() */

/* Allocate a minimum of @size bytes of memory. */
void *xmalloc(size_t size)
{
    void *pointer;

    if (UNLIKELY(size == 0)) {
        pointer = NULL;
    } else {
        pointer = malloc(size);
        ASSERT(pointer != NULL, strerror(errno));
    }
    return pointer;
}

/* Allocate @number_of_elements number of elements with each element being
 * @size_per_element large.
 */
void *xcalloc(size_t number_of_elements, size_t size_per_element)
{
    void *pointer;

    if (UNLIKELY(number_of_elements == 0 || size_per_element == 0)) {
        pointer = NULL;
    } else {
        pointer = calloc(number_of_elements, size_per_element);
        ASSERT(pointer != NULL, strerror(errno));
    }
    return pointer;
}

/* Grow or shrink a previously allocated memory region. */
void *xrealloc(void *pointer, size_t size)
{
    if (size == 0) {
        free(pointer);
        pointer = NULL;
    } else {
        pointer = realloc(pointer, size);
        ASSERT(pointer != NULL, strerror(errno));
    }
    return pointer;
}

/* Same as `xrealloc()` but instead of using bytes as argument, use
 * @number_of_elements * @size_per_element.
 */
void *xreallocarray(void *pointer, size_t number_of_elements, size_t size)
{
    size_t byte_count;

#if __has_builtin(__builtin_mul_overflow)
    ASSERT(!__builtin_mul_overflow(number_of_elements, size, &byte_count),
            "unsigned integer overflow");
#else
    if (number_of_elements == 0 || size == 0) {
        byte_count = 0;
    } else {
        ASSERT(SIZE_MAX / number_of_elements >= size,
                "unsigned integer overflow");
        byte_count = number_of_elements * size;
    }
#endif
    return xrealloc(pointer, byte_count);
}

/* Combination of `xmalloc()` and `memcpy()`. */
void *xmemdup(const void *pointer, size_t size)
{
    char *duplicate;

    if (size == 0) {
        duplicate = NULL;
    } else {
        duplicate = malloc(size);
        ASSERT(duplicate != NULL, strerror(errno));
        (void) memcpy(duplicate, pointer, size);
    }
    return duplicate;
}

/* Duplicates the null-terminated @string pointer by creating a copy. */
char *xstrdup(const char *string)
{
    size_t length;
    char *result;

    if (string == NULL) {
        result = NULL;
    } else {
        /* +1 for the null terminator */
        length = strlen(string) + 1;
        result = xmalloc(length);
        (void) memcpy(result, string, length);
    }
    return result;
}

/* Like `xstrdup()` but stop at @length when the null-terminator is not yet
 * encountered.
 */
char *xstrndup(const char *string, size_t length)
{
    char *result;

    length = strnlen(string, length);
    /* +1 for the null terminator */
    result = xmalloc(length + 1);
    result[length] = '\0';
    return memcpy(result, string, length);
}

/* Combination of `xmalloc()` and `sprintf()`. */
char *xasprintf(const char *format, ...)
{
    va_list list;
    int total_size;
    char *result;

    /* get the length of the expanded format */
    va_start(list, format);
    total_size = vsnprintf(NULL, 0, format, list);
    va_end(list);

    ASSERT(total_size >= 0, strerror(errno));

    va_start(list, format);
    /* +1 for the null terminator */
    result = xmalloc((size_t) total_size + 1);
    (void) vsprintf(result, format, list);
    va_end(list);

    return result;
}
