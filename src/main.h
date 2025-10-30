#pragma once
#include "vulkan_basics.h"
#include "camera.h"
#include "nuklear.h"
#include "compute_graph.h"
#include <stdbool.h>
#include <stdio.h>


//! The number of frames in flight, i.e. how many frames the host submits to
//! the GPU before waiting for the oldest one to finish
#define FRAME_IN_FLIGHT_COUNT 3
//! How many attributes are stored per brick to accelerate volume rendering
//! (grid_indices.glsl provides a list)
#define GRID_ATTRIBUTE_COUNT 5
//! The maximal number of cameras that can be utilized simultaneously
#define MAX_CAMERA_COUNT 1
//! The maximal number of slides
#define MAX_SLIDE_COUNT 300


//! Available tonemapping operators
typedef enum {
	//! Simply convert the linear radiance values to sRGB, clamping channels
	//! that are above 1
	tonemapper_op_clamp,
	//! The tonemapper from the Academy Color Encoding System
	tonemapper_op_aces,
	//! The Khronos PBR neutral tone mapper as documented here:
	//! https://github.com/KhronosGroup/ToneMapping/blob/main/PBR_Neutral/README.md
	tonemapper_op_khronos_pbr_neutral,
	//! The number of available tonemapping operators
	tonemapper_op_count,
	//! Used to force an int32_t
	tonemapper_op_force_int_32 = 0x7fffffff,
} tonemapper_op_t;


//! Defines how the color and extinction of the volume are determined
typedef enum {
	//! The albedo of the volume is a constant, user-defined color, the
	//! extinction is as defined in the volume
	volume_color_mode_constant,
	//! The albedo of the volume is taken from a transfer function, the
	//! extinction is as defined in the volume
	volume_color_mode_transfer_function,
	/*! The albedo of the volume is taken from a transfer function, the
		extinction is replaced by the alpha from this transfer function,
		scaled by the user-defined extinction factor.*/
	volume_color_mode_transfer_function_with_extinction,
	//! There are two separate volumes: One holds extinction values, the other
	//! holds colors.
	volume_color_mode_separate,
	//! Number of different modes
	volume_color_mode_count,
	//! Used to force an int32_t
	volume_color_mode_force_int_32 = 0x7fffffff,
} volume_color_mode_t;


//! An enumeration of all supported modes of interpolation for volume data
typedef enum {
	//! Nearest-neighbor interpolation
	volume_interpolation_nearest,
	//! Trilinear interpolation
	volume_interpolation_linear,
	//! Approximate trilinear interpolation using stochastic jittering of ray
	//! origins
	volume_interpolation_linear_stochastic,
	//! Number of available interpolation methods
	volume_interpolation_count,
	//! Used to force an int32_t
	volume_interpolation_force_int_32 = 0x7fffffff,
} volume_interpolation_t;


//! Different compression methods for low-resolution grids derived from a high-
//! resolution volume texture
typedef enum {
	//! Corresponds to VK_FORMAT_R16_SFLOAT
	grid_compression_float_16,
	//! Corresponds to VK_FORMAT_BC4_UNORM_BLOCK
	grid_compression_bc4,
	//! Number of available compression methods
	grid_compression_count,
	//! Used to force an int32_t for the enum (not a compression technique)
	grid_compression_force_int_32 = 0x7fffffff,
} grid_compression_t;


//! Enumerates all available techniques for distance sampling in volume
//! rendering
typedef enum {
	//! Ray marching with uniform jittered sampling. This method is biased.
	distance_sampler_ray_marching,
	//! Regular tracking
	distance_sampler_regular_tracking,
	//! Delta tracking based on null scattering with upper bounds on extinction
	distance_sampler_delta_tracking,
	//! Number of available techniques
	distance_sampler_count,
	//! Used to force an int32_t
	distance_sampler_force_int_32 = 0x7fffffff,
} distance_sampler_t;


//! The technique used to estimate optical depth for techniques that rely on
//! unbiased, independent, identically distributed estimates.
typedef enum {
	/*! Ray marching with roughly equidistant steps and user-defined sample
		count. If uniform jittered sampling is enabled, they are exactly
		equidistant.*/
	optical_depth_estimator_equidistant,
	/*! Ray marching with adaptive sample count and importance sampling based
		on mean extinction as described by Kettunen et al. [2021]:
		https://doi.org/10.1145/3450626.3459937 */
	optical_depth_estimator_kettunen,
	//! Places samples in proportion to the mean estimated on the low-
	//! resolution grid and evaluates without using a control variate.
	optical_depth_estimator_mean_sampling,
	//! Same importance sampling as for optical_depth_estimator_mean_sampling
	//! but with a fixed sample count
	optical_depth_estimator_mean_sampling_fixed_count,
	/*! Uses a control variate w.r.t. the minimal extinction at low resolution.
		The sample density follows the square root of second moments of
		extinctions on the low resolution, relative to the minimum.*/
	optical_depth_estimator_variance_aware,
	//! Same importance sampling as for optical_depth_estimator_variance_aware
	//! but with a fixed sample count
	optical_depth_estimator_variance_aware_fixed_count,
	//! Number of available techniques
	optical_depth_estimator_count,
	//! Used to force an int32_t
	optical_depth_estimator_force_int_32 = 0x7fffffff,
} optical_depth_estimator_t;


