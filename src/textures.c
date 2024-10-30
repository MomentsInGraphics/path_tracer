#include "textures.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//! The header of a *.vkt texture file
typedef struct {
	//! This file format marker should be 0xbc1bc1 (although formats other than
	//! BC1 are supported)
	uint32_t marker;
	//! The used file format version (should be 1)
	uint32_t version;
	//! The number of mipmap levels
	uint32_t mipmap_count;
	//! The resolution of the highest-resolution mipmap level in pixels
	VkExtent2D extent;
	//! The format used for the texture
	VkFormat format;
	//! The data size in bytes of all mipmaps combined (excluding headers)
	VkDeviceSize size;
} texture_header_t;


//! Each mipmap in a *.vkt file begins with such a header
typedef struct {
	//! The extent of this mipmap in pixels
	VkExtent2D extent;
	//! The size of the raw data for the mipmap in bytes, excluding headers
	VkDeviceSize size;
	//! The offset from the beginning of the payload to the mipmap in bytes
	VkDeviceSize offset;
} mipmap_header_t;


//! Handles data needed for loading a single texture
typedef struct {
	//! A file handle for the texture file that is being loaded
	FILE* file;
	//! The header of the texture file
	texture_header_t header;
	//! header.mipmap_count headers of mipmaps
	mipmap_header_t* mipmap_headers;
} texture_t;


/*! Handles data needed whilst loading textures. Unlike most other structures
	like this, this one is freed once loading is finished and only the images
	are preserved.*/
typedef struct {
	//! \see load_textures()
	images_t* images;
	//! \see load_textures()
	uint32_t texture_count;
	//! Data about the individual textures being loaded
	texture_t* textures;
} textures_t;


/*! Opens the given texture file, loads its header and the headers of its
	mipmaps. Returns 0 upon success. Upon failure, the calling side must free
	the partially initialized texture.*/
int init_texture(texture_t* texture, const char* texture_file_path) {
	memset(texture, 0, sizeof(*texture));
	FILE* file = texture->file = fopen(texture_file_path, "rb");
	if (!file) {
		printf("Failed to open the texture file at %s. Please check path and permissions.\n", texture_file_path);
		return 1;
	}
	// Load the texture header
	fread(&texture->header.marker, sizeof(uint32_t), 1, file);
	if (texture->header.marker != 0xbc1bc1) {
		printf("The file at %s does not appear to be a valid *.vkt texture file as it does not start with the appropriate file marker. Images from widely used formats must be piped through a texture conversion tool before loading them.\n", texture_file_path);
		return 1;
	}
	fread(&texture->header.version, sizeof(uint32_t), 1, file);
	if (texture->header.version != 1) {
		printf("The texture file at %s uses file format version %u, which is not supported.\n", texture_file_path, texture->header.version);
		return 1;
	}
	fread(&texture->header.mipmap_count, sizeof(uint32_t), 1, file);
	fread(&texture->header.extent, sizeof(uint32_t), 2, file);
	fread(&texture->header.format, sizeof(uint32_t), 1, file);
	fread(&texture->header.size, sizeof(uint64_t), 1, file);
	// Load mipmap headers
	texture->mipmap_headers = calloc(texture->header.mipmap_count, sizeof(mipmap_header_t));
	for (uint32_t i = 0; i != texture->header.mipmap_count; ++i) {
		mipmap_header_t* header = &texture->mipmap_headers[i];
		fread(&header->extent, sizeof(uint32_t), 2, file);
		fread(&header->size, sizeof(uint64_t), 1, file);
		fread(&header->offset, sizeof(uint64_t), 1, file);
	}
	return 0;
}


//! A callback for fill_images() that loads a single mipmap from an already
//! opened texture file
void write_mipmap(void* image_data, uint32_t image_index, const VkImageSubresource* subresource, VkDeviceSize buffer_size, const VkImageCreateInfo* image_info, const VkExtent3D* subresource_extent, const void* context) {
	const textures_t* textures = (const textures_t*) context;
	const texture_t* texture = &textures->textures[image_index];
	if (buffer_size == texture->mipmap_headers[subresource->mipLevel].size)
		fread(image_data, sizeof(uint8_t), buffer_size, texture->file);
	else
		printf("The data block for mipmap %u of texture %u was supposed to have %lu bytes but had %lu bytes. Skipping this mipmap.\n", subresource->mipLevel, image_index, buffer_size, texture->mipmap_headers[subresource->mipLevel].size);
}


void free_textures(textures_t* textures, const device_t* device);


int load_textures(images_t* images, const device_t* device, const char* const* texture_file_paths, uint32_t texture_count, VkImageUsageFlags usage, VkImageLayout image_layout) {
	memset(images, 0, sizeof(*images));
	textures_t textures = {
		.images = images,
		.texture_count = texture_count,
	};
	textures.textures = calloc(texture_count, sizeof(texture_t));
	// Load texture meta data and create images
	image_request_t* image_requests = calloc(texture_count, sizeof(image_request_t));
	usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	for (uint32_t i = 0; i != texture_count; ++i) {
		texture_t* texture = &textures.textures[i];
		if (init_texture(texture, texture_file_paths[i])) {
			printf("Failed to load texture %u out of %u. Its file path is %s. Aborting.\n", i, texture_count, texture_file_paths[i]);
			free(image_requests);
			free_textures(&textures, device);
			return 1;
		}
		image_requests[i] = (image_request_t) {
			.image_info = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.arrayLayers = 1,
				.extent = { texture->header.extent.width, texture->header.extent.height, 1, },
				.format = texture->header.format,
				.imageType = VK_IMAGE_TYPE_2D,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.mipLevels = texture->header.mipmap_count,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.usage = usage,
			},
			.view_info = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				},
			},
		};
	}
	if (create_images(images, device, image_requests, texture_count, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
		printf("Failed to create/allocate images for %u textures.\n", texture_count);
		free(image_requests);
		free_textures(&textures, device);
		return 1;
	}
	free(image_requests);
	image_requests = NULL;
	if (fill_images(images, device, &write_mipmap, VK_IMAGE_LAYOUT_UNDEFINED, image_layout, &textures)) {
		printf("Failed to copy texture data for %u textures from files onto the GPU.\n", texture_count);
		free_textures(&textures, device);
		return 1;
	}
	textures.images = NULL;
	free_textures(&textures, device);
	return 0;
}


void free_textures(textures_t* textures, const device_t* device) {
	if (textures->images) free_images(textures->images, device);
	if (textures->textures) {
		for (uint32_t i = 0; i != textures->texture_count; ++i) {
			texture_t* texture = &textures->textures[i];
			if (texture->file) fclose(texture->file);
			free(texture->mipmap_headers);
		}
	}
	free(textures->textures);
	memset(textures, 0, sizeof(*textures));
}
