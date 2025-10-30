#include "serializer.h"
#include <stdlib.h>
#include <string.h>


int serialize_block(void* block, size_t size, serializer_t* serializer) {
	if (size == 0) return 0;
	if (!serializer->file || !block) return 1;
	if (serializer->write)
		return fwrite(block, size, 1, serializer->file) != 1;
	else
		return fread(block, size, 1, serializer->file) != 1;
}


int serialize_array(uint64_t* element_count, void** array, size_t element_size, serializer_t* serializer) {
	if (element_size == 0) return 0;
	if (!serializer->file || !element_count || !array) return 1;
	if (serializer->write) {
		if (*element_count > 0 && !*array) return 1;
		return fwrite(element_count, sizeof(*element_count), 1, serializer->file) != 1
			|| fwrite(*array, element_size, *element_count, serializer->file) != *element_count;
	}
	else {
		uint64_t new_count;
		if (fread(&new_count, sizeof(new_count), 1, serializer->file) != 1)
			return 1;
		if (new_count == 0) {
			(*array) = NULL;
			(*element_count) = new_count;
			return 0;
		}
		else {
			void* new_array = malloc(element_size * new_count);
			if (!new_array) return 1;
			if (fread(new_array, element_size, new_count, serializer->file) != new_count) {
				free(new_array);
				return 1;
			}
			else {
				(*array) = new_array;
				(*element_count) = new_count;
				return 0;
			}
		}
	}
}


int serialize_string(char** string, serializer_t* serializer) {
	if (!serializer->file || !string) return 1;
	if (serializer->write) {
		if (!*string) return 1;
		uint64_t length = (uint64_t) strlen(*string);
		return fwrite(&length, sizeof(uint64_t), 1, serializer->file) != 1
			|| fwrite(*string, sizeof(char), length + 1, serializer->file) != length + 1;
	}
	else {
		uint64_t length;
		if (fread(&length, sizeof(uint64_t), 1, serializer->file) != 1)
			return 1;
		char* new_string = malloc(length + 1);
		if (fread(new_string, sizeof(char), length + 1, serializer->file) != length + 1) {
			free(new_string);
			return 1;
		}
		free(*string);
		(*string) = new_string;
		return 0;
	}
}
