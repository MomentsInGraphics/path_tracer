#include "vulkan_basics.h"
#include "string_utilities.h"
#include "math_utilities.h"
#include "vulkan_formats.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


int create_device(device_t* device, const char* app_name, uint32_t physical_device_index) {
	memset(device, 0, sizeof(*device));
	device->physical_device_index = physical_device_index;
	// Initialize GLFW
	if (!glfwInit()) {
		printf("GLFW initialization failed.\n");
		return 1;
	}
	// Create an instance with extensions required according to GLFW and
	// validation layers in the debug build
	const char* validation_layer_name = "VK_LAYER_KHRONOS_validation";
	VkInstanceCreateInfo instance_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &(VkApplicationInfo) {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.applicationVersion = 100,
			.engineVersion = 100,
			.apiVersion = VK_MAKE_VERSION(1, 3, 0),
			.pApplicationName = app_name,
			.pEngineName = app_name,
		},
		.ppEnabledLayerNames = &validation_layer_name,
#ifndef NDEBUG
		.enabledLayerCount = 1,
#endif
	};
	instance_info.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&instance_info.enabledExtensionCount);
	VkResult result;
	if (result = vkCreateInstance(&instance_info, NULL, &device->instance)) {
		printf("Failed to create a Vulkan instance. vkCreateInstance() returned the VkResult error code %d.\n", result);
		free_device(device);
		return 1;
	}
	// Print info about available physical devices
	if (vkEnumeratePhysicalDevices(device->instance, &device->physical_device_count, NULL)) {
		printf("Failed to enumerate physical devices (i.e. GPUs).\n");
		free_device(device);
		return 1;
	}
	if (device->physical_device_count == 0) {
		printf("Found no physical devices. Make sure that you have proper GPU drivers installed.\n");
		free_device(device);
		return 1;
	}
	device->physical_devices = calloc(device->physical_device_count, sizeof(VkPhysicalDevice));
	vkEnumeratePhysicalDevices(device->instance, &device->physical_device_count, device->physical_devices);
	printf("The following GPUs are available to Vulkan:\n");
	for (uint32_t i = 0; i != device->physical_device_count; ++i) {
		VkPhysicalDeviceVulkan11Properties props_11 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES };
		VkPhysicalDeviceProperties2 props = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &props_11 };
		vkGetPhysicalDeviceProperties2(device->physical_devices[i], &props);
		printf("%u%s %s\n", i, (i == physical_device_index) ? " (used):" : ":       ", props.properties.deviceName);
		if (i == physical_device_index) {
			device->physical_device_properties = props.properties;
			device->physical_device_properties_11 = props_11;
		}
	}
	if (physical_device_index >= device->physical_device_count) {
		printf("The requested physical device index %u is not available. Either you lower the index or you install more GPUs.\n", physical_device_index);
		free_device(device);
		return 1;
	}
	device->physical_device = device->physical_devices[physical_device_index];
	// Get info about memory heaps and types
	vkGetPhysicalDeviceMemoryProperties(device->physical_device, &device->memory_properties);
	// Enumerate queue families
	vkGetPhysicalDeviceQueueFamilyProperties(device->physical_device, &device->queue_family_count, NULL);
	if (device->queue_family_count == 0) {
		printf("Found zero queue families. Aborting device creation.\n");
		free_device(device);
		return 1;
	}
	device->queue_family_properties = calloc(device->queue_family_count, sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(device->physical_device, &device->queue_family_count, device->queue_family_properties);
	// Pick a queue that supports graphics and compute
	VkQueueFlagBits required_queue_flags = VK_QUEUE_GRAPHICS_BIT & VK_QUEUE_COMPUTE_BIT;
	device->queue_family_index = device->queue_family_count;
	for (uint32_t i = 0; i != device->queue_family_count; ++i) {
		if ((device->queue_family_properties[i].queueFlags & required_queue_flags) == required_queue_flags) {
			device->queue_family_index = i;
			break;
		}
	}
	if (device->queue_family_index == device->queue_family_count) {
		printf("Could not find a Vulkan queue that supports graphics and compute.\n");
		free_device(device);
		return 1;
	}
	// Pick extensions
	const char* extension_names[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
		VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
	};
	// Create a device
	VkDeviceQueueCreateInfo queue_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueCount = 1,
		.queueFamilyIndex = device->queue_family_index,
		.pQueuePriorities = &(float) { 1.0f },
	};
	VkPhysicalDeviceFeatures enabled_features = {
		.samplerAnisotropy = VK_TRUE,
		.shaderSampledImageArrayDynamicIndexing = VK_TRUE,
	};
	VkPhysicalDeviceVulkan12Features enabled_new_features = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.descriptorIndexing = VK_TRUE,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.shaderUniformTexelBufferArrayDynamicIndexing = VK_TRUE,
	};
	VkDeviceCreateInfo device_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &enabled_new_features,
		.ppEnabledExtensionNames = extension_names,
		.enabledExtensionCount = COUNT_OF(extension_names),
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queue_info,
		.pEnabledFeatures = &enabled_features,
	};
	if (result = vkCreateDevice(device->physical_device, &device_info, NULL, &device->device)) {
		printf("Failed to create a Vulkan device (error code %u). The following extensions were to be used:\n", result);
		for (uint32_t i = 0; i != device_info.enabledExtensionCount; ++i)
			printf("%s\n", device_info.ppEnabledExtensionNames[i]);
		free_device(device);
		return 1;
	}
	// Query the queue from the device
	vkGetDeviceQueue(device->device, device->queue_family_index, 0, &device->queue);
	// Create a command pool
	VkCommandPoolCreateInfo cmd_pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.queueFamilyIndex = device->queue_family_index,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
	};
	if (vkCreateCommandPool(device->device, &cmd_pool_info, NULL, &device->cmd_pool)) {
		printf("Failed to create a command pool.\n");
		free_device(device);
		return 1;
	}
	return 0;
}


void free_device(device_t* device) {
	if (device->cmd_pool) vkDestroyCommandPool(device->device, device->cmd_pool, NULL);
	if (device->device) vkDestroyDevice(device->device, NULL);
	if (device->instance) vkDestroyInstance(device->instance, NULL);
	free(device->queue_family_properties);
	free(device->physical_devices);
	glfwTerminate();
	memset(device, 0, sizeof(*device));
}