//! Enumerates all available techniques for transmittance estimation in volume
//! rendering
typedef enum {
	//! Always estimates the transmittance to be one. Can be useful for
	//! timings of distance sampling that are not contaminated by the cost of
	//! transmittance estimation.
	transmittance_estimator_dummy,
	//! An unbiased ray-marching transmittance estimator as described by
	//! Kettunen et al. [2021]: https://doi.org/10.1145/3450626.3459937
	transmittance_estimator_unbiased_ray_marching,
	//! A biased ray-marching transmittance estimator as described by Kettunen
	//! et al. [2021]: https://doi.org/10.1145/3450626.3459937
	transmittance_estimator_biased_ray_marching,
	/*! A ray-marching transmittance estimator that estimates mean and variance
		of optical depth and uses that to reduce its bias by assuming a
		Gaussian distribution.*/
	transmittance_estimator_jackknife,
	//! Regular tracking
	transmittance_estimator_regular_tracking,
	//! Track length estimation (with a binary outcome) based on upper bounds
	//! on extinction
	transmittance_estimator_track_length,
	//! Ratio tracking based on null scattering with upper bounds on extinction
	transmittance_estimator_ratio_tracking,
	//! Number of available techniques
	transmittance_estimator_count,
	//! Used to force an int32_t
	transmittance_estimator_force_int_32 = 0x7fffffff,
} transmittance_estimator_t;


//! Enumerates all available techniques to estimate transmittance-weighted MIS
//! weights in volume rendering
typedef enum {
	//! A dummy technique that returns the result for a transmittance of zero
	mis_weight_estimator_dummy,
	//! Our estimation of MIS weights using a power series and ray marching
	mis_weight_estimator_unbiased_ray_marching,
	//! Our estimation of MIS weights using a degree-zero power series and ray
	//! marching
	mis_weight_estimator_biased_ray_marching,
	//! Our estimation of MIS weights using jackknife statistics
	mis_weight_estimator_jackknife,
	//! Exact computation of the MIS weight through exact transmittance
	//! computation
	mis_weight_estimator_regular_tracking,
	//! Number of available techniques
	mis_weight_estimator_count,
	//! Used to force an int32_t
	mis_weight_estimator_force_int_32 = 0x7fffffff,
} mis_weight_estimator_t;


//! Which sampling strategies are being used and how they are being combined
typedef enum {
	//! Distance sampling based on the light source
	sampling_strategies_light,
	//! Distance sampling proportional to extinction times transmittance
	sampling_strategies_transmittance,
	//! A combination of light and transmittance-based distance sampling using
	//! multiple importance sampling with the balance heuristic
	sampling_strategies_light_transmittance_mis,
	//! A combination of light and transmittance-based distance sampling using
	//! multiple importance sampling in such a way that biased MIS weights do
	//! not cause bias in the overall estimate.
	sampling_strategies_light_transmittance_approximate_mis,
	/*! A combination of light and transmittance-based distance sampling, which
		uses paths with null-scattering vertices to implement multiple
		importance sampling: https://doi.org/10.1145/3306346.3323025 */
	sampling_strategies_light_transmittance_miller_mis,
	//! The number of available strategies
	sampling_strategies_count,
	//! Used to force an int32_t
	sampling_strategies_force_int_32 = 0x7fffffff,
} sampling_strategies_t;


//! An enumeration of different quantities that may get accumulated in the
//! frame buffer. Mostly used for debugging and analysis.
typedef enum {
	//! RGB radiance estimates, i.e. a proper rendering
	displayed_quantity_radiance,
	//! Luminance estimates as red, squares of luminance estimates (i.e. second
	//! moments) as green, sample counts as blue
	displayed_quantity_luminance_stats,
	//! Transmittance estimates, replicated to all three color channels
	displayed_quantity_transmittance,
	//! Transmittance estimates as red, squares of transmittance estimates
	//! (i.e. second moments) as green, sample counts as blue
	displayed_quantity_transmittance_stats,
	//! Transmittance estimates as red, 1 in green if their sign was negative,
	//! 0 otherwise
	displayed_quantity_transmittance_sign,
	//! A generalized MIS weight with user-defined parameters in all channels
	displayed_quantity_mis_weight,
	//! Like displayed_quantity_transmittance_stats but with MIS weights
	displayed_quantity_mis_weight_stats,
	//! The number of available options
	displayed_quantity_count,
	//! Used to force an int32_t
	displayed_quantity_force_int_32 = 0x7fffffff,
} displayed_quantity_t;


//! A specification of a high-dynamic range color that can be conveniently
//! edited in a user interface
typedef struct {
	//! The color as (non-linear) sRGB
	float srgb_color[3];
	//! A brightness scaling factor that applies to the linear color (Rec. 709)
	float linear_factor;
} hdr_color_spec_t;


//! An idealized point light with equal emission into all directions
typedef struct {
	//! The position of the point light
	float pos[3];
	//! The radiant power of the point light
	hdr_color_spec_t power;
} point_light_t;


