#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdint.h>
#include <stdbool.h>


//! Uses device_t* device to load the Vulkan function with the given name. The
//! identifier for the function pointer is the name prefixed with p.
#define VK_LOAD(FUNCTION_NAME) PFN_##FUNCTION_NAME p##FUNCTION_NAME = (PFN_##FUNCTION_NAME) glfwGetInstanceProcAddress(device->instance, #FUNCTION_NAME);


//! Gathers Vulkan objects created up to device creation including meta data
typedef struct {
	//! The instance used to invoke Vulkan functions
	VkInstance instance;
	//! All physical devices enumerated by vkEnumeratePhysicalDevices()
	VkPhysicalDevice* physical_devices;
	//! The number of entries and the used index in physical_devices
	uint32_t physical_device_count, physical_device_index;
	//! The physical device that is being used
	VkPhysicalDevice physical_device;
	//! Properties of physical_device
	VkPhysicalDeviceProperties physical_device_properties;
	//! Properties of physical_device regarding Vulkan 1.1 functionality
	VkPhysicalDeviceVulkan11Properties physical_device_properties_11;
	//! Memory types and heaps available through physical_device
	VkPhysicalDeviceMemoryProperties memory_properties;
	//! The Vulkan device created using physical_device
	VkDevice device;
	//! A queue that supports graphics and compute
	VkQueue queue;
	//! All queue family properties for all possible queues as enumerated by
	//! vkGetPhysicalDeviceQueueFamilyProperties()
	VkQueueFamilyProperties* queue_family_properties;
	//! The number of entries and the used index in queue_family_properties
	uint32_t queue_family_count, queue_family_index;
	//! A command pool for the device and queue above
	VkCommandPool cmd_pool;
} device_t;


//! Possible return values for create_swapchain()
typedef enum {
	//! The swapchain has been created successfully
	swapchain_result_success = 0,
	//! Creation of the swapchain has failed and there is no apparent way to
	//! recover from this failure
	swapchain_result_failure = 1,
	//! Swapchain creation has failed because the window reported a resolution
	//! with zero pixels. Presumably, it is minimized and once that changes,
	//! swapchain creation may succeed again.
	swapchain_result_minimized = 0x76543210,
} swapchain_result_t;


//! A swapchain along with corresponding images to be swapped
typedef struct {
	//! A Vulkan surface for the window associated with the swapchain
	VkSurfaceKHR surface;
	//! The present mode that is being used for the swapchain
	VkPresentModeKHR present_mode;	
	//! The swapchain itself
	VkSwapchainKHR swapchain;
	//! The format of all held images
	VkFormat format;
	//! The extent of all held images in pixels (i.e. the resolution on screen
	//! and the size of the client area of the window)
	VkExtent2D extent;
	//! The number of array entries in images and views
	uint32_t image_count;
	//! The images that are swapped by this swapchain
	VkImage* images;
	//! Views onto the images
	VkImageView* views;
} swapchain_t;


//! Arrays of buffer requests are to be passed to create_buffers(). They
//! combine a buffer create info with a view create info.
typedef struct {
	//! Complete creation info for the buffer
	VkBufferCreateInfo buffer_info;
	/*! Incpomplete creation info for a view onto the buffer. create_buffers()
		will use an appropriate value for buffer. If range is 0, VK_WHOLE_SIZE
		is used. If this is left zero-initialized, it indicates that no view
		should be created.*/
	VkBufferViewCreateInfo view_info;
} buffer_request_t;


//! A single buffer handled by buffers_t. It keeps track of everything that
//! buffers_t has to keep track of per buffer.
typedef struct {
	//! A copy of the used buffer request with missing fields filled in
	buffer_request_t request;
	//! The buffer itself
	VkBuffer buffer;
	//! A view onto the buffer as requested or NULL if none was requested
	VkBufferView view;
	//! The offset in bytes from the start of the memory allocation to this
	//! buffer
	VkDeviceSize memory_offset;
	//! The size in bytes of the memory block for this buffer in its
	//! allocation. At least as large as the buffer itself, often bigger.
	VkDeviceSize memory_size;
} buffer_t;


//! Handles a list of buffers that all share one memory allocation
typedef struct {
	//! The number of held buffers
	uint32_t buffer_count;
	//! An array of all held buffers
	buffer_t* buffers;
	//! The memory allocation used to serve all buffers
	VkDeviceMemory allocation;
	//! The total size of the memory allocation used for all buffers
	VkDeviceSize size;
} buffers_t;