swapchain_result_t create_swapchain(swapchain_t* swapchain, const device_t* device, GLFWwindow* window, bool use_vsync) {
	memset(swapchain, 0, sizeof(*swapchain));
	// Create a surface
	if (glfwCreateWindowSurface(device->instance, window, NULL, &swapchain->surface)) {
		const char* err = NULL;
		glfwGetError(&err);
		printf("Failed to create a Vulkan surface for a window:\n%s\n", err);
		free_swapchain(swapchain, device);
		return 1;
	}
	// Make sure that presenting is supported
	VkBool32 supports_present;
	if (vkGetPhysicalDeviceSurfaceSupportKHR(device->physical_device, device->queue_family_index, swapchain->surface, &supports_present) || !supports_present) {
		printf("Failed to ascertain that the Vulkan surface supports presentation.\n");
		free_swapchain(swapchain, device);
		return 1;
	}
	// Enumerate supported formats
	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device->physical_device, swapchain->surface, &format_count, NULL);
	VkSurfaceFormatKHR* formats = calloc(format_count, sizeof(VkSurfaceFormatKHR));
	if (!format_count || vkGetPhysicalDeviceSurfaceFormatsKHR(device->physical_device, swapchain->surface, &format_count, formats)) {
		printf("Failed to enumerate supported formats for a Vulkan surface.\n");
		free_swapchain(swapchain, device);
		return 1;
	}
	// Pick one
	for (uint32_t i = 0; i != format_count; ++i) {
		VkFormat fmt = formats[i].format;
		if (formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR && (fmt == VK_FORMAT_B8G8R8A8_SRGB || fmt == VK_FORMAT_R8G8B8A8_SRGB)) {
			swapchain->format = fmt;
			break;
		}
	}
	free(formats);
	formats = NULL;
	// Enumerate present modes
	uint32_t mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device, swapchain->surface, &mode_count, NULL);
	VkPresentModeKHR* modes = calloc(mode_count, sizeof(VkPresentModeKHR));
	if (!mode_count || vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device, swapchain->surface, &mode_count, modes)) {
		printf("Failed to enumerate supported present modes for a Vulkan surface.\n");
		free_swapchain(swapchain, device);
		return 1;
	}
	// Pick one. In the second round v-sync is forced on.
	swapchain->present_mode = VK_PRESENT_MODE_MAX_ENUM_KHR;
	for (uint32_t i = 0; i != 2; ++i) {
		for (uint32_t j = 0; j != mode_count; ++j) {
			if ((use_vsync && (modes[j] == VK_PRESENT_MODE_MAILBOX_KHR || modes[j] == VK_PRESENT_MODE_FIFO_KHR))
			|| (!use_vsync && modes[j] == VK_PRESENT_MODE_IMMEDIATE_KHR))
			{
				swapchain->present_mode = modes[j];
				break;
			}
		}
		if (!use_vsync && swapchain->present_mode == VK_PRESENT_MODE_MAX_ENUM_KHR) {
			printf("Failed to find a present mode without v-sync. Forcing v-sync on.\n");
			use_vsync = true;
		}
		else
			break;
	}
	free(modes);
	modes = NULL;
	if (swapchain->present_mode == VK_PRESENT_MODE_MAX_ENUM_KHR) {
		printf("Failed to find a suitable present mode.\n");
		free_swapchain(swapchain, device);
		return 1;
	}
	// Figure out what resolution to use
	int window_width = 0, window_height = 0;
	glfwGetFramebufferSize(window, &window_width, &window_height);
	VkExtent2D window_size = { (uint32_t) window_width, (uint32_t) window_height };
	VkSurfaceCapabilitiesKHR surface_caps;
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical_device, swapchain->surface, &surface_caps)) {
		printf("Failed to retrieve surface capabilities.\n");
		free_swapchain(swapchain, device);
		return 1;
	}
	swapchain->extent.width = (surface_caps.currentExtent.width == 0xffffffff) ? window_size.width : surface_caps.currentExtent.width;
	swapchain->extent.height = (surface_caps.currentExtent.height == 0xffffffff) ? window_size.height : surface_caps.currentExtent.height;
	if (swapchain->extent.width == 0 || swapchain->extent.height == 0) {
		free_swapchain(swapchain, device);
		return 2;
	}
	// We do not care about the composite alpha mode and just take the last
	// one that is supported (trick from Hacker's delight chapter 2.1)
	VkCompositeAlphaFlagBitsKHR alpha_mode = surface_caps.supportedCompositeAlpha;
	alpha_mode = alpha_mode & -alpha_mode;
	// Figure out the image count
	uint32_t image_count = 2;
	if (image_count < surface_caps.minImageCount)
		image_count = surface_caps.minImageCount;
	if (image_count > surface_caps.maxImageCount && surface_caps.maxImageCount != 0)
		image_count = surface_caps.maxImageCount;
	// Create the swapchain
	VkSwapchainCreateInfoKHR swapchain_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.imageExtent = swapchain->extent,
		.imageFormat = swapchain->format,
		.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
		.compositeAlpha = alpha_mode,
		.surface = swapchain->surface,
		.presentMode = swapchain->present_mode,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &device->queue_family_index,
		.minImageCount = image_count,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
	};
	if (vkCreateSwapchainKHR(device->device, &swapchain_info, NULL, &swapchain->swapchain)) {
		printf("Failed to create a swapchain with resolution %ux%u.\n", swapchain->extent.width, swapchain->extent.height);
		free_swapchain(swapchain, device);
		return 1;
	}
	// Grab the images that have been created for the swapchain
	vkGetSwapchainImagesKHR(device->device, swapchain->swapchain, &swapchain->image_count, NULL);
	swapchain->images = calloc(swapchain->image_count, sizeof(VkImage));
	if (!swapchain->image_count || vkGetSwapchainImagesKHR(device->device, swapchain->swapchain, &swapchain->image_count, swapchain->images)) {
		printf("Failed to retrieve images from the swapchain.\n");
		free_swapchain(swapchain, device);
		return 1;
	}
	// Create image views
	swapchain->views = calloc(swapchain->image_count, sizeof(VkImageView));
	for (uint32_t i = 0; i != swapchain->image_count; ++i) {
		VkImageViewCreateInfo view_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = swapchain->format,
			.image = swapchain->images[i],
			.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1, .levelCount = 1, },
		};
		if (vkCreateImageView(device->device, &view_info, NULL, &swapchain->views[i])) {
			printf("Failed to retrieve images from the swapchain.\n");
			free_swapchain(swapchain, device);
			return 1;
		}
	}
	printf("The swapchain resolution in pixels is %ux%u.\n", swapchain->extent.width, swapchain->extent.height);
	return 0;
}


