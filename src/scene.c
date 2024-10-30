#include "scene.h"
#include "textures.h"
#include "string_utilities.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


//! Holds a pointer to a scene and temporary objects needed to load it
typedef struct {
	//! The device that is being used to load the scene
	const device_t* device;
	//! A pointer to the scene being loaded or NULL once it is loaded
	scene_t* scene;
	//! A temporary copy of the quantized positions
	uint32_t* quantized_poss;
	//! One buffer for geometry/instance data per BVH level
	buffers_t geometry_buffers;
	//! Scratch memory buffers for acceleration structure build
	buffers_t scratch_buffers;
	//! The command buffer used to record build commands
	VkCommandBuffer cmd;
	//! The file from which the scene is being loaded
	FILE* file;
} scene_loader_t;


void free_scene_loader(scene_loader_t* loader, const device_t* device);


//! Callback for fill_buffers() to write geometry data for BVHs
void write_geometry_buffers(void* buffer_data, uint32_t buffer_index, VkDeviceSize buffer_size, const void* context) {
	const scene_loader_t* loader = (const scene_loader_t*) context;
	const scene_t* scene = loader->scene;
	const device_t* device = loader->device;
	switch (buffer_index) {
		case bvh_level_bottom: {
			// Dequantize all vertex positions
			scene_file_header_t header = scene->header;
			uint32_t vertex_count = 3 * scene->header.triangle_count;
			const uint32_t* src = loader->quantized_poss;
			float* dst = (float*) buffer_data;
			for (uint32_t i = 0; i != vertex_count; ++i) {
				uint32_t a = src[2 * i + 0];
				uint32_t b = src[2 * i + 1];
				float pos[3] = {
					(float) (a & 0x1fffff),
					(float) (((a & 0xffe00000) >> 21) | ((b & 0x3ff) << 11)),
					(float) ((b & 0x7ffffc00) >> 10),
				};
				for (uint32_t j = 0; j != 3; ++j) {
					(*dst) = pos[j] * header.dequantization_factor[j] + header.dequantization_summand[j];
					++dst;
				}
			}
			break;
		}
		case bvh_level_top: {
			// A single instance of the bottom level with identity transform
			VK_LOAD(vkGetAccelerationStructureDeviceAddressKHR);
			VkAccelerationStructureDeviceAddressInfoKHR address_info = {
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
				.accelerationStructure = scene->bvhs.bvhs[bvh_level_bottom],
			};
			VkAccelerationStructureInstanceKHR instance = {
				.accelerationStructureReference = (*pvkGetAccelerationStructureDeviceAddressKHR)(loader->device->device, &address_info),
				.transform = { .matrix = {
						{ 1.0f, 0.0f, 0.0f, 0.0f, },
						{ 0.0f, 1.0f, 0.0f, 0.0f, },
						{ 0.0f, 0.0f, 1.0f, 0.0f, },
					}
				},
				.mask = 0xFF,
				.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR | VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR,
			};
			memcpy(buffer_data, &instance, sizeof(instance));
			break;
		}
		default:
			break;
	}
}


/*! Creates a BVH for a scene being loaded.
	\param loader An active scene loader with quantized positions readily
		available. The calling side is responsible for freeing it.
	\param device Output of create_device.
	\return 0 upon success.*/
