#include "string_utilities.h"
#include <string.h>
#include <stdlib.h>


char* cat_strings(const char* const* const strings, size_t string_count) {
	size_t total_length = 0;
	for (size_t i = 0; i != string_count; ++i)
		total_length += strlen(strings[i]);
	char* result = malloc(total_length + 1);
	char* cursor = result;
	for (size_t i = 0; i != string_count; ++i) {
		size_t length = strlen(strings[i]);
		memcpy(cursor, strings[i], length);
		cursor += length;
	}
	(*cursor) = '\0';
	return result;
}


char* copy_string(const char* string) {
	size_t length = strlen(string);
	char* result = malloc(length + 1);
	memcpy(result, string, length + 1);
	return result;
}