//! Arrays of image requests are to be passed to create_images(). They combine
//! an image create info with a view create info.
typedef struct {
	//! Complete creation info for the image
	VkImageCreateInfo image_info;
	/*! Incpomplete creation info for a view onto the image. create_images()
		will use an appropriate value for image. If format,
		subresourceRange.layerCount or subresourceRange.levelCount are zero,
		create_images() uses values that match the image and use the whole
		range. If this is left zero-initialized, it indicates that no view
		should be created.*/
	VkImageViewCreateInfo view_info;
} image_request_t;


//! A single image handled by images_t. It keeps track of everything that
//! images_t has to keep track of per image.
typedef struct {
	//! A copy of the request with a few incomplete entries filled in
	image_request_t request;
	//! The image itself
	VkImage image;
	//! A view onto the image or NULL if none was requested
	VkImageView view;
	//! The index of the memory allocation in images_t that serves this image
	uint32_t allocation_index;
	//! Whether the allocation for this image is only used by this image
	//! because it has indicated a preference for dedicated allocations
	bool uses_dedicated_allocation;
	//! The offset in bytes from the start of the memory allocation to this
	//! image
	VkDeviceSize memory_offset;
	//! The size in bytes of the memory block for this image in its allocation.
	//! At least as large as the image data itself, often bigger.
	VkDeviceSize memory_size;
} image_t;


//! Handles a list of images, maybe with views, along with enough memory
//! allocations to serve all of them
typedef struct {
	//! The number of held images
	uint32_t image_count;
	//! An array of all held images
	image_t* images;
	//! The number of memory allocations used to serve all images
	uint32_t allocation_count;
	//! The memory allocations used to serve all images
	VkDeviceMemory* allocations;
} images_t;


//! Different types of copies between buffers and images
typedef enum {
	//! \see copy_buffer_to_buffer_t
	copy_type_buffer_to_buffer,
	//! \see copy_buffer_to_image_t
	copy_type_buffer_to_image,
	//! \see copy_image_to_image_t
	copy_type_image_to_image,
} copy_type_t;


//! Specification of how a memory region within one buffer should be copied to
//! another buffer
typedef struct {
	//! The source and destination buffers (with appropriate transfer usage
	//! flags)
	VkBuffer src, dst;
	//! Specification of the memory region to be copied within these buffers
	VkBufferCopy copy;
} copy_buffer_to_buffer_t;


//! Specification of how data from a buffer should be copied to an image
typedef struct {
	//! The source buffer (with transfer source usage)
	VkBuffer src;
	//! The destination image (with transfer destination usage)
	VkImage dst;
	//! The layout of dst before and after the copy (copying makes a transition)
	VkImageLayout dst_old_layout, dst_new_layout;
	//! Specification of the parts of the buffer and image between which
	//! copying takes place
	VkBufferImageCopy copy;
} copy_buffer_to_image_t;


//! Specification of how part of an image should be copied to another image
typedef struct {
	//! The source and destination image (with appropriate transfer usage
	//! flags)
	VkImage src, dst;
	//! The layout of src at the time of the copy
	VkImageLayout src_layout;
	//! The layout of dst before and after the copy (copying makes a transition)
	VkImageLayout dst_old_layout, dst_new_layout;
	//! Specification of the image regions to be copied within these images
	VkImageCopy copy;
} copy_image_to_image_t;


//! A union holding one of the copy_*_to_*_t types
typedef struct {
	//! Indicates which entry of the union is meaningful
	copy_type_t type;
	union copy_request_u {
		copy_buffer_to_buffer_t buffer_to_buffer;
		copy_buffer_to_image_t buffer_to_image;
		copy_image_to_image_t image_to_image;
	} u;
} copy_request_t;


//! Specifies all the flags and defines that are needed to compile a GLSL
//! or HLSL shader into SPIR-V
typedef struct {
	//! The file path to the source code for the shader
	char* shader_path;
	/*! Defines that are to be used by the preprocessor whilst compiling the
		shader. These are null-terminated strings of the format "IDENTIFIER"
		or "IDENTIFIER=VALUE".*/
	char** defines;
	//! The number of array-entries in defines
	uint32_t define_count;
	//! A single shader stage that is to be used for compilation
	VkShaderStageFlags stage;
	//! The name of the function used as entry point. Usually "main".
	char* entry_point;
	//! Additional arguments to be passed to glslangValidator. The options
	//! -e -g -o -Od -S --target-env -V are used internally. NULL for none.
	char* args;
	//! The file path for the output file containing the SPIR-V code. If this
	//! is NULL, it defaults to file_path with .spv attached.
	char* spirv_path;
} shader_compilation_request_t;