int create_bvh(scene_loader_t* loader, const device_t* device) {
	bvhs_t* bvhs = &loader->scene->bvhs;
	VK_LOAD(vkGetAccelerationStructureBuildSizesKHR);
	VK_LOAD(vkCreateAccelerationStructureKHR);
	VK_LOAD(vkCmdBuildAccelerationStructuresKHR);
	// Map levels to BVH types
	VkAccelerationStructureTypeKHR types[bvh_level_count];
	types[bvh_level_bottom] = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	types[bvh_level_top] = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	// Create buffers for geometry/instance data
	uint32_t triangle_count = loader->scene->header.triangle_count;
	uint32_t vertex_count = 3 * triangle_count;
	buffer_request_t geo_requests[bvh_level_count];
	buffer_request_t bottom_geo_request = {
		.buffer_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = vertex_count * 3 * sizeof(float),
			.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		},
	};
	geo_requests[bvh_level_bottom] = bottom_geo_request;
	buffer_request_t top_geo_request = {
		.buffer_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = sizeof(VkAccelerationStructureInstanceKHR),
			.usage = bottom_geo_request.buffer_info.usage,
		},
	};
	geo_requests[bvh_level_top] = top_geo_request;
	if (create_buffers(&loader->geometry_buffers, device, geo_requests, COUNT_OF(geo_requests), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1))
		return printf("Failed to create buffers to hold the geometry data for building an acceleration structure.\n");
	// Define characteristics of the geometry for both BVH levels
	VkAccelerationStructureGeometryKHR geometries[bvh_level_count];
	VkBufferDeviceAddressInfo bottom_buffer_address = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = loader->geometry_buffers.buffers[bvh_level_bottom].buffer,
	};
	VkAccelerationStructureGeometryKHR bottom_geometry = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
		.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
		.geometry = {
			.triangles = {
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
				.vertexData = { .deviceAddress = vkGetBufferDeviceAddress(device->device, &bottom_buffer_address) },
				.maxVertex = vertex_count - 1,
				.vertexStride = 3 * sizeof(float),
				.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
				.indexType = VK_INDEX_TYPE_NONE_KHR,
			},
		},
	};
	geometries[bvh_level_bottom] = bottom_geometry;
	VkBufferDeviceAddressInfo top_buffer_address = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = loader->geometry_buffers.buffers[bvh_level_top].buffer,
	};
	VkAccelerationStructureGeometryKHR top_geometry = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
		.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
		.geometry = {
			.instances = {
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
				.arrayOfPointers = VK_FALSE,
				.data = { .deviceAddress = vkGetBufferDeviceAddress(device->device, &top_buffer_address) },
			},
		},
	};
	geometries[bvh_level_top] = top_geometry;
	// Figure out size requirements for acceleration structures
	VkAccelerationStructureBuildSizesInfoKHR sizes[bvh_level_count];
	memset(sizes, 0, sizeof(sizes));
	uint32_t primitive_counts[bvh_level_count];
	primitive_counts[bvh_level_bottom] = triangle_count;
	primitive_counts[bvh_level_top] = 1;
	VkAccelerationStructureBuildGeometryInfoKHR build_infos[bvh_level_count];
	for (uint32_t i = 0; i != bvh_level_count; ++i) {
		VkAccelerationStructureBuildGeometryInfoKHR build_info = {
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
			.type = types[i],
			.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
			.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
			.geometryCount = 1,
			.pGeometries = &geometries[i],
		};
		build_infos[i] = build_info;
		sizes[i].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		(*pvkGetAccelerationStructureBuildSizesKHR)(device->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_infos[i], &primitive_counts[i], &sizes[i]);
	}
	// Create buffers to hold acceleration structures
	buffer_request_t bvh_buffer_requests[bvh_level_count];
	for (int i = 0; i != bvh_level_count; ++i) {
		buffer_request_t request = {
			.buffer_info = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = sizes[i].accelerationStructureSize,
				.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			},
		};
		bvh_buffer_requests[i] = request;
	}
	if (create_buffers(&bvhs->buffers, device, bvh_buffer_requests, COUNT_OF(bvh_buffer_requests), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1))
		return printf("Failed to create buffers to hold acceleration structures.\n");
	// Create acceleration structures
	for (uint32_t i = 0; i != bvh_level_count; ++i) {
		VkAccelerationStructureCreateInfoKHR bvh_info = {
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
			.buffer = bvhs->buffers.buffers[i].buffer,
			.size = sizes[i].accelerationStructureSize,
			.type = types[i],
		};
		if ((*pvkCreateAccelerationStructureKHR)(device->device, &bvh_info, NULL, &loader->scene->bvhs.bvhs[i]))
			return printf("Failed to create an acceleration structure.\n");
	}
	// Now that we can get a pointer to the bottom-level for the top-level,
	// fill the geometry buffers
	if (fill_buffers(&loader->geometry_buffers, device, &write_geometry_buffers, loader))
		return printf("Failed to upload geometry data for building acceleration structures to the GPU.\n");
	// Create buffers for scratch data
	buffer_request_t scratch_requests[bvh_level_count];
	for (int i = 0; i != bvh_level_count; ++i) {
		buffer_request_t request = {
			.buffer_info = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = sizes[i].buildScratchSize,
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			},
		};
		scratch_requests[i] = request;
	}
	if (create_buffers(&loader->scratch_buffers, device, scratch_requests, COUNT_OF(scratch_requests), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device->bvh_properties.minAccelerationStructureScratchOffsetAlignment))
		return printf("Failed to create scratch buffers for the acceleration structure build.\n");

	// Prepare to record commands
	VkCommandBufferAllocateInfo cmd_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = device->cmd_pool,
		.commandBufferCount =  1,
	};
	if (vkAllocateCommandBuffers(device->device, &cmd_info, &loader->cmd))
		return printf("Failed to create a command buffer to record acceleration structure build commands.\n");
	VkCommandBufferBeginInfo begin_info = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	if (vkBeginCommandBuffer(loader->cmd, &begin_info))
		return printf("Failed to begin recording a command buffer for building acceleration structures.\n");
	// Build bottom- and top-level acceleration structures in this order
	for (uint32_t i = 0; i != bvh_level_count; ++i) {
		build_infos[i].dstAccelerationStructure = bvhs->bvhs[i];
		VkBufferDeviceAddressInfo scratch_adress_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = loader->scratch_buffers.buffers[i].buffer,
		};
		build_infos[i].scratchData.deviceAddress = vkGetBufferDeviceAddress(device->device, &scratch_adress_info);
		VkAccelerationStructureBuildRangeInfoKHR build_range = { .primitiveCount = primitive_counts[i] };
		const VkAccelerationStructureBuildRangeInfoKHR* build_range_ptr = &build_range;
		pvkCmdBuildAccelerationStructuresKHR(loader->cmd, 1, &build_infos[i], &build_range_ptr);
		// Enforce synchronization
		VkMemoryBarrier after_build_barrier = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
			.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
		};
		vkCmdPipelineBarrier(loader->cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
				VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0,
				1, &after_build_barrier, 0, NULL, 0, NULL);
	}
	// Submit the command buffer
	VkSubmitInfo cmd_submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1, .pCommandBuffers = &loader->cmd,
	};
	if (vkEndCommandBuffer(loader->cmd) || vkQueueSubmit(device->queue, 1, &cmd_submit, NULL))
		return printf("Failed to end and submit the command buffer for building acceleration structures.\n");
	return 0;
}


