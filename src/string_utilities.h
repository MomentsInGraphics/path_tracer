#pragma once
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>


//! Provides the number of entries in the given object of array type using a
//! ratio of sizeof() values. It does not work for pointers.
#define COUNT_OF(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))


//! Concatenates arbitrarily many null-terminated strings. The returned
//! null-terminated string that is returned must be freed by the calling side.
char* cat_strings(const char* const* const strings, size_t string_count);


//! Creates a copy of the given null-terminated string that must be freed by
//! the calling side
char* copy_string(const char* string);


/*! Given a format string that expects exactly one uint32_t (e.g. due to one
	%u), this function constructs a null-terminated formatted string that has
	to be freed by the calling side.*/
static inline char* format_uint(const char* format, uint32_t integer) {
	int size = snprintf(NULL, 0, format, integer) + 1;
	char* result = (char*) malloc(size);
	sprintf(result, format, integer);
	return result;
}