//! Handles multiple descriptor sets with a common layout and associated
//! objects
typedef struct {
	//! The descriptor set layout used by pipeline_layout
	VkDescriptorSetLayout descriptor_set_layout;
	//! A pipeline layout that matches descriptor_set_layout
	VkPipelineLayout pipeline_layout;
	//! The descriptor pool used to allocate descriptor_sets
	VkDescriptorPool descriptor_pool;
	//! An array of descriptor sets with the layout above
	VkDescriptorSet* descriptor_sets;
	//! The number of array entries in descriptor_sets
	uint32_t descriptor_set_count;
} descriptor_sets_t;


/*! Creates a Vulkan device and related objects.
	\param device The output. Clean up with free_device().
	\param app_name An application name that goes into the application info.
	\param physical_device_index The index of the physical device (i.e. GPU) to
		be used. Available GPUs are printed by create_device(). If unsure, pass
		0.
	\return 0 upon success.*/
int create_device(device_t* device, const char* app_name, uint32_t physical_device_index);


void free_device(device_t* device);


/*! Creates a swapchain in the given window.
	\param swapchain The output. Clean up with free_swapchain().
	\param device Output of create_device().
	\param window A window, typically created using glfwCreateWindow(). Make
		sure to invoke glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API) before
		creating it.
	\param use_vsync true to use a present mode that waits for vertical
		synchronization, thus capping the frame rate to the refresh rate of the
		display. false to churn out frames as fast as possible, tolerating
		tearing artifacts in the process. This request may be ignored.
	\return 0 upon success. In general one of the swapchain_result_t codes.
		swapchain_result_minimized deserves special treatment.*/
swapchain_result_t create_swapchain(swapchain_t* swapchain, const device_t* device, GLFWwindow* window, bool use_vsync);


void free_swapchain(swapchain_t* swapchain, const device_t* device);


/*! Finds a Vulkan memory type matching given conditions.
	\param type_index Overwritten by the smallest matching index.
	\param device Memory types of the physical device in this device are
		considered.
	\param memory_type_bits A bit mask as in
		VkMemoryRequirements.memoryTypeBits. The bit for the output type index
		will be set in this mask.
	\param memory_properties Flags that have to be met by the returned memory
		type.
	\return 0 upon success.*/
int find_memory_type(uint32_t* type_index, const device_t* device, uint32_t memory_type_bits, VkMemoryPropertyFlags memory_properties);


//! Returns the smallest integer multiple of alignment that is at least as
//! large as offset
static inline VkDeviceSize align_offset(VkDeviceSize offset, VkDeviceSize alignment) {
	return ((offset + alignment - 1) / alignment) * alignment;
}


/*! Creates a list of buffers along with a single memory allocation serving all
	of them.
	\param buffers The output. Clean up with free_buffers().
	\param device Output of create_device().
	\param requests The complete create infos for each individual buffer to be
		created and optionally for the view onto it.
	\param request_count The number of buffers to be created. Also the number
		of valid array entries in requests.
	\param memory_properties Flags that are to be enforced for the used memory
		allocation.
	\param alignment The offset of each buffer in bytes within the memory
		allocation is guaranteed to be a multiple of this non-zero integer.
		Pass one, if you do not care. Alignment requirements of individual
		buffers are respected in all cases.
	\return 0 upon success.*/
int create_buffers(buffers_t* buffers, const device_t* device, const buffer_request_t* requests, uint32_t request_count, VkMemoryPropertyFlags memory_properties, VkDeviceSize alignment);


void free_buffers(buffers_t* buffers, const device_t* device);


//! Prints all key information about the given image requests to stdout. Useful
//! for error reporting.
void print_image_requests(const image_request_t* requests, uint32_t request_count);


/*! Creates an arbitrary number of images, allocates memory for them and
	optionally creates views.
	\param images The output. Clean up with free_images().
	\param device Output of create_device().
	\param requests A description of every image that is to be created.
	\param request_count Number of array entries in requests.
	\param memory_properties Flags that are to be enforced for the used memory
		allocation.
	\return 0 upon success.*/
