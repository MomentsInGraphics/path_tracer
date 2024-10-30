#include "vulkan_basics.h"


//! Holds all header data for a scene file
typedef struct {
	//! Should be 0xabcabc to mark a file as the right file format
	uint32_t marker;
	//! Should be 1 for a static scene
	uint32_t version;
	//! The number of unique materials
	uint64_t material_count;
	//! The number of triangles
	uint64_t triangle_count;
	//! Component-wise multiplication of quantized integer xyz coordinates by
	//! these factors followed by addition of these summands gives world-space
	//! coordinates
	float dequantization_factor[3], dequantization_summand[3];
	//! An array holding material_count null-terminated strings for material
	//! names (with UTF-8 encoding)
	char** material_names;
} scene_file_header_t;


//! Enumeration of the different buffers that are needed to store a mesh
typedef enum {
	//! Each position is stored in 64 bits using 21-bit quantization for each
	//! coordinate. Three subsequent positions form a triangle.
	mesh_buffer_type_positions,
	/*! Per position, this buffer provides 64 bits for the corresponding normal
		and texture coordinate. The normal uses 2x16 bits for an octahedral map
		the texture coordinate 2x16 bits for fixed-point values from 0 to 8.*/
	mesh_buffer_type_normals_and_tex_coords,
	//! An 8-bit index per triangle indicating the used material
	mesh_buffer_type_material_indices,
	//! The number of valid values for this enum
	mesh_buffer_type_count,
} mesh_buffer_type_t;


//! Each material is defined completely by exactly three textures, as listed in
//! this enumeration
typedef enum {
	/*! The diffuse albedo is derived from this base color. For metallic
		materials, the base color also controls the specular albedo. The format
		is usually VK_FORMAT_BC1_RGB_SRGB_BLOCK.*/
	material_texture_type_base_color,
	/*! A texture with three parameters controlling the specular BRDF. The
		format is usually VK_FORMAT_BC1_RGB_UNORM_BLOCK.
		R: An occlusion coefficient that is not used by the renderer currently,
		G: An artist-friendly roughness parameter between zero and one.
		B: Metallicity, i.e. 0 for dielectrics and 1 for metals.*/
	material_texture_type_specular,
	/*! The normal vector of the surface in tangent space using Cartesian
		coordinates. It uses unsigned values such that the geometric normal is
		(0.5, 0.5, 1). The format is usually VK_FORMAT_BC5_UNORM_BLOCK.*/
	material_texture_type_normal,
	//! The number of available texture types
	material_texture_type_count,
} material_texture_type_t;


//! The levels of acceleration structures in Vulkan (top and bottom level)
typedef enum {
	//! Bottom-level acceleration structure (BLAS)
	bvh_level_bottom,
	//! Top-level acceleration structure (TLAS)
	bvh_level_top,
	//! Number of levels
	bvh_level_count,
} bvh_level_t;


//! Holds all acceleration structures for a scene (also known as bounding
//! volume hierarchies, or BVH for short)
typedef struct {
	//! The top- and bottom-level acceleration structure
	VkAccelerationStructureKHR bvhs[bvh_level_count];
	//! The buffers that hold the acceleration structures
	buffers_t buffers;
} bvhs_t;


//! A scene that has been loaded from a scene file and is now device-local
typedef struct {
	//! Header data as it was found in the scene file
	scene_file_header_t header;
	//! mesh_buffer_type_count buffers providing geometry information
	buffers_t mesh_buffers;
	//! material_texture_type_count consecutive textures per material
	images_t textures;
	//! The ray-tracing acceleration structures
	bvhs_t bvhs;
} scene_t;


/*! Loads a scene file and copies its data to device-local memory (i.e. to the
	VRAM of the GPU).
	\param scene The output. Clean up with free_scene().
	\param device Output of create_device().
	\param file_path Path to a *.vks file that is to be loaded.
	\param texture_path Path to a directory containing texture files in the
		*.vkt format.
	\return 0 upon success.*/
int load_scene(scene_t* scene, const device_t* device, const char* file_path, const char* texture_path);


void free_scene(scene_t* scene, const device_t* device);
