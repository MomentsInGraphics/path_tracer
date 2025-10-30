#include "noise.glsl"
#include "grid_indices.glsl"
#include "volume_quantization.glsl"


//! Expansion of power series for techniques that use one is limited to a
//! maximal degree, because dynamic allocation in shaders is hard and costly
#define MAX_POWER_SERIES_DEGREE 8
//! The maximal number of estimates of optical depth that may be used for
//! transmittance estimation and the like
#if TRANSMITTANCE_ESTIMATOR_UNBIASED_RAY_MARCHING
#define MAX_ESTIMATE_COUNT (MAX_POWER_SERIES_DEGREE + 1)
#elif TRANSMITTANCE_ESTIMATOR_JACKKNIFE || MIS_WEIGHT_ESTIMATOR_JACKKNIFE
#define MAX_ESTIMATE_COUNT 2
#elif TRANSMITTANCE_ESTIMATOR_BIASED_RAY_MARCHING
#define MAX_ESTIMATE_COUNT 1
#else
#define MAX_ESTIMATE_COUNT 1
#endif


/*! The volume texture at the original resolution to which the transfer
	function should be applied or the texture with shaded colors for
	VOLUME_COLOR_MODE_SEPARATE.*/
layout (binding = 1) uniform sampler3D g_volume_texture;
/*! A volume texture holding extinction values (that still need to be
	multiplied by the extinction factor). May be the same as g_volume_texture
	or not depending on whether a transfer function is used.*/
layout (binding = 2) uniform sampler3D g_volume_extinction_texture;
//! An array of volume textures containing various precomputed quantities about
//! g_volume_extinction_texture at lower resolutions
layout (binding = 3) uniform sampler3D g_volume_as[GRID_ATTRIBUTE_COUNT];
//! Dequantization coefficients a,b,c to be used for each of the images in
//! g_volume_as. See get_dequantization_coeffs() for details.
layout (binding = 4) uniform dequantization_coeff_buffer {
	vec3 g_as_dequantization_coeffs[GRID_ATTRIBUTE_COUNT];
};
//! The transfer function used to get colors and opacities for the volume
layout (binding = 5) uniform sampler1D g_transfer_function;


//! Used to count how often the high-resolution volume
//! g_volume_extinction_texture is sampled in total
uint g_volume_sample_count = 0;


//! Attributes that only need to be computed once per ray segment to be able to
//! estimate optical depth efficiently. Use prepare_ray_attribs() to fill it.
struct ray_attribs_t {
#if OPTICAL_DEPTH_ESTIMATOR_KETTUNEN
	//! The number of samples to use per estimate of optical depth
	uint sample_count;
#endif
#if OPTICAL_DEPTH_ESTIMATOR_KETTUNEN || OPTICAL_DEPTH_ESTIMATOR_MEAN_SAMPLING_FIXED_COUNT || OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE_FIXED_COUNT
	//! The exact integral of the unnormalized sampling density (also known as
	//! importance) along the ray
	float total_density;
	//! The lowest and highest ray parameter along the ray at which a non-empty
	//! super-voxel was encountered
	float t_min, t_max;
#else
	//! We need nothing here, but empty structs are illegal in GLSL
	float dummy;
#endif
};


//! Applies the dequantization transform to a quantized value obtained from
//! some quantized volume texture and applies the extinction factor.
float dequantize_extinction(float quantized, vec3 dequantization_coeffs) {
	float dequantized = dequantize_volume(quantized, dequantization_coeffs);
	return g_extinction_factor * max(0.0, dequantized);
}


/*! This function samples the high-resolution version of the volume texture at
	the given position in texel space using interpolation (even when jittering
	is enabled) and returns its value. This is not necessarily an extinction,
	but a valid input for a transfer function.*/
float get_volume_value_interpolated(vec3 position_texel) {
#if VOLUME_INTERPOLATION_NEAREST
	return texelFetch(g_volume_texture, ivec3(position_texel), 0).r;
#elif VOLUME_INTERPOLATION_LINEAR || VOLUME_INTERPOLATION_LINEAR_STOCHASTIC
	return texture(g_volume_texture, position_texel * g_inv_volume_extent, 0).r;
#endif
}


//! Like get_volume_value_interpolated() but samples the volume color. Only
//! meaningful with VOLUME_COLOR_MODE_SEPARATE
vec3 get_volume_color_interpolated(vec3 position_texel) {
#if VOLUME_INTERPOLATION_NEAREST
	return texelFetch(g_volume_texture, ivec3(position_texel), 0).rgb;
#elif VOLUME_INTERPOLATION_LINEAR || VOLUME_INTERPOLATION_LINEAR_STOCHASTIC
	return texture(g_volume_texture, position_texel * g_inv_volume_extent, 0).rgb;
#endif
}


/*! This function samples the high-resolution version of the volume extinction
	at the given position in texel space using appropriate interpolation (but
	no jittering) and does whatever is necessary to convert the result into an
	extinction.*/
