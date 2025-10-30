#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_control_flow_attributes : enable
#extension GL_EXT_ray_query : enable
#include "constants.glsl"
#include "camera_utilities.glsl"
#include "volume_utilities.glsl"
#include "phase_functions.glsl"

#define M_PI 3.141592653589793238462643

//! The light probe used to illuminate the scene
layout (binding = 6) uniform sampler2D g_light_probe;
//! The alias table used for importance sampling of the light probe
layout (binding = 7) uniform utextureBuffer g_light_probe_alias_table;


//! The index of the current sample within a frame
layout (location = 0) flat in uint g_frame_sample_index;

//! The outgoing radiance towards the camera as sRGB color
layout (location = 0) out vec4 g_out_color;


//! The inverse of the error function (used to sample Gaussians).
float erfinv(float x) {
	// From https://people.maths.ox.ac.uk/gilesm/files/gems_erfinv.pdf
	float w = -log(max(1.0e-37, 1.0 - x * x));
	float a = w - 2.5;
	float b = sqrt(w) - 3.0;
	return x * ((w < 5.0)
		? fma(fma(fma(fma(fma(fma(fma(fma(2.81022636e-08, a, 3.43273939e-07), a, -3.5233877e-06), a, -4.39150654e-06), a, 0.00021858087), a, -0.00125372503), a, -0.00417768164), a, 0.246640727), a, 1.50140941)
		: fma(fma(fma(fma(fma(fma(fma(fma(-0.000200214257, b, 0.000100950558), b, 0.00134934322), b, -0.00367342844), b, 0.00573950773), b, -0.0076224613), b, 0.00943887047), b, 1.00167406), b, 2.83297682));
}


//! Given a texel space coordinate, this function returns the albedo (i.e.
//! color) to use for the volume at this location
vec3 get_volume_color(vec3 position_texel) {
#if VOLUME_COLOR_MODE_CONSTANT
	return g_volume_color_constant;
#elif VOLUME_COLOR_MODE_TRANSFER_FUNCTION || VOLUME_COLOR_MODE_TRANSFER_FUNCTION_WITH_EXTINCTION
	float dequantized = get_volume_value_interpolated(position_texel);
	return texture(g_transfer_function, dequantized * g_transfer_input_factor + g_transfer_input_summand).rgb;
#elif VOLUME_COLOR_MODE_SEPARATE
	return get_volume_color_interpolated(position_texel);
#endif
}


//! Retrieves the ambient radiance for the given normalized direction from the
//! light probe
vec3 get_ambient_radiance(vec3 world_space_direction) {
	float inclination = acos(world_space_direction.z);
	float azimuth = atan(world_space_direction.y, world_space_direction.x);
	vec2 uv = vec2(azimuth * (0.5 / M_PI), inclination * (1.0 / M_PI));
	return textureLod(g_light_probe, uv, 0.0).rgb * g_ambient_brightness;
}


/*! Produces a direction sampled from the ambient radiance.
	\param out_radiance The ambient radiance for the sampled direction
		(assuming that the ray is not occluded).
	\param out_dir The sampled normalized direction vector.
	\param rand Source of random numbers.
	\return The density of the produced sample w.r.t. solid angle measure.*/