void free_swapchain(swapchain_t* swapchain, const device_t* device) {
	if (swapchain->views)
		for (uint32_t i = 0; i != swapchain->image_count; ++i)
			if (swapchain->views[i])
				vkDestroyImageView(device->device, swapchain->views[i], NULL);
	free(swapchain->views);
	free(swapchain->images);
	if (swapchain->swapchain) vkDestroySwapchainKHR(device->device, swapchain->swapchain, NULL);
	if (swapchain->surface) vkDestroySurfaceKHR(device->instance, swapchain->surface, NULL);
	memset(swapchain, 0, sizeof(*swapchain));
}


int find_memory_type(uint32_t* type_index, const device_t* device, uint32_t memory_type_bits, VkMemoryPropertyFlags memory_properties) {
	for (uint32_t i = 0; i != device->memory_properties.memoryTypeCount; ++i) {
		if (memory_type_bits & (1 << i)) {
			if ((device->memory_properties.memoryTypes[i].propertyFlags & memory_properties) == memory_properties) {
				(*type_index) = i;
				return 0;
			}
		}
	}
	return 1;
}


int create_buffers(buffers_t* buffers, const device_t* device, const buffer_request_t* requests, uint32_t request_count, VkMemoryPropertyFlags memory_properties, VkDeviceSize alignment) {
	memset(buffers, 0, sizeof(*buffers));
	// Early out
	if (request_count == 0)
		return 0;
	// Create the individual buffers
	buffers->buffer_count = request_count;
	buffers->buffers = calloc(request_count, sizeof(buffer_t));
	for (uint32_t i = 0; i != request_count; ++i) {
		buffer_t* buffer = &buffers->buffers[i];
		buffer->request = requests[i];
		if (vkCreateBuffer(device->device, &buffer->request.buffer_info, NULL, &buffer->buffer)) {
			printf("Failed to create buffer %u of %u with %lu bytes.\n", i, request_count, buffer->request.buffer_info.size);
			free_buffers(buffers, device);
			return 1;
		}
	}
	// Figure out requirements for a shared memory allocation
	VkDeviceSize offset = 0;
	uint32_t shared_memory_types = 0xffffffff;
	VkMemoryAllocateFlags allocation_flags = 0;
	for (uint32_t i = 0; i != request_count; ++i) {
		buffer_t* buffer = &buffers->buffers[i];
		VkMemoryRequirements memory_requirements;
		vkGetBufferMemoryRequirements(device->device, buffer->buffer, &memory_requirements);
		VkDeviceSize combined_alignment = least_common_multiple((uint32_t) memory_requirements.alignment, (uint32_t) alignment);
		buffer->memory_offset = align_offset(offset, combined_alignment);
		buffer->memory_size = align_offset(memory_requirements.size, combined_alignment);
		shared_memory_types &= memory_requirements.memoryTypeBits;
		offset = buffer->memory_offset + buffer->memory_size;
		if (requests[i].buffer_info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
			allocation_flags |= VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
	}
	buffers->size = offset;
	// Allocate and bind the shared memory allocation
	VkMemoryAllocateInfo allocation_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = &(VkMemoryAllocateFlagsInfoKHR) {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR,
			.flags = allocation_flags,
		},
		.allocationSize = buffers->size,
	};
	if (find_memory_type(&allocation_info.memoryTypeIndex, device, shared_memory_types, memory_properties)) {
		printf("Failed to find a suitable memory type for %u buffers.\n", request_count);
		free_buffers(buffers, device);
		return 1;
	}
	if (vkAllocateMemory(device->device, &allocation_info, NULL, &buffers->allocation)) {
		printf("Failed to allocate memory for %u buffers with a combined size of %lu.\n", request_count, buffers->size);
		free_buffers(buffers, device);
		return 1;
	}
	for (uint32_t i = 0; i != request_count; ++i) {
		buffer_t* buffer = &buffers->buffers[i];
		if (vkBindBufferMemory(device->device, buffer->buffer, buffers->allocation, buffer->memory_offset)) {
			printf("Failed to bind a memory allocation to buffer %u of size %lu.\n", i, buffer->memory_size);
			free_buffers(buffers, device);
			return 1;
		}
	}
	// Create buffer views
	for (uint32_t i = 0; i != request_count; ++i) {
		VkBufferViewCreateInfo view_info = buffers->buffers[i].request.view_info;
		if (view_info.sType != VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO)
			continue;
		view_info.buffer = buffers->buffers[i].buffer;
		if (view_info.range == 0) view_info.range = VK_WHOLE_SIZE;
		if (vkCreateBufferView(device->device, &view_info, NULL, &buffers->buffers[i].view)) {
			printf("Failed to create a view onto buffer %u.\n", i);
			free_buffers(buffers, device);
			return 1;
		}
		buffers->buffers[i].request.view_info = view_info;
	}
	return 0;
}


void free_buffers(buffers_t* buffers, const device_t* device) {
	if (buffers->buffers) {
		for (uint32_t i = 0; i != buffers->buffer_count; ++i) {
			if (buffers->buffers[i].buffer)
				vkDestroyBuffer(device->device, buffers->buffers[i].buffer, NULL);
			if (buffers->buffers[i].view)
				vkDestroyBufferView(device->device, buffers->buffers[i].view, NULL);
		}
	}
	free(buffers->buffers);
	if (buffers->allocation) vkFreeMemory(device->device, buffers->allocation, NULL);
	memset(buffers, 0, sizeof(*buffers));
}


void print_image_requests(const image_request_t* requests, uint32_t request_count) {
	printf("A description of the %u requested images follows:\n", request_count);
	for (uint32_t i = 0; i != request_count; ++i) {
		const VkImageCreateInfo* info = &requests[i].image_info;
		printf("%3u: %uD, %4ux%4ux%4u, %u mipmaps, %u layers, %u samples\n", i, ((uint32_t) info->imageType) + 1, info->extent.width, info->extent.height, info->extent.depth, info->mipLevels, info->arrayLayers, info->samples);
	}
}


