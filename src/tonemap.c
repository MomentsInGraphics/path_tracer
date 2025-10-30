#include "tonemap.h"
#include "math_utilities.h"
#include <stdint.h>
#include <math.h>
#include <string.h>


void tonemapper_khronos_pbr_neutral(float* out_color, const float* in_color) {
	float color[3] = { in_color[0], in_color[1], in_color[2] };
	// The end of the brightness range where colors are mapped linearly
	const float start_compression = 0.8f - 0.04f;
	// A parameter controling the strength of desaturation for oversaturated
	// colors
	const float desaturation = 0.15f;
	// Non-linearly compress dark colors to compensate for the Fresnel term
	// in shading models
	float darkest = fminf(color[0], fminf(color[1], color[2]));
	float offset = (darkest < 0.08f) ? (darkest - 6.25f * darkest * darkest) : 0.04f;
	for (uint32_t i = 0; i != 3; ++i)
		color[i] -= offset;
	// If no channel has a value above start_compression, we only use this
	// linear offset
	float brightest = fmaxf(color[0], fmaxf(color[1], color[2]));
	if (brightest < start_compression)
		memcpy(out_color, color, sizeof(color));
	else {
		// Otherwise compress colors using a rational function
		float compressed = 1.0 - start_compression;
		float new_brightest = 1.0 - compressed * compressed / (brightest + compressed - start_compression);
		for (uint32_t i = 0; i != 3; ++i)
			color[i] *= new_brightest / brightest;
		// Desaturate bright colors a bit
		float weight = 1.0 - 1.0 / (desaturation * (brightest - new_brightest) + 1.0);
		for (uint32_t i = 0; i != 3; ++i)
			out_color[i] = color[i] * (1.0f - weight) + new_brightest * weight;
	}
}


void tonemapper_aces(float* out_color, const float* color) {
	// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
	const float aces_in[3 * 3] = {
		0.59719f, 0.35458f, 0.04823f,
		0.07600f, 0.90834f, 0.01566f,
		0.02840f, 0.13383f, 0.83777f
	};
	// ODT_SAT => XYZ => D60_2_D65 => sRGB
	const float aces_out[3 * 3] = {
		 1.60475f, -0.53108f, -0.07367f,
		-0.10208f,  1.10813f, -0.00605f,
		-0.00327f, -0.07276f,  1.07602f
	};
	float v[3];
	mat_vec_mul(v, aces_in, color, 3, 3);
	float w[3];
	for (uint32_t i = 0; i != 3; ++i)
		w[i] = (v[i] * (v[i] + 0.0245786f) - 0.000090537f) / (v[i] * (0.983729f * v[i] + 0.4329510f) + 0.238081f);
	mat_vec_mul(out_color, aces_out, w, 3, 3);
}
