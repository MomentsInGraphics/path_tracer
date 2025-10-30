#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_control_flow_attributes : enable
#include "constants.glsl"
#include "camera_utilities.glsl"

#define M_PI 3.141592653589793238462643

//! The render target containing HDR radiance values
layout (binding = 1, input_attachment_index = 0) uniform subpassInput g_hdr_radiance;


//! The sRGB color and alpha to draw to the screen
layout (location = 0) out vec4 g_out_color;


/*! Applies the Khronos PBR neutral tone mapper to the given Rec. 709 color
	(a.k.a. linear sRGB) and returns a tonemapped Rec. 709 color. For details
	see:
	https://github.com/KhronosGroup/ToneMapping/blob/main/PBR_Neutral/README.md */
vec3 tonemapper_khronos_pbr_neutral(vec3 color) {
	// The end of the brightness range where colors are mapped linearly
	const float start_compression = 0.8 - 0.04;
	// A parameter controling the strength of desaturation for oversaturated
	// colors
	const float desaturation = 0.15;
	// Non-linearly compress dark colors to compensate for the Fresnel term
	// in shading models
	float darkest = min(color.r, min(color.g, color.b));
	float offset = (darkest < 0.08) ? (darkest - 6.25 * darkest * darkest) : 0.04;
	color -= vec3(offset);
	// If no channel has a value above start_compression, we only use this
	// linear offset
	float brightest = max(color.r, max(color.g, color.b));
	if (brightest < start_compression)
		return color;
	else {
		// Otherwise compress colors using a rational function
		float compressed = 1.0 - start_compression;
		float new_brightest = 1.0 - compressed * compressed / (brightest + compressed - start_compression);
		color *= new_brightest / brightest;
		// Desaturate bright colors a bit
		float weight = 1.0 - 1.0 / (desaturation * (brightest - new_brightest) + 1.0);
		return mix(color, vec3(new_brightest), weight);
	}
}


/*! An implementation of the ACES tonemapper using an approximation derived by
	Stephen Hill and found here:
	https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl */
vec3 tonemapper_aces(vec3 color) {
	// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
	const mat3 aces_in = transpose(mat3(
		0.59719, 0.35458, 0.04823,
		0.07600, 0.90834, 0.01566,
		0.02840, 0.13383, 0.83777));
	// ODT_SAT => XYZ => D60_2_D65 => sRGB
	const mat3 aces_out = transpose(mat3(
		 1.60475, -0.53108, -0.07367,
		-0.10208,  1.10813, -0.00605,
		-0.00327, -0.07276,  1.07602));
	vec3 v = aces_in * color;
	vec3 w = (v * (v + 0.0245786) - 0.000090537) / (v * (0.983729 * v + 0.4329510) + 0.238081);
	return aces_out * w;
}


void main() {
	vec4 hdr_radiance = subpassLoad(g_hdr_radiance);
	float factor = g_exposure / float(g_accum_frame_count);
	g_out_color = hdr_radiance * vec4(vec3(factor), hdr_radiance.a);
#if TONEMAPPER_OP_ACES
	g_out_color.rgb = tonemapper_aces(g_out_color.rgb);
#elif TONEMAPPER_OP_KHRONOS_PBR_NEUTRAL
	g_out_color.rgb = tonemapper_khronos_pbr_neutral(g_out_color.rgb);
#endif
	// Color NaN pixels violett
	if (isnan(hdr_radiance.r) || isnan(hdr_radiance.g) || isnan(hdr_radiance.b) || isnan(hdr_radiance.a))
		g_out_color = vec4(1.0, 0.0, 1.0, 1.0);
	// And INF pixels red
	if (isinf(hdr_radiance.r) || isinf(hdr_radiance.g) || isinf(hdr_radiance.b) || isinf(hdr_radiance.a))
		g_out_color = vec4(1.0, 0.0, 0.0, 1.0);
}