int create_images(images_t* images, const device_t* device, const image_request_t* requests, uint32_t request_count, VkMemoryPropertyFlags memory_properties) {
	memset(images, 0, sizeof(*images));
	// Early out for zero images
	if (request_count == 0)
		return 0;
	// Create the images
	images->image_count = request_count;
	images->images = calloc(request_count, sizeof(image_t));
	for (uint32_t i = 0; i != request_count; ++i) {
		image_t* image = &images->images[i];
		image->request = requests[i];
		if (vkCreateImage(device->device, &image->request.image_info, NULL, &image->image)) {
			printf("Failed to create image %u.\n", i);
			print_image_requests(requests, request_count);
			free_images(images, device);
			return 1;
		}
	}
	// Figure out which images prefer dedicated allocations
	for (uint32_t i = 0; i != request_count; ++i) {
		VkImageMemoryRequirementsInfo2 requirements_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
			.image = images->images[i].image,
		};
		VkMemoryDedicatedRequirements dedicated_requirements = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS,
		};
		VkMemoryRequirements2 requirements = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
			.pNext = &dedicated_requirements,
		};
		vkGetImageMemoryRequirements2(device->device, &requirements_info, &requirements);
		if (dedicated_requirements.prefersDedicatedAllocation) {
			images->images[i].uses_dedicated_allocation = VK_TRUE;
			images->images[i].allocation_index = images->allocation_count;
			++images->allocation_count;
		}
	}
	// All others share a single allocation
	uint32_t shared_index = images->allocation_count;
	if (images->allocation_count < images->image_count) {
		for (uint32_t i = 0; i != request_count; ++i)
			if (!images->images[i].uses_dedicated_allocation)
				images->images[i].allocation_index = shared_index;
		++images->allocation_count;
	}
	images->allocations = calloc(images->allocation_count, sizeof(VkDeviceMemory));
	// Allocate and bind the dedicated memory allocations
	for (uint32_t i = 0; i != images->image_count; ++i) {
		if (!images->images[i].uses_dedicated_allocation)
			continue;
		image_t* image = &images->images[i];
		VkMemoryRequirements memory_requirements;
		vkGetImageMemoryRequirements(device->device, image->image, &memory_requirements);
		VkMemoryAllocateInfo allocation_info = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memory_requirements.size,
		};
		if (find_memory_type(&allocation_info.memoryTypeIndex, device, memory_requirements.memoryTypeBits, memory_properties)) {
			printf("Failed to find a suitable memory type for image %u. ", i);
			print_image_requests(requests, request_count);
			free_images(images, device);
			return 1;
		}
		if (vkAllocateMemory(device->device, &allocation_info, NULL, &images->allocations[image->allocation_index])) {
			printf("Failed to allocate dedicated memory for image %u. ", i);
			print_image_requests(requests, request_count);
			free_images(images, device);
			return 1;
		}
		if (vkBindImageMemory(device->device, image->image, images->allocations[image->allocation_index], image->memory_offset)) {
			printf("Failed to bind a dedicated memory allocation to image %u. ", i);
			print_image_requests(requests, request_count);
			free_images(images, device);
			return 1;
		}
		image->memory_size = allocation_info.allocationSize;
	}
	// Figure out requirements for a shared memory allocation
	VkDeviceSize offset = 0;
	uint32_t shared_memory_types = 0xffffffff;
	for (uint32_t i = 0; i != images->image_count; ++i) {
		if (images->images[i].uses_dedicated_allocation)
			continue;
		image_t* image = &images->images[i];
		VkMemoryRequirements memory_requirements;
		vkGetImageMemoryRequirements(device->device, image->image, &memory_requirements);
		image->memory_offset = align_offset(offset, memory_requirements.alignment);
		image->memory_size = align_offset(memory_requirements.size, memory_requirements.alignment);
		shared_memory_types &= memory_requirements.memoryTypeBits;
		offset = image->memory_offset + image->memory_size;
	}
	// Allocate and bind the shared memory allocation
	if (shared_index < images->allocation_count) {
		VkMemoryAllocateInfo allocation_info = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = offset,
		};
		if (find_memory_type(&allocation_info.memoryTypeIndex, device, shared_memory_types, memory_properties)) {
			printf("Failed to find a suitable memory type for images without a dedicated allocation. ");
			print_image_requests(requests, request_count);
			free_images(images, device);
			return 1;
		}
		if (vkAllocateMemory(device->device, &allocation_info, NULL, &images->allocations[shared_index])) {
			printf("Failed to allocate memory for images without a dedicated allocation. ");
			print_image_requests(requests, request_count);
			free_images(images, device);
			return 1;
		}
		for (uint32_t i = 0; i != images->image_count; ++i) {
			if (images->images[i].uses_dedicated_allocation)
				continue;
			image_t* image = &images->images[i];
			if (vkBindImageMemory(device->device, image->image, images->allocations[image->allocation_index], image->memory_offset)) {
				printf("Failed to bind a memory allocation to image %u. ", i);
				print_image_requests(requests, request_count);
				free_images(images, device);
				return 1;
			}
		}
	}
	// Create views as requested
	for (uint32_t i = 0; i != images->image_count; ++i) {
		image_t* image = &images->images[i];
		if (image->request.view_info.sType != VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO)
			continue;
		image->request.view_info.image = images->images[i].image;
		if (image->request.view_info.format == 0)
			image->request.view_info.format = image->request.image_info.format;
		if (image->request.view_info.subresourceRange.layerCount == 0)
			image->request.view_info.subresourceRange.layerCount = image->request.image_info.arrayLayers - image->request.view_info.subresourceRange.baseArrayLayer;
		if (image->request.view_info.subresourceRange.levelCount == 0)
			image->request.view_info.subresourceRange.levelCount = image->request.image_info.mipLevels - image->request.view_info.subresourceRange.baseMipLevel;
		if (vkCreateImageView(device->device, &image->request.view_info, NULL, &image->view)) {
			printf("Failed to create a view for image %u.\n", i);
			print_image_requests(requests, request_count);
			free_images(images, device);
			return 1;
		}
	}
	return 0;
}


void free_images(images_t* images, const device_t* device) {
	for (uint32_t i = 0; i != images->image_count; ++i) {
		if (images->images[i].view) vkDestroyImageView(device->device, images->images[i].view, NULL);
		if (images->images[i].image) vkDestroyImage(device->device, images->images[i].image, NULL);
	}
	for (uint32_t i = 0; i != images->allocation_count; ++i)
		if (images->allocations[i]) vkFreeMemory(device->device, images->allocations[i], NULL);
	free(images->images);
	free(images->allocations);
	memset(images, 0, sizeof(*images));
}