float get_volume_extinction(vec3 position_texel) {
	++g_volume_sample_count;
#if VOLUME_INTERPOLATION_NEAREST || VOLUME_INTERPOLATION_LINEAR_STOCHASTIC
	float value = texelFetch(g_volume_extinction_texture, ivec3(position_texel), 0).r;
#elif VOLUME_INTERPOLATION_LINEAR
	float value = texture(g_volume_extinction_texture, position_texel * g_inv_volume_extent, 0).r;
#endif
	return g_extinction_factor * max(0.0, value);
}


//! Like get_volume_extinction() but actually applies jittering when enabled,
//! using the given seed
float get_volume_extinction_jittered(vec3 position_texel, inout uvec3 seed) {
#if VOLUME_INTERPOLATION_LINEAR_STOCHASTIC
	position_texel += pcg_3d(seed) - vec3(0.5);
#endif
	return get_volume_extinction(position_texel);
}


//! Like get_volume_extinction() but uses interpolation even when jittering is
//! supposed to take care of that
float get_volume_extinction_interpolated(vec3 position_texel) {
	++g_volume_sample_count;
#if VOLUME_INTERPOLATION_NEAREST
	float value = texelFetch(g_volume_extinction_texture, ivec3(position_texel), 0).r;
#elif VOLUME_INTERPOLATION_LINEAR || VOLUME_INTERPOLATION_LINEAR_STOCHASTIC
	float value = texture(g_volume_extinction_texture, position_texel * g_inv_volume_extent, 0).r;
#endif
	return g_extinction_factor * max(0.0, value);
}


//! Overload for cases in which linear interpolation is not supported.
float get_volume_extinction(ivec3 position_texel) {
	++g_volume_sample_count;
#if VOLUME_INTERPOLATION_NEAREST || VOLUME_INTERPOLATION_LINEAR_STOCHASTIC
	float value = texelFetch(g_volume_extinction_texture, position_texel, 0).r;
#elif VOLUME_INTERPOLATION_LINEAR
	float value = 0.0;
#endif
	return g_extinction_factor * max(0.0, value);
}


/*! Samples a value from the volume acceleration structure, i.e. from the low-
	resolution grids that store statistics such as minimum, maximum and mean.
	\param low_resolution_voxel The voxel to sample at the low resolution.
	\param grid_index A value such as GRID_INDEX_MAXIMUM. Should be uniform.
	\return The dequantized value.*/
float get_volume_as_value(ivec3 low_resolution_voxel, uint grid_index) {
	float quantized = texelFetch(g_volume_as[grid_index], low_resolution_voxel, 0).r;
	return dequantize_extinction(quantized, g_as_dequantization_coeffs[grid_index]);
}


/*! The initialization procedures of a digital differential analyzer for 3D
	grid traversal.
	\param out_ts_next Ray parameters after making a step along each of the
		coordinate axes.
	\param out_voxel The index of the voxel in which ray_origin resides.
	\param out_inv_dir Component-wise reciprocals of ray_dir.
	\param ray_origin The ray origin in texel space. Must be in range.
	\param ray_dir The ray direction in texel space.*/
void dda_init(out vec3 out_ts_next, out ivec3 out_voxel, out vec3 out_inv_dir, vec3 ray_origin, vec3 ray_dir) {
	out_inv_dir = vec3(1.0) / ray_dir;
	out_voxel = ivec3(ray_origin);
	[[unroll]]
	for (uint i = 0; i != 3; ++i)
		out_ts_next[i] = (((out_inv_dir[i] > 0.0) ? 1.0 : 0.0) + float(out_voxel[i]) - ray_origin[i]) * out_inv_dir[i];
}


/*! An iteration step of a digital differential analyzer for 3D grid traversal.
	\param ts_next Output of dda_init(). Gets updated appropriately.
	\param voxel The current voxel index, which gets updated appropriately.
	\param inv_dir Component-wise reciprocals of the ray direction, as output
		by dda_init().
	\param t_max The maximal permissible ray parameter t.
	\return The ray parameter t for the next sample (which is at the entrance
		point of the voxel with the output index or at t_max).*/
float dda_step(inout vec3 ts_next, inout ivec3 voxel, vec3 inv_dir, float t_max) {
	float t_next = min(min(ts_next.x, ts_next.y), ts_next.z);
	[[unroll]]
	for (uint i = 0; i != 3; ++i) {
		if (ts_next[i] == t_next) {
			ts_next[i] += abs(inv_dir[i]);
			voxel[i] += (inv_dir[i] > 0.0) ? 1 : -1;
		}
	}
	return min(t_next, t_max);
}


/*! Performs delta tracking to sample a free-flight distance.
	\param out_null_probability_product Internally, the method produces null-
		scattering events. Each of these events has had a known probability to
		become null and this is the product of these probabilities. Only needed
		to implement this technique: https://doi.org/10.1145/3306346.3323025
	\param ray_origin The ray origin in texel space of the high-resolution
		volume.
	\param ray_dir The ray direction in texel space.
	\param t_max The ray parameter at which the ray ends. It starts at 0.
	\param rand Source of random numbers.
	\return The sampled ray parameter. If no real scattering event was
		generated, it will return t_max.*/
