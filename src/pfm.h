#include <stdint.h>

/*! Writes an HDR image to disk as PFM file.
	\param file_path The full path to the output file.
	\param pixels HDR pixel data with at least three channels and densely
		packed pixels (scanline by scanline).
	\param width The width of the image in pixels.
	\param height The height of the image in pixels.
	\param pixel_stride The stride between two subsequent pixels in a scanline
		as multiple of sizeof(float).
	\return 0 on success.*/
int write_pfm(const char* file_path, const float* pixels, uint32_t width, uint32_t height, uint32_t pixel_stride);
