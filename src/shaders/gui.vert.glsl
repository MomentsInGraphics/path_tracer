#version 460
#extension GL_GOOGLE_include_directive : enable
#include "constants.glsl"

//! The vertex position in pixels from the left top of the viewport
layout (location = 0) in vec2 g_pos;

//! The texture coordinate for the glyph image
layout (location = 1) in vec2 g_tex_coord;

//! The sRGB color and alpha
layout (location = 2) in uvec4 g_color;

//! The left, top, right and bottom of the scissor rectangle in pixels from the
//! left top of the viewport
layout (location = 3) in ivec4 g_scissor;


//! Passed through texture coordinate
layout (location = 0) smooth out vec2 g_out_tex_coord;

//! Passed through color
layout (location = 1) smooth out vec4 g_out_color;

//! Passed through scissor
layout (location = 2) flat out ivec4 g_out_scissor;


void main() {
	gl_Position = vec4(g_pos * g_inv_viewport_size * 2.0 - vec2(1.0), 0.5, 1.0);
	g_out_tex_coord = g_tex_coord;
	g_out_color = vec4(g_color) * (1.0 / 255.0);
	g_out_scissor = g_scissor;
	// Graphs get special treatment
	if (g_color.b == 253 && (g_color.a & 0xfc) == 48) {
		g_out_tex_coord.x = ((g_color.a & 2) != 0) ? 1.0 : 0.0;
		g_out_tex_coord.y = ((g_color.a & 1) != 0) ? 1.0 : 0.0;
		uint graph_index = 256 * g_color.r + g_color.g;
		g_out_color = vec4(253.0, 48.0, graph_index, 1.0);
	}
}