float delta_tracking(out float out_null_probability_product, vec3 ray_origin, vec3 ray_dir, float t_max, inout random_generator rand) {
	float t = 0.0;
	out_null_probability_product = 1.0;
	// Generate a seed for PCG 3D with which to produce jitters. May be unused.
	uvec3 seed = uvec3(rand.seed, rand.seed[1] * 747796405u + 2891336453u);
	// Perform DDA traversal on the bounds
	vec3 inv_dir, ts_next, prev_ts_next;
	ivec3 voxel, prev_voxel;
	dda_init(ts_next, voxel, inv_dir, ray_origin * g_inv_brick_size, ray_dir * g_inv_brick_size);
	while (t < t_max) {
		// We perform distance sampling on the bounds
		vec2 randoms = get_random_numbers(rand);
		float target_transmittance = 1.0 - randoms[0];
		float target_optical_depth = -log(target_transmittance);
		float optical_depth = 0.0;
		float extinction_bound;
		while (t < t_max && optical_depth <= target_optical_depth) {
			extinction_bound = get_volume_as_value(voxel, GRID_INDEX_MAXIMUM);
			prev_ts_next = ts_next;
			prev_voxel = voxel;
			float t_next = dda_step(ts_next, voxel, inv_dir, t_max);
			optical_depth += (t_next - t) * extinction_bound;
			t = t_next;
		}
		if (optical_depth > target_optical_depth) {
			// Rewind to where the optical depth matches exactly
			t -= (optical_depth - target_optical_depth) / extinction_bound;
			ts_next = prev_ts_next;
			voxel = prev_voxel;
			// Sample the exact extinction and compute the probability of
			// acceptance
			float extinction = get_volume_extinction_jittered(ray_origin + t * ray_dir, seed);
			float prob = extinction / extinction_bound;
			// Decide whether the ray terminates there
			if (randoms[1] < prob)
				return t;
			else
				out_null_probability_product *= 1.0 - prob;
		}
	}
	return t_max;
}


/*! Performs ratio tracking to estimate transmittance along a ray segment.
	Parameters are analogue to delta_tracking(). Returns an unbiased
	transmittance estimate that is always between 0 and 1.
	https://doi.org/10.1145/2661229.2661292 */
float ratio_tracking(vec3 ray_origin, vec3 ray_dir, float t_max, inout random_generator rand) {
	float transmittance = 1.0;
	float t = 0.0;
	// Generate a seed for PCG 3D with which to produce jitters. May be unused.
	uvec3 seed = uvec3(rand.seed, rand.seed[1] * 747796405u + 2891336453u);
	// Perform DDA traversal on the bounds
	vec3 inv_dir, ts_next, prev_ts_next;
	ivec3 voxel, prev_voxel;
	dda_init(ts_next, voxel, inv_dir, ray_origin * g_inv_brick_size, ray_dir * g_inv_brick_size);
	while (t < t_max) {
		// We perform distance sampling on the bounds
		vec2 randoms = get_random_numbers(rand);
		float target_transmittance = 1.0 - randoms[0];
		float target_optical_depth = -log(target_transmittance);
		float optical_depth = 0.0;
		float extinction_bound;
		while (t < t_max && optical_depth <= target_optical_depth) {
			extinction_bound = get_volume_as_value(voxel, GRID_INDEX_MAXIMUM);
			prev_ts_next = ts_next;
			prev_voxel = voxel;
			float t_next = dda_step(ts_next, voxel, inv_dir, t_max);
			optical_depth += (t_next - t) * extinction_bound;
			t = t_next;
		}
		if (optical_depth > target_optical_depth) {
			// Rewind to where the optical depth matches exactly
			t -= (optical_depth - target_optical_depth) / extinction_bound;
			ts_next = prev_ts_next;
			voxel = prev_voxel;
			// Sample the exact extinction and compute the probability of
			// acceptance
			float extinction = get_volume_extinction_jittered(ray_origin + t * ray_dir, seed);
			float prob = extinction / extinction_bound;
			// Accumulate the ratio
			transmittance *= 1.0 - prob;
		}
	}
	return transmittance;
}


/*! Samples a distance for a ray passing through the volume proportional to
	extinction times transmittance. That is known as free-flight distance
	sampling.
	\param ray_origin The origin of the ray in texel space. That means integer
		coordinates pertain to boundaries of a voxel and the volume spans the
		range [0,width]x[0,depth]x[0,height]. Assumed to be in range.
	\param ray_dir The ray direction in texel space. It must be normalized in
		such a way that its length in world space (not texel space) is one.
	\param t_max The considered line segment ends at
		ray_origin + t_max * ray_dir (which is also assumed to be in range).
	\param rand Source of random numbers for Monte Carlo methods.
	\return The sampled distance as ray parameter in [0, t_max]. A value of
		t_max indicates that no scattering event has been sampled.*/
