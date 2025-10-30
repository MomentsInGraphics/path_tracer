#include "pfm.h"
#include <stdio.h>


int write_pfm(const char* file_path, const float* pixels, uint32_t width, uint32_t height, uint32_t pixel_stride) {
	FILE* file = fopen(file_path, "wb");
	if (!file) {
		printf("Failed to open %s for writing. Please check path and permissions.\n", file_path);
		return 1;
	}
	int length = fprintf(file, "PF\n%u %u\n-1.0", width, height);
	for (uint32_t i = 0; i != 63 - length; ++i)
		fprintf(file, "0");
	fprintf(file, "\n");
	size_t pixel_count = ((size_t) width) * ((size_t) height);
	if (pixel_stride == 3)
		fwrite(pixels, sizeof(float) * 3, pixel_count, file);
	else
		for (size_t i = 0; i != pixel_count; ++i)
			fwrite(&pixels[pixel_stride * i], sizeof(float), 3, file);
	fclose(file);
	return 0;
}