int transition_image_layouts(images_t* images, const device_t* device, const VkImageLayout* old_layouts, const VkImageLayout* new_layouts, const VkImageSubresourceRange* ranges) {
	if (images->image_count == 0)
		return 0;
	// Prepare the barriers
	VkImageMemoryBarrier* barriers = calloc(images->image_count, sizeof(VkImageMemoryBarrier));
	uint32_t barrier_count = 0;
	for (uint32_t i = 0; i != images->image_count; ++i)
		if (new_layouts[i] != VK_IMAGE_LAYOUT_UNDEFINED)
			barriers[barrier_count++] = (VkImageMemoryBarrier) {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.oldLayout = (old_layouts != NULL) ? old_layouts[i] : images->images[i].request.image_info.initialLayout,
				.newLayout = new_layouts[i],
				.image = images->images[i].image,
				.subresourceRange = (ranges != NULL) ? ranges[i] : (VkImageSubresourceRange) {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.layerCount = images->images[i].request.image_info.arrayLayers,
					.levelCount = images->images[i].request.image_info.mipLevels,
				},
			};
	// If there is no work ot be done, we are done
	if (!barrier_count) {
		free(barriers);
		return 0;
	}
	// Transition targets that are read from shaders to an appropriate layout
	VkCommandBufferAllocateInfo cmd_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandPool = device->cmd_pool,
		.commandBufferCount = 1,
	};
	VkCommandBuffer cmd = NULL;
	if (vkAllocateCommandBuffers(device->device, &cmd_info, &cmd)) {
		printf("Failed to create a command buffer for layout transitions.\n");
		free(barriers);
		return 1;
	}
	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	if (vkBeginCommandBuffer(cmd, &begin_info)) {
		printf("Failed to begin recording commands for layout transitions.\n");
		free(barriers);
		vkFreeCommandBuffers(device->device, device->cmd_pool, 1, &cmd);
		return 1;
	}
	// Use barriers to transition all images to appropriate layouts for the
	// following operations
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, barrier_count, barriers);
	// Submit the commands and wait for it to finish
	vkEndCommandBuffer(cmd);
	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pCommandBuffers = &cmd,
		.commandBufferCount = 1,
	};
	VkResult result = vkQueueSubmit(device->queue, 1, &submit_info, NULL) | vkQueueWaitIdle(device->queue);
	vkFreeCommandBuffers(device->device, device->cmd_pool, 1, &cmd);
	free(barriers);
	if (result) {
		printf("Failed to submit or execute commands for image layout transitions.\n");
		return 1;
	}
	return 0;
}


int copy_buffers_or_images(const device_t* device, const copy_request_t* requests, uint32_t request_count) {
	// Early out
	if (request_count == 0)
		return 0;
	// Create a command buffer that will be used exactly once to perform these
	// copies
	VkCommandBufferAllocateInfo cmd_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandPool = device->cmd_pool,
		.commandBufferCount = 1,
	};
	VkCommandBuffer cmd;
	if (vkAllocateCommandBuffers(device->device, &cmd_info, &cmd)) {
		printf("Failed to create a command buffer for copying.\n");
		return 1;
	}
	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	if (vkBeginCommandBuffer(cmd, &begin_info)) {
		printf("Failed to begin recording commands for copying.\n");
		vkFreeCommandBuffers(device->device, device->cmd_pool, 1, &cmd);
		return 1;
	}
	// Use barriers to transition all images to appropriate layouts for the
	// following operations
	VkImageMemoryBarrier* barriers = calloc(2 * request_count, sizeof(VkImageMemoryBarrier));
	uint32_t barrier_count = 0;
	for (uint32_t i = 0; i != request_count; ++i) {
		const copy_request_t* req = &requests[i];
		switch (req->type) {
			case copy_type_buffer_to_image: {
				VkImageMemoryBarrier barrier = {
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
					.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
					.oldLayout = req->u.buffer_to_image.dst_old_layout,
					.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.image = req->u.buffer_to_image.dst,
					.subresourceRange = image_subresource_layers_to_range(&req->u.buffer_to_image.copy.imageSubresource),
				};
				barriers[barrier_count++] = barrier;
				break;
			}
			case copy_type_image_to_image: {
				if (req->u.image_to_image.src_layout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && req->u.image_to_image.src_layout != VK_IMAGE_LAYOUT_GENERAL) {
					VkImageMemoryBarrier barrier = {
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
						.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
						.oldLayout = req->u.image_to_image.src_layout,
						.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						.image = req->u.image_to_image.src,
						.subresourceRange = image_subresource_layers_to_range(&req->u.image_to_image.copy.srcSubresource),
					};
					barriers[barrier_count++] = barrier;
				}
				VkImageMemoryBarrier barrier = {
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
					.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
					.oldLayout = req->u.image_to_image.dst_old_layout,
					.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.image = req->u.image_to_image.dst,
					.subresourceRange = image_subresource_layers_to_range(&req->u.image_to_image.copy.dstSubresource),
				};
				barriers[barrier_count++] = barrier;
				break;
			}
			default:
				break;
		}
	}
	if (barrier_count)
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, barrier_count, barriers);
	// Perform the copies
	for (uint32_t i = 0; i != request_count; ++i) {
		const copy_request_t* req = &requests[i];
		switch (req->type) {
			case copy_type_buffer_to_buffer:
				vkCmdCopyBuffer(cmd, req->u.buffer_to_buffer.src, req->u.buffer_to_buffer.dst, 1, &req->u.buffer_to_buffer.copy);
				break;
			case copy_type_buffer_to_image:
				vkCmdCopyBufferToImage(cmd, req->u.buffer_to_image.src, req->u.buffer_to_image.dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &req->u.buffer_to_image.copy);
				break;
			case copy_type_image_to_image: {
				VkImageLayout src_layout = (req->u.image_to_image.src_layout == VK_IMAGE_LAYOUT_GENERAL) ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				vkCmdCopyImage(cmd, req->u.image_to_image.src, src_layout, req->u.image_to_image.dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &req->u.image_to_image.copy);
				break;
			}
			default:
				break;
		}
	}
	// Use barriers to bring source images back to their original layouts and
	// destination images to the new layout
	barrier_count = 0;
	for (uint32_t i = 0; i != request_count; ++i) {
		const copy_request_t* req = &requests[i];
		switch (req->type) {
			case copy_type_buffer_to_image: {
				VkImageMemoryBarrier barrier = {
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
					.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.newLayout = req->u.buffer_to_image.dst_new_layout,
					.image = req->u.buffer_to_image.dst,
					.subresourceRange = image_subresource_layers_to_range(&req->u.buffer_to_image.copy.imageSubresource),
				};
				barriers[barrier_count++] = barrier;
				break;
			}
			case copy_type_image_to_image: {
				if (req->u.image_to_image.src_layout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && req->u.image_to_image.src_layout != VK_IMAGE_LAYOUT_GENERAL) {
					VkImageMemoryBarrier barrier = {
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
						.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
						.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						.newLayout = req->u.image_to_image.src_layout,
						.image = req->u.image_to_image.src,
						.subresourceRange = image_subresource_layers_to_range(&req->u.image_to_image.copy.srcSubresource),
					};
					barriers[barrier_count++] = barrier;
				}
				if (req->u.image_to_image.dst_old_layout != VK_IMAGE_LAYOUT_UNDEFINED) {
					VkImageMemoryBarrier barrier = {
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
						.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
						.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						.newLayout = req->u.image_to_image.dst_old_layout,
						.image = req->u.image_to_image.dst,
						.subresourceRange = image_subresource_layers_to_range(&req->u.image_to_image.copy.dstSubresource),
					};
					barriers[barrier_count++] = barrier;
				}
				break;
			}
			default:
				break;
		}
	}
	if (barrier_count)
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, barrier_count, barriers);
	free(barriers);
	barriers = NULL;
	// Submit the commands and wait for it to finish
	vkEndCommandBuffer(cmd);
	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pCommandBuffers = &cmd,
		.commandBufferCount = 1,
	};
	VkResult result = vkQueueSubmit(device->queue, 1, &submit_info, NULL);
	result |= vkQueueWaitIdle(device->queue);
	vkFreeCommandBuffers(device->device, device->cmd_pool, 1, &cmd);
	return result;
}


