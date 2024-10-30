#include "mesh_quantization.glsl"
#include "constants.glsl"


//! Three textures for each material providing base color, specular and normal
//! parameters
layout (binding = 1) uniform sampler2D g_textures[3 * MATERIAL_COUNT];

//! Provides quantized world-space positions for each vertex
layout (binding = 3) uniform utextureBuffer g_quantized_vertex_poss;
//! Provides normals and texture coordinates for each vertex
layout (binding = 4) uniform textureBuffer g_octahedral_normal_and_tex_coords;
//! Provides a material index for each triangle
layout (binding = 5) uniform utextureBuffer g_material_indices;


//! Full description of a shading point on a surface and its BRDF. All vectors
//! use the same coordinate frame, which has to be isometric to world space.
struct shading_data_t {
	//! Position of the shading point
	vec3 pos;
	//! The normalized shading normal vector
	vec3 normal;
	//! The normalized outgoing light direction (i.e. towards the camera along
	//! the path)
	vec3 out_dir;
	//! dot(normal, out_dir)
	float lambert_out;
	//! The emitted radiance (Rec. 709) from this shading point into direction
	//! out_dir
	vec3 emission;
	//! The diffuse albedo specified using Rec. 709 (i.e. linear sRGB)
	vec3 diffuse_albedo;
	//! The color of specular reflection (Rec. 709) when looking at the surface
	//! from the zenith
	vec3 fresnel_0;
	//! The roughness parameter of the GGX distribution of normals
	float roughness;
};


/*! Assembles the shading data for a point on the surface of the scene.
	\param triangle_index The index of the scene triangle on which a point is
		given.
	\param barycentrics Barycentric coordinates of the point on the triangle.
		The first barycentric coordinate is omitted.
	\param front true iff the ray hit the front of the triangle (i.e. the
		triangle vertices are clockwise when observed from the ray origin).
	\param out_dir The normalized direction towards the camera along the path.
	\return The complete shading data.*/
shading_data_t get_shading_data(int triangle_index, vec2 barycentrics, bool front, vec3 out_dir) {
	shading_data_t s;
	// Interpolate all vertex attributes for all triangle vertices
	vec3 barys = vec3(1.0 - barycentrics[0] - barycentrics[1], barycentrics[0], barycentrics[1]);
	vec3 poss[3];
	s.pos = vec3(0.0);
	vec3 normal_geo = vec3(0.0);
	vec2 tex_coords[3];
	vec2 tex_coord = vec2(0.0);
	[[unroll]]
	for (int i = 0; i != 3; ++i) {
		int vert_index = triangle_index * 3 + i;
		uvec2 quantized_pos = texelFetch(g_quantized_vertex_poss, vert_index).rg;
		vec4 normal_and_tex_coords = texelFetch(g_octahedral_normal_and_tex_coords, vert_index);
		poss[i] = dequantize_position(quantized_pos, g_dequantization_factor, g_dequantization_summand);
		s.pos += barys[i] * poss[i];
		normal_geo += barys[i] * dequantize_normal(normal_and_tex_coords.xy);
		tex_coords[i] = normal_and_tex_coords.zw * vec2(8.0, -8.0) + vec2(0.0, 1.0);
		tex_coord += barys[i] * tex_coords[i];
	}
	normal_geo = normalize(normal_geo);
	// Sample the material textures
	uint material_index = texelFetch(g_material_indices, triangle_index).r;
	vec3 base_color_tex = texture(g_textures[nonuniformEXT(3 * material_index + 0)], tex_coord).rgb;
	vec3 specular_tex = texture(g_textures[nonuniformEXT(3 * material_index + 1)], tex_coord).rgb;
	vec2 normal_tex = texture(g_textures[nonuniformEXT(3 * material_index + 2)], tex_coord).rg;
	vec3 normal_local;
	normal_local.xy = normal_tex * 2.0 - vec2(1.0);
	normal_local.z = sqrt(max(0.0, (1.0 - normal_local.x * normal_local.x) - normal_local.y * normal_local.y));
	// Transform the normal to world space
	mat2 tex_edges = mat2(tex_coords[1] - tex_coords[0], tex_coords[2] - tex_coords[0]);
	vec3 pre_tangent_0 = cross(normal_geo, poss[1] - poss[0]);
	vec3 pre_tangent_1 = cross(normal_geo, poss[0] - poss[2]);
	vec3 tangent_0 = pre_tangent_1 * tex_edges[0][0] + pre_tangent_0 * tex_edges[1][0];
	vec3 tangent_1 = pre_tangent_1 * tex_edges[0][1] + pre_tangent_0 * tex_edges[1][1];
	float mean_length = sqrt(0.5 * (dot(tangent_0, tangent_0) + dot(tangent_1, tangent_1)));
	mat3 tangent_to_world_space = mat3(tangent_0, tangent_1, normal_geo);
	normal_local.z *= max(1.0e-8, mean_length);
	s.normal = normalize(tangent_to_world_space * normal_local);
	s.normal = front ? s.normal : -s.normal;
	// Adapt the normal such that out_dir is in the upper hemisphere
	s.out_dir = out_dir;
	float normal_offset = max(0.0f, 1.0e-3f - dot(s.normal, s.out_dir));
	s.normal = normalize(fma(vec3(normal_offset), s.out_dir, s.normal));
	// Complete the shading data
	s.lambert_out = dot(s.normal, s.out_dir);
	float metalicity = specular_tex.b;
	s.diffuse_albedo = base_color_tex - metalicity * base_color_tex;
	s.fresnel_0 = mix(vec3(0.02), base_color_tex, metalicity);
	s.roughness = max(0.006, specular_tex.g * specular_tex.g);
	s.emission = (material_index == EMISSION_MATERIAL_INDEX) ? g_emission_material_radiance : vec3(0.0);
	return s;
}