int create_images(images_t* images, const device_t* device, const image_request_t* requests, uint32_t request_count, VkMemoryPropertyFlags memory_properties);


void free_images(images_t* images, const device_t* device);


/*! Performs layout transitions for images using image memory barriers.
	\param images The images on which to operate.
	\param device Output of create_device().
	\param old_layouts Array of size images->image_count or NULL if all images
		to transition are still in the layout they had at creation. Indicates
		the current layout for each image.
	\param new_layouts Array of size images->image_count indicating the layouts
		to which the images should be transitioned. Entries of
		VK_IMAGE_LAYOUT_UNDEFINED (which is 0) indicate that no layout
		transition should be performed for this image.
	\param ranges Subresource ranges to be transitioned for each image or NULL
		to use all layers, all levels and the color aspect.
	\return 0 upon success.*/
int transition_image_layouts(images_t* images, const device_t* device, const VkImageLayout* old_layouts, const VkImageLayout* new_layouts, const VkImageSubresourceRange* ranges);


//! This little helper converts VkImageSubresourceLayers to
//! VkImageSubresourceRange in the natural way
static inline VkImageSubresourceRange image_subresource_layers_to_range(const VkImageSubresourceLayers* layers) {
	VkImageSubresourceRange result = {
		.aspectMask = layers->aspectMask,
		.baseArrayLayer = layers->baseArrayLayer,
		.baseMipLevel = layers->mipLevel,
		.levelCount = 1,
		.layerCount = layers->layerCount,
	};
	return result;
}


/*! Performs the requested copies between buffers and images and blocks
	execution until this work is completed.
	\param device Output of create_device().
	\param requests An array of copy requests.
	\param request_count Number of array entries.
	\return 0 upon success. Upon failure the contents and layouts of
		destination buffers/images are undefined.*/
int copy_buffers_or_images(const device_t* device, const copy_request_t* requests, uint32_t request_count);


//! Specialized version of copy_buffers_or_images() to copy buffers to buffers
int copy_buffers(const device_t* device, const copy_buffer_to_buffer_t* requests, uint32_t request_count);


//! Specialized version of copy_buffers_or_images() to copy buffers to images
int copy_buffers_to_images(const device_t* device, const copy_buffer_to_image_t* requests, uint32_t request_count);


//! Specialized version of copy_buffers_or_images() to copy images to images
int copy_images(const device_t* device, const copy_image_to_image_t* requests, uint32_t request_count);


/*! A callback type used for fill_buffers(). It has to fill a range of mapped
	memory for a buffer with the data that is to go into that buffer.
	\param buffer_data The start of the mapped memory range into which the
		buffer data should be written.
	\param buffer_index The index of the buffer for which data is to be written
		within its buffers_t object. Upon success, it is guaranteed that
		fill_buffers() invokes this callback exactly once with each valid value
		of buffer_index in increasing order.
	\param buffer_size The number of bytes that should be written to
		buffer_data.
	\param context Passed through by fill_buffers().*/
typedef void (*write_buffer_t)(void* buffer_data, uint32_t buffer_index, VkDeviceSize buffer_size, const void* context);


/*! Creates a host-local staging buffer for each of the given buffers, maps it
	and lets a callback fill it with arbitrary data. Then these staging buffers
	are copied to the given buffers and destroyed.
	\param buffers Output of create_buffers(). The data stored by these buffers
		will be overwritten.
	\param device Output of create_device().
	\param write_buffer See write_buffer_t.
	\param context Passed to (*write_buffer)() as last parameter. Typically a
		pointer to a user-defined struct providing information needed by the
		callback.
	\param 0 upon success. Upon failure, the contents of the buffers are
		undefined.*/
int fill_buffers(const buffers_t* buffers, const device_t* device, write_buffer_t write_buffer, const void* context);


/*! A callback type used for fill_images(). It has to fill a range of mapped
	memory for a buffer whose data will be copied into a subresource of an
	image with the image data. The buffer should be tightly packed.
	\param image_data The start of the mapped memory range into which the
		image data should be written.
	\param image_index The index of the image for which data is to be written
		within its images_t object.
	\param subresource The subresource of the image for which data is to be
		written (i.e. a mipmap of a level of an aspect of the image). The
		aspectMask is always VK_IMAGE_ASPECT_COLOR_BIT.
	\param buffer_size The number of bytes that should be written to
		image_data.
	\param image_info Info about the image for which data is to be written.
	\param subresource_extent The resolution of the subresource being written.
	\param context Passed through by fill_images().
	\note fill_images will use each valid tuple (image_index,
		subresource.mipLevel, subresource.arrayLayer) exactly once and in
		lexicographic order.*/
