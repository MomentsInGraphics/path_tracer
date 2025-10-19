#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_control_flow_attributes : enable
#extension GL_EXT_ray_query : enable
#include "camera_utilities.glsl"
// Lots of other includes and bindings come indirectly through this one
#include "brdfs.glsl"


//! The BVH containing all scene geometry
layout(binding = 2) uniform accelerationStructureEXT g_bvh;


//! The outgoing radiance towards the camera as sRGB color
layout (location = 0) out vec4 g_out_color;


/*! Generates a pair of pseudo-random numbers.
	\param seed Integers that change with each invocation. They get updated so
		that you can reuse them.
	\return A uniform, pseudo-random point in [0,1)^2.*/
vec2 get_random_numbers(inout uvec2 seed) {
	// PCG2D, as described here: https://jcgt.org/published/0009/03/02/
	seed = 1664525u * seed + 1013904223u;
	seed.x += 1664525u * seed.y;
	seed.y += 1664525u * seed.x;
	seed ^= (seed >> 16u);
	seed.x += 1664525u * seed.y;
	seed.y += 1664525u * seed.x;
	seed ^= (seed >> 16u);
	// Multiply by 2^-32 to get floats
	return vec2(seed) * 2.32830643654e-10;
}


//! The inverse of the error function (used to sample Gaussians).
float erfinv(float x) {
	float w = -log(max(1.0e-37, 1.0 - x * x));
	float a = w - 2.5;
	float b = sqrt(w) - 3.0;
	return x * ((w < 5.0)
		? fma(fma(fma(fma(fma(fma(fma(fma(2.81022636e-08, a, 3.43273939e-07), a, -3.5233877e-06), a, -4.39150654e-06), a, 0.00021858087), a, -0.00125372503), a, -0.00417768164), a, 0.246640727), a, 1.50140941)
		: fma(fma(fma(fma(fma(fma(fma(fma(-0.000200214257, b, 0.000100950558), b, 0.00134934322), b, -0.00367342844), b, 0.00573950773), b, -0.0076224613), b, 0.00943887047), b, 1.00167406), b, 2.83297682));
}


//! Samples a direction vector in the upper hemisphere (non-negative z) by
//! sampling spherical coordinates uniformly. Not a good strategy.
vec3 sample_hemisphere_spherical(vec2 randoms) {
	float azimuth = (2.0 * M_PI) * randoms[0] - M_PI;
	float inclination = (0.5 * M_PI) * randoms[1];
	float radius = sin(inclination);
	return vec3(radius * cos(azimuth), radius * sin(azimuth), cos(inclination));
}


//! Returns the density w.r.t. solid angle sampled by
//! sample_hemisphere_spherical(). Only needs the local z-coordinate as input.
float get_hemisphere_spherical_density(float sampled_dir_z) {
	if (sampled_dir_z < 0.0)
		return 0.0;
	return 1.0 / ((M_PI * M_PI) * sqrt(max(0.0, 1.0 - sampled_dir_z * sampled_dir_z)));
}


//! Returns the solid angle of the given spherical light for the given shading
//! point divided by 2.0 * M_PI, or 0.0 if it is completely below the horizon.
float get_spherical_light_importance(vec3 center, float radius, vec3 shading_pos, vec3 normal) {
	// If the light is completely below the horizon, return 0
	vec3 center_dir = center - shading_pos;
	if (dot(normal, center_dir) < -radius)
		return 0.0;
	// Compute the solid angle. We want z_range = 1.0 - z_min, where
	// z_min = sqrt(1.0 - sin_2). Computing it like that results in
	// cancelation for small sin_2, so instead we put the square root into
	// the denominator to solve the same quadratic equation:
	// https://en.wikipedia.org/wiki/Quadratic_formula#Square_root_in_the_denominator
	float center_dist_2 = dot(center_dir, center_dir);
	float sin_2 = radius * radius / center_dist_2;
	float z_range = sin_2 / (1.0 + sqrt(max(0.0, 1.0 - sin_2)));
	return z_range;
}


