#pragma once
#include "vulkan_basics.h"
#include "scene.h"
#include "camera.h"
#include "nuklear.h"
#include <stdbool.h>
#include <stdio.h>


//! The number of frames in flight, i.e. how many frames the host submits to
//! the GPU before waiting for the oldest one to finish
#define FRAME_IN_FLIGHT_COUNT 3
//! The maximal number of spherical lights that can be placed in the scene.
//! When changing this, also change the array size in shaders/constants.glsl.
#define MAX_SPHERICAL_LIGHT_COUNT 32
//! The maximal number of slides
#define MAX_SLIDE_COUNT 100


//! An enumeration of available scenes (i.e. *.vks files)
typedef enum {
	scene_file_bistro_outside,
	scene_file_cornell_box,
	scene_file_arcade,
	scene_file_attic,
	scene_file_bistro_inside,
	scene_file_living_room_day,
	scene_file_living_room_night,
	//! Number of different scenes
	scene_file_count,
} scene_file_t;


//! Available tonemapping operators
typedef enum {
	//! Simply convert the linear radiance values to sRGB, clamping channels
	//! that are above 1
	tonemapper_clamp,
	//! The tonemapper from the Academy Color Encoding System
	tonemapper_aces,
	//! The Khronos PBR neutral tone mapper as documented here:
	//! https://github.com/KhronosGroup/ToneMapping/blob/main/PBR_Neutral/README.md
	tonemapper_khronos_pbr_neutral,
	//! The number of available tonemapping operators
	tonemapper_count,
	//! Used to force an int32_t
	tonemapper_force_int_32 = 0x7fffffff,
} tonemapper_t;



/*! A complete specification of what is to be rendered without specifying
	different techniques that are supposed to render the same thing. This
	struct is what quicksave/quickload deal with.*/
typedef struct {
	//! The scene file that is to be loaded
	scene_file_t scene_file;
	//! The camera used to observe the scene
	camera_t camera;
	//! The tonemapping operator used to present colors
	tonemapper_t tonemapper;
	//! The factor by which HDR radiance is scaled during tonemapping
	float exposure;
	//! The index of the frame being rendered for the purpose of random seed
	//! generation
	uint32_t frame_index;
	//! The color of the sky (using Rec. 709, a.k.a. linear sRGB)
	float sky_color[3];
	//! A factor applied to the sky color to get radiance
	float sky_strength;
	//! The color of light emitted by the material called _emission (Rec. 709)
	float emission_material_color[3];
	//! A factor applied to the emission color to get radiance
	float emission_material_strength;
	//! Four floats that can be controlled from the GUI directly and can be
	//! used for any purpose while developing shaders
	float params[4];
} scene_spec_t;


//! Different sampling strategies to use in a path tracer. More detailed
//! documentation is available in the path_trace_*() functions in the shader.
typedef enum {
	//! Sampling of the hemisphere by choosing spherical coordinates uniformly
	sampling_strategy_spherical,
	//! Projected-solid angle sampling in the whole hemisphere
	sampling_strategy_psa,
	//! BRDF sampling
	sampling_strategy_brdf,
	//! Next event estimation
	sampling_strategy_nee,
	//! The number of different sampling strategies
	sampling_strategy_count,
} sampling_strategy_t;


//! A specification of all the techniques and parameters used to render the
//! scene without specifying the scene itself
typedef struct {
	//! The sampling strategy to use for path tracing
	sampling_strategy_t sampling_strategy;
	//! The maximal number of vertices along a path, excluding the one at the
	//! eye
	uint32_t path_length;
} render_settings_t;


//! Available image file formats for taking screenshots
typedef enum {
	//! Portable network graphics using 3*8-bit RGB
	image_file_format_png,
	//! Joint Photographic Experts Group (JPEG) using 3*8-bit RGB
	image_file_format_jpg,
	//! High-dynamic range image with 3*32-bit RGB using single-precision
	//! floats
	image_file_format_hdr,
} image_file_format_t;


//! Defines a slide, i.e. a configuration of the renderer that leads to a
//! specific image and can be used in a presentation
typedef struct {
	//! The file path to the quicksave that is to be loaded for this slide
	char* quicksave;
	//! The render settings to use for this slide
	render_settings_t render_settings;
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
	//! The number of frames which have been accumulated in the HDR radiance
	//! render target
	uint32_t accum_frame_count;
} render_targets_t;