//! The representation of a point light in a constant buffer, including padding
typedef struct {
	float pos[3], pad_0, power[3], importance;
} point_light_constants_t;


/*! A complete specification of what is to be rendered without specifying
	different techniques that are supposed to render the same thing. This
	struct is what quicksave/quickload deal with.*/
typedef struct {
	//! The cameras used to observe the scene
	camera_t cameras[MAX_CAMERA_COUNT];

	//! The tonemapping operator used to present colors
	tonemapper_op_t tonemapper;
	//! The factor by which HDR radiance is scaled during tonemapping
	float exposure;

	//! The file path or file path pattern from which one or more volumes will
	//! be loaded
	char* volume_file_path;
	/*! The file path for the compute shader that is to be used to turn volumes
		loaded from files into volumes for display. An empty string indicates
		that the first loaded volume is used as is.*/
	char* volume_shader_path;
	//! Defines how colors are computed for a volume
	volume_color_mode_t volume_color_mode;
	//! When a constant color is used as volume albedo, it is this color
	hdr_color_spec_t volume_color_constant;
	/*! When a transfer function is used, this is the value from the volume
		texture that maps to the left/right-most value in the transfer
		function. Everything beyond that uses the same color.*/
	float transfer_min_input, transfer_max_input;
	//! Whether the transfer function should use bilinear interpolation (true)
	//! or nearest neighbor
	bool interpolate_transfer_function;
	//! The factor by which volume extinctions are multiplied before they are
	//! used for any purpose
	float extinction_factor;
	//! The droplet size (diameter in micrometers) used to control the Mie-
	//! scattering phase function
	float droplet_size;
	//! How values in the volume are being interpolated
	volume_interpolation_t volume_interpolation;

	//! The file path of the transfer function to be used
	char* transfer_function_path;

	//! The index of the frame being rendered for the purpose of random seed
	//! generation
	uint32_t frame_index;

	//! Whether ambient light should be used at all
	bool use_ambient_light;
	//! The file path to the light probe that is being used (a *.hdr file)
	char* light_probe_path;
	//! The radiance emitted by the ambient space in all directions
	hdr_color_spec_t ambient_radiance;
	/*! A scaling factor for the ambient radiance when it is used to illuminate
		a volume rather than being observed directly. This is a non-physical
		hack to avoid overexposure of the background.*/
	float ambient_lighting_factor;

	//! Number of point lights in the scene
	uint64_t point_light_count;
	//! Point lights in the scene
	point_light_t* point_lights;

	//! Four floats that can be controlled from the GUI directly and can be
	//! used for any purpose while developing shaders
	float params[4];
} scene_spec_t;


//! A specification of all the techniques and parameters used to render the
//! scene without specifying the scene itself
typedef struct {
	//! Whether jittering of primary rays is enabled or not
	bool jitter_primary_rays;
	//! The brick size as edge length in voxels used to generate the
	//! low-resolution grids
	uint32_t brick_size;
	//! The method used to compress low-resolution grids
	grid_compression_t grid_compression;
	//! The technique used to sample free-flight distances in the volume
	distance_sampler_t distance_sampler;
	//! The technique used to estimate optical depth
	optical_depth_estimator_t optical_depth_estimator;
	//! Whether jittered (false) or uniform  jittered (true) sampling should be
	//! used to estimate optical depth
	bool uniform_jittered;
	//! The technique used to estimate transmittance for segments through the
	//! volume
	transmittance_estimator_t transmittance_estimator;
	//! The technique used to estimate transmittance-weighted MIS weights for
	//! distance sampling
	mis_weight_estimator_t mis_weight_estimator;
	//! Which sampling strategies are being used and how they are being
	//! combined
	sampling_strategies_t sampling_strategies;
	//! Which quantities should be rendered to the HDR buffer
	displayed_quantity_t displayed_quantity;
	//! The number of samples to be used for volumetric ray marching
	uint32_t ray_march_sample_count;
	/*! The spacing between two samples at high resolution in terms of the
		integral over the unnormalized density computed from low-resolution
		values. Antiproportional to the sample count.*/
	float optical_depth_cdf_step;
	//! A factor for the sample count used by techniques of Kettunen et al.
	float kettunen_sample_multiplier;
	//! The number of samples per pixel per frame per type of sample
	uint32_t sample_count;
	//! In experiments where generalized MIS weights are being displayed
	//! directly, these are the used parameters
	float mis_weight_numerator[2], mis_weight_denominator[2];
} render_settings_t;


//! Available image file formats for taking screenshots
typedef enum {
	//! Portable network graphics using 3*8-bit RGB
	image_file_format_png,
	//! Joint Photographic Experts Group (JPEG) using 3*8-bit RGB
	image_file_format_jpg,
	//! High-dynamic range image with shared exponent format
	image_file_format_hdr,
	//! High-dynamic range image with 3*32-bit RGB using single-precision
	//! floats
	image_file_format_pfm,
} image_file_format_t;