float sample_ambient_radiance(out vec3 out_radiance, out vec3 out_dir, inout random_generator rand) {
	// Take a sample from the alias table
	vec2 rands = get_random_numbers(rand);
	uint sample_idx = uint(rands[0] * float(LIGHT_PROBE_WIDTH * LIGHT_PROBE_HEIGHT));
	uint table = texelFetch(g_light_probe_alias_table, int(sample_idx)).r;
	uint alias = table % (LIGHT_PROBE_WIDTH * LIGHT_PROBE_HEIGHT);
	uint quanitzed_prob = table / (LIGHT_PROBE_WIDTH * LIGHT_PROBE_HEIGHT);
	const uint prob_count = 0xffffffffu / (LIGHT_PROBE_WIDTH * LIGHT_PROBE_HEIGHT);
	float prob = float(quanitzed_prob) * (1.0 / (prob_count - 1));
	sample_idx = (rands[1] < prob) ? sample_idx : alias;
	// Turn the sample index into a direction
	uvec2 sample_pos = uvec2(sample_idx % LIGHT_PROBE_WIDTH, sample_idx / LIGHT_PROBE_WIDTH);
	const float inclination_factor = M_PI / float(LIGHT_PROBE_HEIGHT);
	const float inclination_summand = 0.5 * inclination_factor;
	float inclination = fma(sample_pos.y, inclination_factor, inclination_summand);
	const float azimuth_factor = 2.0 * M_PI / float(LIGHT_PROBE_WIDTH);
	const float azimuth_summand = 0.5 * azimuth_factor;
	float azimuth = fma(sample_pos.x, azimuth_factor, azimuth_summand);
	float sin_inclination = sin(inclination);
	out_dir = vec3(cos(azimuth) * sin_inclination, sin(azimuth) * sin_inclination, cos(inclination));
	// Fetch the color for the sampled texel
	out_radiance = texelFetch(g_light_probe, ivec2(sample_pos), 0).rgb * g_ambient_brightness;
	// Compute the density (which integrates to 1 over the sphere and is
	// proportional to luminance)
	const vec3 luminance_weights = vec3(0.17697, 0.8124, 0.01063);
	float density = dot(luminance_weights, out_radiance) * g_inv_light_probe_luminance_integral;
	return density;
}


/*! Determines the segment in texel space along which a world-space ray passes
	through the volume and optionally lies within the clipped region of space.
	\param out_origin_texel The ray origin in texel space. If ray_origin is
		already in the (clipped) volume, this is the same point in different
		coordinates. Otherwise, it is the entry-point into the clipped volume.
	\param out_dir_texel ray_dir transformed to texel space.
	\param out_t_new_origin The ray parameter of the original ray that
		corresponds to out_origin_texel.
	\param ray_origin The ray origin in world space.
	\param ray_dir The ray direction in world space.
	\return The ray parameter at which the output ray exits the (clipped)
		volume. A negative value indicates that the given ray does not
		intersect the bounds of the (clipped) volume at all.*/
float get_texel_space_ray_segment(out vec3 out_origin_texel, out vec3 out_dir_texel, out float out_t_new_origin, vec3 ray_origin, vec3 ray_dir) {
	// Transform to texel space
	out_origin_texel = (g_world_to_texel_space[0] * vec4(ray_origin, 1.0f)).xyz;
	out_dir_texel = (g_world_to_texel_space[0] * vec4(ray_dir, 0.0f)).xyz;
	// Figure out the interval that the ray travels through the volume
	vec3 t_in_xyz = (vec3(0.0) - out_origin_texel) / out_dir_texel;
	vec3 t_out_xyz = (g_volume_extent - out_origin_texel) / out_dir_texel;
	vec3 t_min_xyz = min(t_in_xyz, t_out_xyz);
	vec3 t_max_xyz = max(t_in_xyz, t_out_xyz);
	float t_in = max(t_min_xyz.x, max(t_min_xyz.y, t_min_xyz.z));
	float t_out = min(t_max_xyz.x, min(t_max_xyz.y, t_max_xyz.z));
	t_in = max(0.0, t_in);
	t_out = max(0.0, t_out);
	// Final output processing
	out_origin_texel += t_in * out_dir_texel;
	out_t_new_origin = t_in;
	return t_out - t_in;
}


//! Computes the transmittance along the given ray through the volume. ray_dir
//! must be normalized.
float get_ray_transmittance(vec3 ray_origin, vec3 ray_dir, float t_max, inout random_generator rand) {
	vec3 origin_texel, dir_texel;
	float t_in;
	float t_out = get_texel_space_ray_segment(origin_texel, dir_texel, t_in, ray_origin, ray_dir);
	if (t_out <= 0.0)
		return 1.0;
	t_max = min(t_out, t_max - t_in);
	// Perform transmittance estimation
	ray_attribs_t ray_attribs = prepare_ray_attribs(origin_texel, dir_texel, t_max);
	return estimate_volume_transmittance(origin_texel, dir_texel, t_max, ray_attribs, rand);
}