float sample_volume_distance(vec3 ray_origin, vec3 ray_dir, float t_max, inout random_generator rand) {
#if DISTANCE_SAMPLER_RAY_MARCHING
	// Generate a seed for PCG 3D with which to produce jitters. May be unused.
	uvec3 seed = uvec3(rand.seed, rand.seed[1] * 747796405u + 2891336453u);
	// Decide what steps to take
	float t_step = t_max * g_inv_ray_march_sample_count;
	vec3 ray_step = ray_dir * t_step;
	vec2 randoms = get_random_numbers(rand);
	float jitter = randoms[0];
	ray_origin += jitter * ray_step;
	// Decide what optical depth we want in the end
	float target_optical_depth = -log(1.0 - randoms[1]);
	float target_extinction_sum = target_optical_depth / t_step;
	// Perform the ray marching
	float extinction_sum = 0.0;
	uint i = 0;
	float sample_extinction;
	for (; i != g_ray_march_sample_count && extinction_sum < target_extinction_sum; ++i) {
		vec3 sample_pos = ray_origin + float(i) * ray_step;
		sample_extinction = get_volume_extinction_jittered(sample_pos, seed);
		extinction_sum += sample_extinction;
	}
	// Rewind to the sample location or indicate that no scattering event has
	// been sampled
	if (extinction_sum >= target_extinction_sum)
		return (float(i) - (extinction_sum - target_extinction_sum) / sample_extinction) * t_step;
	else
		return t_max;

#elif DISTANCE_SAMPLER_REGULAR_TRACKING
	vec3 inv_dir, ts_next;
	ivec3 voxel;
	dda_init(ts_next, voxel, inv_dir, ray_origin, ray_dir);
	float target_optical_depth = -log(1.0 - get_random_numbers(rand)[0]);
	float optical_depth = 0.0;
	float t = 0.0;
	float extinction;
	while (t < t_max && optical_depth <= target_optical_depth) {
		extinction = get_volume_extinction(voxel);
		float t_next = dda_step(ts_next, voxel, inv_dir, t_max);
		// Accumulate the contribution and proceed
		optical_depth += (t_next - t) * extinction;
		t = t_next;
	}
	// Rewind to the sample location or indicate that no scattering event has
	// been sampled
	if (optical_depth > target_optical_depth)
		return t - (optical_depth - target_optical_depth) / extinction;
	else
		return t_max;

#elif DISTANCE_SAMPLER_DELTA_TRACKING
	float unused;
	return delta_tracking(unused, ray_origin, ray_dir, t_max, rand);
#endif
	return t_max;
}


//! The method used by Kettunen et al. [2021] to select a ray marching sample
//! count
uint get_ray_marching_sample_count(float control_optical_depth) {
	// Evaluate a cubic polynomial with roots -0.015, -0.65, -60.3 using
	// Horner's method
	float x = control_optical_depth;
	float poly = fma(fma(fma(1.0, x, 60.965), x, 40.10925), x, 0.587925);
	uint N_CMF = uint(ceil(pow(poly, 1.0 / 3.0)));
	float N_BK = 0.31945;
	return max(1, uint(round(g_kettunen_sample_multiplier * (N_CMF / (N_BK + 1)))));
}


/*! Performs aggressive Bhanot & Kennedy roulette as described by Kettunen et
	al. [2021]. Returns the expansion order (at most MAX_POWER_SERIES_DEGREE)
	and outputs probabilities for expansion to at least a given order for all
	orders up to the returned expansion order.*/
uint get_power_series_degree(out float out_probabilities[MAX_POWER_SERIES_DEGREE + 1], float p_Z, inout random_generator rand) {
	// Use an order-zero expansion with probability p_Z
	out_probabilities[0] = 1.0;
	float P = 1.0 - p_Z;
	float u = get_random_numbers(rand)[0];
	if (P <= u)
		return 0;
	// Otherwise, expand at least to order K
	const uint K = min(2, MAX_POWER_SERIES_DEGREE);
	const float c = 2.0;
	[[unroll]]
	for (uint k = 1; k != K + 1; ++k)
		out_probabilities[k] = P;
	// Beyond that, use BK-Russian roulette
	[[unroll]]
	for (uint k = K + 1; k < MAX_POWER_SERIES_DEGREE + 1; ++k) {
		float c_k = min(1.0, c / float(k));
		P *= c_k;
		if (P <= u)
			return k - 1;
		out_probabilities[k] = P;
	}
	return MAX_POWER_SERIES_DEGREE;
}


/*! Any technique that requires estimates of optical depth (i.e. several
	techniques for transmittance and MIS weight estimation) requires a few
	preparatory steps. This function performs these steps and outputs the
	intermediate results so that they can be reused.*/