//! Defines a slide, i.e. a configuration of the renderer that leads to a
//! specific image and can be used in a presentation
typedef struct {
	//! The file path to the quicksave that is to be loaded for this slide
	char* quicksave;
	//! The render settings to use for this slide
	render_settings_t render_settings;
	//! An override for the used volume interpolation or an invalid value to
	//! use what the quicksave specifies
	volume_interpolation_t volume_interpolation;
	//! An override for the used tonemapper or an invalid value to use what the
	//! quicksave specifies
	tonemapper_op_t tonemapper;
	//! The file path to which a screenshot should be saved or NULL if no
	//! screen shot should be saved
	char* screenshot_path;
	//! The file format to use for the screenshot
	image_file_format_t screenshot_format;
	/*! The number of accumulated frames at which the screenshot should be
		taken. This essentially controls the sample count. After the screenshot
		the slideshow advances to the next slide.*/
	uint32_t screenshot_frame;
} slide_t;


//! A set of slides through which the presentation can advance
typedef struct {
	//! All slides that make up this slideshow
	slide_t slides[MAX_SLIDE_COUNT];
	//! The number of slides
	uint32_t slide_count;
	//! The first slide that is to be displayed and the slide at which the
	//! application should terminate. Equal if no slides are used.
	uint32_t slide_begin, slide_end;
	//! The slide that is currently being displayed
	uint32_t slide_current;
} slideshow_t;


//! Parameters for the application
typedef struct {
	//! The requested initial extent in pixels for the window. May be ignored
	//! initially and may diverge at run time.
	VkExtent2D initial_window_extent;
	//! Whether screenshots of slides should be taken automatically
	bool slide_screenshots;
	//! Whether the GUI should be displayed
	bool gui;
	//! Whether vertical synchronization should be enabled
	bool v_sync;
} app_params_t;


//! The vertex buffer for GUI rendering is an array of vertices in the
//! following format
typedef struct {
	//! The position in pixels from the left top of the viewport
	float pos[2];
	//! The texture coordinate for the glyph image
	float tex_coord[2];
	//! The color (RGBA) using sRGB
	uint8_t color[4];
	//! The left, right, top and bottom of the scissor rectangle
	int16_t scissor[4];
} gui_vertex_t;


//! The graphical user interface using Nuklear
typedef struct {
	//! The font atlas used for text in the GUI
	struct nk_font_atlas atlas;
	//! The font used for text in the GUI
	struct nk_font* font;
	//! The glyph image using VK_FORMAT_R8_UNORM
	images_t glyph_image;
	//! Points to a white pixel in the glyph image
	struct nk_draw_null_texture null_texture;
	//! The Nuklear context, which handles persistent state of the GUI
	struct nk_context context;
	//! The maximal number of triangles supported for the GUI. If more are
	//! needed, GUI rendering fails.
	uint32_t max_triangle_count;
	//! Host-visible versions of the vertex buffer for GUI rendering. This
	//! buffer is stored redundantly per frame in flight (a.k.a. workload).
	buffers_t staging;
	//! A pointer to the mapped memory range for the shared allocation of all
	//! staging buffers
	void* staging_data;
	//! For each staging vertex buffer, this count indicates how many triangles
	//! need to be rendered
	uint32_t used_triangle_counts[FRAME_IN_FLIGHT_COUNT];
	//! A single device-local buffer that contains the vertex buffer for the
	//! frame being rendered, thanks to a copy command at the start of
	//! rendering
	buffers_t buffer;
	//! Whether the cursor has been disabled because a property value is being
	//! dragged
	bool cursor_disabled;
} gui_t;


//! Defines unique indices for all the render targets handled by
//! render_targets_t
typedef enum {
	//! An RGBA render target with single-precision floats at the same
	//! resolution as the swapchain to hold HDR radiance
	render_target_index_hdr_radiance,
	//! A depth buffer with the same resolution as the swapchain
	render_target_index_depth_buffer,
	//! The number of used render targets
	render_target_index_count,
} render_target_index_t;


//! Handles all render targets
typedef struct {
	//! All the render targets, indexed by render_target_index_t
	images_t targets;
	//! The number of samples which have been accumulated in the HDR radiance
	//! render target
	uint32_t accum_frame_count;
} render_targets_t;


/*! The CPU-side version of the constants defined in constants.glsl. Refer to
	this file for member documentation. Includes padding as required by GLSL.
	Variable-size arrays are excluded.*/
typedef struct {
	float world_to_projection_space[MAX_CAMERA_COUNT][4 * 4];
	float projection_to_world_space[MAX_CAMERA_COUNT][4 * 4];
	float viewport_transforms[MAX_CAMERA_COUNT][4];
	float viewport_size[2], inv_viewport_size[2];
	float exposure;
	uint32_t frame_index;
	uint32_t accum_frame_count;
	uint32_t frame_sample_count;
	float volume_extent[3], pad_3, inv_volume_extent[3], pad_4;
	float volume_color_constant[3];
	float transfer_min_input, transfer_max_input, transfer_input_factor, transfer_input_summand;
	float extinction_factor;
	float mie_fit_params[4];
	float brick_counts[3], pad_6, inv_brick_counts[3];
	uint32_t brick_size; // 4
	float brick_size_float, inv_brick_size;
	float ambient_brightness;
	float ambient_lighting_factor; // 4
	float inv_light_probe_luminance_integral;
	uint32_t ray_march_sample_count;
	float inv_ray_march_sample_count;
	float kettunen_sample_multiplier; // 4
	float optical_depth_cdf_step, pad_7;
	float mis_weight_numerator[2]; // 4
	float mis_weight_denominator[2], pad_8[2]; // 4
	float params[4];
} fixed_size_constants_t;