//! Given two random numbers in [0,1), this function samples a point on the
//! unit sphere uniformly at random.
vec3 sample_unit_sphere(vec2 randoms) {
	const float pi = 3.1415926535897932384626433;
	float azimuth = (2.0 * pi) * randoms[0] - pi;
	float z = 2.0 * randoms[1] - 1.0;
	float r = sqrt(max(0.0, 1.0 - z * z));
	return vec3(r * cos(azimuth), r * sin(azimuth), z);
}


/*! Performs volume rendering using distance sampling followed by ambient
	occlusion based on transmittance estimation. Returns whether a sample has
	been generated at all. If so, it outputs the corresponding radiance,
	otherwise zero.*/
bool ray_traced_ambient_occlusion(out vec3 out_radiance, vec3 ray_origin, vec3 ray_dir, float t_max, inout random_generator rand) {
	vec3 origin_texel, dir_texel;
	float t_in;
	float new_t_max = get_texel_space_ray_segment(origin_texel, dir_texel, t_in, ray_origin, ray_dir);
	t_max = min(t_max - t_in, new_t_max);
	out_radiance = vec3(0.0);
	if (t_max <= 0.0)
		return false;
	ray_origin += t_in * ray_dir;
	// Sample a scattering event inside the volume
	float t = sample_volume_distance(origin_texel, dir_texel, t_max, rand);
	if (t >= t_max)
		return false;
	vec3 scattering_point = ray_origin + t * ray_dir;
	vec3 albedo = get_volume_color(origin_texel + t * dir_texel);
	// Sample ambient occlusion rays using multiple importance sampling between
	// Mie scattering and uniform sampling
	vec3 ao_ray_dirs[2];
	vec3 ao_ambient_radiances[2];
	float ao_probe_densities[2];
	ao_ray_dirs[0] = sample_mie_phase_function(-ray_dir, get_random_numbers(rand), g_mie_fit_params);
	ao_ambient_radiances[0] = get_ambient_radiance(ao_ray_dirs[0]);
	const vec3 luminance_weights = vec3(0.17697, 0.8124, 0.01063);
	ao_probe_densities[0] = dot(luminance_weights, ao_ambient_radiances[0]) * g_inv_light_probe_luminance_integral;
	ao_probe_densities[1] = sample_ambient_radiance(ao_ambient_radiances[1], ao_ray_dirs[1], rand);
	[[unroll]]
	for (uint i = 0; i != 2; ++i) {
		// Compute transmittance from the scattering point to the sky
		float transmittance = get_ray_transmittance(scattering_point, ao_ray_dirs[i], 1.0e38, rand);
		// Compute the out-scattered radiance using a constant phase function
		float phase_function = mie_phase_function(-ray_dir, ao_ray_dirs[i], g_mie_fit_params);
		float density_sum = phase_function + ao_probe_densities[i];
		vec3 incoming_radiance = ao_ambient_radiances[i] * g_ambient_lighting_factor;
		out_radiance += albedo * incoming_radiance * (transmittance * phase_function / density_sum);
	}
	return true;
}


//! Geometric quantities relating a ray to a point
struct ray_point_geometry_t {
	//! The reciprocal of the minimal distance between the infinite line for
	//! the ray and the point
	float rcp_min_dist;
	/*! The signed distance (i.e. difference in ray parameters) from the ray
		origin and the endpoint of the ray to the point that realizes the
		returned minimal distance.*/
	float s_min, s_max;
	//! atan(s_min * rcp_min_dist)
	float angle_min;
	//! atan(s_max * rcp_min_dist) - angle_min
	float angle_diff;
};


/*! Given a ray and a point, this function computes geometric quantities that
	are useful for distance sampling in presence of point lights.
	\param ray_origin Ray origin in world space.
	\param ray_dir Normalized ray direction in world space.
	\param t_max Maximal ray parameter.
	\param point The point in world space.
	\return The requested geometric quantities.*/