int copy_buffers(const device_t* device, const copy_buffer_to_buffer_t* requests, uint32_t request_count) {
	copy_request_t* copy_requests = calloc(request_count, sizeof(copy_request_t));
	for (uint32_t i = 0; i != request_count; ++i) {
		copy_requests[i].type = copy_type_buffer_to_buffer;
		copy_requests[i].u.buffer_to_buffer = requests[i];
	}
	int result = copy_buffers_or_images(device, copy_requests, request_count);
	free(copy_requests);
	return result;
}


int copy_buffers_to_images(const device_t* device, const copy_buffer_to_image_t* requests, uint32_t request_count) {
	copy_request_t* copy_requests = calloc(request_count, sizeof(copy_request_t));
	for (uint32_t i = 0; i != request_count; ++i) {
		copy_requests[i].type = copy_type_buffer_to_image;
		copy_requests[i].u.buffer_to_image = requests[i];
	}
	int result = copy_buffers_or_images(device, copy_requests, request_count);
	free(copy_requests);
	return result;
}


int copy_images(const device_t* device, const copy_image_to_image_t* requests, uint32_t request_count) {
	copy_request_t* copy_requests = calloc(request_count, sizeof(copy_request_t));
	for (uint32_t i = 0; i != request_count; ++i) {
		copy_requests[i].type = copy_type_image_to_image;
		copy_requests[i].u.image_to_image = requests[i];
	}
	int result = copy_buffers_or_images(device, copy_requests, request_count);
	free(copy_requests);
	return result;
}


int fill_buffers(const buffers_t* buffers, const device_t* device, write_buffer_t write_buffer, const void* context) {
	// Early out
	if (buffers->buffer_count == 0)
		return 0;
	// Create staging buffers
	buffers_t staging;
	buffer_request_t* requests = calloc(buffers->buffer_count, sizeof(buffer_request_t));
	for (uint32_t i = 0; i != buffers->buffer_count; ++i) {
		requests[i] = buffers->buffers[i].request;
		requests[i].buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		memset(&requests[i].view_info, 0, sizeof(requests[i].view_info));
	}
	int result = create_buffers(&staging, device, requests, buffers->buffer_count, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 16);
	free(requests);
	requests = NULL;
	if (result) {
		printf("Failed to create staging buffers.\n");
		free(requests);
		return 1;
	}
	// Map their memory
	void* staged_data = NULL;
	if (vkMapMemory(device->device, staging.allocation, 0, staging.size, 0, &staged_data)) {
		printf("Failed to map the memory of staging buffers.\n");
		free_buffers(&staging, device);
		return 1;
	}
	// Fill the staging buffers with data
	for (uint32_t i = 0; i != buffers->buffer_count; ++i) {
		const buffer_t* buffer = &staging.buffers[i];
		(*write_buffer)((void*) ((uint8_t*) staged_data + buffer->memory_offset), i, buffer->request.buffer_info.size, context);
	}
	// Unmap memory
	vkUnmapMemory(device->device, staging.allocation);
	// Copy the staging buffers to the device-local buffers
	copy_buffer_to_buffer_t* copy_requests = calloc(buffers->buffer_count, sizeof(copy_buffer_to_buffer_t));
	for (uint32_t i = 0; i != buffers->buffer_count; ++i) {
		copy_buffer_to_buffer_t copy_request = {
			.src = staging.buffers[i].buffer,
			.dst = buffers->buffers[i].buffer,
			.copy = {
				.size = buffers->buffers[i].request.buffer_info.size,
			},
		};
		copy_requests[i] = copy_request;
	}
	result = copy_buffers(device, copy_requests, buffers->buffer_count);
	free(copy_requests);
	copy_requests = NULL;
	if (result) {
		printf("Failed to copy staging buffers to device-local buffers.\n");
		free_buffers(&staging, device);
		return 1;
	}
	// Tidy up
	free_buffers(&staging, device);
	return 0;
}


