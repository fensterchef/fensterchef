#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

/* If the compiler does not have __has_attribute, always make it false. */
#ifndef __has_attribute
#  define __has_attribute(x) 0
#endif

/* Mark a function as "inline this function no matter what".
 *
 * Functions should be inlined like this when they are performance critical.
 */
#if defined __GNUC__ || __has_attribute(always_inline)
#   define INLINE __attribute__((always_inline)) inline
#else
#   define INLINE inline
#endif

/* Mark a function parameter as "allowed to be NULL". */
#define _Nullable
/* Mark a function parameter as "NOT allowed to be NULL", this should be implied
 * by excluding `_Nullable`, but can be used to put emphasize on it.
 */
#define _Nonnull

/* Mark a pointer parameter to a function as "stores output of the function". */
#define _Out

/* Mark a pointer parameter to a function as "stores output of the function" but
 * also that it acts as input parameter.
 */
#define _Inout

#endif