//! The CPU-side version of the constants defined in constants.glsl. Refer to
//! this file for member documentation. Includes padding as required by GLSL.
typedef struct {
	float world_to_projection_space[4 * 4];
	float projection_to_world_space[4 * 4];
	float camera_pos[3];
	int camera_type;
	float hemispherical_camera_normal[3];
	float pad_1;
	float dequantization_factor[3], pad_2, dequantization_summand[3], pad_3;
	float viewport_size[2], inv_viewport_size[2];
	float exposure;
	uint32_t frame_index;
	uint32_t accum_frame_count;
	float pad_4;
	float sky_radiance[3];
	float pad_5;
	float emission_material_radiance[3];
	float pad_6;
	float params[4];
	float spherical_lights[MAX_SPHERICAL_LIGHT_COUNT][4];
} constants_t;


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


//! The triangle mesh that is being displayed and a specification of the light
//! sources in this scene
typedef struct {
	//! The triangle mesh that is being displayed
	scene_t scene;
	//! The number of spherical lights placed in the scene
	uint32_t spherical_light_count;
	//! Positions and radii of all spherical lights
	float spherical_lights[MAX_SPHERICAL_LIGHT_COUNT][4];
} lit_scene_t;


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
	//! A sampler for material textures
	VkSampler sampler;
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
	lit_scene_t lit_scene;
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
	bool device, window, gui, swapchain, render_targets, constant_buffers, lit_scene, render_pass, scene_subpass, tonemap_subpass, gui_subpass, frame_workloads;
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


/*! Outputs the name, the path to the *.vks file, to the textures, to the
	lights and to the quicksave for the given scene file. Returns 0 upon
	success. Output strings must not be freed. Passing NULL for strings that
	are not required is legal.*/
int get_scene_file(scene_file_t scene_file, const char** scene_name, const char** scene_file_path, const char** texture_path, const char** light_path, const char** quicksave_path);


//! Saves the scene specification to the quicksave file (overwriting it).
//! Returns 0 upon success.
int quicksave(const scene_spec_t* spec);


/*! Loads the scene specification from the quicksave file and flags required
	application updates. Pass NULL as save_path for a default. Returns 0 upon
	success.*/
int quickload(scene_spec_t* spec, app_update_t* update, const char* save_path);


//! Given an old and a new scene specification, this function marks the updates
//! that need to be performed in response to this change
void get_scene_spec_updates(app_update_t* update, const scene_spec_t* old_spec, const scene_spec_t* new_spec);


//! Initializes the given scene specification with defaults or loads it from a
//! quicksave
void init_scene_spec(scene_spec_t* spec);


//! Initializes the given render settings with defaults
void init_render_settings(render_settings_t* settings);


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
int create_constant_buffers(constant_buffers_t* constant_buffers, const device_t* device);


void free_constant_buffers(constant_buffers_t* constant_buffers, const device_t* device);


//! Updates constant_buffers->constants based on the state of the app and moves
//! its contents to the staging buffer with the given index instantly
int write_constant_buffer(constant_buffers_t* constant_buffers, const app_t* app, uint32_t buffer_index);


//! Forwards to load_scene() using parameters that are appropriate for the
//! given scene specification and additionally loads light sources
int create_lit_scene(lit_scene_t* lit_scene, const device_t* device, const scene_spec_t* scene_spec);


void free_lit_scene(lit_scene_t* lit_scene, const device_t* device);


//! \see render_pass_t
int create_render_pass(render_pass_t* render_pass, const device_t* device, const swapchain_t* swapchain, const render_targets_t* targets);


void free_render_pass(render_pass_t* render_pass, const device_t* device);


//! \see scene_subpass_t
int create_scene_subpass(scene_subpass_t* subpass, const device_t* device, const scene_spec_t* scene_spec, const render_settings_t* render_settings, const swapchain_t* swapchain, const constant_buffers_t* constant_buffers, const lit_scene_t* lit_scene, const render_pass_t* render_pass);


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


/*! Responds to user input, updating the state of the given app accordingly and
	triggering all sorts of requested actions. Invoke this once per frame.
	\param app The app whose state is to be updated.
	\param update Used to report required updates.
	\return 0 iff the application should keep running.*/
int handle_user_input(app_t* app, app_update_t* update);


/*! Defines the GUI for one frame with all of its windows and elements and
	updates application state.
	\param ctx The Nuklear context to be used for the GUI.
	\param scene_spec Scene specification to be modified.
	\param render_settings Render settings to be modified.
	\param update Used to report required updates.
	\param render_targets Used to query the sample count.
	\param timestamps The timestamps from frame_workloads_t.
	\param timestamp_period The value from VkPhysicalDeviceLimits::timestampPeriod.*/
void define_gui(struct nk_context* ctx, scene_spec_t* scene_spec, render_settings_t* render_settings, app_update_t* update, const render_targets_t* render_targets, uint64_t timestamps[timestamp_index_count], float timestamp_period);


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