/*! Samples a direction in the solid angle of the given spherical light
	uniformly.
	\param center The center position of the spherical light.
	\param importance The importance of the spherical light as returned by
		get_spherical_light_importance(). Must not be zero. The sampled density
		w.r.t. solid angle is 1.0 / (2.0 * M_PI * importance) or zero.
	\param shading_pos The position of the shading point.
	\param randoms A uniformly distributed point in [0,1)^2.
	\return A normalized direction vector towards the spherical light.*/
vec3 sample_spherical_light(vec3 center, float importance, vec3 shading_pos, vec2 randoms) {
	// Produce a sample in local coordinates
	float azimuth = (2.0 * M_PI) * randoms[0] - M_PI;
	float z_range = importance;
	float z = 1.0 - z_range * randoms[1];
	float r = sqrt(max(0.0, 1.0 - z * z));
	vec3 local_dir = vec3(r * cos(azimuth), r * sin(azimuth), z);
	// Construct a coordinate frame where the vector towards the spherical
	// light is the z-axis and transform to world space
	mat3 light_to_world_space = get_shading_space(normalize(center - shading_pos));
	return light_to_world_space * local_dir;
}


/*! Randomly picks one of the spherical lights in the scene using selection
	probabilities proportional to get_spherical_light_importance() and samples
	a direction towards it, sampling its solid angle uniformly.
	\param out_total_importance The sum of importance values across all lights.
		Needed for density computation.
	\param shading_pos Position of the shading point w.r.t. which the solid
		angle is computed.
	\param normal The shading normal at the shading point.
	\param randoms A random point distributed uniformly in [0, 1)^2.
	\return The sampled direction towards a light as normalized vector. Zero if
		the total importance is zero (all lights below the horizon).*/
vec3 sample_lights(out float out_total_importance, vec3 shading_pos, vec3 normal, vec2 randoms) {
	// Compute the total importance of all lights combined
	out_total_importance = 0.0;
	[[loop]]
	for (uint i = 0; i != SPHERICAL_LIGHT_COUNT; ++i)
		out_total_importance += get_spherical_light_importance(g_spherical_lights[i].xyz, g_spherical_lights[i].w, shading_pos, normal);
	// Pick one
	float target_importance = randoms[0] * out_total_importance;
	float prefix_importance = 0.0;
	[[loop]]
	for (uint i = 0; i != SPHERICAL_LIGHT_COUNT; ++i) {
		vec4 light = g_spherical_lights[i];
		float importance = get_spherical_light_importance(light.xyz, light.w, shading_pos, normal);
		prefix_importance += importance;
		if (prefix_importance > target_importance) {
			// Reuse the random number
			randoms[0] = (target_importance + importance - prefix_importance) / importance;
			// Sample the light
			return sample_spherical_light(light.xyz, importance, shading_pos, randoms);
		}
	}
	// Only happens if randoms[0] >= 1.0 or out_total_importance <= 0.0
	return vec3(0.0);
}


/*! Returns the density w.r.t. solid angle that is sampled by sample_lights().
	sampled_dir must be above the horizon, otherwise results may be incorrect.
	Pass true for is_light_dir if sampled_dir has been generated as direction
	towards a spherical light. This is a work around for numerical issues.*/
