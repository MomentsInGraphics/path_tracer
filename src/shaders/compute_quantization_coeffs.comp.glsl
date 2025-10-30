#version 460
#extension GL_GOOGLE_include_directive : enable
#include "volume_quantization.glsl"


//! The buffer to which quantization coefficients are stored
layout (binding = 0) buffer writeonly quantization_coeff_output {
	vec4 g_dequantization_coeffs[3];
};
//! The buffers from which stats are read
layout (binding = 1) buffer readonly stat_reduce_output_0 {
	vec4 g_stats_0;
};
layout (binding = 2) buffer readonly stat_reduce_output_1 {
	vec4 g_stats_1;
};
layout (binding = 3) buffer readonly stat_reduce_output_2 {
	vec4 g_stats_2;
};
layout (binding = 4) buffer readonly stat_reduce_output_3 {
	vec4 g_stats_3;
};
layout (binding = 5) buffer readonly stat_reduce_output_4 {
	vec4 g_stats_4;
};


//! This shader runs briefly, but with very few threads
layout (local_size_x = GRID_ATTRIBUTE_COUNT, local_size_y = 1, local_size_z = 1) in;


/*! Optimizes the scaling factor inside the exponential, which is to be used
	for quantization of extinction values.
	\param minimum The minimal value that is to be quantized.
	\param maximum The maximal value that is to be quantized.
	\param moments Power moments of degree 0, 1 and 2 for the distribution of
		values. For normalized distributions, moments[0] == 1.0.
	\param lerp A parameter between 0 and 1 that controls how aggressively
		large values will be compressed. 0.6 is a reasonable default.
	\return The optimal scaling factor inside the exponential or 0.0 if lineqr
		quantization should be used.*/
float optimize_exponent_factor(float minimum, float maximum, vec3 moments, float lerp) {
	float b0 = moments[0], b1 = moments[1], b2 = moments[2];
	float center = (b1 * minimum - b2) / (b0 * minimum - b1);
	float range = maximum - minimum;
	float offset = center - minimum;
	// Find the critical point of the objective function. If it is not
	// negative, it is best to use linear quantization.
	float critical_a = log(lerp * range / offset) / (center - maximum);
	if (critical_a >= 0.0)
		return 0.0;
	// Use bisection to find the optimal parameter
	float left = log(1.0 - lerp) / offset;
	float right = critical_a;
	while (right - left > abs(left) * 1.0e-6) {
		float fac = 0.5 * (right + left);
		bool fac_too_small = (exp(fac * offset) - lerp * exp(fac * range) < 1.0 - lerp);
		left = fac_too_small ? fac : left;
		right = fac_too_small ? right : fac;
	}
	float fac = 0.5 * (right + left);
	// Tiny values in the exponential (in magnitude) are prone to cancelation,
	// so we fall back to linear quantization then
	return (fac * range < 1.0e-2) ? 0.0 : fac;
}


/*! Uses the given statistics (see optimize_exponent_factor()) to compute
	quantization coefficients, which turn volume values into somewhat uniformly
	distributed values between 0 and 1 suitable for fixed-point quantization.*/
vec3 get_quantization_coeffs(float minimum, float maximum, vec3 moments) {
	float exponent_factor = optimize_exponent_factor(minimum, maximum, moments, 0.6);
	float mapped_min = (exponent_factor == 0.0) ? minimum : exp(exponent_factor * minimum);
	float mapped_max = (exponent_factor == 0.0) ? maximum : exp(exponent_factor * maximum);
	float factor = 1.0 / (mapped_max - mapped_min);
	float summand = -mapped_min * factor;
	return vec3(exponent_factor, factor, summand);
}


void main() {
	// Read the stats describing the grid
	vec4 stats;
	if (gl_GlobalInvocationID.x == 0) stats = g_stats_0;
	else if (gl_GlobalInvocationID.x == 1) stats = g_stats_1;
	else if (gl_GlobalInvocationID.x == 2) stats = g_stats_2;
	else if (gl_GlobalInvocationID.x == 3) stats = g_stats_3;
	else if (gl_GlobalInvocationID.x == 4) stats = g_stats_4;
	float minimum = stats[0];
	float maximum = stats[1];
	vec3 moments = vec3(BRICK_COUNT, stats[2], stats[3]);
	// Optimize the quantization coefficients
	vec3 quantization_coeffs = get_quantization_coeffs(minimum, maximum, moments);
	vec3 dequantization_coeffs = get_dequantization_coeffs(quantization_coeffs);
	g_dequantization_coeffs[gl_GlobalInvocationID.x] = vec4(dequantization_coeffs, 0.0);
}