//! Handles all constant buffers that the application works with
typedef struct {
	//! Host-visible versions of the constant buffer. These buffers are stored
	//! redundantly per frame in flight.
	buffers_t staging;
	//! A pointer to the mapped memory range for the shared allocation of all
	//! staging buffers
	void* staging_data;
	//! A single device-local buffer that contains the constant data for the
	//! frame that is currently being rendered, because at the start of each
	//! frame, data from the appropriate staging buffer is copied over.
	buffers_t buffer;
} constant_buffers_t;


//! One or more volumes to be visualized. This is the version of the volumes
//! prior to mapping.
typedef struct {
	//! The number of 3D volumes held by this object.
	uint32_t volume_count;
	/*! Either a format string that expects one uint32_t to produce the file
		path of the volume with a given index, or NULL if the path from the
		scene specification is used directly.*/
	char* file_path_pattern;
	//! A file for each volume, from which it is being loaded. NULL pointers
	//! once loading has concluded.
	FILE** files;
	//! Consecutive 3x4 matrices describing the tranform from texel space to
	//! world space for each volume
	float* texel_to_world_space;
	//! One 3D image per volume
	images_t volume;
} volume_t;


/*! Various kinds of acceleration structures (AS) used for volume rendering and
	the compute workloads needed to keep them up to date when the volume
	changes. For the most part these are just grids with various aggregate
	quantities but if there is a volume shader it is also handled here.*/
typedef struct {
	//! If there is no volume shader, this object is empty. Otherwise, it holds
	//! the output values and optionally output colors of the volume shader.
	images_t shaded_volume;
	//! One 3D image per grid of precomputed quantities. Indices match
	//! grid_indices.glsl.
	images_t grids;
	/*! The first buffer holds one float[4] per grid to store dequantization
		parameters. Additionally, when BC4 compression is used, there is one
		buffer per grid holding intermediate values and output of the reduce
		operation to produce statistics needed to determine optimal
		quantization coefficients.*/
	buffers_t grid_props;
	//! Image views to sample block-compressed textures (if available)
	VkImageView bc_views[GRID_ATTRIBUTE_COUNT];
	//! A compute workload that, when dispatched, turns the current data of the
	//! volume into the required grids
	compute_workload_t workload;
	/*! The transfer_min_input and transfer_max_input values of scene_spec_t
		that were used when the compute workload was last run. Used to detect
		the need to rerun it. Both zero if it was not run at all.*/
	float transfer_min_input, transfer_max_input;
} volume_as_t;


//! An RGB and HDR light probe in spherical coordinates along with an alias
//! table for importance sampling
typedef struct {
	//! Image data of the HDR image or NULL once creation has finished
	float* image_data;
	//! The HDR light probe using an RGBA format
	images_t probe;
	//! A buffer for the alias table
	buffers_t alias_table;
	//! The sum of all the weights that were used to build the alias table
	float weight_sum;
	//! The sampler used to sample the light probe
	VkSampler sampler;
} light_probe_t;


//! Holds a transfer function in the form of a 1D texture
typedef struct {
	//! A single 1D image describing the transfer function
	images_t transfer_function;
	//! The name of the transfer function that is being used
	char* name;
	//! If the transfer function specified in scene specification is not found,
	//! a sensible default is used and this bool is true.
	bool found;
	//! A sampler with linear or nearest-neighbor interpolation used to access
	//! the transfer function
	VkSampler sampler;
} transfer_function_t;


//! The render pass that performs all rasterization work for rendering one
//! frame of the application and the framebuffers that it uses
typedef struct {
	//! The render pass itself
	VkRenderPass render_pass;
	//! The framebuffer used with render_pass. Duplicated once per swapchain
	//! image.
	VkFramebuffer* framebuffers;
	//! The number of array entries in framebuffers
	uint32_t framebuffer_count;
} render_pass_t;


//! The objects needed for a subpass that renders the scene
typedef struct {
	//! A sampler for 2D textures
	VkSampler texture_sampler;
	//! A sampler for volume textures
	VkSampler volume_sampler;
	//! The descriptor set used to render the scene
	descriptor_sets_t descriptor_set;
	//! A graphics pipelines used to render the scene, which discards the
	//! current contents of the render target
	VkPipeline pipeline_discard;
	//! Like pipeline_discard but adds onto the current contents of the render
	//! target for the purpose of progressive rendering
	VkPipeline pipeline_accum;
	//! The fragment and vertex shaders used to render the scene
	VkShaderModule vert_shader, frag_shader;
} scene_subpass_t;


//! The objects needed to copy the HDR render target to the screen with tone
//! mapping
typedef struct {
	//! The descriptor set used for tone mapping
	descriptor_sets_t descriptor_set;
	//! The graphics pipeline used for tone mapping
	VkPipeline pipeline;
	//! The fragment and vertex shaders used for tone mapping
	VkShaderModule vert_shader, frag_shader;
} tonemap_subpass_t;