//! Callback for fill_buffers() that writes mesh data from the scene file
//! directly to staging buffers
void write_mesh_buffer(void* buffer_data, uint32_t buffer_index, VkDeviceSize buffer_size, const void* context) {
	const scene_loader_t* loader = (const scene_loader_t*) context;
	if (buffer_index == mesh_buffer_type_positions) {
		// Create a temporary copy of the quantized positions for acceleration
		// structure build
		fread(loader->quantized_poss, sizeof(uint8_t), buffer_size, loader->file);
		memcpy(buffer_data, loader->quantized_poss, buffer_size);
	}
	else
		fread(buffer_data, sizeof(uint8_t), buffer_size, loader->file);
}


int load_scene(scene_t* scene, const device_t* device, const char* file_path, const char* texture_path) {
	memset(scene, 0, sizeof(*scene));
	scene_loader_t loader = { .scene = scene, .device = device };
	// Open the source file
	FILE* file = loader.file = fopen(file_path, "rb");
	if (!file) {
		printf("Failed to open the scene file at %s. Please check path and permissions.\n", file_path);
		free_scene_loader(&loader, device);
		return 1;
	}
	// Load the header
	fread(&scene->header.marker, sizeof(uint32_t), 1, file);
	if (scene->header.marker != 0xabcabc) {
		printf("The scene file at %s is not a valid *.vks file. Its marker does not match.\n", file_path);
		free_scene_loader(&loader, device);
		return 1;
	}
	fread(&scene->header.version, sizeof(uint32_t), 1, file);
	if (scene->header.version != 1) {
		printf("This renderer only supports *.vks files using version 1 of the file format, but the scene file at %s uses version %u.\n", file_path, scene->header.version);
		free_scene_loader(&loader, device);
		return 1;
	}
	fread(&scene->header.material_count, sizeof(uint64_t), 1, file);
	fread(&scene->header.triangle_count, sizeof(uint64_t), 1, file);
	fread(&scene->header.dequantization_factor, sizeof(float), 3, file);
	fread(&scene->header.dequantization_summand, sizeof(float), 3, file);
	// Load material names
	scene->header.material_names = calloc(scene->header.material_count, sizeof(char*));
	for (uint64_t i = 0; i != scene->header.material_count; ++i) {
		uint64_t length = 0;
		fread(&length, sizeof(length), 1, file);
		scene->header.material_names[i] = malloc(length + 1);
		fread(scene->header.material_names[i], sizeof(char), length + 1, file);
	}
	// Create buffers for the geometry
	buffer_request_t buffer_requests[mesh_buffer_type_count];
	memset(buffer_requests, 0, sizeof(buffer_requests));
	for (uint32_t i = 0; i != mesh_buffer_type_count; ++i) {
		VkBufferCreateInfo* buffer_info = &buffer_requests[i].buffer_info;
		buffer_info->sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
		if (i == mesh_buffer_type_positions || i == mesh_buffer_type_normals_and_tex_coords) {
			buffer_info->usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			buffer_info->size = sizeof(uint32_t) * 2 * scene->header.triangle_count * 3;
		}
		else if (i == mesh_buffer_type_material_indices)
			buffer_info->size = sizeof(uint8_t) * scene->header.triangle_count;
		VkBufferViewCreateInfo* view_info = &buffer_requests[i].view_info;
		view_info->sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
		switch (i) {
			case mesh_buffer_type_positions:
				view_info->format = VK_FORMAT_R32G32_UINT;
				break;
			case mesh_buffer_type_normals_and_tex_coords:
				view_info->format = VK_FORMAT_R16G16B16A16_UNORM;
				break;
			case mesh_buffer_type_material_indices:
				view_info->format = VK_FORMAT_R8_UINT;
				break;
			default:
				view_info->sType = 0;
				break;
		}
	}
	if (create_buffers(&scene->mesh_buffers, device, buffer_requests, mesh_buffer_type_count, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1)) {
		printf("Failed to create geometry buffers for the scene file at %s with %lu triangles.\n", file_path, scene->header.triangle_count);
		free_scene_loader(&loader, device);
		return 1;
	}
	// While filling buffers, we keep quantized positions around for acceleration structure build
	loader.quantized_poss = malloc(buffer_requests[mesh_buffer_type_positions].buffer_info.size);
	// Fill the geometry buffers with data from the file
	if (fill_buffers(&scene->mesh_buffers, device, &write_mesh_buffer, &loader)) {
		printf("Failed to write mesh data of the scene file at %s to device-local buffers.\n", file_path);
		free_scene_loader(&loader, device);
		return 1;
	}
	// If everything went well, we have reached an end of file marker now
	uint32_t eof_marker = 0;
	fread(&eof_marker, sizeof(eof_marker), 1, file);
	if (eof_marker != 0xe0fe0f) {
		printf("Finished reading data from the scene file at %s but did not encounter an end-of-file marker where expected. Either the file is invalid or the loader is buggy.\n", file_path);
		free_scene_loader(&loader, device);
		return 1;
	}
	// Close the scene file
	fclose(file);
	file = loader.file = NULL;
	// Build acceleration structures
	if (create_bvh(&loader, device)) {
		printf("Failed to create ray-tracing acceleration structures for the scene file at %s.\n", file_path);
		free_scene_loader(&loader, device);
		return 1;
	}
	// Load textures
	VkDeviceSize texture_count = scene->header.material_count * material_texture_type_count;
	char** texture_file_paths = calloc(texture_count, sizeof(char*));
	const char* suffixes[material_texture_type_count];
	suffixes[material_texture_type_base_color] = "_BaseColor.vkt";
	suffixes[material_texture_type_specular] = "_Specular.vkt";
	suffixes[material_texture_type_normal] = "_Normal.vkt";
	for (VkDeviceSize i = 0; i != scene->header.material_count; ++i) {
		for (uint32_t j = 0; j != material_texture_type_count; ++j) {
			const char* parts[] = { texture_path, "/", scene->header.material_names[i], suffixes[j] };
			texture_file_paths[i * material_texture_type_count + j] = cat_strings(parts, COUNT_OF(parts));
		}
	}
	int result = load_textures(&scene->textures, device, (const char* const*) texture_file_paths, (uint32_t) texture_count, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	for (VkDeviceSize i = 0; i != texture_count; ++i)
		free(texture_file_paths[i]);
	free(texture_file_paths);
	texture_file_paths = NULL;
	if (result) {
		printf("Failed to load textures for the scene file at %s.\n", file_path);
		free_scene_loader(&loader, device);
		return 1;
	}
	// Tidy up temporary objects created whilst loading
	loader.scene = NULL;
	free_scene_loader(&loader, device);
	return 0;
}


void free_scene(scene_t* scene, const device_t* device) {
	VK_LOAD(vkDestroyAccelerationStructureKHR);
	free_images(&scene->textures, device);
	free_buffers(&scene->mesh_buffers, device);
	if (scene->header.material_names)
		for (uint64_t i = 0; i != scene->header.material_count; ++i)
			free(scene->header.material_names[i]);
	for (uint32_t i = 0; i != bvh_level_count; ++i)
		if (scene->bvhs.bvhs[i])
			(*pvkDestroyAccelerationStructureKHR)(device->device, scene->bvhs.bvhs[i], NULL);
	free_buffers(&scene->bvhs.buffers, device);
	free(scene->header.material_names);
	memset(scene, 0, sizeof(*scene));
}


void free_scene_loader(scene_loader_t* loader, const device_t* device) {
	if (loader->scene) free_scene(loader->scene, device);
	free(loader->quantized_poss);
	if (loader->file) fclose(loader->file);
	free_buffers(&loader->geometry_buffers, device);
	free_buffers(&loader->scratch_buffers, device);
	if (loader->cmd) vkFreeCommandBuffers(device->device, device->cmd_pool, 1, &loader->cmd);
}