float get_lights_density(float total_importance, vec3 shading_pos, vec3 normal, vec3 sampled_dir, bool is_light_dir) {
	if (total_importance <= 0.0)
		return 0.0;
	// Count how many lights are intersected by the given ray
	float light_count = 0.0;
	[[loop]]
	for (uint i = 0; i != SPHERICAL_LIGHT_COUNT; ++i) {
		vec4 light = g_spherical_lights[i];
		vec3 center_dir = light.xyz - shading_pos;
		float center_dist_2 = dot(center_dir, center_dir);
		float center_dot_dir = dot(center_dir, sampled_dir);
		float radius_2 = light.w * light.w;
		float in_sphere = center_dist_2 - radius_2;
		float discriminant = center_dot_dir * center_dot_dir - in_sphere;
		// Add 1 if there is an intersection with non-negative ray parameter t
		light_count += (discriminant >= 0.0 && in_sphere >= 0.0 && center_dot_dir >= 0.0) ? 1.0 : 0.0;
	}
	// For small distant light sources, the procedure above will sometimes fail
	// to count them correctly. If that results in a light count of zero for
	// light samples, it distorts Monte Carlo estimates heavily. Thus, we set
	// light_count to at least 1 if we know that it must be like that.
	if (is_light_dir)
		light_count = max(1.0, light_count);
	// The density is proportional to this count
	return light_count / (2.0 * M_PI * total_importance);
}


/*! Traces the given ray (with normalized ray_dir). If it hits a scene surface,
	it constructs the shading data and returns true. Otherwise, it returns
	false and only writes the sky emission to the shading data.*/
bool trace_ray(out shading_data_t out_shading_data, vec3 ray_origin, vec3 ray_dir) {
	// Trace a ray
	rayQueryEXT ray_query;
	rayQueryInitializeEXT(ray_query, g_bvh, gl_RayFlagsOpaqueEXT, 0xff, ray_origin, 1.0e-3, ray_dir, 1e38);
	while (rayQueryProceedEXT(ray_query)) {}
	// If there was no hit, use the sky color
	if (rayQueryGetIntersectionTypeEXT(ray_query, true) == gl_RayQueryCommittedIntersectionNoneEXT) {
		out_shading_data.emission = g_sky_radiance;
		return false;
	}
	// Construct shading data
	else {
		int triangle_index = rayQueryGetIntersectionPrimitiveIndexEXT(ray_query, true);
		vec2 barys = rayQueryGetIntersectionBarycentricsEXT(ray_query, true);
		bool front = rayQueryGetIntersectionFrontFaceEXT(ray_query, true);
		out_shading_data = get_shading_data(triangle_index, barys, front, -ray_dir);
		return true;
	}
}


//! Like trace_ray() but only returns the emission of the shading data
vec3 trace_ray_emission(vec3 ray_origin, vec3 ray_dir) {
	// Trace a ray
	rayQueryEXT ray_query;
	rayQueryInitializeEXT(ray_query, g_bvh, gl_RayFlagsOpaqueEXT, 0xff, ray_origin, 1.0e-3, ray_dir, 1e38);
	while (rayQueryProceedEXT(ray_query)) {}
	// If there was no hit, use the sky color
	if (rayQueryGetIntersectionTypeEXT(ray_query, true) == gl_RayQueryCommittedIntersectionNoneEXT)
		return g_sky_radiance;
	// Otherwise, check if it is an emissive material
	else {
		int triangle_index = rayQueryGetIntersectionPrimitiveIndexEXT(ray_query, true);
		if (texelFetch(g_material_indices, triangle_index).r == EMISSION_MATERIAL_INDEX)
			return g_emission_material_radiance;
		else
			return vec3(0.0);
	}
}


//! Like path_trace_psa() but samples spherical coordiantes uniformly for
//! instructive purposes
vec3 path_trace_spherical(vec3 ray_origin, vec3 ray_dir, inout uvec2 seed) {
	vec3 throughput_weight = vec3(1.0);
	vec3 radiance = vec3(0.0);
	[[unroll]]
	for (uint k = 1; k != PATH_LENGTH + 1; ++k) {
		shading_data_t s;
		bool hit = trace_ray(s, ray_origin, ray_dir);
		radiance += throughput_weight * s.emission;
		if (hit && k < PATH_LENGTH) {
			// Update the ray and the throughput weight
			mat3 shading_to_world_space = get_shading_space(s.normal);
			ray_origin = s.pos;
			vec3 sampled_dir = sample_hemisphere_spherical(get_random_numbers(seed));
			ray_dir = shading_to_world_space * sampled_dir;
			float lambert_in = sampled_dir.z;
			float density = get_hemisphere_spherical_density(sampled_dir.z);
			throughput_weight *= frostbite_brdf(s, ray_dir) * lambert_in / density;
		}
		else
			// End the path
			break;
	}
	return radiance;
}


