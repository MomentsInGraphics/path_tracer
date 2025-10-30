#pragma once
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>


//! Provides the number of entries in the given object of array type using a
//! ratio of sizeof() values. It does not work for pointers.
#define COUNT_OF(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))


//! Copies the given null-terminated string. The calling-side has to free it.
char* copy_string(const char* string);


//! Concatenates arbitrarily many null-terminated strings. The returned
//! null-terminated string that is returned must be freed by the calling side.
char* cat_strings(const char* const* const strings, size_t string_count);


/*! Given a format string that expects exactly one uint32_t (e.g. due to one
	%u), this function constructs a null-terminated formatted string that has
	to be freed by the calling side.*/
static inline char* format_uint(const char* format, uint32_t integer) {
	int size = snprintf(NULL, 0, format, integer) + 1;
	char* result = (char*) malloc(size);
	sprintf(result, format, integer);
	return result;
}


/*! Given a format string that expects exactly one float (e.g. due to one
	%f), this function constructs a null-terminated formatted string that has
	to be freed by the calling side.*/
static inline char* format_float(const char* format, float number) {
	int size = snprintf(NULL, 0, format, number) + 1;
	char* result = (char*) malloc(size);
	sprintf(result, format, number);
	return result;
}


/*! Given a format string that expects exactly one string (e.g. due to one
	%s), this function constructs a null-terminated formatted string that has
	to be freed by the calling side.*/
static inline char* format_string(const char* format, const char* substitution) {
	int size = snprintf(NULL, 0, format, substitution) + 1;
	char* result = (char*) malloc(size);
	sprintf(result, format, substitution);
	return result;
}


/*! Produces a decimal string representation of a number, using scientific
	notation for particularly small or large numbers.
	\param string The (already allocated) string, which will be overwritten by
		the output null-terminated string. Must be at least 1024 bytes large.
	\param n The number to be represented.
	\return The pointer string.
	\note The main purpose of this function is for use in Nuklear.*/
char* double_to_string(char* string, double n);


/*! Turns a decimal string representation of a number into a double.
	\param string A null-terminated string providing the number, possibly with
		leading white space.
	\param end If this is not NULL, it will be set to a pointer to the first
		character in string (possibly \0), which is not part of the read
		number.
	\return The number read from the string, 0.0 to indicate failure.*/
double string_to_double(const char* string, const char** end);