ray_point_geometry_t get_ray_point_geometry(vec3 ray_origin, vec3 ray_dir, float t_max, vec3 point) {
	ray_point_geometry_t g;
	vec3 offset = ray_origin - point;
	g.s_min = dot(ray_dir, offset);
	float sq_begin_dist = dot(offset, offset);
	float sq_min_dist = fma(-g.s_min, g.s_min, sq_begin_dist);
	g.rcp_min_dist = inversesqrt(max(1.0e-37, sq_min_dist));
	g.s_max = g.s_min + t_max;
	g.angle_min = atan(g.s_min * g.rcp_min_dist);
	g.angle_diff = atan(g.s_max * g.rcp_min_dist) - g.angle_min;
	return g;
}


//! Implements equiangular sampling, given output of get_ray_point_geometry().
//! Takes a uniform random number in [0,1) and returns a ray parameter.
float sample_equiangular(ray_point_geometry_t g, float random_number) {
	float angle = fma(random_number, g.angle_diff, g.angle_min);
	float s = tan(angle) / g.rcp_min_dist;
	float t = s - g.s_min;
	return t;
}


//! Returns the density that is sampled by sample_equiangular()
float get_equiangular_density(ray_point_geometry_t g, float ray_param) {
	float s = ray_param + g.s_min;
	float s_over_dist = s * g.rcp_min_dist;
	return g.rcp_min_dist / (g.angle_diff * fma(s_over_dist, s_over_dist, 1.0));
}


/*! Computes outscattered radiance for a point in a volume due to scattering of
	light received from a single point light.
	\param scattering_point World-space location of the scattering point.
	\param scattering_point_texel Texel-space location of the scattering point.
	\param out_dir Normalized world-space outgoing light direction.
	\param light The point light that is considered.
	\param rand Source of random numbers for transmittance estimation.
	\return A Monte Carlo estimate (due to transmittance estimation) of the
		differential outscattered radiance. Must be multiplied by transmittance
		and extinction along a ray and integrated to get actual radiance.*/
vec3 get_outscattered_radiance(vec3 scattering_point, vec3 scattering_point_texel, vec3 out_dir, point_light_t light, inout random_generator rand) {
	// Compute transmittance towards the point light
	vec3 light_dir = light.pos - scattering_point;
	float t_max_shadow_ray = length(light_dir);
	float rcp_light_dist = 1.0 / t_max_shadow_ray;
	light_dir *= rcp_light_dist;
	float transmittance = get_ray_transmittance(scattering_point, light_dir, t_max_shadow_ray, rand);
	// We need the integral over incoming radiance due to the point light
	// across all directions. To this end, we consider a sphere of radius
	// t_max_shadow_ray centered on the point light. Since light hits this
	// sphere orthogonally, this integral matches the irradiance.
	// Irradiance is flux per area and flux for the point light is known.
	// We just have to divide it by the surface area of the sphere.
	vec3 radiance_integral = light.power * (rcp_light_dist * rcp_light_dist * (0.25 / M_PI));
	// Evaluate the phase function
	vec3 albedo = get_volume_color(scattering_point_texel);
	vec3 phase_function = albedo * mie_phase_function(out_dir, light_dir, g_mie_fit_params);
	// Account for transmittance to the point light to get an estimate of
	// outscattered radiance
	vec3 outscattered_radiance = phase_function * radiance_integral * transmittance;
	return outscattered_radiance;
}