ray_attribs_t prepare_ray_attribs(vec3 ray_origin, vec3 ray_dir, float t_max) {
	ray_attribs_t attribs;
#if OPTICAL_DEPTH_ESTIMATOR_KETTUNEN || OPTICAL_DEPTH_ESTIMATOR_MEAN_SAMPLING_FIXED_COUNT || OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE_FIXED_COUNT
	// Initialize the relevant ray interval with extreme values
	attribs.t_min = t_max;
	attribs.t_max = 0.0;
	// Perform regular tracking at the low resoution
	vec3 inv_dir, ts_next;
	ivec3 voxel;
	dda_init(ts_next, voxel, inv_dir, ray_origin * g_inv_brick_size, ray_dir * g_inv_brick_size);
	attribs.total_density = 0.0;
	float t = 0.0;
	while (t < t_max) {
		// Sample the current voxel
#if OPTICAL_DEPTH_ESTIMATOR_KETTUNEN || OPTICAL_DEPTH_ESTIMATOR_MEAN_SAMPLING_FIXED_COUNT
		float density = get_volume_as_value(voxel, GRID_INDEX_MEAN);
#elif OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE_FIXED_COUNT
		float density = get_volume_as_value(voxel, GRID_INDEX_SQRT_MOMENT_2);
#endif
		// Update the ray start if the voxel is non-empty
		attribs.t_min = (density != 0.0) ? min(attribs.t_min, t) : attribs.t_min;
		// Advance traversal
		float t_next = dda_step(ts_next, voxel, inv_dir, t_max);
		float t_step = t_next - t;
		t = t_next;
		// Update the ray end if the voxel is non-empty
		attribs.t_max = (density != 0.0) ? max(attribs.t_max, t) : attribs.t_max;
		// Accumulate the contributions
		attribs.total_density += t_step * density;
	}
#if OPTICAL_DEPTH_ESTIMATOR_KETTUNEN
	// Estimate the number of ray marching samples to use
	attribs.sample_count = get_ray_marching_sample_count(attribs.total_density);
#endif
#endif
	return attribs;
}


//! Finds the minimal value among the first count values in the given array
//! returns it and outputs its index
float find_min(out uint out_arg_min, float values[MAX_ESTIMATE_COUNT], uint count) {
	out_arg_min = 0;
	float minimum = values[0];
	[[unroll]]
	for (uint i = 1; i < MAX_ESTIMATE_COUNT; ++i) {
		bool is_min = (i < count && values[i] < minimum);
		out_arg_min = is_min ? i : out_arg_min;
		minimum = is_min ? values[i] : minimum;
	}
	return minimum;
}


/*! Produces multiple estimates of optical depth using a version of ray
	marching with importance sampling.
	\param out_optical_depths The requested estimates are written to the start
		of this array. The estimates are independent and identically
		distributed.
	\param estimate_count The number of estimates to produce. At most
		MAX_ESTIMATE_COUNT. Only indices less than this count are used.
	\param ray_origin The ray origin in texel space of the high-resolution
		volume.
	\param ray_dir The ray direction in texel space.
	\param t_max The ray parameter at which the ray ends. It starts at 0.
	\param ray_attribs The output of
		prepare_ray_attribs(ray_origin, ray_dir, t_max).
	\param rand Jittering and volume interpolation may be using random
		numbers.*/
