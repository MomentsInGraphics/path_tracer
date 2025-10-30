#version 460
#extension GL_EXT_control_flow_attributes : enable
#extension GL_KHR_shader_subgroup_arithmetic : enable


//! The buffer from which stats are read
layout (binding = 0) buffer restrict readonly stat_reduce_input {
	vec4 g_stats[INPUT_STAT_COUNT];
};
//! The buffer to which stats are stored
layout (binding = 1) buffer restrict writeonly stat_reduce_output {
	vec4 g_out_stats[(INPUT_STAT_COUNT + 255) / 256];
};

//! The group consists of exactly one subgroup
layout (local_size_x = SUBGROUP_SIZE, local_size_y = 1, local_size_z = 1) in;


void main() {
	// Each subgroup reads 256 input stats
	vec4 reduced = vec4(1.0e38, -1.0e38, 0.0, 0.0);
	[[unroll]]
	for (int i = 0; i != 256 / SUBGROUP_SIZE; ++i) {
		int src_index = int(gl_WorkGroupID.x * 256 + gl_LocalInvocationID.x + i * SUBGROUP_SIZE);
		if (src_index < INPUT_STAT_COUNT) {
			vec4 src = g_stats[src_index];
			reduced[0] = min(reduced[0], src[0]);
			reduced[1] = max(reduced[1], src[1]);
			reduced[2] += src[2];
			reduced[3] += src[3];
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
