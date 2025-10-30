#version 460
#extension GL_GOOGLE_include_directive : enable
#include "constants.glsl"

//! The index of the current sample within a frame
layout (location = 0) flat out uint g_out_frame_sample_index;

void main() {
	int id = gl_VertexIndex;
	vec2 pos = vec2(0.0);
	if (id == 0) pos = vec2(-1.1, -1.1);
	if (id == 1) pos = vec2(-1.1, 3.5);
	if (id == 2) pos = vec2(3.5, -1.1);
	gl_Position = vec4(pos, 0.0, 1.0);
	// We repurpose the instance index to communicate the sample index within
	// the frame to the shader
	g_out_frame_sample_index = uint(gl_InstanceIndex);
}
