#include "string_utilities.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>


char* copy_string(const char* string) {
	if (!string) return NULL;
	size_t length = strlen(string);
	char* result = malloc(length + 1);
	memcpy(result, string, sizeof(char) * (length + 1));
	return result;
}


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


char* double_to_string(char* string, double n) {
	//! \todo Use scientific notation
	sprintf(string, "%.5lf", n);
	return string;
}


double string_to_double(const char* string, const char** end){
	if (!string) return 0.0;
	double result = 0.0;
	sscanf(string, "%lf", &result);
	//! \todo Determine end properly
	if (end)
		(*end) = string + strlen(string);
	return result;
}