//! Renders a volume illuminated by point lights
vec3 ray_traced_volume_with_point_lights(vec3 ray_origin, vec3 ray_dir, inout random_generator rand) {
	vec3 origin_texel, dir_texel;
	float t_in;
	float t_max = get_texel_space_ray_segment(origin_texel, dir_texel, t_in, ray_origin, ray_dir);
	if (t_max <= 0.0 || POINT_LIGHT_COUNT == 0)
		return vec3(0.0);
	ray_origin += t_in * ray_dir;
#if POINT_LIGHT_COUNT <= 1
	point_light_t light = g_point_lights[0];
	float light_prob = 1.0;
#else
	// Select one of the point lights stochastically based on its estimated
	// contribution to shading along the ray
	float importance_sum = 0.0;
	float importances[max(1, POINT_LIGHT_COUNT)];
	[[unroll]]
	for (uint i = 0; i != POINT_LIGHT_COUNT; ++i) {
		// Compute the importance
		ray_point_geometry_t g = get_ray_point_geometry(ray_origin, ray_dir, t_max, g_point_lights[i].pos);
		float importance = g_point_lights[i].importance * g.rcp_min_dist * g.angle_diff;
		importances[i] = importance;
		importance_sum += importance;
	}
	// A percentage of samples is distributed uniformly between all lights
	const float uniform_ratio = 0.3;
	const float uniform_factor = uniform_ratio / (1.0 - uniform_ratio);
	float uniform_importance = importance_sum * (uniform_factor / POINT_LIGHT_COUNT);
	importance_sum = fma(importance_sum, uniform_factor, importance_sum);
	// Perform the selection
	float target_importance = get_random_numbers(rand)[0] * importance_sum;
	uint light_index = 0;
	float light_prob = 0.0;
	importance_sum = 0.0;
	[[unroll]]
	for (uint i = 0; i != POINT_LIGHT_COUNT; ++i) {
		float importance = importances[i] + uniform_importance;
		if (importance_sum <= target_importance) {
			light_index = i;
			light_prob = importance;
		}
		importance_sum += importance;
	}
	light_prob /= importance_sum;
	point_light_t light = g_point_lights[light_index];
#endif

#if SAMPLING_STRATEGIES_LIGHT_TRANSMITTANCE_MILLER_MIS
	// Sample one distance using equiangular sampling and another using delta
	// tracking
	float ts[2], null_probs[2];
	ray_point_geometry_t g = get_ray_point_geometry(ray_origin, ray_dir, t_max, light.pos);
	ts[0] = sample_equiangular(g, get_random_numbers(rand)[0]);
	ts[1] = delta_tracking(null_probs[1], origin_texel, dir_texel, t_max, rand);
	// Perform ratio tracking for the sample that came out of equiangular
	// sampling
	null_probs[0] = ratio_tracking(origin_texel, dir_texel, ts[0], rand);
	// Process all distance samples in the same manner
	vec3 radiance = vec3(0.0);
	[[unroll]]
	for (uint i = 0; i != 2; ++i) {
		float t = ts[i];
		[[branch]]
		if (t >= t_max)
			continue;
		// Compute differential outscattered radiance, accounting for the fact
		// that we have only sampled one light
		vec3 scattering_point_texel = origin_texel + t * dir_texel;
		vec3 outscattered_radiance = get_outscattered_radiance(ray_origin + t * ray_dir, scattering_point_texel, -ray_dir, light, rand);
		outscattered_radiance *= 1.0 / light_prob;
		// Determine the extinction at the shading point
		float extinction = get_volume_extinction_interpolated(scattering_point_texel);
		// Construct an MIS estimate
		float equiangular_density = get_equiangular_density(g, t);
		float ext_trans = extinction * null_probs[i];
		radiance += outscattered_radiance * (ext_trans / (ext_trans + equiangular_density));
	}

#else // Everything except Miller et al. [2019]
	// Sample a scattering event inside the volume using either light or
	// transmittance-based sampling (or both)
	float ts[1 + SAMPLING_STRATEGIES_LIGHT_TRANSMITTANCE_MIS + SAMPLING_STRATEGIES_LIGHT_TRANSMITTANCE_APPROXIMATE_MIS];
#if SAMPLING_STRATEGIES_LIGHT
	ray_point_geometry_t g = get_ray_point_geometry(ray_origin, ray_dir, t_max, light.pos);
	ts[0] = sample_equiangular(g, get_random_numbers(rand)[0]);

#elif SAMPLING_STRATEGIES_TRANSMITTANCE
	ts[0] = sample_volume_distance(origin_texel, dir_texel, t_max, rand);

#elif SAMPLING_STRATEGIES_LIGHT_TRANSMITTANCE_MIS || SAMPLING_STRATEGIES_LIGHT_TRANSMITTANCE_APPROXIMATE_MIS
	ray_point_geometry_t g = get_ray_point_geometry(ray_origin, ray_dir, t_max, light.pos);
	ts[0] = sample_volume_distance(origin_texel, dir_texel, t_max, rand);
	ts[1] = sample_equiangular(g, get_random_numbers(rand)[0]);
#endif

	// Process all distance samples in the same manner
	vec3 radiance = vec3(0.0);
	[[unroll]]
	for (uint i = 0; i != 1 + SAMPLING_STRATEGIES_LIGHT_TRANSMITTANCE_MIS + SAMPLING_STRATEGIES_LIGHT_TRANSMITTANCE_APPROXIMATE_MIS; ++i) {
		float t = ts[i];
		[[branch]]
		if (t >= t_max)
			continue;
		// Compute differential outscattered radiance, accounting for the fact
		// that we have only sampled one light
		vec3 scattering_point_texel = origin_texel + t * dir_texel;
		vec3 outscattered_radiance = get_outscattered_radiance(ray_origin + t * ray_dir, scattering_point_texel, -ray_dir, light, rand);
		outscattered_radiance *= 1.0 / light_prob;
		// Prepare to estimate optical depth
		ray_attribs_t ray_attribs = prepare_ray_attribs(origin_texel, dir_texel, t);
#if SAMPLING_STRATEGIES_LIGHT
		// Determine the extinction at the shading point
		float extinction = get_volume_extinction_interpolated(scattering_point_texel);
		// Estimate transmittance
		float ray_transmittance = estimate_volume_transmittance(origin_texel, dir_texel, t, ray_attribs, rand);
		// Construct the Monte Carlo estimate
		float equiangular_density = get_equiangular_density(g, t);
		radiance += outscattered_radiance * (extinction * ray_transmittance / equiangular_density);

#elif SAMPLING_STRATEGIES_TRANSMITTANCE
		// The sampling density and factors in the integrand cancel, so things
		// are really simple
		radiance += outscattered_radiance;

#elif SAMPLING_STRATEGIES_LIGHT_TRANSMITTANCE_APPROXIMATE_MIS
		// Determine the extinction at the shading point
		float extinction = get_volume_extinction_interpolated(scattering_point_texel);
		// Construct an MIS estimate
		float equiangular_density = get_equiangular_density(g, t);
		float transmittance_mis_weight = estimate_volume_mis_weight(origin_texel, dir_texel, t, ray_attribs, vec2(extinction, 0.0), vec2(extinction, equiangular_density), rand);
		radiance += transmittance_mis_weight * outscattered_radiance;

#elif SAMPLING_STRATEGIES_LIGHT_TRANSMITTANCE_MIS
		// Determine the MIS weight for transmittance-based sampling
		float extinction = get_volume_extinction_interpolated(scattering_point_texel);
		float equiangular_density = get_equiangular_density(g, t);
		float mis_weight = estimate_volume_mis_weight(origin_texel, dir_texel, t, ray_attribs, vec2(extinction, 0.0), vec2(extinction, equiangular_density), rand);
		if (i == 0)
			// Contribution of the transmittance-based sample
			radiance += mis_weight * outscattered_radiance;
		else {
			// Contribution of the light sample
			float primary_transmittance = estimate_volume_transmittance(origin_texel, dir_texel, t, ray_attribs, rand);
			radiance += (1.0 - mis_weight) * extinction * primary_transmittance / equiangular_density * outscattered_radiance;
		}
#endif
	}
#endif
	return radiance;
}


