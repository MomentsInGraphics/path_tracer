layout (row_major, std140, binding = 0) uniform constants {
	//! The world to projection space transform
	mat4 g_world_to_projection_space;
	//! The projection to world space transform
	mat4 g_projection_to_world_space;
	//! The camera position in world-space coordinates
	vec3 g_camera_pos;
	//! The type of camera to be used, as indicated by the camera_type_t enum
	int g_camera_type;
	//! The normalized normal to feed to get_shading_space() to obtain a local
	//! frame for a projection based on spherical coordinates.
	vec3 g_hemispherical_camera_normal;
	//! Component-wise multiplication of quantized integer xyz coordinates by
	//! these factors followed by addition of these summands gives world-space
	//! coordinates of positions in the loaded scene
	vec3 g_dequantization_factor, g_dequantization_summand;
	//! The width and height of the viewport
	vec2 g_viewport_size;
	//! The reciprocal width and height of the viewport
	vec2 g_inv_viewport_size;
	//! The factor by which the HDR radiance is scaled during tonemapping
	float g_exposure;
	//! The index of the frame being rendered for the purpose of random seed
	//! generation
	uint g_frame_index;
	//! The number of frames which have been accumulated in the HDR radiance
	//! render target. Divided out during tone mapping.
	uint g_accum_frame_count;
	//! The radiance for rays that leave the scene (using Rec. 709, a.k.a.
	//! linear sRGB)
	vec3 g_sky_radiance;
	//! The radiance emitted by the material called _emission (Rec. 709)
	vec3 g_emission_material_radiance;
	//! Four floats that can be controlled from the GUI directly and can be
	//! used for any purpose while developing shaders
	vec4 g_params;
	//! Positions and radii for all spherical lights
	vec4 g_spherical_lights[32];
};