/*! Computes a Monte Carlo estimate of the radiance received along a ray. It
	uses path tracing with uniform sampling of the projected solid angle.
	\param ray_origin The ray origin. A small ray offset is used automatically
		to avoid incorrect self-shadowing.
	\param ray_dir Normalized ray direction.
	\param seed Used for get_random_numbers().
	\return A Monte Carlo estimate of the incoming radiance in linear sRGB
		(a.k.a. Rec. 709).*/
vec3 path_trace_psa(vec3 ray_origin, vec3 ray_dir, inout uvec2 seed) {
	vec3 throughput_weight = vec3(1.0);
	vec3 radiance = vec3(0.0);
	[[unroll]]
	for (uint k = 1; k != PATH_LENGTH + 1; ++k) {
		shading_data_t s;
		bool hit = trace_ray(s, ray_origin, ray_dir);
		radiance += throughput_weight * s.emission;
		if (hit && k < PATH_LENGTH) {
			// Update the ray and the throughput weight
			mat3 shading_to_world_space = get_shading_space(s.normal);
			ray_origin = s.pos;
			vec3 sampled_dir = sample_hemisphere_psa(get_random_numbers(seed));
			ray_dir = shading_to_world_space * sampled_dir;
			float lambert_in = sampled_dir.z;
			float density = get_hemisphere_psa_density(sampled_dir.z);
			throughput_weight *= frostbite_brdf(s, ray_dir) * lambert_in / density;
		}
		else
			// End the path
			break;
	}
	return radiance;
}


//! Like path_trace_psa() but with proper importance sampling of the BRDF times
//! cosine.
vec3 path_trace_brdf(vec3 ray_origin, vec3 ray_dir, inout uvec2 seed) {
	vec3 throughput_weight = vec3(1.0);
	vec3 radiance = vec3(0.0);
	[[unroll]]
	for (uint k = 1; k != PATH_LENGTH + 1; ++k) {
		shading_data_t s;
		bool hit = trace_ray(s, ray_origin, ray_dir);
		radiance += throughput_weight * s.emission;
		if (hit && k < PATH_LENGTH) {
			// Sample the BRDF and update the ray
			ray_origin = s.pos;
			ray_dir = sample_frostbite_brdf(s, get_random_numbers(seed));
			float density = get_frostbite_brdf_density(s, ray_dir);
			// The sample may have ended up in the lower hemisphere
			float lambert_in = dot(s.normal, ray_dir);
			if (lambert_in <= 0.0)
				break;
			// Update the throughput weight
			throughput_weight *= frostbite_brdf(s, ray_dir) * lambert_in / density;
		}
		else
			// End the path
			break;
	}
	return radiance;
}