//! The objects needed for a subpass that renders the GUI to the screen
typedef struct {
	//! A sampler for the glyph image
	VkSampler sampler;
	//! The descriptor set used to render the GUI
	descriptor_sets_t descriptor_set;
	//! The graphics pipeline used to render the GUI
	VkPipeline pipeline;
	//! The fragment and vertex shaders used to render the GUI
	VkShaderModule vert_shader, frag_shader;
} gui_subpass_t;


//! Indices for timestamp queries in query pools
typedef enum {
	//! Encloses the commands that perform the main shading work
	timestamp_index_shading_begin, timestamp_index_shading_end,
	//! The number of used timestamps
	timestamp_index_count,
} timestamp_index_t;


//! A command buffer for rendering a single frame and all the synchronization
//! primitives that are needed for it
typedef struct {
	//! The command buffer containing all commands for rendering a single
	//! frame. It is reused.
	VkCommandBuffer cmd;
	//! Signaled once an image has been acquired from the swapchain
	VkSemaphore image_acquired;
	//! Signaled once the command buffer has completed execution
	VkSemaphore queue_finished[2];
	//! Signaled once the command buffer has completed execution
	VkFence frame_finished;
	//! The query pool used to retrieve timestamps for GPU work execution. Its
	//! timestamps are indexed by timestamp_index_t.
	VkQueryPool query_pool;
} frame_workload_t;


//! Handles an array of frame workloads
typedef struct {
	//! Before waiting for a fence of a frame workload, the host will submit
	//! further frames. The size of this array determines how many.
	frame_workload_t frames_in_flight[FRAME_IN_FLIGHT_COUNT];
	//! The number of frames that have been rendered since this object was
	//! created. The workload index is frame_index % FRAME_IN_FLIGHT_COUNT.
	uint64_t frame_index;
	//! The most recently retrieved values of GPU timestamps
	uint64_t timestamps[timestamp_index_count];
} frame_workloads_t;


//! All state of the application that has a chance of persisting across a frame
//! is found somewhere in the depths of this structure
typedef struct {
	scene_spec_t scene_spec;
	render_settings_t render_settings;
	slideshow_t slideshow;
	//! Parameters specified when running the application. Some of them can be
	//! changed continuously.
	app_params_t params;
	//! The Vulkan device used for rendering and other GPU work
	device_t device;
	//! The window to which the application renders
	GLFWwindow* window;
	gui_t gui;
	//! The swapchain used to present images to the window
	swapchain_t swapchain;
	render_targets_t render_targets;
	constant_buffers_t constant_buffers;
	transfer_function_t transfer_function;
	volume_t volume;
	volume_as_t volume_as;
	light_probe_t light_probe;
	render_pass_t render_pass;
	scene_subpass_t scene_subpass;
	tonemap_subpass_t tonemap_subpass;
	gui_subpass_t gui_subpass;
	frame_workloads_t frame_workloads;
} app_t;


/*! Holds one boolean per object in app_t that requires work to be freed. If
	the boolean is true, the object and all objects that depend on it will be
	freed and recreated by update_app().*/
typedef struct {
	bool device, window, gui, swapchain, render_targets, constant_buffers, transfer_function, volume, volume_as, light_probe, render_pass, scene_subpass, tonemap_subpass, gui_subpass, frame_workloads;
} app_update_t;


//! Temporary objects needed to take a screenshot
typedef struct {
	//! The image in host memory to which the off-screen render target is
	//! copied
	images_t staging;
	//! A copy of the image using single-precision floats
	float* hdr_copy;
	//! A copy of the image using 24-bit LDR colors
	uint8_t* ldr_copy;
} screenshot_t;


//! Saves the scene specification to the given quicksave file (overwriting it).
//! Returns 0 upon success. path defaults to data/quicksave.jack_save.
int quicksave(const scene_spec_t* spec, const char* path);


/*! Loads the scene specification from the given quicksave file and flags
	required application updates. Returns 0 upon success. path defaults to
	data/quicksave.jack_save.*/
int quickload(scene_spec_t* spec, app_update_t* update, const char* path);


//! Given an old and a new scene specification, this function marks the updates
//! that need to be performed in response to this change
void get_scene_spec_updates(app_update_t* update, const scene_spec_t* old_spec, const scene_spec_t* new_spec);


//! Given an old and a new set of rendering settings, this function marks the
//! updates that need to be performed in response to this change
void get_render_settings_updates(app_update_t* update, const render_settings_t* old_settings, const render_settings_t* new_settings);


//! Returns true if sample accumulation in progressive rendering should be
//! reset because the scene spec or render settings have changed
bool check_accumulation_reset(const scene_spec_t* old_spec, const scene_spec_t* new_spec, const render_settings_t* old_settings, const render_settings_t* new_settings, const app_params_t* old_params, const app_params_t* new_params);


//! Not every combination of techniques can render every scene. This function
//! modifies the given render settings in place to ensure that they can.
void make_render_settings_valid(render_settings_t* settings, const scene_spec_t* scene_spec);


