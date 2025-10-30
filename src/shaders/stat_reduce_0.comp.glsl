#version 460
#extension GL_EXT_control_flow_attributes : enable
#extension GL_KHR_shader_subgroup_arithmetic : enable


//! The grid for which stats are being computed
layout (binding = 0) uniform sampler3D g_grid;
//! The buffer to which stats are stored
layout (binding = 1) buffer restrict stat_reduce_output {
	vec4 g_out_stats[(BRICK_COUNT + 255) / 256];
};

//! The group consists of exactly one subgroup
layout (local_size_x = SUBGROUP_SIZE, local_size_y = 1, local_size_z = 1) in;


void main() {
	// One subgroup reduces 256 values
	vec4 reduced = vec4(1.0e38, -1.0e38, 0.0, 0.0);
	[[unroll]]
	for (int i = 0; i != 256 / SUBGROUP_SIZE; ++i) {
		int src_index = int(gl_WorkGroupID.x * 256 + gl_LocalInvocationID.x + i * SUBGROUP_SIZE);
		// Disable threads outside of the grid
		if (src_index < BRICK_COUNT) {
			// Read the value from the grid
			ivec3 brick_index = ivec3(
				src_index % BRICK_COUNT_X,
				(src_index / BRICK_COUNT_X) % BRICK_COUNT_Y,
				src_index / (BRICK_COUNT_X * BRICK_COUNT_Y)
			);
			float grid_value = texelFetch(g_grid, brick_index, 0).r;
			// Perform the reduction
			float grid_square = grid_value * grid_value;
			reduced[0] = min(reduced[0], grid_value);
			reduced[1] = max(reduced[1], grid_value);
			reduced[2] += grid_value;
			reduced[3] += grid_square;
		}
	}
	// Now perform further reduction using subgroup intrinsics
	reduced = vec4(
		subgroupMin(reduced[0]),
		subgroupMax(reduced[1]),
		subgroupAdd(reduced[2]),
		subgroupAdd(reduced[3])
	);
	// Write out the result
	if (gl_LocalInvocationID.x == 0)
		g_out_stats[gl_WorkGroupID.x] = reduced;
}