void estimate_optical_depth(out float out_optical_depths[MAX_ESTIMATE_COUNT], uint estimate_count, vec3 ray_origin, vec3 ray_dir, float t_max, ray_attribs_t ray_attribs, inout random_generator rand) {
	[[branch]]
	if (estimate_count == 0)
		return;
	// For each estimate, this array tracks the value of the normalized CDF at
	// which the next sample should be taken. Normalization is done such that
	// one sample per increment of 1 in the CDF is expected.
	float next_normd_cdfs[MAX_ESTIMATE_COUNT];
	[[unroll]]
	for (uint i = 1; i < MAX_ESTIMATE_COUNT; ++i)
		next_normd_cdfs[i] = 1.0;
	[[unroll]]
	for (uint i = 0; i < MAX_ESTIMATE_COUNT; i += 2) {
		if (i < estimate_count) {
			vec2 rands = get_random_numbers(rand);
			next_normd_cdfs[i] = rands[0];
			if (i + 1 < MAX_ESTIMATE_COUNT)
				next_normd_cdfs[i + 1] = (i + 1 < estimate_count) ? rands[1] : 1.0;
		}
	}
#if USE_JITTERED
	// Produce a seed for cheap random number generation
	uint seed_1d = rand.seed[0];
	pcg(seed_1d);
#endif
	// Generate a seed for PCG 3D with which to produce jitters. May be unused.
	uvec3 seed = uvec3(rand.seed, rand.seed[1] * 747796405u + 2891336453u);

#if OPTICAL_DEPTH_ESTIMATOR_EQUIDISTANT
	// Initialize outputs
	[[unroll]]
	for (uint i = 0; i != MAX_ESTIMATE_COUNT; ++i)
		out_optical_depths[i] = 0.0;
	// Make the same number of ray marching steps for each estimate
	float ray_step = t_max * g_inv_ray_march_sample_count;
	for (uint i = 0; i != g_ray_march_sample_count; ++i) {
		[[unroll]]
		for (uint j = 0; j != MAX_ESTIMATE_COUNT; ++j) {
			if (j < estimate_count) {
				float sample_t = next_normd_cdfs[j] * ray_step;
				vec3 sample_pos = ray_origin + sample_t * ray_dir;
				out_optical_depths[j] = fma(ray_step, get_volume_extinction_jittered(sample_pos, seed), out_optical_depths[j]);
#if USE_JITTERED
				next_normd_cdfs[j] = ceil(next_normd_cdfs[j]) + pcg(seed_1d);
#elif USE_UNIFORM_JITTERED
				next_normd_cdfs[j] += 1.0;
#endif
			}
		}
	}

#elif OPTICAL_DEPTH_ESTIMATOR_KETTUNEN || OPTICAL_DEPTH_ESTIMATOR_MEAN_SAMPLING || OPTICAL_DEPTH_ESTIMATOR_MEAN_SAMPLING_FIXED_COUNT || OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE || OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE_FIXED_COUNT
	// Initialize outputs
	[[unroll]]
	for (uint i = 0; i != MAX_ESTIMATE_COUNT; ++i)
		out_optical_depths[i] = 0.0;
#if OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE || OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE_FIXED_COUNT
	float control_optical_depth = 0.0;
#endif
#if OPTICAL_DEPTH_ESTIMATOR_KETTUNEN || OPTICAL_DEPTH_ESTIMATOR_MEAN_SAMPLING_FIXED_COUNT || OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE_FIXED_COUNT
	// We have ensured that we do not have zero density in non-zero regions of
	// the volume. If the density is all zero, so is the optical depth.
	[[branch]]
	if (ray_attribs.total_density <= 0.0)
		return;
	// Skip empty regions at the start and the end of the ray
	ray_origin += ray_attribs.t_min * ray_dir;
	t_max = ray_attribs.t_max - ray_attribs.t_min;
#endif
	// Decide what the integral of the density between two samples for the same
	// estimate should be (in expectation)
#if OPTICAL_DEPTH_ESTIMATOR_KETTUNEN
	float cdf_step = ray_attribs.total_density / float(ray_attribs.sample_count);
#elif OPTICAL_DEPTH_ESTIMATOR_MEAN_SAMPLING_FIXED_COUNT || OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE_FIXED_COUNT
	float cdf_step = ray_attribs.total_density * g_inv_ray_march_sample_count;
#elif OPTICAL_DEPTH_ESTIMATOR_MEAN_SAMPLING || OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE
	float cdf_step = g_optical_depth_cdf_step;
#endif
	// Initialize the iteration over all required samples
	uint next_estimate;
	float next_cdf = find_min(next_estimate, next_normd_cdfs, estimate_count) * cdf_step;
	// Perform DDA traversal for the low-resolution grid
	vec3 ts_next, inv_dir;
	ivec3 voxel;
	dda_init(ts_next, voxel, inv_dir, ray_origin * g_inv_brick_size, ray_dir * g_inv_brick_size);
	float current_cdf = 0.0, t = 0.0, t_next = 0.0, density = 0.0;
#if OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE || OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE_FIXED_COUNT
	float minimum = 0.0;
#endif
	[[loop]]
	while (next_cdf < current_cdf || t_next < t_max) {
		// Traverse the low-resolution grid until the CDF exceeds the CDF at
		// the next sample or until the end of the ray
		[[loop]]
		while (next_cdf >= current_cdf && t_next < t_max) {
			t = t_next;
			// Determine the size of the upcoming step
			t_next = min(min(min(t_max, ts_next.x), ts_next.y), ts_next.z);
			float t_step = t_next - t;
			// Sample relevant quantities for the brick, determine the density
			// and update the control variate
#if OPTICAL_DEPTH_ESTIMATOR_KETTUNEN || OPTICAL_DEPTH_ESTIMATOR_MEAN_SAMPLING || OPTICAL_DEPTH_ESTIMATOR_MEAN_SAMPLING_FIXED_COUNT
			density = get_volume_as_value(voxel, GRID_INDEX_MEAN);
#elif OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE || OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE_FIXED_COUNT
			density = get_volume_as_value(voxel, GRID_INDEX_SQRT_MOMENT_2);
			minimum = get_volume_as_value(voxel, GRID_INDEX_MINIMUM);
			control_optical_depth = fma(t_step, minimum, control_optical_depth);
#endif
			current_cdf = fma(t_step, density, current_cdf);
			// Advance the low-resolution DDA traversal
			dda_step(ts_next, voxel, inv_dir, t_max);
		}
		// If we are not done with high-resolution samples yet
		[[branch]]
		if (next_cdf < current_cdf) {
			// Rewind a bit and take a sample at the high resolution
			float sample_t = t_next - (current_cdf - next_cdf) / density;
			vec3 sample_pos = ray_origin + sample_t * ray_dir;
			float extinction = get_volume_extinction_jittered(sample_pos, seed);
			// Contribute to the Monte Carlo estimate of optical depth
#if OPTICAL_DEPTH_ESTIMATOR_KETTUNEN || OPTICAL_DEPTH_ESTIMATOR_MEAN_SAMPLING || OPTICAL_DEPTH_ESTIMATOR_MEAN_SAMPLING_FIXED_COUNT
			float contribution = extinction;
#elif OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE || OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE_FIXED_COUNT
			float contribution = extinction - minimum;
#endif
			contribution /= density;
			// Update the estimate and increment the normalized CDF without
			// causing register spilling
#if USE_JITTERED
			float jitter = pcg(seed_1d);
#endif
			[[unroll]]
			for (uint i = 0; i != MAX_ESTIMATE_COUNT; ++i) {
				if (MAX_ESTIMATE_COUNT == 1 || i == next_estimate) {
					out_optical_depths[i] = fma(contribution, cdf_step, out_optical_depths[i]);
#if USE_JITTERED
					next_normd_cdfs[i] = ceil(next_normd_cdfs[i]) + jitter;
#elif USE_UNIFORM_JITTERED
					next_normd_cdfs[i] += 1.0;
#endif
				}
			}
			// Advance to the next sample/estimate
			next_cdf = find_min(next_estimate, next_normd_cdfs, estimate_count) * cdf_step;
		}
	}
	// Add the control optical depth
#if OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE || OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE_FIXED_COUNT
	[[unroll]]
	for (uint i = 0; i != MAX_ESTIMATE_COUNT; ++i)
		out_optical_depths[i] += control_optical_depth;
#endif
#endif
}