/*! Returns the scene specification with all entries set to defaults. The
	calling side is responsible for freeing it, but when allow_malloc is false
	fields that require memory allocation will be left zero initialized.*/
scene_spec_t get_default_scene_spec(bool allow_malloc);


//! Creates a deep copy of the given scene spec
scene_spec_t copy_scene_spec(const scene_spec_t* spec);


void free_scene_spec(scene_spec_t* spec);


//! Initializes the given render settings with defaults
void init_render_settings(render_settings_t* settings);


//! Turns a color specification into a Rec. 709 (a.k.a. linear sRGB) color
void get_linear_color(float linear_color[3], const hdr_color_spec_t* color_spec);


//! Writes all available slides into the given array and returns the slide
//! count. Defined in slides.c.
uint32_t create_slides(slide_t* slides);


//! Creates all slides and starts a slideshow as requested by slide_begin/
//! slide_end. Returns 0 upon success.
int create_slideshow(slideshow_t* slideshow, scene_spec_t* scene_spec, render_settings_t* render_settings, app_update_t* update);


//! Begins showing the slide with the given index and updates app objects
//! accordingly. Returns 0 upon success.
int show_slide(slideshow_t* slideshow, scene_spec_t* scene_spec, render_settings_t* render_settings, app_update_t* update, uint32_t slide_index);


//! Frees memory associated with a slideshow
void free_slideshow(slideshow_t* slideshow);


//! Creates the application window using the given extent in pixels, returns 0
//! upon success
int create_window(GLFWwindow** window, const VkExtent2D* extent);


void free_window(GLFWwindow** window);


//! \see gui_t
int create_gui(gui_t* gui, const device_t* device, GLFWwindow* window);


//! Feeds user input retrieved via GLFW to the GUI (also polls events)
void handle_gui_input(gui_t* gui, GLFWwindow* window);


//! Lets Nuklear produce vertex/index buffers and updates the set of staging
//! buffers for GUI geometry with the given workload index.
int write_gui_geometry(gui_t* gui, const device_t* device, uint32_t workload_index);


void free_gui(gui_t* gui, const device_t* device, GLFWwindow* window);


//! \see render_targets_t
int create_render_targets(render_targets_t* render_targets, const device_t* device, const swapchain_t* swapchain);


void free_render_targets(render_targets_t* render_targets, const device_t* device);


//! \see constant_buffers_t
int create_constant_buffers(constant_buffers_t* constant_buffers, const device_t* device, const scene_spec_t* scene_spec, const volume_t* volume);


void free_constant_buffers(constant_buffers_t* constant_buffers, const device_t* device);


//! Updates constant_buffers->constants based on the state of the app and moves
//! its contents to the staging buffer with the given index instantly
int write_constant_buffer(constant_buffers_t* constant_buffers, const app_t* app, uint32_t buffer_index);


//! \see volume_t
int create_volume(volume_t* volume, const device_t* device, const scene_spec_t* scene);


void free_volume(volume_t* volume, const device_t* device);


//! \see volume_as_t
int create_volume_acceleration_structure(volume_as_t* as, const device_t* device, const volume_t* volume, const transfer_function_t* transfer, const constant_buffers_t* constants, const scene_spec_t* scene_spec, const render_settings_t* settings);


void free_volume_acceleration_structure(volume_as_t* as, const device_t* device);


//! Returns a pointer to the volume texture after volume shading (if any) and
//! before application of a transfer function (if any)
static inline const image_t* get_volume_values(const volume_t* volume, const volume_as_t* as) {
	return (as->shaded_volume.image_count > 0) ? &as->shaded_volume.images[0] : &volume->volume.images[0];
}


//! Returns a pointer to the volume texture holding colors per voxel or NULL if
//! that does not exist
static inline const image_t* get_volume_colors(const volume_as_t* as) {
	return (as->shaded_volume.image_count > 1) ? &as->shaded_volume.images[1] : NULL;
}


//! \see transfer_function_t
int create_transfer_function(transfer_function_t* transfer, const device_t* device, const scene_spec_t* spec);


void free_transfer_function(transfer_function_t* transfer, const device_t* device);


//! \see light_probe_t
int create_light_probe(light_probe_t* probe, const device_t* device, const char* probe_path);


void free_light_probe(light_probe_t* probe, const device_t* device);


//! \see render_pass_t
int create_render_pass(render_pass_t* render_pass, const device_t* device, const swapchain_t* swapchain, const render_targets_t* targets);


void free_render_pass(render_pass_t* render_pass, const device_t* device);


//! \see scene_subpass_t
int create_scene_subpass(scene_subpass_t* subpass, const device_t* device, const scene_spec_t* scene_spec, const render_settings_t* settings, const swapchain_t* swapchain, const constant_buffers_t* constant_buffers, const volume_t* volume, const volume_as_t* as, const transfer_function_t* transfer, const light_probe_t* probe, const render_pass_t* render_pass);


void free_scene_subpass(scene_subpass_t* subpass, const device_t* device);


//! \see tonemap_subpass_t
int create_tonemap_subpass(tonemap_subpass_t* subpass, const device_t* device, const render_targets_t* render_targets, const constant_buffers_t* constant_buffers, const render_pass_t* render_pass, const scene_spec_t* scene_spec);


