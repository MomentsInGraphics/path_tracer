#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_control_flow_attributes : enable
#include "grid_indices.glsl"


//! The volume texture at the original resolution to be used as input
layout (binding = 0) uniform sampler3D g_volume_texture;
//! The output textures of lower resolution
layout (binding = 1) uniform writeonly image3D g_out_grids[GRID_ATTRIBUTE_COUNT];

//! We process 4x4x4 bricks per thread group
layout (local_size_x = 4, local_size_y = 4, local_size_z = 4) in;


//! Contains all quantities for which a reduce is performed, to avoid a bit of
//! code duplication
struct reduced_values_t {
	uint value_count;
	float minimum, maximum, mean, moment_2;
};


//! Combines the given value into the given reduced values
void reduce(inout reduced_values_t reduced, float value) {
	++reduced.value_count;
	reduced.minimum = min(reduced.minimum, value);
	reduced.maximum = max(reduced.maximum, value);
	reduced.mean += value;
	reduced.moment_2 += value * value;
}


void main() {
	ivec3 base_voxel = ivec3(gl_GlobalInvocationID * BRICK_SIZE);
	reduced_values_t reduced;
	reduced.value_count = 0;
	reduced.minimum = 3.4e38;
	reduced.maximum = -3.4e38;
	reduced.mean = reduced.moment_2 = 0.0;
#if VOLUME_INTERPOLATION_NEAREST
	ivec3 window_min = ivec3(0);
	ivec3 window_max = ivec3(BRICK_SIZE);
#elif VOLUME_INTERPOLATION_LINEAR || VOLUME_INTERPOLATION_LINEAR_STOCHASTIC
	ivec3 window_min = ivec3(-1);
	ivec3 window_max = ivec3(BRICK_SIZE + 1);
#endif
	[[loop]]
	for (int x = window_min.x; x != window_max.x; ++x)
		[[loop]]
		for (int y = window_min.y; y != window_max.y; ++y)
			[[loop]]
			for (int z = window_min.z; z != window_max.z; ++z)
				reduce(reduced, texelFetch(g_volume_texture, base_voxel + ivec3(x, y, z), 0).r);
	// Normalize and process the reduced values
	float inv_voxel_count = 1.0 / reduced.value_count;
	reduced.mean *= inv_voxel_count;
	reduced.moment_2 *= inv_voxel_count;
	float std = sqrt(max(0.0, reduced.moment_2 - reduced.mean * reduced.mean));
	float sqrt_moment_2 = sqrt(max(0.0, reduced.moment_2 + reduced.minimum * (reduced.minimum - 2.0 * reduced.mean)));
	// If the minimum is non-zero we do not want a zero sampling density, since
	// the minimum control variate will be subjected to rounding errors
	sqrt_moment_2 = max(0.01 * reduced.minimum, sqrt_moment_2);
	// Write the results to the output grids
	imageStore(g_out_grids[GRID_INDEX_MINIMUM], ivec3(gl_GlobalInvocationID), vec4(reduced.minimum));
	imageStore(g_out_grids[GRID_INDEX_MAXIMUM], ivec3(gl_GlobalInvocationID), vec4(reduced.maximum));
	imageStore(g_out_grids[GRID_INDEX_MEAN], ivec3(gl_GlobalInvocationID), vec4(reduced.mean));
	imageStore(g_out_grids[GRID_INDEX_STD], ivec3(gl_GlobalInvocationID), vec4(std));
	imageStore(g_out_grids[GRID_INDEX_SQRT_MOMENT_2], ivec3(gl_GlobalInvocationID), vec4(sqrt_moment_2));
}