//! Computes elementary means using the algorithm of Kettunen et al. [2021]
void compute_elementary_means(out float out_means[MAX_POWER_SERIES_DEGREE + 1], float inputs[max(1, MAX_POWER_SERIES_DEGREE)], uint input_count) {
	out_means[0] = 1.0;
	[[unroll]]
	for (uint i = 1; i != MAX_POWER_SERIES_DEGREE + 1; ++i)
		out_means[i] = 0.0;
	[[unroll]]
	for (uint n = 0; n != MAX_POWER_SERIES_DEGREE; ++n)
		if (n < input_count)
			[[unroll]]
			for (uint k = n + 1; k != 0; --k)
				out_means[k] += float(k) / float(n + 1) * (out_means[k - 1] * inputs[n] - out_means[k]);
}


//! An implementation of transmittance estimation using unbiased ray marching
//! as described by Kettunen et al. [2021].
float estimate_transmittance_unbiased_ray_marching(vec3 ray_origin, vec3 ray_dir, float t_max, ray_attribs_t ray_attribs, inout random_generator rand) {
	// Decide on an expansion order
	float degree_probabilities[MAX_POWER_SERIES_DEGREE + 1];
	uint degree = get_power_series_degree(degree_probabilities, 0.9, rand);
	// Generate the required number of independent estimates of optical depth
	float optical_depths[MAX_ESTIMATE_COUNT];
	estimate_optical_depth(optical_depths, degree + 1, ray_origin, ray_dir, t_max, ray_attribs, rand);
	// Evaluate the power series with U-statistics
	float factorials[] = { 1.0, 1.0, 2.0, 6.0, 24.0, 120.0, 720.0, 5040.0, 40320.0, 362880.0, 3628800.0, 39916800.0, 479001600.0, 6227020800.0, 87178291200.0, 1307674368000.0, 20922789888000.0, 355687428096000.0, 6402373705728000.0, 121645100408832000.0, 2432902008176640000.0, 51090942171709440000.0, 1124000727777607680000.0, 25852016738884976640000.0 };
	float transmittance = 0.0;
	[[unroll]]
	for (uint i = 0; i != MAX_ESTIMATE_COUNT; ++i) {
		if (i <= degree) {
			float offset_estimates[max(1, MAX_POWER_SERIES_DEGREE)];
			[[unroll]]
			for (uint j = 0; j != MAX_POWER_SERIES_DEGREE; ++j)
				offset_estimates[j] = optical_depths[i] - optical_depths[j + ((i <= j) ? 1 : 0)];
			float elementary_means[MAX_POWER_SERIES_DEGREE + 1];
			compute_elementary_means(elementary_means, offset_estimates, degree);
			float transmittance_term = 0.0;
			[[unroll]]
			for (uint j = 0; j != MAX_POWER_SERIES_DEGREE + 1; ++j)
				if (j <= degree)
					transmittance_term += elementary_means[j] / (factorials[j] * degree_probabilities[j]);
			transmittance += exp(-optical_depths[i]) * transmittance_term;
		}
	}
	transmittance /= float(degree + 1);
	return transmittance;
}


/*! Estimates the transmittance for a ray passing through the volume.
	Parameters are mostly as for sample_volume_distance(). ray_attribs is
	output of prepare_ray_attribs(ray_origin, ray_dir, t_max). Returns a
	transmittance estimate with an expected value in [0,1].*/
