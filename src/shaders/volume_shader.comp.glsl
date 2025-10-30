#version 460
#extension GL_GOOGLE_include_directive : enable
#include "constants.glsl"

//! The output textures
layout (binding = 1) uniform writeonly image3D g_out_volumes[1];
//! The input volumes (loaded from files)
layout (binding = 2) uniform sampler3D g_volumes[VOLUME_COUNT];
//! The transfer function used to get colors and opacities for the volume
layout (binding = 3) uniform sampler1D g_transfer_function;

//! We process 4x4x4 voxels per thread group
layout (local_size_x = 4, local_size_y = 4, local_size_z = 4) in;


void main() {
	ivec3 texel = ivec3(gl_GlobalInvocationID);
	if (texel.x >= int(g_volume_extent.x) || texel.y >= int(g_volume_extent.y) || texel.z >= int(g_volume_extent.z))
		return;
	vec4 pos_texel = vec4(texel, 1.0);
	vec4 pos_world = g_texel_to_world_space[0] * pos_texel;
	float src_value = texelFetch(g_volumes[0], texel, 0).r;
	float dist = length(pos_world - vec4(-8.8, 44.0, 28.0, 1.0));
	float gauss = exp(-3.0e-4 * dist * dist);
	src_value *= gauss;
	imageStore(g_out_volumes[0], ivec3(gl_GlobalInvocationID), vec4(src_value));
}