typedef void (*write_image_subresource_t)(void* image_data, uint32_t image_index, const VkImageSubresource* subresource, VkDeviceSize buffer_size, const VkImageCreateInfo* image_info, const VkExtent3D* subresource_extent, const void* context);


/*! Creates a host-local staging buffer for each subresource of the given
	images, maps it and lets a callback fill it with image data. Then these
	staging buffers are copied to the given images and destroyed.
	\param images Output of create_images(). The contents of these images will
		be overwritten. All of these images must have aspect color and any
		other aspects will be silently ignored.
	\param device Output of create_device().
	\param write_subresource See write_image_subresource_t.
	\param old_layout The layout that the given images have prior to the
		operation.
	\param new_layout The layout that the given images should have after the
		operation concludes. A common choice would be
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
	\param context Passed to (*write_subresource)() as last parameter.
		Typically a pointer to a user-defined struct providing information
		needed by the callback.
	\param 0 upon success. Upon failure, the contents of the images are
		undefined.*/
int fill_images(const images_t* images, const device_t* device, write_image_subresource_t write_subresource, VkImageLayout old_layout, VkImageLayout new_layout, const void* context);


//! \return The name of the given shader stage as expected by glslangValidator
//!		or an empty string if stage is not a single stage bit.
const char* get_shader_stage_name(VkShaderStageFlags stage);


//! Compiles a GLSL or HLSL shader to SPIR-V as specified in the given request.
int compile_shader(const shader_compilation_request_t* request);


//! Forwards to compile_shader(), but if that fails, it prompts the user on the
//! command line whether to try again.
int compile_shader_with_retry(const shader_compilation_request_t* request);


//! Forwards to compile_shader() or compile_shader_with_retry() and creates a
//! Vulkan shader module out of the compiled SPIR-V code
int compile_and_create_shader_module(VkShaderModule* module, const device_t* device, const shader_compilation_request_t* request, bool retry);


/*! A utility function to avoid redundant code when defining bindings in
	descriptor sets.
	\param bindings Binding specifications that are to be modified in place.
	\param binding_count The number of array entries in bindings.
	\param min_descriptor_count The minimal value for descriptorCount, e.g. 1.
	\param shared_stages stageFlags is ORed with these flags.*/
void complete_descriptor_set_layout_bindings(VkDescriptorSetLayoutBinding* bindings, uint32_t binding_count, uint32_t min_descriptor_count, VkShaderStageFlags shared_stages);


/*! A utility function to avoid redundant code when defining descriptor set
	writes. For each write, it sets sType and dstSet appropriately and copies
	descriptorType and descriptorCount from the binding where binding matches
	dstBinding (if any).
	\param writes Descriptor set writes to modify in place.
	\param write_count Number of array entries in writes.
	\param bindings Bindings from which to copy information.
	\param binding_count Number of array entries in bindings.
	\param destination_set The value to use for dstSet in all writes. Pass NULL
		to leave it unchanged.*/
void complete_descriptor_set_writes(VkWriteDescriptorSet* writes, uint32_t write_count, const VkDescriptorSetLayoutBinding* bindings, uint32_t binding_count, VkDescriptorSet destination_set);


/*! Creates all the objects needed to create descriptor sets, the descriptor
	sets themselves and a matching pipeline layout.
	\param descriptor_sets The output. Clean up with free_descriptor_sets().
	\param device Output of create_device().
	\param bindings Specifications of the bindings. You may want to use
		complete_descriptor_set_layout_bindings() to populate this array.
	\param binding_count The number of array entries in bindings.
	\param descriptor_set_count The number of descriptor sets with identical
		layout which should be created.
	\return 0 upon success.*/
int create_descriptor_sets(descriptor_sets_t* descriptor_sets, const device_t* device, VkDescriptorSetLayoutBinding* bindings, uint32_t binding_count, uint32_t descriptor_set_count);


void free_descriptor_sets(descriptor_sets_t* descriptor_sets, const device_t* device);
