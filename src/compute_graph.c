#include "compute_graph.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


//! Helper for create_compute_workload() that processes a single dispatch
int create_compute_dispatch_workload(compute_dispatch_workload_t* workload, const device_t* device, const compute_graph_t* graph, uint32_t dispatch_index) {
	memset(workload, 0, sizeof(*workload));
	compute_dispatch_t* dispatch = &graph->dispatches[dispatch_index];
	memcpy(workload->group_counts, dispatch->group_counts, sizeof(workload->group_counts));
	// Create the shader module
	if (compile_and_create_shader_module(&workload->shader, device, &dispatch->shader_request, true)) {
		printf("Failed to compile a compute shader.\n");
		return 1;
	}
	// Create the pipeline layout and the descriptor sets
	VkDescriptorSetLayoutBinding* bindings = calloc(dispatch->binding_count, sizeof(VkDescriptorSetLayoutBinding));
	uint32_t descriptor_count = 0;
	for (uint32_t i = 0; i != dispatch->binding_count; ++i) {
		bindings[i] = dispatch->bindings[i].desc;
		bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		descriptor_count += bindings[i].descriptorCount;
	}
	int result = create_descriptor_sets(&workload->descriptor_sets, device, bindings, dispatch->binding_count, 1);
	free(bindings);
	bindings = NULL;
	if (result) {
		printf("Failed to create a descriptor set for a compute workload.\n");
		return 1;
	}
	// Write to the created descriptor set
	VkWriteDescriptorSet* writes = calloc(dispatch->binding_count, sizeof(VkWriteDescriptorSet));
	VkDescriptorImageInfo* image_infos = calloc(descriptor_count, sizeof(VkDescriptorImageInfo));
	VkDescriptorBufferInfo* buffer_infos = calloc(descriptor_count, sizeof(VkDescriptorBufferInfo));
	VkBufferView* buffer_views = calloc(descriptor_count, sizeof(VkBufferView));
	uint32_t buffer_output_count = 0, image_output_count = 0;
	for (uint32_t i = 0; i != dispatch->binding_count; ++i) {
		const VkDescriptorSetLayoutBinding* desc = &dispatch->bindings[i].desc;
		writes[i] = (VkWriteDescriptorSet) {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.descriptorCount = desc->descriptorCount,
			.descriptorType = desc->descriptorType,
			.dstBinding = desc->binding,
			.dstSet = workload->descriptor_sets.descriptor_sets[0],
		};
		const dispatch_binding_t* binding = &dispatch->bindings[i];
		// Validate indices
		bool use_image_set = false;
		switch (desc->descriptorType) {
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				use_image_set = true;
				break;
			default:
				break;
		}
		if (use_image_set) {
			if (binding->set >= graph->image_set_count) {
				printf("The image set index %u of binding %u in compute dispatch %u is out of range.\n", binding->set, i, dispatch_index);
				return 1;
			}
			if (binding->entry + binding->desc.descriptorCount > graph->image_sets[binding->set]->image_count) {
				printf("The image set %u used by binding %u in compute dispatch %u does not have enough images.\n", binding->set, i, dispatch_index);
				return 1;
			}
		}
		else {
			if (binding->set >= graph->buffer_set_count) {
				printf("The buffer set index %u of binding %u in compute dispatch %u is out of range.\n", binding->set, i, dispatch_index);
				return 1;
			}
			if (binding->entry + binding->desc.descriptorCount > graph->buffer_sets[binding->set]->buffer_count) {
				printf("The buffer set %u used by binding %u in compute dispatch %u does not have enough buffers.\n", binding->set, i, dispatch_index);
				return 1;
			}
		}
		// Define the write
		switch (desc->descriptorType) {
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				if (!graph->sampler) {
					printf("Binding %u of dispatch %u requires a sampler, but none was provided.\n", i, dispatch_index);
					return 1;
				}
				for (uint32_t j = 0; j != desc->descriptorCount; ++j)
					image_infos[j] = (VkDescriptorImageInfo) {
						.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						.imageView = graph->image_sets[binding->set]->images[binding->entry + j].view,
						.sampler = graph->sampler,	
					};
				writes[i].pImageInfo = image_infos;
				break;
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				image_output_count += desc->descriptorCount;
				for (uint32_t j = 0; j != desc->descriptorCount; ++j)
					image_infos[j] = (VkDescriptorImageInfo) {
						.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
						.imageView = graph->image_sets[binding->set]->images[binding->entry + j].view,
					};
				writes[i].pImageInfo = image_infos;
				break;
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				for (uint32_t j = 0; j != desc->descriptorCount; ++j)
					image_infos[j] = (VkDescriptorImageInfo) {
						.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						.imageView = graph->image_sets[binding->set]->images[binding->entry + j].view,
					};
				writes[i].pImageInfo = image_infos;
				break;
			case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
				buffer_output_count += desc->descriptorCount;
			case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
				for (uint32_t j = 0; j != desc->descriptorCount; ++j)
					buffer_views[j] = graph->buffer_sets[binding->set]->buffers[binding->entry + j].view;
				writes[i].pTexelBufferView = buffer_views;
				break;
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				buffer_output_count += desc->descriptorCount;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				for (uint32_t j = 0; j != desc->descriptorCount; ++j)
					buffer_infos[j] = (VkDescriptorBufferInfo) {
						.buffer = graph->buffer_sets[binding->set]->buffers[binding->entry + j].buffer,
						.range = VK_WHOLE_SIZE,
					};
				writes[i].pBufferInfo = buffer_infos;
				break;
			default:
				printf("Descriptor type %u for binding %u of dispatch %u is not supported.\n", desc->descriptorType, i, dispatch_index);
				return 1;
		}
		// Only one of these is actually being used per binding, but this way
		// the allocation is easier
		image_infos += desc->descriptorCount;
		buffer_infos += desc->descriptorCount;
		buffer_views += desc->descriptorCount;
	}
	vkUpdateDescriptorSets(device->device, dispatch->binding_count, writes, 0, NULL);
	free(writes);
	image_infos -= descriptor_count;
	free(image_infos);
	image_infos = NULL;
	buffer_infos -= descriptor_count;
	free(buffer_infos);
	buffer_infos = NULL;
	buffer_views -= descriptor_count;
	free(buffer_views);
	buffer_views = NULL;
	// Create the compute pipeline
	VkComputePipelineCreateInfo pipeline_info = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.layout = workload->descriptor_sets.pipeline_layout,
		.stage = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.module = workload->shader,
			.pName = dispatch->shader_request.entry_point,
			.flags = dispatch->stage_flags,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		},
	};
	if (vkCreateComputePipelines(device->device, NULL, 1, &pipeline_info, NULL, &workload->pipeline)) {
		printf("Failed to create a compute pipeline.\n");
		return 1;
	}
	// Specify barriers for all outputs
	workload->image_barriers = calloc(image_output_count, sizeof(VkImageMemoryBarrier));
	workload->buffer_barriers = calloc(buffer_output_count, sizeof(VkBufferMemoryBarrier));
	workload->image_barrier_count = workload->buffer_barrier_count = 0;
	for (uint32_t i = 0; i != dispatch->binding_count; ++i) {
		const VkDescriptorSetLayoutBinding* desc = &dispatch->bindings[i].desc;
		const dispatch_binding_t* binding = &dispatch->bindings[i];
		// Define the write
		switch (desc->descriptorType) {
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				for (uint32_t j = 0; j != desc->descriptorCount; ++j)
					workload->image_barriers[workload->image_barrier_count++] = (VkImageMemoryBarrier) {
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						.image = graph->image_sets[binding->set]->images[binding->entry + j].image,
						.oldLayout = VK_IMAGE_LAYOUT_GENERAL,
						.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
						.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
						.subresourceRange = {
							.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
							.layerCount = graph->image_sets[binding->set]->images[binding->entry + j].request.image_info.arrayLayers,
							.levelCount = graph->image_sets[binding->set]->images[binding->entry + j].request.image_info.mipLevels,
						},
					};
				break;
			case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				for (uint32_t j = 0; j != desc->descriptorCount; ++j)
					workload->buffer_barriers[workload->buffer_barrier_count++] = (VkBufferMemoryBarrier) {
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						.buffer = graph->buffer_sets[binding->set]->buffers[binding->entry + j].buffer,
						.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
						.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
						.size = VK_WHOLE_SIZE,
					};
				break;
			default:
				break;
		}
	}
	return 0;
}