void main() {
	// Jitter the subpixel position using a Gaussian with the given standard
	// deviation in pixels
	random_generator rand;
	uint sample_index = g_frame_index * g_frame_sample_count + g_frame_sample_index;
	rand.seed = uvec2(gl_FragCoord) ^ uvec2(sample_index << 16, (sample_index + 237) << 16);
	const float std = 0.9;
#if JITTER_PRIMARY_RAYS
	vec2 randoms = 2.0 * get_random_numbers(rand) - vec2(1.0);
	vec2 jitter = (std * sqrt(2.0)) * vec2(erfinv(randoms.x), erfinv(randoms.y));
	vec2 screen_jittered = gl_FragCoord.xy + jitter;
#else
	vec2 screen_jittered = gl_FragCoord.xy;
#endif
	// Compute the camera ray
	vec2 viewport_coord = fma(screen_jittered, g_viewport_transforms[0].xy, g_viewport_transforms[0].zw);
	vec3 ray_origin = get_camera_ray_origin(viewport_coord, g_projection_to_world_space[0]);
	vec3 ray_dir = get_camera_ray_direction(viewport_coord, g_world_to_projection_space[0]);
	// Compute the pixel color
	g_out_color.a = 1.0;
#if DISPLAYED_QUANTITY_TRANSMITTANCE
	g_out_color.rgb = vec3(get_ray_transmittance(ray_origin, ray_dir, 1.0e38, rand));

#elif DISPLAYED_QUANTITY_TRANSMITTANCE_STATS
	float transmittance = get_ray_transmittance(ray_origin, ray_dir, 1.0e38, rand);
	g_out_color.rgb = vec3(transmittance, transmittance * transmittance, g_volume_sample_count);

#elif DISPLAYED_QUANTITY_TRANSMITTANCE_SIGN
	float transmittance = get_ray_transmittance(ray_origin, ray_dir, 1.0e38, rand);
	g_out_color.rgb = vec3(transmittance, (transmittance < 0.0) ? 1.0 : 0.0, 0.0);

#elif DISPLAYED_QUANTITY_MIS_WEIGHT || DISPLAYED_QUANTITY_MIS_WEIGHT_STATS
	vec3 origin_texel, dir_texel;
	float t_in;
	float t_max = get_texel_space_ray_segment(origin_texel, dir_texel, t_in, ray_origin, ray_dir);
	g_out_color.rgb = vec3(0.0);
	if (t_max <= 0.0)
		g_out_color.rgb = vec3(0.0);
	ray_attribs_t ray_attribs = prepare_ray_attribs(origin_texel, dir_texel, t_max);
	float mis_weight = estimate_volume_mis_weight(origin_texel, dir_texel, t_max, ray_attribs, g_mis_weight_numerator, g_mis_weight_denominator, rand);
#if DISPLAYED_QUANTITY_MIS_WEIGHT
	g_out_color.rgb = vec3(mis_weight);
#elif DISPLAYED_QUANTITY_MIS_WEIGHT_STATS
	g_out_color.rgb = vec3(mis_weight, mis_weight * mis_weight, g_volume_sample_count);
#endif

#elif DISPLAYED_QUANTITY_RADIANCE || DISPLAYED_QUANTITY_LUMINANCE_STATS
	// Compute the contribution of volume rendering using Monte Carlo
	// integration
	vec3 volume_color = vec3(0.0);
#if USE_AMBIENT_LIGHT
	vec3 ao;
	if (ray_traced_ambient_occlusion(ao, ray_origin, ray_dir, 1.0e38, rand))
		volume_color += ao;
	else
		volume_color += get_ambient_radiance(ray_dir);
#endif
#if POINT_LIGHT_COUNT > 0
	volume_color += ray_traced_volume_with_point_lights(ray_origin, ray_dir, rand);
#endif
#if DISPLAYED_QUANTITY_RADIANCE
	g_out_color.rgb = volume_color;
#elif DISPLAYED_QUANTITY_LUMINANCE_STATS
	const vec3 luminance_weights = vec3(0.17697, 0.8124, 0.01063);
	float luminance = dot(volume_color, luminance_weights);
	g_out_color.rgb = vec3(luminance, luminance * luminance, g_volume_sample_count);
#endif
#endif
}
