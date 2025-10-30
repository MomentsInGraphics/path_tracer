#ifndef VOLUME_COUNT
	#define VOLUME_COUNT 0
#endif
#ifndef POINT_LIGHT_COUNT
	#define POINT_LIGHT_COUNT 0
#endif
#define MAX_CAMERA_COUNT 1


//! An idealized point light with equal emission into all directions
struct point_light_t {
	//! World-space position of the point light
	vec3 pos;
	//! Radiant power emitted by the point light in linear sRGB
	vec3 power;
	//! The importance of this point light for the sake of importance sampling
	float importance;
};


layout (row_major, std140, binding = 0) uniform constants {
	//! The world to projection space transform
	mat4 g_world_to_projection_space[MAX_CAMERA_COUNT];
	//! The projection to world space transform
	mat4 g_projection_to_world_space[MAX_CAMERA_COUNT];
	//! To get from pixel coordinates within the window to viewport coordinates
	//! ranging from 0 to 1, multiply by xy and add zw
	vec4 g_viewport_transforms[MAX_CAMERA_COUNT];
	//! The width and height of the viewport  (in pixels)
	vec2 g_viewport_size;
	//! The reciprocal width and height of the viewport
	vec2 g_inv_viewport_size;
	//! The factor by which HDR radiance is scaled during tonemapping
	float g_exposure;
	//! The index of the frame being rendered for the purpose of random seed
	//! generation
	uint g_frame_index;
	//! The number of samples which have been accumulated in the HDR radiance
	//! render target
	uint g_accum_frame_count;
	//! The number of samples that are taken per frame
	uint g_frame_sample_count;
	//! The width, height and depth of the volume in voxels
	vec3 g_volume_extent;
	//! Reciprocals of the values in g_volume_extent
	vec3 g_inv_volume_extent;
	//! When a constant color is used as volume albedo, it is this color
	vec3 g_volume_color_constant;
	/*! When a transfer function is used, this is the value from the volume
		texture that maps to the left/right-most value in the transfer
		function. Everything beyond that uses the same color.*/
	float g_transfer_min_input, g_transfer_max_input;
	//! A factor and summand to get from dequantized volume values to inputs
	//! for the transfer function. Computed from g_transfer_min/max_input.
	float g_transfer_input_factor, g_transfer_input_summand;
	//! The factor by which volume extinctions are multiplied before they are
	//! used for any purpose
	float g_extinction_factor;
	//! The parameters characterizing the fit for the Mie-scattering phase
	//! function
	vec4 g_mie_fit_params;
	//! The size in voxels of 3D textures holding upper bounds on extinction
	//! values and the component-wise reciprocal
	vec3 g_brick_counts, g_inv_brick_counts;
	//! The size in voxels of bricks used to compute bounds on extinction
	//! values
	uint g_brick_size;
	//! g_brick_size as float and its reciprocal
	float g_brick_size_float, g_inv_brick_size;
	//! The factor by which radiance values from the light probe are multiplied
	float g_ambient_brightness;
	//! The factor by which the ambient radiance is multiplied when it is used
	//! to illuminate a volume
	float g_ambient_lighting_factor;
	//! The reciprocal of the integral over luminance of the light probe
	//! (including a factor of g_ambient_brightness)
	float g_inv_light_probe_luminance_integral;
	//! The number of samples to be used for ray marching
	uint g_ray_march_sample_count;
	//! Reciprocal of g_ray_march_sample_count
	float g_inv_ray_march_sample_count;
	//! A factor for the sample count used by techniques of Kettunen et al.
	float g_kettunen_sample_multiplier;
	/*! The spacing between two samples at high resolution in terms of the
		integral over the unnormalized density computed from low-resolution
		values. Antiproportional to the sample count.*/
	float g_optical_depth_cdf_step;
	//! In experiments where generalized MIS weights are being displayed
	//! directly, these are the used parameters
	vec2 g_mis_weight_numerator, g_mis_weight_denominator;
	//! Four floats that can be controlled from the GUI directly and can be
	//! used for any purpose while developing shaders
	vec4 g_params;
	//! A transform from texel space for each volume (i.e. voxel boundaries are
	//! at integer coordinates) to world space
	mat4 g_texel_to_world_space[max(1, VOLUME_COUNT)];
	//! Inverse of g_texel_to_world_space
	mat4 g_world_to_texel_space[max(1, VOLUME_COUNT)];
	//! The point lights illuminating the scene
	point_light_t g_point_lights[max(1, POINT_LIGHT_COUNT)];
};
