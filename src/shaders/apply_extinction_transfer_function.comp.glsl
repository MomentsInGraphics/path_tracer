#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_control_flow_attributes : enable
#include "grid_indices.glsl"
#include "constants.glsl"


//! The volume texture at the original resolution to which the transfer
//! function should be applied
layout (binding = 1) uniform sampler3D g_volume_texture;
//! The transfer function with extinction values in its alpha channel
layout (binding = 2) uniform sampler1D g_transfer_function;
//! The output texture holding full-resolution extinction values
layout (binding = 3) uniform writeonly image3D g_out_volume_extinction_texture;

//! We process 4x4x4 bricks per thread group
layout (local_size_x = 4, local_size_y = 4, local_size_z = 4) in;


void main() {
	ivec3 texel = ivec3(gl_GlobalInvocationID);
	if (texel.x >= int(g_volume_extent.x) || texel.y >= int(g_volume_extent.y) || texel.z >= int(g_volume_extent.z))
		return;
	// Read a single voxel
	float value = texelFetch(g_volume_texture, texel, 0).r;
	// Map it
	float extinction = texture(g_transfer_function, value * g_transfer_input_factor + g_transfer_input_summand).a;
	// Write it
	imageStore(g_out_volume_extinction_texture, texel, vec4(extinction));
}
