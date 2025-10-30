#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>


//! A macro to pass a pointer to VARIABLE and its size to serialize_block.
//! Assumes that the scope contains a pointer serializer_t* ser.
#define SERIALIZE_BLOCK(VARIABLE) serialize_block(&(VARIABLE), sizeof(VARIABLE), ser)


//! An object used to serialize binary data to/from a file.
typedef struct {
	//! The file to write to/read from or NULL if the serializer is not ready
	//! for use
	FILE* file;
	//! Whether this serializer writes data to a file (true) or reads from it
	bool write;
} serializer_t;


//! Serializes size bytes of binary data reading from or writing to the array
//! pointed to by block. Returns 0 upon success.
int serialize_block(void* block, size_t size, serializer_t* serializer);


/*! Serializes an array from/to a file.
	\param element_count When writing to the file, this pointer points to the
		number of elements in the array at *array that will be written. When
		reading, it will be overwritten by the element count found in the file.
	\param array When writing, *array has to be a pointer to *element_count
		consecutive objects of size element_size that will be written. When
		reading, *array will be set to a freshly malloc'ed memory range that
		gets filled with array elements from the file. For zero-size arrays,
		*array will be set to NULL. The calling side must free(*array).
	\param element_size The size in bytes of array elements and also the stride
		between them.
	\param serializer Source or destination of data.
	\return 0 upon success. Upon failure only the state of the file may have
		changed.*/
int serialize_array(uint64_t* element_count, void** array, size_t element_size, serializer_t* serializer);


/*! Serializes a null-terminated string from/to a file.
	\param string When writing to the file, *string must point to a null-
		terminated string that will be written to the file (preceeded by its
		length). When reading, *string will be set to a newly malloc'ed memory
		range that will be filled with the null-terminated string read from the
		file. The calling side must free(*string).
	\param serializer Source or destination of data.
	\return 0 upon success. Upon failure only the state of the file may have
		changed.*/
int serialize_string(char** string, serializer_t* serializer);