//! Like path_trace_brdf() but additionally uses next-event estimation
vec3 path_trace_nee(vec3 ray_origin, vec3 ray_dir, inout uvec2 seed) {
	vec3 throughput_weight = vec3(1.0);
	vec3 nee_throughput_weight = throughput_weight;
	vec3 radiance = vec3(0.0);
	[[unroll]]
	for (uint k = 1; k != PATH_LENGTH + 1; ++k) {
		shading_data_t s;
		bool hit = trace_ray(s, ray_origin, ray_dir);
		radiance += nee_throughput_weight * s.emission;
		if (hit && k < PATH_LENGTH) {
			// Sample a direction towards a light
			float total_light_importance;
			vec3 light_dir = sample_lights(total_light_importance, s.pos, s.normal, get_random_numbers(seed));
			// Discard the sample if it is in the lower hemisphere
			float lambert_in_0 = dot(s.normal, light_dir);
			if (lambert_in_0 > 0.0) {
				// Trace a ray towards the light and retrieve the emission
				vec3 light_emission = trace_ray_emission(s.pos, light_dir);
				// For MIS, compute the density for this direction with light
				// and BRDF sampling
				float light_density_0 = get_lights_density(total_light_importance, s.pos, s.normal, light_dir, true);
				float brdf_density_0 = get_frostbite_brdf_density(s, light_dir);
				// Evaluate the MIS estimate
				radiance += throughput_weight * frostbite_brdf(s, light_dir) * light_emission * (lambert_in_0 / (light_density_0 + brdf_density_0));
			}
			// Sample the BRDF for MIS and to continue the path
			ray_origin = s.pos;
			ray_dir = sample_frostbite_brdf(s, get_random_numbers(seed));
			float lambert_in_1 = dot(s.normal, ray_dir);
			// Abort path construction, if the sample is in the lower
			// hemisphere
			if (lambert_in_1 <= 0.0)
				break;
			// Compute the throughput weight for emission at the next vertex,
			// accounting for MIS
			float light_density_1 = get_lights_density(total_light_importance, s.pos, s.normal, ray_dir, false);
			float brdf_density_1 = get_frostbite_brdf_density(s, ray_dir);
			vec3 brdf_lambert_1 = frostbite_brdf(s, ray_dir) * lambert_in_1;
			nee_throughput_weight = throughput_weight * (brdf_lambert_1 / (light_density_1 + brdf_density_1));
			// Update the throughput-weight for the path
			throughput_weight *= brdf_lambert_1 * (1.0 / brdf_density_1);
		}
		else
			// End the path
			break;
	}
	return radiance;
}


void main() {
	// Jitter the subpixel position using a Gaussian with the given standard
	// deviation in pixels
	uvec2 seed = uvec2(gl_FragCoord) ^ uvec2(g_frame_index << 16, (g_frame_index + 237) << 16);
	const float std = 0.9;
	vec2 randoms = 2.0 * get_random_numbers(seed) - vec2(1.0);
	vec2 jitter = (std * sqrt(2.0)) * vec2(erfinv(randoms.x), erfinv(randoms.y));
	vec2 jittered = gl_FragCoord.xy + jitter;
	// Compute the primary ray using either a pinhole camera or a hemispherical
	// camera
	vec3 ray_origin, ray_dir;
	if (g_camera_type <= 1) {
		vec2 ray_tex_coord = jittered * g_inv_viewport_size;
		ray_origin = get_camera_ray_origin(ray_tex_coord, g_projection_to_world_space);
		ray_dir = get_camera_ray_direction(ray_tex_coord, g_world_to_projection_space);
	}
	else {
		mat3 hemisphere_to_world_space = get_shading_space(g_hemispherical_camera_normal);
		ray_origin = g_camera_pos;
		vec2 sphere_factor = vec2(1.0, (g_camera_type == 3) ? 2.0 : 1.0);
		ray_dir = hemisphere_to_world_space * sample_hemisphere_spherical(jittered * sphere_factor * g_inv_viewport_size);
	}
	// Perform path tracing using the requested technique
#if SAMPLING_STRATEGY_SPHERICAL
	vec3 ray_radiance = path_trace_spherical(ray_origin, ray_dir, seed);
#elif SAMPLING_STRATEGY_PSA
	vec3 ray_radiance = path_trace_psa(ray_origin, ray_dir, seed);
#elif SAMPLING_STRATEGY_BRDF
	vec3 ray_radiance = path_trace_brdf(ray_origin, ray_dir, seed);
#elif SAMPLING_STRATEGY_NEE
	vec3 ray_radiance = path_trace_nee(ray_origin, ray_dir, seed);
#endif
	g_out_color = vec4(ray_radiance, 1.0);
}