float estimate_volume_transmittance(vec3 ray_origin, vec3 ray_dir, float t_max, ray_attribs_t ray_attribs, inout random_generator rand) {
#if TRANSMITTANCE_ESTIMATOR_DUMMY
	return 1.0;

#elif TRANSMITTANCE_ESTIMATOR_UNBIASED_RAY_MARCHING
	return estimate_transmittance_unbiased_ray_marching(ray_origin, ray_dir, t_max, ray_attribs, rand);

#elif TRANSMITTANCE_ESTIMATOR_JACKKNIFE
	// We use the UMVU estimate with two samples
	float optical_depths[MAX_ESTIMATE_COUNT];
	estimate_optical_depth(optical_depths, 2, ray_origin, ray_dir, t_max, ray_attribs, rand);
	float mean = 0.5 * (optical_depths[0] + optical_depths[1]);
	float std = abs(0.5 * (optical_depths[0] - optical_depths[1]));
	return cos(std) * exp(-mean);

#elif TRANSMITTANCE_ESTIMATOR_BIASED_RAY_MARCHING
	float optical_depths[MAX_ESTIMATE_COUNT];
	estimate_optical_depth(optical_depths, 1, ray_origin, ray_dir, t_max, ray_attribs, rand);
	return exp(-optical_depths[0]);

#elif TRANSMITTANCE_ESTIMATOR_REGULAR_TRACKING
	vec3 inv_dir, ts_next;
	ivec3 voxel;
	dda_init(ts_next, voxel, inv_dir, ray_origin, ray_dir);
	float optical_depth = 0.0;
	float t = 0.0;
	while (t < t_max) {
		float extinction = get_volume_extinction(voxel);
		float t_next = dda_step(ts_next, voxel, inv_dir, t_max);
		// Accumulate the contribution and proceed
		optical_depth += (t_next - t) * extinction;
		t = t_next;
	}
	return exp(-optical_depth);

#elif TRANSMITTANCE_ESTIMATOR_TRACK_LENGTH
	float unused;
	float t = delta_tracking(unused, ray_origin, ray_dir, t_max, rand);
	return (t >= t_max) ? 1.0 : 0.0;

#elif TRANSMITTANCE_ESTIMATOR_RATIO_TRACKING
	return ratio_tracking(ray_origin, ray_dir, t_max, rand);
#endif
}


/*! This function generalizes transmittance estimation, MIS weight computation
	and combinations of the two. For the given ray (in texel space), it
	provides an estimate (biased or unbiased depending on the used technique)
	of:
	(numerator.x * transmittance + numerator.y) / (denominator.x * transmittance + denominator.y) */
float estimate_volume_mis_weight(vec3 ray_origin, vec3 ray_dir, float t_max, ray_attribs_t ray_attribs, vec2 numerator, vec2 denominator, inout random_generator rand) {
#if MIS_WEIGHT_ESTIMATOR_DUMMY
	return numerator.y / denominator.y;

#elif MIS_WEIGHT_ESTIMATOR_UNBIASED_RAY_MARCHING
	return estimate_mis_weight_unbiased_ray_marching(ray_origin, ray_dir, t_max, ray_attribs, numerator, denominator, rand);

#elif MIS_WEIGHT_ESTIMATOR_BIASED_RAY_MARCHING
	float optical_depths[MAX_ESTIMATE_COUNT];
	estimate_optical_depth(optical_depths, 1, ray_origin, ray_dir, t_max, ray_attribs, rand);
	float transmittance = exp(-optical_depths[0]);
	return (numerator.x * transmittance + numerator.y) / (denominator.x * transmittance + denominator.y);

#elif MIS_WEIGHT_ESTIMATOR_JACKKNIFE
	float optical_depths[MAX_ESTIMATE_COUNT];
	estimate_optical_depth(optical_depths, 2, ray_origin, ray_dir, t_max, ray_attribs, rand);
	float mean = 0.5 * (optical_depths[0] + optical_depths[1]);
	float std = abs(0.5 * (optical_depths[0] - optical_depths[1]));
	float transmittance = exp(-mean);
	float cos_std_trans = cos(std) * transmittance;
	float sin_std_trans = sin(std) * transmittance;
	// Real part
	vec2 num, denom;
	num.x = fma(numerator.x, cos_std_trans, numerator.y);
	denom.x = fma(denominator.x, cos_std_trans, denominator.y);
	// Imaginary part
	num.y = numerator.x * sin_std_trans;
	denom.y = denominator.x * sin_std_trans;
	return (num.x * denom.x + num.y * denom.y) / dot(denom, denom);

#elif MIS_WEIGHT_ESTIMATOR_REGULAR_TRACKING
	vec3 inv_dir, ts_next;
	ivec3 voxel;
	dda_init(ts_next, voxel, inv_dir, ray_origin, ray_dir);
	float optical_depth = 0.0, t = 0.0, extinction = 0.0;
	while (t < t_max) {
		extinction = get_volume_extinction(voxel);
		float t_next = dda_step(ts_next, voxel, inv_dir, t_max);
		// Accumulate the contribution and proceed
		optical_depth += (t_next - t) * extinction;
		t = t_next;
	}
	float transmittance = exp(-optical_depth);
	return (numerator.x * transmittance + numerator.y) / (denominator.x * transmittance + denominator.y);
#endif
}