int create_compute_workload(compute_workload_t* workload, const device_t* device, const compute_graph_t* graph) {
	memset(workload, 0, sizeof(*workload));
	workload->dispatch_count = graph->dispatch_count;
	workload->dispatches = calloc(workload->dispatch_count, sizeof(compute_dispatch_workload_t));
	for (uint32_t i = 0; i != workload->dispatch_count; ++i) {
		if (create_compute_dispatch_workload(&workload->dispatches[i], device, graph, i)) {
			printf("Failed to create compute dispatch workload %u of %u.\n", i, workload->dispatch_count);
			free_compute_workload(workload, device);
			return 1;
		}
	}
	return 0;
}


void record_compute_graph_commands(VkCommandBuffer cmd, const compute_workload_t* workload) {
	for (uint32_t i = 0; i != workload->dispatch_count; ++i) {
		const compute_dispatch_workload_t* dispatch = &workload->dispatches[i];
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, dispatch->pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, dispatch->descriptor_sets.pipeline_layout, 0, 1, dispatch->descriptor_sets.descriptor_sets, 0, NULL);
		vkCmdDispatch(cmd, dispatch->group_counts[0], dispatch->group_counts[1], dispatch->group_counts[2]);
		if (dispatch->buffer_barrier_count || dispatch->image_barrier_count)
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, dispatch->buffer_barrier_count, dispatch->buffer_barriers, dispatch->image_barrier_count, dispatch->image_barriers);
	}
}


void free_compute_workload(compute_workload_t* workload, const device_t* device) {
	for (uint32_t i = 0; i != workload->dispatch_count; ++i) {
		compute_dispatch_workload_t* dispatch = &workload->dispatches[i];
		if (dispatch->pipeline) vkDestroyPipeline(device->device, dispatch->pipeline, NULL);
		if (dispatch->shader) vkDestroyShaderModule(device->device, dispatch->shader, NULL);
		free_descriptor_sets(&dispatch->descriptor_sets, device);
		free(dispatch->buffer_barriers);
		free(dispatch->image_barriers);
	}
	free(workload->dispatches);
	memset(workload, 0, sizeof(*workload));
}
