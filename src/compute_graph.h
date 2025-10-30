#pragma once
#include "vulkan_basics.h"


//! A specification of a bound input or output resource for a compute dispatch
typedef struct {
	/*! A description of the binding for Vulkan. The stageFlags entry is set
		automatically. Supported types are:
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
		VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
		VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER */
	VkDescriptorSetLayoutBinding desc;
	//! The index of the set from which images or buffers are bound
	uint32_t set;
	//! The index of the first relevant entry within the set. The last relevant
	//! entry is entry + desc.descriptorCount - 1.
	uint32_t entry;
} dispatch_binding_t;


//! A specification of a single compute dispatch within a compute graph
typedef struct {
	//! The number of bindings used by the dispatch
	uint32_t binding_count;
	//! An array of binding_count bindings used by the compute dispatch
	dispatch_binding_t* bindings;
	//! The compilation request for the shader that is to be used in the
	//! dispatch
	shader_compilation_request_t shader_request;
	//! The group counts along x-, y- and z-axis, respectively
	uint32_t group_counts[3];
	//! The value used for VkPipelineShaderStageCreateInfo::flags when creating
	//! the compute pipeline. Can be 0.
	VkPipelineShaderStageCreateFlagBits stage_flags;
} compute_dispatch_t;


/*! A specification of a compute graph consisting of multiple compute
	dispatches and operating on already allocated images and buffers. This
	object only specifies the operations to be performed, a compute_workload_t
	is needed to actually perform them.*/
typedef struct {
	//! Number of array entries in buffer_sets
	uint32_t buffer_set_count;
	//! Sets of buffers that are used for input and output
	const buffers_t** buffer_sets;
	//! Number of array entries in image_sets
	uint32_t image_set_count;
	//! Sets of images that are used for input and output
	const images_t** image_sets;
	//! The sampler used for all descriptor set writes that require a sampler.
	//! Can be NULL if there are no such descriptors.
	VkSampler sampler;
	//! Number of array entries in dispatches
	uint32_t dispatch_count;
	//! The compute dispatches which are to be dispatched for this graph in the
	//! order of dispatch
	compute_dispatch_t* dispatches;
} compute_graph_t;


//! The counterpart of compute_dispatch_t to be used in a workload.
typedef struct {
	//! The group counts along x-, y- and z-axis, respectively
	uint32_t group_counts[3];
	//! The used compute shader
	VkShaderModule shader;
	//! The compute pipeline to use for this dispatch
	VkPipeline pipeline;
	//! The descriptor sets and associated objects to be used for this dispatch
	descriptor_sets_t descriptor_sets;
	//! Number of array entries in image_barriers
	uint32_t image_barrier_count;
	//! Image barriers to use after this dispatch to ensure that results will
	//! be visible
	VkImageMemoryBarrier* image_barriers;
	//! Number of array entries in buffer_barriers
	uint32_t buffer_barrier_count;
	//! Buffer barriers to use after this dispatch to ensure that results will
	//! be visible
	VkBufferMemoryBarrier* buffer_barriers;
} compute_dispatch_workload_t;


//! A version of the compute graph that is ready to be dispatched.
typedef struct {
	//! The number of array entries in dispatches
	uint32_t dispatch_count;
	//! The dispatches to execute in sequence to perform this workload
	compute_dispatch_workload_t* dispatches;
} compute_workload_t;



/*! Turns a compute graph into a compute workload to make it possible to
	dispatch all this compute work.*/
int create_compute_workload(compute_workload_t* workload, const device_t* device, const compute_graph_t* graph);


//! Records commands to dispatch all compute shaders in the given workload into
//! the given command buffer (including barriers)
void record_compute_graph_commands(VkCommandBuffer cmd, const compute_workload_t* workload);


//! Frees resources of the given compute workload
void free_compute_workload(compute_workload_t* workload, const device_t* device);