void free_tonemap_subpass(tonemap_subpass_t* subpass, const device_t* device);


//! \see gui_subpass_t
int create_gui_subpass(gui_subpass_t* subpass, const device_t* device, const gui_t* gui, const swapchain_t* swapchain, const constant_buffers_t* constant_buffers, const render_pass_t* render_pass);


void free_gui_subpass(gui_subpass_t* subpass, const device_t* device);


//! \see frame_workloads_t
int create_frame_workloads(frame_workloads_t* workloads, const device_t* device);


void free_frame_workloads(frame_workloads_t* workloads, const device_t* device);


//! Returns true iff anything is marked for update.
bool update_needed(const app_update_t* update);


/*! Whenever objects in app_t are created, recreated or freed, this function
	triggers it.
	\param app The app in which objects will be freed (if necessary) and
		(re-)created if requested.
	\param update All objects marked in this struct will be freed (if
		necessary) and (re-)created (if requested). Additionally, all objects
		which depend on freed objects will be freed (if necessary) and
		(re-)created (if requested).
	\param recreate false to only free objects, true to ensure that upon
		success all objects in app_t have been created.
	\return 0 upon success. With recreate == false, success is guaranteed. Upon
		failure, the return code of the create function that has failed is
		returned.*/
int update_app(app_t* app, const app_update_t* update, bool recreate);


/*! Copies in the given objects (if any), initializes scene specification and
	render settings and forwards to update_app() to create all other objects in
	the given app.*/
int create_app(app_t* app, const app_params_t* app_params, const slideshow_t* slideshow);


//! Short-hand to use update_app() to free all objects in the given app
void free_app(app_t* app);


//! \return true iff the key with the given GLFW_KEY_* keycode is pressed now
//!		but was not pressed at the last query (or there was no last query)
bool key_pressed(GLFWwindow* window, int key_code);


/*! Computes the camera ray for a given pixel.
	\param out_ray_origin The output world-space ray origin.
	\param out_ray_direction The output normalized world-space ray direction.
	\param camera The used camera.
	\param viewport left, right, top, bottom in pixels of the viewport as
		produced by compute_camera_viewports().
	\param pixel The screen-space coordinate for which a ray should be computed
		in pixels.*/
void get_camera_ray(float out_ray_origin[3], float out_ray_dir[3], const camera_t* camera, const float viewport[4], const float pixel[2]);


/*! Responds to user input, updating the state of the given app accordingly and
	triggering all sorts of requested actions. Invoke this once per frame.
	\param app The app whose state is to be updated.
	\param update Used to report required updates.
	\return 0 iff the application should keep running.*/
int handle_user_input(app_t* app, app_update_t* update);


//! Computes the viewport for each camera. Outputs tuples (left, right, top,
//! bottom) with coordinates in pixels.
void compute_camera_viewports(float viewports[MAX_CAMERA_COUNT][4], const VkExtent2D* swapchain_extent, const scene_spec_t* scene_spec, const app_params_t* app_params);


/*! Defines the GUI for one frame with all of its windows and elements and
	updates application state.
	\param ctx The Nuklear context to be used for the GUI.
	\param scene_spec Scene specification to be modified.
	\param render_settings Render settings to be modified.
	\param update Used to report required updates.
	\param app_params Used to get the GUI extent.
	\param render_targets Used to report the accumulated sample count.
	\param volume Used to fit cameras to view.
	\param transfer Used to display whether the desired file was found.
	\param swapchain Used to query the resolution.
	\param timestamps The timestamps from frame_workloads_t.
	\param timestamp_period The value from VkPhysicalDeviceLimits::timestampPeriod.
	\return true iff text entry is active such that other things depending on
		letter keys (e.g. camera controls) should seize to take input.*/
bool define_gui(struct nk_context* ctx, scene_spec_t* scene_spec, render_settings_t* render_settings, app_update_t* update, app_params_t* app_params, const render_targets_t* render_targets, const volume_t* volume, const transfer_function_t* transfer, const swapchain_t* swapchain, uint64_t timestamps[timestamp_index_count], float timestamp_period);


/*! Updates constant buffers, takes care of synchronization, renders a single
	frame and presents it.
	\param app The app for which a frame is being rendered.
	\param update If something goes wrong with the swapchain, it is flagged for
		an update but the application does not terminate.
	\return 0 upon success, error code of the failed Vulkan function on
		failure (or 1 for non-Vulkan errors).*/
VkResult render_frame(app_t* app, app_update_t* update);


/*! Saves a screenshot reflecting the state of the off-screen render target
	(without GUI) to a file.
	\param file_path The path to the output file.
	\param format The image file format to be used to store the image. For LDR
		formats, a basic tone mapping is performed.
	\param device Output of create_device().
	\param render_targets The HDR radiance render target from this object is
		stored.
	\param scene_spec The scene specification with tone mapping parameters.
	\return 0 upon success.*/
int save_screenshot(const char* file_path, image_file_format_t format, const device_t* device, const render_targets_t* render_targets, const scene_spec_t* scene_spec);


void free_screenshot(screenshot_t* screenshot, const device_t* device);
