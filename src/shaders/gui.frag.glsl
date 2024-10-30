#version 460
#extension GL_GOOGLE_include_directive : enable
#include "srgb_utility.glsl"

//! The glyph image produced by Nuklear
layout (binding = 1) uniform sampler2D g_glyph_image;

//! The texture coordinate to use to access the glyph image
layout (location = 0) smooth in vec2 g_tex_coord;

//! The sRGB color with alpha multiplier
layout (location = 1) smooth in vec4 g_color;

//! The left, top, right and bottom of the scissor rectangle in pixels from the
//! left top of the viewport
layout (location = 2) flat in ivec4 g_scissor;


//! The sRGB color and alpha to draw to the screen
layout (location = 0) out vec4 g_out_color;


void main() {
	// Apply the scissor rectangle
	vec4 scissor = vec4(g_scissor);
	if (gl_FragCoord.x < scissor.x || gl_FragCoord.x > scissor.z || gl_FragCoord.y < scissor.y || gl_FragCoord.y > scissor.w)
		discard;
	// Sample the texture and produce the output color
	float alpha = texture(g_glyph_image, g_tex_coord).r;
	g_out_color = vec4(srgb_to_linear_rgb(g_color.rgb), g_color.a * alpha);
}