int fill_images(const images_t* images, const device_t* device, write_image_subresource_t write_subresource, VkImageLayout old_layout, VkImageLayout new_layout, const void* context) {
	// Early out
	if (images->image_count == 0)
		return 0;
	// Count subresources
	uint32_t subresource_count = 0;
	for (uint32_t i = 0; i != images->image_count; ++i)
		subresource_count += images->images[i].request.image_info.mipLevels * images->images[i].request.image_info.arrayLayers;
	// Create staging buffers of appropriate size
	buffers_t staging;
	buffer_request_t* requests = calloc(subresource_count, sizeof(buffer_request_t));
	VkExtent3D* extents = calloc(subresource_count, sizeof(VkExtent3D));
	uint32_t subresource_index = 0;
	for (uint32_t i = 0; i != images->image_count; ++i) {
		VkImageCreateInfo* image_info = &images->images[i].request.image_info;
		format_description_t format = get_format_description(image_info->format);
		for (uint32_t mip_level = 0; mip_level != image_info->mipLevels; ++mip_level) {
			for (uint32_t layer = 0; layer != image_info->arrayLayers; ++layer) {
				// Correct according to: https://registry.khronos.org/vulkan/specs/1.3-khr-extensions/html/chap12.html#resources-image-mip-level-sizing
				VkExtent3D extent = {
					image_info->extent.width >> mip_level,
					image_info->extent.height >> mip_level,
					image_info->extent.depth >> mip_level,
				};
				if (image_info->imageType == VK_IMAGE_TYPE_1D)
					extent.height = extent.depth = 1;
				else if (image_info->imageType == VK_IMAGE_TYPE_2D)
					extent.depth = 1;
				extents[subresource_index] = extent;
				uint32_t texel_count = extent.width * extent.height * extent.depth;
				VkBufferCreateInfo request = {
					.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
					.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					.size = (texel_count / format.texels_per_block) * format.block_size,
				};
				requests[subresource_index++].buffer_info = request;
			}
		}
	}
	if (create_buffers(&staging, device, requests, subresource_count, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 16)) {
		printf("Failed to create staging buffers for images.\n");
		free(requests);
		free(extents);
		return 1;
	}
	free(requests);
	requests = NULL;
	// Map their memory
	void* staged_data = NULL;
	if (vkMapMemory(device->device, staging.allocation, 0, staging.size, 0, &staged_data)) {
		printf("Failed to map the memory of staging buffers for images.\n");
		free_buffers(&staging, device);
		free(extents);
		return 1;
	}
	// Fill the staging buffers with data
	subresource_index = 0;
	for (uint32_t i = 0; i != images->image_count; ++i) {
		VkImageCreateInfo* image_info = &images->images[i].request.image_info;
		for (uint32_t mip_level = 0; mip_level != image_info->mipLevels; ++mip_level) {
			for (uint32_t layer = 0; layer != image_info->arrayLayers; ++layer) {
				VkExtent3D extent = extents[subresource_index];
				const buffer_t* buffer = &staging.buffers[subresource_index++];
				VkImageSubresource subresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = mip_level,
					.arrayLayer = layer,
				};
				(*write_subresource)((uint8_t*) staged_data + buffer->memory_offset, i, &subresource, buffer->request.buffer_info.size, image_info, &extent, context);
			}
		}
	}
	// Unmap memory
	vkUnmapMemory(device->device, staging.allocation);
	// Copy the staging buffers to the device-local images
	copy_buffer_to_image_t* copy_requests = calloc(subresource_count, sizeof(copy_buffer_to_image_t));
	subresource_index = 0;
	for (uint32_t i = 0; i != images->image_count; ++i) {
		VkImageCreateInfo* image_info = &images->images[i].request.image_info;
		for (uint32_t mip_level = 0; mip_level != image_info->mipLevels; ++mip_level) {
			for (uint32_t layer = 0; layer != image_info->arrayLayers; ++layer) {
				VkExtent3D extent = extents[subresource_index];
				const buffer_t* buffer = &staging.buffers[subresource_index];
				copy_buffer_to_image_t copy_request = {
					.src = buffer->buffer,
					.dst = images->images[i].image,
					.dst_old_layout = old_layout,
					.dst_new_layout = new_layout,
					.copy = {
						.imageExtent = extent,
						.imageSubresource = {
							.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
							.mipLevel = mip_level,
							.baseArrayLayer = layer,
							.layerCount = 1,
						},
					},
				};
				copy_requests[subresource_index] = copy_request;
				++subresource_index;
			}
		}
	}
	free(extents);
	extents = NULL;
	if (copy_buffers_to_images(device, copy_requests, subresource_count)) {
		printf("Failed to copy staging buffers to device-local images.\n");
		free_buffers(&staging, device);
		free(copy_requests);
		return 1;
	}
	// Tidy up
	free_buffers(&staging, device);
	free(copy_requests);
	return 0;
}


const char* get_shader_stage_name(VkShaderStageFlags stage) {
	switch (stage) {
		case VK_SHADER_STAGE_VERTEX_BIT:
			return "vert";
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
			return "tesc";
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
			return "tese";
		case VK_SHADER_STAGE_GEOMETRY_BIT:
			return "geom";
		case VK_SHADER_STAGE_FRAGMENT_BIT:
			return "frag";
		case VK_SHADER_STAGE_COMPUTE_BIT:
			return "comp";
		case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
			return "rgen";
		case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
			return "rahit";
		case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
			return "rchit";
		case VK_SHADER_STAGE_MISS_BIT_KHR:
			return "rmiss";
		case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
			return "rint";
		case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
			return "rcall";
		case VK_SHADER_STAGE_TASK_BIT_EXT:
			return "task";
		case VK_SHADER_STAGE_MESH_BIT_EXT:
			return "mesh";
		default:
			return "";
	}
}


int compile_shader(const shader_compilation_request_t* request) {
	// Check whether command processing is available at all. Commented out,
	// because it breaks Nsight Graphics.
	//if (!system(NULL)) {
	//	printf("Cannot compile the shader at %s because command processing is not available.\n", request->shader_path);
	//	return 1;
	//}
	// Figure out the output path
	const char* default_spirv_path_segments[] = { request->shader_path, ".spv" };
	char* default_spirv_path = cat_strings(default_spirv_path_segments, COUNT_OF(default_spirv_path_segments));
	char* spirv_path = request->spirv_path ? request->spirv_path : default_spirv_path;
	// Concatenate all the defines
	uint32_t piece_count = 2 * request->define_count;
	const char** define_pieces = calloc(piece_count, sizeof(const char*));
	for (uint32_t i = 0; i != request->define_count; ++i) {
		define_pieces[2 * i + 0] = " -D";
		define_pieces[2 * i + 1] = request->defines[i];
	}
	char* defines = cat_strings(define_pieces, piece_count);
	// Assemble a command for glslangValidator
	const char* args = request->args ? request->args : "";
	const char* cmd_pieces[] = {
		"glslangValidator --target-env spirv1.6 -V100 ",
#ifndef NDEBUG
		"-g -Od ",
#endif
		"-S ", get_shader_stage_name(request->stage), " ",
		"-e ", request->entry_point, " ",
		args, " ",
		defines, " ",
		"-o \"", spirv_path, "\" \"",
		request->shader_path, "\"",
	};
	char* cmd = cat_strings(cmd_pieces, COUNT_OF(cmd_pieces));
	int result = system(cmd);
	if (result)
		printf("Failed to compile the shader at %s. The full command line is:\n%s\n", request->shader_path, cmd);
	free(cmd);
	free(defines);
	free(default_spirv_path);
	free(define_pieces);
	return result;
}


int compile_shader_with_retry(const shader_compilation_request_t* request) {
	while (compile_shader(request)) {
		printf("Try again (Y/n)? ");
		char answer = '\0';
		scanf("%1c", &answer);
		if (answer == 'N' || answer == 'n') {
			printf("Giving up.\n");
			return 1;
		}
	}
	return 0;
}


