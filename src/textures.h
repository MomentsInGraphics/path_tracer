#pragma once
#include "vulkan_basics.h"


/*! Loads textures from *.vkt files into device-local memory.
	\param images The output. Clean up using free_images().
	\param device Output of create_device().
	\param texture_file_paths An array of absolute or relative paths to *.vkt
		files. Each entry corresponds to one entry of images->images.
	\param texture_count The number of array entries in texture_file_paths.
	\param usage The Vulkan usage flags that are to be used for all textures.
		Transfer destination usage is always added.
	\param image_layout The layout of all created images upon success.
	\return 0 upon success.*/
int load_textures(images_t* images, const device_t* device, const char* const* texture_file_paths, uint32_t texture_count, VkImageUsageFlags usage, VkImageLayout image_layout);