int compile_and_create_shader_module(VkShaderModule* module, const device_t* device, const shader_compilation_request_t* request, bool retry) {
	if (retry) {
		if (compile_shader_with_retry(request))
			return 1;
	}
	else {
		if (compile_shader(request))
			return 1;
	}
	// Load the SPIR-V file
	const char* default_spirv_path_segments[] = { request->shader_path, ".spv" };
	char* default_spirv_path = cat_strings(default_spirv_path_segments, COUNT_OF(default_spirv_path_segments));
	char* spirv_path = request->spirv_path ? request->spirv_path : default_spirv_path;
	FILE* file = fopen(spirv_path, "rb");
	if (!file) {
		printf("Failed to open the compiled shader at %s.\n", spirv_path);
		return 1;
	}
	size_t spirv_size = 0;
	if (fseek(file, 0, SEEK_END) || (spirv_size = ftell(file)) <= 0) {
		printf("Failed to determine the file size of the compiled shader at %s.\n", spirv_path);
		free(default_spirv_path);
		fclose(file);
		return 1;
	}
	fseek(file, 0, SEEK_SET);
	uint32_t* spirv = malloc(spirv_size + sizeof(uint32_t));
	spirv_size = fread(spirv, sizeof(uint8_t), spirv_size, file);
	fclose(file);
	// Create the shader module
	VkShaderModuleCreateInfo module_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pCode = spirv,
		.codeSize = spirv_size,
	};
	if (vkCreateShaderModule(device->device, &module_info, NULL, module)) {
		printf("Failed to create a shader module from the compiled shader at %s.\n", spirv_path);
		free(default_spirv_path);
		free(spirv);
		return 1;
	}
	free(default_spirv_path);
	free(spirv);
	return 0;
}


void complete_descriptor_set_layout_bindings(VkDescriptorSetLayoutBinding* bindings, uint32_t binding_count, uint32_t min_descriptor_count, VkShaderStageFlags shared_stages) {
	for (uint32_t i = 0; i != binding_count; ++i) {
		if (bindings[i].descriptorCount < min_descriptor_count)
			bindings[i].descriptorCount = min_descriptor_count;
		bindings[i].stageFlags |= shared_stages;
	}	
}


void complete_descriptor_set_writes(VkWriteDescriptorSet* writes, uint32_t write_count, const VkDescriptorSetLayoutBinding* bindings, uint32_t binding_count, VkDescriptorSet destination_set) {
	// Create an array of bindings where the binding index is the array index
	uint32_t max_binding = 0;
	for (uint32_t i = 0; i != binding_count; ++i)
		if (max_binding < bindings[i].binding)
			max_binding = bindings[i].binding;
	VkDescriptorSetLayoutBinding* sorted_bindings = malloc((max_binding + 1) * sizeof(VkDescriptorSetLayoutBinding));
	memset(sorted_bindings, 0xff, (max_binding + 1) * sizeof(VkDescriptorSetLayoutBinding));
	for (uint32_t i = 0; i != binding_count; ++i)
		sorted_bindings[bindings[i].binding] = bindings[i];
	// Complete the writes
	for (uint32_t i = 0; i != write_count; ++i) {
		writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		if (destination_set)
			writes[i].dstSet = destination_set;
		const VkDescriptorSetLayoutBinding* binding = &sorted_bindings[writes[i].dstBinding];
		if (binding->binding == i) {
			writes[i].descriptorType = binding->descriptorType;
			writes[i].descriptorCount = binding->descriptorCount;
		}
	}
	free(sorted_bindings);
}


int create_descriptor_sets(descriptor_sets_t* descriptor_sets, const device_t* device, VkDescriptorSetLayoutBinding* bindings, uint32_t binding_count, uint32_t descriptor_set_count) {
	memset(descriptor_sets, 0, sizeof(*descriptor_sets));
	VkDescriptorSetLayoutCreateInfo set_layout_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pBindings = bindings,
		.bindingCount = binding_count,
	};
	if (vkCreateDescriptorSetLayout(device->device, &set_layout_info, NULL, &descriptor_sets->descriptor_set_layout)) {
		printf("Failed to create a descriptor set layout.\n");
		free_descriptor_sets(descriptor_sets, device);
		return 1;
	}
	VkPipelineLayoutCreateInfo pipeline_layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pSetLayouts = &descriptor_sets->descriptor_set_layout,
		.setLayoutCount = 1,
	};
	if (vkCreatePipelineLayout(device->device, &pipeline_layout_info, NULL, &descriptor_sets->pipeline_layout)) {
		printf("Failed to create a pipeline layout.\n");
		free_descriptor_sets(descriptor_sets, device);
		return 1;
	}
	VkDescriptorPoolSize* pool_sizes = malloc(binding_count * sizeof(VkDescriptorPoolSize));
	for (uint32_t i = 0; i != binding_count; ++i) {
		pool_sizes[i].type = bindings[i].descriptorType;
		pool_sizes[i].descriptorCount = bindings[i].descriptorCount * descriptor_set_count;
	}
	VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = descriptor_set_count,
		.pPoolSizes = pool_sizes,
		.poolSizeCount = binding_count,
	};
	if (vkCreateDescriptorPool(device->device, &pool_info, NULL, &descriptor_sets->descriptor_pool)) {
		printf("Failed to create a descriptor pool.\n");
		free_descriptor_sets(descriptor_sets, device);
		free(pool_sizes);
		return 1;
	}
	free(pool_sizes);
	pool_sizes = NULL;
	descriptor_sets->descriptor_set_count = descriptor_set_count;
	descriptor_sets->descriptor_sets = calloc(descriptor_set_count, sizeof(VkDescriptorSet));
	VkDescriptorSetLayout* layouts = malloc(descriptor_set_count * sizeof(VkDescriptorSetLayout));
	for (uint32_t i = 0; i != descriptor_set_count; ++i)
		layouts[i] = descriptor_sets->descriptor_set_layout;
	VkDescriptorSetAllocateInfo set_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorSetCount = descriptor_set_count,
		.descriptorPool = descriptor_sets->descriptor_pool,
		.pSetLayouts = layouts,
	};
	if (vkAllocateDescriptorSets(device->device, &set_info, descriptor_sets->descriptor_sets)) {
		free_descriptor_sets(descriptor_sets, device);
		free(layouts);
		return 1;
	}
	free(layouts);
	return 0;
}


void free_descriptor_sets(descriptor_sets_t* descriptor_sets, const device_t* device) {
	if (descriptor_sets->descriptor_pool)
		vkDestroyDescriptorPool(device->device, descriptor_sets->descriptor_pool, NULL);
	if (descriptor_sets->pipeline_layout)
		vkDestroyPipelineLayout(device->device, descriptor_sets->pipeline_layout, NULL);
	if (descriptor_sets->descriptor_set_layout)
		vkDestroyDescriptorSetLayout(device->device, descriptor_sets->descriptor_set_layout, NULL);
	free(descriptor_sets->descriptor_sets);
	memset(descriptor_sets, 0, sizeof(*descriptor_sets));
}
