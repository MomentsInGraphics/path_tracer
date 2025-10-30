#include "main.h"
#include "string_utilities.h"
#include "math_utilities.h"
#include "timer.h"
#include "phase_functions.h"
#include "serializer.h"
#include "alias_table.h"
#include "pfm.h"
#include "tonemap.h"
#include "stb_image_write.h"
#include "stb_image.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>


/*! Serializes a scene_spec_t to/from a quicksave file. When reading from the
	file fields that are not part of the file format version are left
	unchanged.*/
int serialize_quicksave(scene_spec_t* spec, serializer_t* ser) {
	// String volsave
	const uint64_t expected_file_type = 0x1ac7701feull, expected_version = 0;
	uint64_t file_type = expected_file_type, version = expected_version;
	if (serialize_block(&file_type, sizeof(file_type), ser) || serialize_block(&version, sizeof(version), ser))
		return 1;
	if (file_type != expected_file_type || version > expected_version) {
		printf("Failed to load a quicksave because its file format marker is invalid or it uses a too new version.\n");
		return 1;
	}
	// Set defaults for fields that were added in later versions
	if (!ser->write) {
		if (version < 1) {
			free(spec->volume_file_path);
			spec->volume_file_path = copy_string("data/volume/intel_cloud.blob");
			free(spec->volume_shader_path);
			spec->volume_shader_path = copy_string("");
		}
	}
	// Serialize the struct field by field
	if (SERIALIZE_BLOCK(spec->cameras)
	 || SERIALIZE_BLOCK(spec->tonemapper)
	 || SERIALIZE_BLOCK(spec->exposure)
	 || serialize_string(&spec->volume_file_path, ser)
	 || serialize_string(&spec->volume_shader_path, ser)
	 || SERIALIZE_BLOCK(spec->volume_color_mode)
	 || SERIALIZE_BLOCK(spec->volume_color_constant)
	 || SERIALIZE_BLOCK(spec->transfer_min_input)
	 || SERIALIZE_BLOCK(spec->transfer_max_input)
	 || SERIALIZE_BLOCK(spec->interpolate_transfer_function)
	 || SERIALIZE_BLOCK(spec->extinction_factor)
	 || SERIALIZE_BLOCK(spec->droplet_size)
	 || SERIALIZE_BLOCK(spec->volume_interpolation)
	 || serialize_string(&spec->transfer_function_path, ser)
	 || SERIALIZE_BLOCK(spec->frame_index)
	 || SERIALIZE_BLOCK(spec->use_ambient_light)
	 || serialize_string(&spec->light_probe_path, ser)
	 || SERIALIZE_BLOCK(spec->ambient_radiance)
	 || SERIALIZE_BLOCK(spec->ambient_lighting_factor)
	 || serialize_array(&spec->point_light_count, (void**) &spec->point_lights, sizeof(point_light_t), ser)
	 || SERIALIZE_BLOCK(spec->params)
	)
		return 1;
	return 0;
}


int quicksave(const scene_spec_t* spec, const char* path) {
	if (!path) path = "data/quicksave.jack_save";
	serializer_t serializer = {
		.file = fopen(path, "wb"),
		.write = true,
	};
	if (serializer.file) {
		if (!serialize_quicksave((scene_spec_t*) spec, &serializer)) {
			printf("Quicksave.\n");
			fclose(serializer.file);
			return 0;
		}
		fclose(serializer.file);
	}
	return 1;
}


int quickload(scene_spec_t* spec, app_update_t* update, const char* path) {
	if (!path) path = "data/quicksave.jack_save";
	serializer_t serializer = {
		.file = fopen(path, "rb"),
		.write = false,
	};
	scene_spec_t old = *spec;
	scene_spec_t new = get_default_scene_spec(false);
	if (serializer.file) {
		if (!serialize_quicksave(&new, &serializer)) {
			printf("Quickload.\n");
			get_scene_spec_updates(update, &old, &new);
			if (old.transfer_function_path && new.transfer_function_path && strcmp(old.transfer_function_path, new.transfer_function_path) != 0)
				update->transfer_function = true;
			free_scene_spec(spec);
			(*spec) = new;
			fclose(serializer.file);
			return 0;
		}
		free_scene_spec(&new);
		fclose(serializer.file);
	}
	return 1;
}


//! Returns the given pointer or a pointer to an empty string if it is NULL
static inline const char* make_null_empty(const char* string) {
	return (string == NULL) ? "" : string;
}


void get_scene_spec_updates(app_update_t* update, const scene_spec_t* old_spec, const scene_spec_t* new_spec) {
	if (old_spec->volume_interpolation != new_spec->volume_interpolation)
		update->volume_as = update->scene_subpass = true;
	if (old_spec->volume_color_mode != new_spec->volume_color_mode)
		update->volume_as = update->scene_subpass = true;
	if (old_spec->use_ambient_light != new_spec->use_ambient_light)
		update->scene_subpass = true;
	if (old_spec->point_light_count != new_spec->point_light_count)
		update->scene_subpass = update->constant_buffers = true;
	if (old_spec->tonemapper != new_spec->tonemapper)
		update->tonemap_subpass = true;
	if (old_spec->interpolate_transfer_function != new_spec->interpolate_transfer_function)
		update->transfer_function = true;
	if (strcmp(make_null_empty(old_spec->volume_shader_path), make_null_empty(new_spec->volume_shader_path)))
		update->volume_as = true;
	if (strcmp(make_null_empty(old_spec->volume_file_path), make_null_empty(new_spec->volume_file_path)))
		update->volume = true;
	if (strcmp(make_null_empty(old_spec->transfer_function_path), make_null_empty(new_spec->transfer_function_path)))
		update->transfer_function = true;
	if (strcmp(make_null_empty(old_spec->light_probe_path), make_null_empty(new_spec->light_probe_path)))
		update->light_probe = true;
}


void get_render_settings_updates(app_update_t* update, const render_settings_t* old_settings, const render_settings_t* new_settings) {
	if (old_settings->brick_size != new_settings->brick_size != old_settings->grid_compression != new_settings->grid_compression)
		update->volume_as = true;
	if (old_settings->jitter_primary_rays != new_settings->jitter_primary_rays
	 || old_settings->distance_sampler != new_settings->distance_sampler	
	 || old_settings->sampling_strategies != new_settings->sampling_strategies
	 || old_settings->displayed_quantity != new_settings->displayed_quantity
	 || old_settings->optical_depth_estimator != new_settings->optical_depth_estimator
	 || old_settings->uniform_jittered != new_settings->uniform_jittered
	 || old_settings->transmittance_estimator != new_settings->transmittance_estimator
	 || old_settings->mis_weight_estimator != new_settings->mis_weight_estimator)
		update->scene_subpass = true;
}


bool check_accumulation_reset(const scene_spec_t* old_spec, const scene_spec_t* new_spec, const render_settings_t* old_settings, const render_settings_t* new_settings, const app_params_t* old_params, const app_params_t* new_params) {
	scene_spec_t mixed;
	memcpy(&mixed, old_spec, sizeof(mixed));
	mixed.frame_index = new_spec->frame_index;
	mixed.exposure = new_spec->exposure;
	mixed.tonemapper = new_spec->tonemapper;
	mixed.point_lights = new_spec->point_lights;
	mixed.transfer_function_path = new_spec->transfer_function_path;
	mixed.volume_shader_path = new_spec->volume_shader_path;
	mixed.volume_file_path = new_spec->volume_file_path;
	mixed.light_probe_path = new_spec->light_probe_path;
	return old_settings->optical_depth_cdf_step != new_settings->optical_depth_cdf_step
		|| old_settings->ray_march_sample_count != new_settings->ray_march_sample_count
		|| old_settings->kettunen_sample_multiplier != new_settings->kettunen_sample_multiplier
		|| memcmp(old_settings->mis_weight_numerator, new_settings->mis_weight_numerator, 2 * sizeof(float))
		|| memcmp(old_settings->mis_weight_denominator, new_settings->mis_weight_denominator, 2 * sizeof(float))
		|| memcmp(&mixed, new_spec, sizeof(mixed))
		|| memcmp(old_spec->point_lights, new_spec->point_lights, old_spec->point_light_count * sizeof(point_light_t));
}


void make_render_settings_valid(render_settings_t* settings, const scene_spec_t* scene_spec) {
	if (scene_spec->volume_interpolation != volume_interpolation_nearest) {
		if (settings->distance_sampler == distance_sampler_regular_tracking)
			settings->distance_sampler = distance_sampler_delta_tracking;
		if (settings->transmittance_estimator == transmittance_estimator_regular_tracking)
			settings->transmittance_estimator = transmittance_estimator_jackknife;
		if (settings->mis_weight_estimator == mis_weight_estimator_regular_tracking)
			settings->mis_weight_estimator = mis_weight_estimator_jackknife;
	}
}


scene_spec_t get_default_scene_spec(bool allow_malloc) {
	camera_t volume_camera = {
		.type = camera_type_first_person,
		.near = 0.05f,
		.far = 1.0e4f,
		.fov = M_PI * 0.4f,
		.height = 10.0f,
		.speed = 200.0f,
		.rotation.angles = { 1.05f, 0.0f, 0.6f },
		.position = { 1.1f, 1.7f, 1.3f },
	};
	scene_spec_t spec = {
		.tonemapper = tonemapper_op_aces,
		.exposure = 1.0f,
		.volume_color_mode = volume_color_mode_constant,
		.volume_color_constant = { { 1.0f, 0.7f, 0.3f}, 1.0f },
		.transfer_min_input = 0.0f,
		.transfer_max_input = 1.0f,
		.extinction_factor = 0.03f,
		.droplet_size = 0.0f,
		.volume_interpolation = volume_interpolation_linear_stochastic,
		.interpolate_transfer_function = true,
		.use_ambient_light = true,
		.ambient_radiance = { { 1.0f, 1.0f, 1.0f}, 1.0f },
		.ambient_lighting_factor = 1.0f,
	};
	if (allow_malloc) {
		spec.volume_file_path = copy_string("data/volume/intel_cloud.blob");
		spec.volume_shader_path = copy_string("");
		spec.transfer_function_path = copy_string("data/transfer_function/cividis.png");
		spec.light_probe_path = copy_string("data/white_1k.hdr");
	}
	for (uint32_t i = 0; i != MAX_CAMERA_COUNT; ++i)
		spec.cameras[i] = volume_camera;
	return spec;
}


scene_spec_t copy_scene_spec(const scene_spec_t* spec) {
	scene_spec_t result;
	memcpy(&result, spec, sizeof(result));
	result.transfer_function_path = copy_string(spec->transfer_function_path);
	result.volume_shader_path = copy_string(spec->volume_shader_path);
	result.volume_file_path = copy_string(spec->volume_file_path);
	result.light_probe_path = copy_string(spec->light_probe_path);
	result.point_lights = malloc(result.point_light_count * sizeof(point_light_t));
	memcpy(result.point_lights, spec->point_lights, result.point_light_count * sizeof(point_light_t));
	return result;
}


void free_scene_spec(scene_spec_t* spec) {
	free(spec->transfer_function_path);
	free(spec->volume_shader_path);
	free(spec->volume_file_path);
	free(spec->light_probe_path);
	free(spec->point_lights);
	memset(spec, 0, sizeof(*spec));
}


void init_render_settings(render_settings_t* settings) {
	(*settings) = (render_settings_t) {
		.jitter_primary_rays = true,
		.brick_size = 16,
		.grid_compression = grid_compression_bc4,
		.distance_sampler = distance_sampler_delta_tracking,
		.optical_depth_estimator = optical_depth_estimator_variance_aware_fixed_count,
		.uniform_jittered = false,
		.transmittance_estimator = transmittance_estimator_jackknife,
		.mis_weight_estimator = mis_weight_estimator_jackknife,
		.sampling_strategies = sampling_strategies_light_transmittance_approximate_mis,
		.displayed_quantity = displayed_quantity_radiance,
		.optical_depth_cdf_step = 0.3f,
		.ray_march_sample_count = 10,
		.sample_count = 1,
		.kettunen_sample_multiplier = 1.0f,
		.mis_weight_numerator = { 1.0f, 0.0f},
		.mis_weight_denominator = { 1.0f , 1.0f },
	};
}


void get_linear_color(float linear_color[3], const hdr_color_spec_t* color_spec) {
	for (uint32_t i = 0; i != 3; ++i) {
		float non_linear = color_spec->srgb_color[i];
		linear_color[i] = (non_linear <= 0.04045f) ? ((1.0f / 12.92f) * non_linear) : powf(non_linear * (1.0f / 1.055f) + 0.055f / 1.055f, 2.4f);
		linear_color[i] *= color_spec->linear_factor;
	}
}


int create_window(GLFWwindow** window, const VkExtent2D* extent) {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	(*window) = glfwCreateWindow(extent->width, extent->height, "Volume renderer", NULL, NULL);
	return (*window) == NULL;
}


void free_window(GLFWwindow** window) {
	if (*window) glfwDestroyWindow(*window);
	(*window) = NULL;
}


//! GLFW callback for character input
void char_callback(GLFWwindow* window, unsigned int codepoint) {
	nk_input_unicode((struct nk_context*) glfwGetWindowUserPointer(window), codepoint);
}


//! GLFW callback for scrolling
void scroll_callback(GLFWwindow* window, double x_offset, double y_offset) {
	struct nk_vec2 scroll = { (float) x_offset, (float) y_offset };
	nk_input_scroll((struct nk_context*) glfwGetWindowUserPointer(window), scroll);
}


//! Callback to write the glyph image to a staging buffer
void write_glyph_image(void* image_data, uint32_t image_index, const VkImageSubresource* subresource, VkDeviceSize buffer_size, const VkImageCreateInfo* image_info, const VkExtent3D* subresource_extent, const void* context) {
	memcpy(image_data, context, buffer_size);
}


//! Fills the given array with colors taken either from the default imgui theme
//! or from the Nuklear style found here:
//! https://github.com/Immediate-Mode-UI/Nuklear/blob/8e5c9f7f345c7a7560f4538ffc42e7f1baff0f10/demo/common/style.c#L98C1-L125C64
void get_nuklear_dark_style(struct nk_color table[NK_COLOR_COUNT]) {
	table[NK_COLOR_TEXT] = nk_rgba_f(1.00f, 1.00f, 1.00f, 1.00f);
	table[NK_COLOR_WINDOW] = nk_rgba_f(0.06f, 0.06f, 0.06f, 0.94f);
	table[NK_COLOR_HEADER] = nk_rgba(41, 74, 122, 255);
	table[NK_COLOR_BORDER] = nk_rgba_f(0.43f, 0.43f, 0.50f, 0.50f);
	table[NK_COLOR_BUTTON] = nk_rgba_f(0.26f, 0.59f, 0.98f, 0.40f);
	table[NK_COLOR_BUTTON_HOVER] = nk_rgba_f(0.26f, 0.59f, 0.98f, 1.00f);
	table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba_f(0.06f, 0.53f, 0.98f, 1.00f);
	table[NK_COLOR_TOGGLE] = nk_rgba(50, 58, 61, 255);
	table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 53, 56, 255);
	table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba_f(0.235f, 0.533f, 0.890f, 1.00f);
	table[NK_COLOR_SELECT] = nk_rgba(57, 67, 61, 255);
	table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(48, 83, 111, 255);
	table[NK_COLOR_SLIDER] = nk_rgba(50, 58, 61, 255);
	table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(48, 83, 111, 245);
	table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
	table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
	table[NK_COLOR_PROPERTY] = nk_rgba(50, 58, 61, 255);
	table[NK_COLOR_EDIT] = nk_rgba(50, 58, 61, 225);
	table[NK_COLOR_EDIT_CURSOR] = nk_rgba(210, 210, 210, 255);
	table[NK_COLOR_COMBO] = nk_rgba(50, 58, 61, 255);
	table[NK_COLOR_CHART] = nk_rgba(50, 58, 61, 255);
	table[NK_COLOR_CHART_COLOR] = nk_rgba(48, 83, 111, 255);
	table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
	table[NK_COLOR_SCROLLBAR] = nk_rgba_f(0.02f, 0.02f, 0.02f, 0.53f);
	table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba_f(0.31f, 0.31f, 0.31f, 1.00f);
	table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba_f(0.41f, 0.41f, 0.41f, 1.00f);
	table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba_f(0.51f, 0.51f, 0.51f, 1.00f);
	table[NK_COLOR_TAB_HEADER] = nk_rgba_f(0.18f, 0.35f, 0.58f, 0.862f);
}


int create_gui(gui_t* gui, const device_t* device, GLFWwindow* window) {
	memset(gui, 0, sizeof(*gui));
	// Create a font atlas with one font
	nk_font_atlas_init_default(&gui->atlas);
	nk_font_atlas_begin(&gui->atlas);
	const char* font_file_path = "data/LinBiolinum_Rah.ttf";
	gui->font = nk_font_atlas_add_from_file(&gui->atlas, font_file_path, 24.0, NULL);
	if (!gui->font) {
		printf("Failed to load the font file at %s. Please check path and permissions. You should be using the parent directory of data as current working directory.\n", font_file_path);
		free_gui(gui, device, window);
		return 1;
	}
	int width = 0, height = 0;
	const void* glyph_image = nk_font_atlas_bake(&gui->atlas, &width, &height, NK_FONT_ATLAS_ALPHA8);
	// Upload the glyph image to the GPU
	VkExtent3D extent = { .width = (uint32_t) width, .height = (uint32_t) height, 1 };
	image_request_t glyph_request = {
		.image_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.arrayLayers = 1,
			.extent = extent,
			.format = VK_FORMAT_R8_UNORM,
			.imageType = VK_IMAGE_TYPE_2D,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.mipLevels = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		},
		.view_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			},
		},
	};
	if (create_images(&gui->glyph_image, device, &glyph_request, 1, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
		printf("Failed to create a GPU-resident glyph image for the GUI.\n");
		free_gui(gui, device, window);
		return 1;
	}
	if (fill_images(&gui->glyph_image, device, &write_glyph_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, glyph_image)) {
		printf("Failed to upload the glyph image for the GUI to the GPU.\n");
		free_gui(gui, device, window);
		return 1;
	}
	// Tidy up
	nk_font_atlas_end(&gui->atlas, nk_handle_id(0), &gui->null_texture);
	nk_font_atlas_cleanup(&gui->atlas);
	// Init Nuklear
	nk_init_default(&gui->context, &gui->font->handle);
	// Apply a style
	struct nk_color style_table[NK_COLOR_COUNT];
	get_nuklear_dark_style(style_table);
	nk_style_from_table(&gui->context, style_table);
	// Create buffers for geometry data
	gui->max_triangle_count = 50000;
	buffer_request_t vertex_request = {
		.buffer_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.size = gui->max_triangle_count * 3 * sizeof(gui_vertex_t),
		},
	};
	buffer_request_t staging_requests[FRAME_IN_FLIGHT_COUNT];
	for (uint32_t i = 0; i != FRAME_IN_FLIGHT_COUNT; ++i)
		staging_requests[i] = vertex_request;
	if (create_buffers(&gui->staging, device, staging_requests, COUNT_OF(staging_requests), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, device->physical_device_properties.limits.nonCoherentAtomSize)) {
		printf("Failed to create vertex staging buffers for the GUI.\n");
		free_gui(gui, device, window);
		return 1;
	}
	vertex_request.buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if (create_buffers(&gui->buffer, device, &vertex_request, 1, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1)) {
		printf("Failed to create device-local vertex buffer for the GUI.\n");
		free_gui(gui, device, window);
		return 1;
	}
	// Map memory of the staging buffers
	if (vkMapMemory(device->device, gui->staging.allocation, 0, gui->staging.size, 0, &gui->staging_data)) {
		printf("Failed to map memory for vertex staging buffers of the GUI.\n");
		free_gui(gui, device, window);
		return 1;
	}
	// Register GLFW callbacks for character input and scrolling
	if (glfwGetWindowUserPointer(window)) {
		printf("There was an attempt to create a second GUI for a window before the first one was freed. That is not allowed.\n");
		free_gui(gui, device, window);
		return 1;
	}
	glfwSetWindowUserPointer(window, &gui->context);
	glfwSetCharCallback(window, &char_callback);
	glfwSetScrollCallback(window, &scroll_callback);
	return 0;
}


void handle_gui_input(gui_t* gui, GLFWwindow* window) {
	struct nk_context* ctx = &gui->context;
	nk_input_begin(ctx);
	// Poll events to have callbacks for text input and scrolling invoked
	glfwPollEvents();
	// Map Nuklear keys to GLFW keys
	int key[NK_KEY_MAX];
	for (int i = 0; i != NK_KEY_MAX; ++i)
		key[i] = GLFW_KEY_UNKNOWN;
	int mod[NK_KEY_MAX];
	memset(mod, 0, sizeof(mod));
	mod[NK_KEY_SHIFT] = GLFW_MOD_SHIFT;
	mod[NK_KEY_CTRL] = GLFW_MOD_CONTROL;
	key[NK_KEY_DEL] = GLFW_KEY_DELETE;
	key[NK_KEY_ENTER] = GLFW_KEY_ENTER;
	key[NK_KEY_TAB] = GLFW_KEY_TAB;
	key[NK_KEY_BACKSPACE] = GLFW_KEY_BACKSPACE;
	key[NK_KEY_COPY] = GLFW_KEY_C;
	mod[NK_KEY_COPY] = GLFW_MOD_CONTROL;
	key[NK_KEY_CUT] = GLFW_KEY_X;
	mod[NK_KEY_CUT] = GLFW_MOD_CONTROL;
	key[NK_KEY_PASTE] = GLFW_KEY_V;
	mod[NK_KEY_PASTE] = GLFW_MOD_CONTROL;
	key[NK_KEY_UP] = GLFW_KEY_UP;
	key[NK_KEY_DOWN] = GLFW_KEY_DOWN;
	key[NK_KEY_LEFT] = GLFW_KEY_LEFT;
	key[NK_KEY_RIGHT] = GLFW_KEY_RIGHT;
	key[NK_KEY_TEXT_LINE_START] = GLFW_KEY_HOME;
	key[NK_KEY_TEXT_LINE_END] = GLFW_KEY_END;
	key[NK_KEY_TEXT_START] = GLFW_KEY_HOME;
	mod[NK_KEY_TEXT_START] = GLFW_MOD_CONTROL;
	key[NK_KEY_TEXT_END] = GLFW_KEY_END;
	mod[NK_KEY_TEXT_END] = GLFW_MOD_CONTROL;
	key[NK_KEY_TEXT_UNDO] = GLFW_KEY_Z;
	mod[NK_KEY_TEXT_UNDO] = GLFW_MOD_CONTROL;
	key[NK_KEY_TEXT_REDO] = GLFW_KEY_Y;
	mod[NK_KEY_TEXT_REDO] = GLFW_MOD_CONTROL;
	key[NK_KEY_TEXT_SELECT_ALL] = GLFW_KEY_A;
	mod[NK_KEY_TEXT_SELECT_ALL] = GLFW_MOD_CONTROL;
	key[NK_KEY_TEXT_WORD_LEFT] = GLFW_KEY_LEFT;
	mod[NK_KEY_TEXT_WORD_LEFT] = GLFW_MOD_ALT;
	key[NK_KEY_TEXT_WORD_RIGHT] = GLFW_KEY_RIGHT;
	mod[NK_KEY_TEXT_WORD_RIGHT] = GLFW_MOD_ALT;
	key[NK_KEY_SCROLL_START] = GLFW_KEY_PAGE_UP;
	mod[NK_KEY_SCROLL_START] = GLFW_MOD_CONTROL;
	key[NK_KEY_SCROLL_END] = GLFW_KEY_PAGE_DOWN;
	mod[NK_KEY_SCROLL_END] = GLFW_MOD_CONTROL;
	key[NK_KEY_SCROLL_DOWN] = GLFW_KEY_PAGE_DOWN;
	key[NK_KEY_SCROLL_UP] = GLFW_KEY_PAGE_UP;
	// Feed key input
	for (int i = 0; i != NK_KEY_MAX; ++i) {
		nk_bool down = nk_true;
		if (key[i] != GLFW_KEY_UNKNOWN)
			down &= glfwGetKey(window, key[i]) == GLFW_PRESS;
		if (mod[i] & GLFW_MOD_CONTROL)
			down &= glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
		if (mod[i] & GLFW_MOD_SHIFT)
			down &= glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
		if (mod[i] & GLFW_MOD_ALT)
			down &= glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
		// One last case that is not handled yet
		if (i == NK_KEY_ENTER)
			down |= glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS;
		nk_input_key(ctx, i, down);
	}
	// Get the cursor position in pixels
	double x, y;
	glfwGetCursorPos(window, &x, &y);
	int wf, hf, ww, hw;
	glfwGetFramebufferSize(window, &wf, &hf);
	glfwGetWindowSize(window, &ww, &hw);
	x *= wf / (double) ww;
	y *= hf / (double) hw;
	// Feed mouse input
	nk_input_motion(ctx, (int) x, (int) y);
	nk_input_button(ctx, NK_BUTTON_LEFT, (int) x, (int) y, glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS);
	nk_input_button(ctx, NK_BUTTON_RIGHT, (int) x, (int) y, glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS);
	nk_input_button(ctx, NK_BUTTON_MIDDLE, (int) x, (int) y, glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_3) == GLFW_PRESS);
	// Done
	nk_input_end(ctx);
}


//! gui_vertex_t without the scissor
typedef struct {
	float pos[2];
	float tex_coord[2];
	uint8_t color[4];
} gui_vertex_incomplete_t;


int write_gui_geometry(gui_t* gui, const device_t* device, uint32_t workload_index) {
	if (workload_index >= gui->staging.buffer_count) {
		printf("Failed to write GUI vertex buffers because the workload index %u is invalid.\n", workload_index);
		return 1;
	}
	// Let Nuklear provide geometry data
	struct nk_context* ctx = &gui->context;
	struct nk_draw_vertex_layout_element vertex_layout[] = {
		{ NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(gui_vertex_incomplete_t, pos), },
		{ NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(gui_vertex_incomplete_t, tex_coord), },
		{ NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(gui_vertex_incomplete_t, color), },
		{ NK_VERTEX_LAYOUT_END, },
	};
	struct nk_convert_config convert_config = {
		.shape_AA = NK_ANTI_ALIASING_ON,
		.line_AA = NK_ANTI_ALIASING_ON,
		.vertex_layout = vertex_layout,
		.vertex_size = sizeof(gui_vertex_incomplete_t),
		.vertex_alignment = NK_ALIGNOF(gui_vertex_incomplete_t),
		.circle_segment_count = 31,
		.curve_segment_count = 31,
		.arc_segment_count = 31,
		.global_alpha = 1.0,
		.tex_null = gui->null_texture,
	};
	struct nk_buffer cmds, verts, idxs;
	nk_buffer_init_default(&cmds);
	nk_buffer_init_default(&verts);
	nk_buffer_init_default(&idxs);
	if (nk_convert(ctx, &cmds, &verts, &idxs, &convert_config)) {
		printf("Failed to generate geometry data for the GUI using Nuklear.\n");
		nk_buffer_free(&cmds);
		nk_buffer_free(&verts);
		nk_buffer_free(&idxs);
		return 1;
	}
	// Count triangles
	const struct nk_draw_command* cmd;
	uint32_t triangle_count = 0;
	nk_draw_foreach(cmd, ctx, &cmds)
		triangle_count += cmd->elem_count;
	triangle_count /= 3;
	if (triangle_count > gui->max_triangle_count) {
		printf("The GUI uses too many triangles (%u/%u). Raise the hard-coded limit in create_gui().\n", triangle_count, gui->max_triangle_count);
		nk_buffer_free(&cmds);
		nk_buffer_free(&verts);
		nk_buffer_free(&idxs);
		return 1;
	}
	gui->used_triangle_counts[workload_index] = triangle_count;
	// Flatten the index and vertex buffer into a vertex buffer and store
	// scissor rectangles per vertex
	const gui_vertex_incomplete_t* src_verts = (const gui_vertex_incomplete_t*) verts.memory.ptr;
	const uint32_t* src_idxs = (const uint32_t*) idxs.memory.ptr;
	gui_vertex_t* dst_verts = (gui_vertex_t*) ((uint8_t*) gui->staging_data + gui->staging.buffers[workload_index].memory_offset);
	uint32_t vert_index = 0;
	nk_draw_foreach(cmd, ctx, &cmds) {
		struct nk_rect rect = cmd->clip_rect;
		int16_t scissor[4] = { (int16_t) rect.x, (int16_t) rect.y, (int16_t) (rect.x + rect.w), (int16_t) (rect.y + rect.h) };
		for (uint32_t i = 0; i != cmd->elem_count; ++i) {
			memcpy(&dst_verts[vert_index], &src_verts[src_idxs[vert_index]], sizeof(gui_vertex_incomplete_t));
			memcpy(dst_verts[vert_index].scissor, scissor, sizeof(scissor));
			++vert_index;
		}
	}
	// Tidy up
	nk_buffer_free(&cmds);
	nk_buffer_free(&verts);
	nk_buffer_free(&idxs);
	// Flush it to the GPU
	VkMappedMemoryRange range = {
		.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		.memory = gui->staging.allocation,
		.offset = gui->staging.buffers[workload_index].memory_offset,
		.size = gui->staging.buffers[workload_index].memory_size,
	};
	if (vkFlushMappedMemoryRanges(device->device, 1, &range)) {
		printf("Failed to flush a staging vertex buffer for the GUI.\n");
		return 1;
	}
	return 0;
}


void free_gui(gui_t* gui, const device_t* device, GLFWwindow* window) {
	if (gui->staging_data) vkUnmapMemory(device->device, gui->staging.allocation);
	free_images(&gui->glyph_image, device);
	free_buffers(&gui->staging, device);
	free_buffers(&gui->buffer, device);
	if (window) {
		glfwSetCharCallback(window, NULL);
		glfwSetScrollCallback(window, NULL);
		glfwSetWindowUserPointer(window, NULL);
	}
	struct nk_context null_context;
	memset(&null_context, 0, sizeof(null_context));
	if (memcmp(&gui->context, &null_context, sizeof(null_context)))
		nk_free(&gui->context);
	struct nk_font_atlas null_atlas;
	memset(&null_atlas, 0, sizeof(null_atlas));
	if (memcmp(&gui->atlas, &null_atlas, sizeof(null_atlas)))
		nk_font_atlas_clear(&gui->atlas);
	memset(gui, 0, sizeof(*gui));
}


int create_render_targets(render_targets_t* render_targets, const device_t* device, const swapchain_t* swapchain) {
	memset(render_targets, 0, sizeof(*render_targets));
	// Make requests for all render targets
	image_request_t requests[render_target_index_count];
	requests[render_target_index_hdr_radiance] = (image_request_t) {
		.image_info = {
			.format = VK_FORMAT_R32G32B32A32_SFLOAT,
			.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		},
		.view_info = {
			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		},
	};
	requests[render_target_index_depth_buffer] = (image_request_t) {
		.image_info = {
			.format = VK_FORMAT_D32_SFLOAT,
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		},
		.view_info = {
			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
		},
	};
	// Fill in some shared fields
	VkExtent3D extent = { swapchain->extent.width, swapchain->extent.height, 1 };
	for (uint32_t i = 0; i != render_target_index_count; ++i) {
		requests[i].image_info.extent = extent;
		requests[i].image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		requests[i].image_info.imageType = VK_IMAGE_TYPE_2D;
		requests[i].image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		requests[i].image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		requests[i].image_info.mipLevels = 1;
		requests[i].image_info.arrayLayers = 1;
		requests[i].view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		requests[i].view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	}
	// Create them
	if (create_images(&render_targets->targets, device, requests, COUNT_OF(requests), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
		printf("Failed to create render targets.\n");
		free_render_targets(render_targets, device);
		return 1;
	}
	// Transition the HDR radiance buffer to the right layout
	VkImageLayout new_layouts[COUNT_OF(requests)] = { 0 };
	new_layouts[render_target_index_hdr_radiance] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	if (transition_image_layouts(&render_targets->targets, device, NULL, new_layouts, NULL)) {
		printf("Failed to transition render targets to the required layouts.\n");
		free_render_targets(render_targets, device);
		return 1;
	}
	return 0;
}


void free_render_targets(render_targets_t* render_targets, const device_t* device) {
	free_images(&render_targets->targets, device);
	memset(render_targets, 0, sizeof(*render_targets));
}


//! Computes the required size for constant buffers holding data from the given
//! scene specification
size_t compute_constant_buffer_size(const scene_spec_t* spec, const volume_t* volume) {
	size_t size = sizeof(fixed_size_constants_t);
	size += volume->volume_count * 2 * 4 * 4 * sizeof(float);
	size += (spec->point_light_count + (spec->point_light_count == 0 ? 1 : 0)) * sizeof(point_light_constants_t);
	return size;
}


int create_constant_buffers(constant_buffers_t* constant_buffers, const device_t* device, const scene_spec_t* scene_spec, const volume_t* volume) {
	memset(constant_buffers, 0, sizeof(*constant_buffers));
	size_t size = compute_constant_buffer_size(scene_spec, volume);
	// Create one staging buffer per frame in flight
	buffer_request_t staging_request = {
		.buffer_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		},
	};
	buffer_request_t staging_requests[FRAME_IN_FLIGHT_COUNT];
	for (uint32_t i = 0; i != FRAME_IN_FLIGHT_COUNT; ++i)
		staging_requests[i] = staging_request;
	if (create_buffers(&constant_buffers->staging, device, staging_requests, COUNT_OF(staging_requests), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, device->physical_device_properties.limits.nonCoherentAtomSize)) {
		printf("Failed to create staging buffers for constant buffers.\n");
		free_constant_buffers(constant_buffers, device);
		return 1;
	}
	// Map the staging memory
	if (vkMapMemory(device->device, constant_buffers->staging.allocation, 0, VK_WHOLE_SIZE, 0, &constant_buffers->staging_data)) {
		printf("Failed to map memory of staging buffers for constant buffers.\n");
		free_constant_buffers(constant_buffers, device);
		return 1;
	}
	// Create a device-local constant buffer that will be used each frame
	buffer_request_t buffer_request = {
		.buffer_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		},
	};
	if (create_buffers(&constant_buffers->buffer, device, &buffer_request, 1, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1)) {
		printf("Failed to create a device-local buffer to be used as constant buffer.\n");
		free_constant_buffers(constant_buffers, device);
		return 1;
	}
	return 0;
}


void free_constant_buffers(constant_buffers_t* constant_buffers, const device_t* device) {
	if (constant_buffers->staging.allocation && constant_buffers->staging_data)
		vkUnmapMemory(device->device, constant_buffers->staging.allocation);
	free_buffers(&constant_buffers->staging, device);
	free_buffers(&constant_buffers->buffer, device);
	memset(constant_buffers, 0, sizeof(*constant_buffers));
}


int write_constant_buffer(constant_buffers_t* constant_buffers, const app_t* app, uint32_t buffer_index) {
	if (buffer_index >= constant_buffers->staging.buffer_count) {
		printf("Failed to write constant buffers because the staging buffer index %u is invalid.\n", buffer_index);
		return 1;
	}
	// Gather all constants from various app objects
	VkExtent2D viewport = app->swapchain.extent;
	const scene_spec_t* spec = &app->scene_spec;
	const render_settings_t* settings = &app->render_settings;
	VkExtent3D volume_extent = app->volume.volume.images[0].request.image_info.extent;
	VkExtent3D bound_extent = app->volume_as.grids.images[0].request.image_info.extent;
	VkExtent3D probe_extent = app->light_probe.probe.images[0].request.image_info.extent;
	fixed_size_constants_t cts = {
		.viewport_size = { (float) viewport.width, (float) viewport.height },
		.inv_viewport_size = { 1.0f / (float) viewport.width, 1.0f / (float) viewport.height },
		.exposure = spec->exposure,
		.frame_index = app->scene_spec.frame_index,
		.accum_frame_count = app->render_targets.accum_frame_count + settings->sample_count,
		.frame_sample_count = settings->sample_count,
		.volume_extent = { (float) volume_extent.width, (float) volume_extent.height, (float) volume_extent.depth },
		.inv_volume_extent = { 1.0f / (float) volume_extent.width, 1.0f / (float) volume_extent.height, 1.0f / (float) volume_extent.depth },
		.transfer_min_input = spec->transfer_min_input,
		.transfer_max_input = spec->transfer_max_input,
		.transfer_input_factor = 1.0f / (spec->transfer_max_input - spec->transfer_min_input),
		.transfer_input_summand = -spec->transfer_min_input / (spec->transfer_max_input - spec->transfer_min_input),
		.extinction_factor = spec->extinction_factor,
		.brick_counts = { (float) bound_extent.width, (float) bound_extent.height, (float) bound_extent.depth },
		.inv_brick_counts = { 1.0f / (float) bound_extent.width, 1.0f / (float) bound_extent.height, 1.0f / (float) bound_extent.depth },
		.brick_size = app->render_settings.brick_size,
		.brick_size_float = (float) app->render_settings.brick_size,
		.inv_brick_size = 1.0f / (float) app->render_settings.brick_size,
		.ray_march_sample_count = app->render_settings.ray_march_sample_count,
		.inv_ray_march_sample_count = 1.0f / ((float) app->render_settings.ray_march_sample_count),
		.optical_depth_cdf_step = app->render_settings.optical_depth_cdf_step,
		.kettunen_sample_multiplier = app->render_settings.kettunen_sample_multiplier,
		.ambient_brightness = spec->ambient_radiance.linear_factor,
		.ambient_lighting_factor = spec->ambient_lighting_factor,
		.inv_light_probe_luminance_integral = 1.0f / (4.0f * M_PI * app->light_probe.weight_sum * spec->ambient_radiance.linear_factor / ((float) (probe_extent.width * probe_extent.height))),
		.mis_weight_numerator = { settings->mis_weight_numerator[0], settings->mis_weight_numerator[1] },
		.mis_weight_denominator = { settings->mis_weight_denominator[0], settings->mis_weight_denominator[1] },
	};
	fit_mie_phase_function(cts.mie_fit_params, app->scene_spec.droplet_size);
	memcpy(cts.params, app->scene_spec.params, sizeof(cts.params));
	float camera_viewports[MAX_CAMERA_COUNT][4];
	compute_camera_viewports(camera_viewports, &app->swapchain.extent, spec, &app->params);
	for (uint32_t i = 0; i != MAX_CAMERA_COUNT; ++i) {
		float port[4] = { camera_viewports[i][0], camera_viewports[i][1], camera_viewports[i][2], camera_viewports[i][3] };
		float aspect = (port[1] - port[0]) / (port[3] - port[2]);
		float trans[4] = { 1.0f / (port[1] - port[0]), 1.0f / (port[3] - port[2]), -port[0] / (port[1] - port[0]), -port[2] / (port[3] - port[2]) };
		memcpy(cts.viewport_transforms[i], trans, sizeof(trans));
		get_world_to_projection_space(cts.world_to_projection_space[i], &app->scene_spec.cameras[i], aspect);
		invert_mat4(cts.projection_to_world_space[i], cts.world_to_projection_space[i]);
	}
	get_linear_color(cts.volume_color_constant, &app->scene_spec.volume_color_constant);
	// Write fixed-size data to the staging buffer
	uint8_t* dst = (uint8_t*) constant_buffers->staging_data + constant_buffers->staging.buffers[buffer_index].memory_offset;
	memcpy(dst, &cts, sizeof(cts));
	dst += sizeof(cts);
	// Write variable-size data to the staging buffer
	for (uint32_t i = 0; i != app->volume.volume_count; ++i) {
		pad_mat3x4_to_mat4((float*) dst, app->volume.texel_to_world_space + i * 3 * 4);
		invert_mat4(((float*) dst) + 4 * 4, (float*) dst);
		dst += 2 * 4 * 4 * sizeof(float);
	}
	for (uint64_t i = 0; i != spec->point_light_count; ++i) {
		const point_light_t* src_light = &spec->point_lights[i];
		point_light_constants_t* dst_light = (point_light_constants_t*) dst;
		for (uint32_t j = 0; j != 3; ++j)
			dst_light->pos[j] = src_light->pos[j];
		get_linear_color(dst_light->power, &src_light->power);
		dst_light->importance = 0.17697f * dst_light->power[0] + 0.8124f * dst_light->power[1] + 0.01063f * dst_light->power[2];
		dst += sizeof(point_light_constants_t);
	}
	if (spec->point_light_count == 0) dst += sizeof(point_light_constants_t);
	// Flush the staging buffer
	VkMappedMemoryRange range = {
		.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		.memory = constant_buffers->staging.allocation,
		.offset = constant_buffers->staging.buffers[buffer_index].memory_offset,
		.size = constant_buffers->staging.buffers[buffer_index].memory_size,
	};
	if (vkFlushMappedMemoryRanges(app->device.device, 1, &range)) {
		printf("Failed to flush a staging buffer for a constant buffer.\n");
		return 1;
	}
	return 0;
}


void write_volume(void* image_data, uint32_t image_index, const VkImageSubresource* subresource, VkDeviceSize buffer_size, const VkImageCreateInfo* image_info, const VkExtent3D* subresource_extent, const void* context) {
	const volume_t* volume = (const volume_t*) context;
	fread(image_data, 1, buffer_size, volume->files[image_index]);
}


int create_volume(volume_t* volume, const device_t* device, const scene_spec_t* scene) {
	memset(volume, 0, sizeof(*volume));
	// Figure out if one or many volumes are being loaded
	volume->volume_count = 1;
	const char* src_path = scene->volume_file_path;
	const char* suffix_0 = strstr(src_path, ".file_");
	if (suffix_0) {
		suffix_0 += strlen(".file_");
		const char* suffix_1 = strstr(suffix_0, "_of_");
		if (suffix_1) {
			suffix_1 += strlen("_of_");
			const char* suffix_2 = strstr(suffix_0, ".");
			if (sscanf(suffix_1, "%u", &volume->volume_count)) {
				const char* pieces[] = { copy_string(src_path), ".file_%u_of_", suffix_1 };
				strstr(pieces[0], ".file_")[0] = '\0';
				volume->file_path_pattern = cat_strings(pieces, COUNT_OF(pieces));
				free((char*) pieces[0]);
			}
		}
	}
	if (volume->volume_count == 0) {
		printf("There was an attempt to load 0 volume files, but that is not permitted.\n");
		free_volume(volume, device);
		return 1;
	}
	// Allocate space for all the volumes
	image_request_t* volume_requests = calloc(volume->volume_count, sizeof(image_request_t));
	volume->files = calloc(volume->volume_count, sizeof(FILE*));
	volume->texel_to_world_space = calloc(volume->volume_count, 3 * 4 * sizeof(float));
	// Open files and read headers for all volumes that are to be loaded
	for (uint32_t i = 0; i != volume->volume_count; ++i) {
		char* file_path = (volume->file_path_pattern != NULL) ? format_uint(volume->file_path_pattern, i) : copy_string(src_path);
		volume->files[i] = fopen(file_path, "rb");
		if (!volume->files[i]) {
			printf("Failed to open the volume file at %s. Please check the path and permissions.\n", file_path);
			free(file_path);
			free(volume_requests);
			free_volume(volume, device);
			return 1;
		}
		// Check whether the file is using the right format
		uint64_t format_marker = 0, format_version = 0xff;
		fread(&format_marker, sizeof(format_marker), 1, volume->files[i]);
		if (format_marker != 0x656d756c6f76ull) {
			printf("The volume file at %s does not use the correct file format.\n", file_path);
			free(file_path);
			free(volume_requests);
			free_volume(volume, device);
			return 1;
		}
		fread(&format_version, sizeof(format_version), 1, volume->files[i]);
		if (format_version != 0) {
			printf("The volume file at %s uses the unsupported file format version %lu.\n", file_path, format_version);
			free(file_path);
			free(volume_requests);
			free_volume(volume, device);
			return 1;
		}
		// Read the volume extent
		uint64_t extent_64[3];
		fread(extent_64, sizeof(uint64_t), 3, volume->files[i]);
		VkExtent3D extent = { (uint32_t) extent_64[0], (uint32_t) extent_64[1], (uint32_t) extent_64[2] };
		// Read the format and quantization coefficients
		VkFormat format = VK_FORMAT_UNDEFINED;
		fread(&format, sizeof(format), 1, volume->files[i]);
		// Read the transform
		fread(volume->texel_to_world_space + i * 3 * 4, 3 * 4 * sizeof(float), 1, volume->files[i]);
		// Create the volume
		volume_requests[i] = (image_request_t) {
			.image_info = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.arrayLayers = 1,
				.extent = extent,
				.format = format,
				.imageType = VK_IMAGE_TYPE_3D,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.mipLevels = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			},
			.view_info = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.viewType = VK_IMAGE_VIEW_TYPE_3D,
				.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT },
			},
		};
		// Report the resolution
		printf("Loading a %ux%ux%u volume from %s.\n", extent.width, extent.height, extent.depth, file_path);
		free(file_path);
	}
	// Create the volumes
	if (create_images(&volume->volume, device, volume_requests, volume->volume_count, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
		printf("Failed to create volume images based on file (pattern) %s.\n", src_path);
		free(volume_requests);
		free_volume(volume, device);
		return 1;
	}
	free(volume_requests);
	volume_requests = NULL;
	// Fill the volumes
	if (fill_images(&volume->volume, device, &write_volume, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, volume)) {
		printf("Failed to upload a volumes based on file (pattern) %s to the GPU.\n", src_path);
		free_volume(volume, device);
		return 1;
	}
	// Close the source files
	for (uint32_t i = 0; i != volume->volume_count; ++i) {
		fclose(volume->files[i]);
		volume->files[i] = NULL;
	}
	return 0;
}


void free_volume(volume_t* volume, const device_t* device) {
	free_images(&volume->volume, device);
	if (volume->files)
		for (uint32_t i = 0; i != volume->volume_count; ++i)
			if (volume->files[i])
				fclose(volume->files[i]);
	free(volume->files);
	free(volume->file_path_pattern);
	free(volume->texel_to_world_space);
	memset(volume, 0, sizeof(*volume));
}


int create_volume_acceleration_structure(volume_as_t* as, const device_t* device, const volume_t* volume, const transfer_function_t* transfer, const constant_buffers_t* constants, const scene_spec_t* scene_spec, const render_settings_t* settings) {
	memset(as, 0, sizeof(*as));
	// Compute low-resolution grid resolutions and how they need to be padded for
	// BC4 compression
	uint32_t brick_size = settings->brick_size;
	VkExtent3D extent = volume->volume.images[0].request.image_info.extent;
	VkExtent3D brick_counts = {
		.width = (extent.width + brick_size - 1) / brick_size,
		.height = (extent.height + brick_size - 1) / brick_size,
		.depth = (extent.depth + brick_size - 1) / brick_size,
	};
	VkExtent3D padded_brick_counts = {
		.width = ((brick_counts.width + 3) / 4) * 4,
		.height = ((brick_counts.height + 3) / 4) * 4,
		.depth = brick_counts.depth,
	};
	// Request for low-resolution grids prior to compression
	image_request_t half_request = {
		.image_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.extent = brick_counts,
			.arrayLayers = 1,
			.format = VK_FORMAT_R16_SFLOAT,
			.imageType = VK_IMAGE_TYPE_3D,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.mipLevels = 1,
			.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			.samples = VK_SAMPLE_COUNT_1_BIT,
		},
		.view_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.format = VK_FORMAT_R16_SFLOAT,
			.viewType = VK_IMAGE_VIEW_TYPE_3D,
			.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT },
		},
	};
	// Request for the full resolution grid holding extinction values from the
	// transfer function
	image_request_t extinction_request = half_request;
	extinction_request.image_info.extent = extent;
	extinction_request.image_info.format = extinction_request.view_info.format = VK_FORMAT_R8_UNORM;
	// Request for low-resolution grids after BC4 compression. A few nifty
	// flags are needed here so that one view can be used to write the blocks
	// from a compute shader, while another can be used to read them with
	// hardware decompression.
	VkImageFormatListCreateInfo bc4_formats = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO,
		.viewFormatCount = 2,
		.pViewFormats = (VkFormat[]) { VK_FORMAT_BC4_UNORM_BLOCK, VK_FORMAT_R32G32_UINT },
	};
	image_request_t bc4_request = half_request;
	bc4_request.image_info.extent = padded_brick_counts;
	bc4_request.image_info.format = VK_FORMAT_BC4_UNORM_BLOCK;
	bc4_request.image_info.flags = VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT | VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT;
	bc4_request.image_info.pNext = &bc4_formats;
	bc4_request.view_info.format = VK_FORMAT_R32G32_UINT;
	image_request_t grid_requests[2 * GRID_ATTRIBUTE_COUNT + 1];
	for (uint32_t i = 0; i != GRID_ATTRIBUTE_COUNT; ++i) {
		grid_requests[0 * GRID_ATTRIBUTE_COUNT + i] = half_request;
		grid_requests[1 * GRID_ATTRIBUTE_COUNT + i] = bc4_request;
	}
	uint32_t grid_count = GRID_ATTRIBUTE_COUNT;
	if (settings->grid_compression == grid_compression_bc4)
		grid_count += GRID_ATTRIBUTE_COUNT;
	if (scene_spec->volume_color_mode == volume_color_mode_transfer_function_with_extinction)
		grid_requests[grid_count++] = extinction_request;
	if (create_images(&as->grids, device, grid_requests, grid_count, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
		printf("Failed to create images for volume acceleration structures.\n");
		free_volume_acceleration_structure(as, device);
		return 1;
	}
	// Create additional views for the block-compressed textures
	if (settings->grid_compression == grid_compression_bc4) {
		for (uint32_t i = 0; i != COUNT_OF(as->bc_views); ++i) {
			VkImageViewUsageCreateInfo image_usage = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO,
				.usage = VK_IMAGE_USAGE_SAMPLED_BIT,
			};
			VkImageViewCreateInfo view_info = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = &image_usage,
				.viewType = VK_IMAGE_VIEW_TYPE_3D,
				.format = VK_FORMAT_BC4_UNORM_BLOCK,
				.image = as->grids.images[GRID_ATTRIBUTE_COUNT + i].image,
				.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1, .levelCount = 1 },
			};
			if (vkCreateImageView(device->device, &view_info, NULL, &as->bc_views[i])) {
				printf("Failed to create image views for volume acceleration structures.\n");
				free_volume_acceleration_structure(as, device);
				return 1;
			}
		}
	}
	// Create volumes to hold the output of the volume shader
	uint32_t shaded_volume_count = 0;
	image_request_t shaded_volume_requests[2];
	bool volume_shading_enabled = (strlen(scene_spec->volume_shader_path) > 0);
	if (volume_shading_enabled) {
		shaded_volume_requests[shaded_volume_count++] = (image_request_t) {
			.image_info = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.imageType = VK_IMAGE_TYPE_3D,
				.format = volume->volume.images[0].request.image_info.format,
				.extent = extent,
				.arrayLayers = 1,
				.mipLevels = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			},
			.view_info = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.viewType = VK_IMAGE_VIEW_TYPE_3D,
				.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT },
			}
		};
		if (scene_spec->volume_color_mode == volume_color_mode_separate) {
			image_request_t color_request = shaded_volume_requests[0];
			color_request.image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
			shaded_volume_requests[shaded_volume_count++] = color_request;
		}
		if (create_images(&as->shaded_volume, device, shaded_volume_requests, shaded_volume_count, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)) {
			printf("Failed to create images for the output of the volume shader.\n");
			free_volume_acceleration_structure(as, device);
			return 1;
		}
	}
	// Transition layouts
	VkImageLayout layouts[COUNT_OF(grid_requests) + COUNT_OF(shaded_volume_requests)];
	for (uint32_t i = 0; i != COUNT_OF(layouts); ++i)
		layouts[i] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	if (transition_image_layouts(&as->grids, device, NULL, layouts, NULL)) {
		printf("Failed to transition layouts for grids used as volume acceleration structures.\n");
		free_volume_acceleration_structure(as, device);
		return 1;
	}
	if (shaded_volume_count > 0 && transition_image_layouts(&as->shaded_volume, device, NULL, layouts, NULL)) {
		printf("Failed to transition layouts for volume shader output.\n");
		free_volume_acceleration_structure(as, device);
		return 1;
	}
	// Create buffers to determine quantization coefficients for BC4 compression
	uint32_t subgroup_size = device->physical_device_properties_11.subgroupSize;
	const uint32_t reduction_factor = 256;
	if (subgroup_size > reduction_factor) subgroup_size = reduction_factor;
	uint32_t total_brick_count = brick_counts.width * brick_counts.height * brick_counts.depth;
	uint32_t stat_counts[5] = { total_brick_count };
	uint32_t reduction_step_count = 0;
	for (uint32_t i = 1; i != COUNT_OF(stat_counts); ++i) {
		stat_counts[i] = (stat_counts[i - 1] + reduction_factor - 1) / reduction_factor;
		if (stat_counts[i] == 1) {
			reduction_step_count = i;
			break;
		}
	}
	buffer_request_t dequantization_coeffs_request = {
		.buffer_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = GRID_ATTRIBUTE_COUNT * 4 * sizeof(float),
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		},
	};
	uint32_t props_count = 1;
	buffer_request_t props_requests[1 + GRID_ATTRIBUTE_COUNT * COUNT_OF(stat_counts)] = { dequantization_coeffs_request };
	if (settings->grid_compression == grid_compression_bc4)
		for (uint32_t i = 0; i != GRID_ATTRIBUTE_COUNT; ++i)
			for (uint32_t j = 0; j != reduction_step_count; ++j)
				props_requests[props_count++] = (buffer_request_t) {
					.buffer_info = {
						.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
						.size = 4 * sizeof(float) * stat_counts[j + 1],
						.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					},
				};
	if (create_buffers(&as->grid_props, device, props_requests, props_count, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1)) {
		printf("Failed to create buffers for grid properties (statistics and quantization coefficients) in volume acceleration structures.\n");
		free_volume_acceleration_structure(as, device);
		return 1;
	}
	// Start defining inputs of compute shaders
	char* defines[] = {
		format_uint("VOLUME_COUNT=%u", volume->volume_count),
		format_uint("VOLUME_INTERPOLATION_NEAREST=%u", scene_spec->volume_interpolation == volume_interpolation_nearest),
		format_uint("VOLUME_INTERPOLATION_LINEAR=%u", scene_spec->volume_interpolation == volume_interpolation_linear),
		format_uint("VOLUME_INTERPOLATION_LINEAR_STOCHASTIC=%u", scene_spec->volume_interpolation == volume_interpolation_linear_stochastic),
		format_uint("BRICK_SIZE=%u", settings->brick_size),
		format_uint("SUBGROUP_SIZE=%u", subgroup_size),
		format_uint("BRICK_COUNT=%u", total_brick_count),
		format_uint("BRICK_COUNT_X=%u", brick_counts.width),
		format_uint("BRICK_COUNT_Y=%u", brick_counts.height),
		format_uint("BRICK_COUNT_Z=%u", brick_counts.depth),
		format_uint("GRID_ATTRIBUTE_COUNT=%u", GRID_ATTRIBUTE_COUNT),
	};
	const images_t* image_sets[] = { &as->grids, &volume->volume, &transfer->transfer_function, &as->shaded_volume };
	const buffers_t* buffer_sets[] = { &as->grid_props, &constants->buffer };
	uint32_t dispatch_count = 0;
	compute_dispatch_t dispatches[4 + (1 + COUNT_OF(stat_counts)) * GRID_ATTRIBUTE_COUNT];
	// Define the dispatch that performs volume shading
	dispatch_binding_t volume_shading_bindings[] = {
		{ .desc = { .binding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }, .entry = 0, .set = 1 },
		{ .desc = { .binding = 1, .descriptorCount = shaded_volume_count, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE }, .entry = 0, .set = 3 },
		{ .desc = { .binding = 2, .descriptorCount = volume->volume_count, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER }, .entry = 0, .set = 1 },
		{ .desc = { .binding = 3, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER }, .entry = 0, .set = 2 },
	};
	if (volume_shading_enabled) {
		dispatches[dispatch_count++] = (compute_dispatch_t) {
			.shader_request = {
				.entry_point = "main",
				.shader_path = scene_spec->volume_shader_path,
				.stage = VK_SHADER_STAGE_COMPUTE_BIT,
				.defines = defines,
				.define_count = COUNT_OF(defines),
			},
			.binding_count = COUNT_OF(volume_shading_bindings),
			.bindings = volume_shading_bindings,
			.group_counts = { (extent.width + 3) / 4, (extent.height + 3) / 4, (extent.depth + 3) / 4 },
		};
	}
	// Define the dispatch that applies the extinction transfer function
	dispatch_binding_t extinction_bindings[4];
	if (scene_spec->volume_color_mode == volume_color_mode_transfer_function_with_extinction) {
		extinction_bindings[0] = (dispatch_binding_t) { .desc = { .binding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }, .entry = 0, .set = 1 };
		extinction_bindings[1] = (dispatch_binding_t) { .desc = { .binding = 1, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER }, .entry = 0, .set = volume_shading_enabled ? 3 : 1 };
		extinction_bindings[2] = (dispatch_binding_t) { .desc = { .binding = 2, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER }, .entry = 0, .set = 2 };
		extinction_bindings[3] = (dispatch_binding_t) { .desc = { .binding = 3, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE }, .entry = as->grids.image_count - 1, .set = 0 };
		dispatches[dispatch_count++] = (compute_dispatch_t) {
			.shader_request = {
				.entry_point = "main",
				.shader_path = "src/shaders/apply_extinction_transfer_function.comp.glsl",
				.stage = VK_SHADER_STAGE_COMPUTE_BIT,
				.defines = defines,
				.define_count = COUNT_OF(defines),
			},
			.binding_count = COUNT_OF(extinction_bindings),
			.bindings = extinction_bindings,
			.group_counts = { (extent.width + 3) / 4, (extent.height + 3) / 4, (extent.depth + 3) / 4 },
		};
	}
	// Define the dispatch that reduces the high-resolution volume to
	// low-resolution grids
	dispatch_binding_t brick_reduce_bindings[] = {
		{
			.desc = { .binding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
			.entry = (scene_spec->volume_color_mode == volume_color_mode_transfer_function_with_extinction) ? (as->grids.image_count - 1) : (volume_shading_enabled ? 0 : 0),
			.set = (scene_spec->volume_color_mode == volume_color_mode_transfer_function_with_extinction) ? 0 : (volume_shading_enabled ? 3 : 1),
		},
		{ .desc = { .binding = 1, .descriptorCount = GRID_ATTRIBUTE_COUNT, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE }, .entry = 0, .set = 0 },
	};
	dispatches[dispatch_count++] = (compute_dispatch_t) {
		.shader_request = {
			.entry_point = "main",
			.shader_path = "src/shaders/brick_reduce.comp.glsl",
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.defines = defines,
			.define_count = COUNT_OF(defines),
		},
		.binding_count = COUNT_OF(brick_reduce_bindings),
		.bindings = brick_reduce_bindings,
		.group_counts = { (brick_counts.width + 3) / 4, (brick_counts.height + 3) / 4, (brick_counts.depth + 3) / 4 },
	};
	// Define the dispatches that reduce low-resolution grids to a few scalar
	// statistics
	dispatch_binding_t stat_bindings[GRID_ATTRIBUTE_COUNT * COUNT_OF(stat_counts)][2];
	char* stat_defines[COUNT_OF(stat_counts) * 2] = { NULL };
	if (settings->grid_compression == grid_compression_bc4) {
		for (uint32_t i = 0; i != reduction_step_count; ++i) {
			stat_defines[2 * i + 0] = format_uint("SUBGROUP_SIZE=%u", subgroup_size);
			stat_defines[2 * i + 1] = format_uint("INPUT_STAT_COUNT=%u", stat_counts[i + 1]);
		}
		uint32_t reduction_count = 0;
		for (uint32_t i = 0; i != GRID_ATTRIBUTE_COUNT; ++i) {
			dispatch_binding_t* bindings = stat_bindings[reduction_count++];
			bindings[0] = (dispatch_binding_t) { .desc = { .binding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER }, .set = 0, .entry = i };
			bindings[1] = (dispatch_binding_t) { .desc = { .binding = 1, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }, .set = 0, .entry = 1 + i * reduction_step_count };
			dispatches[dispatch_count++] = (compute_dispatch_t) {
				.shader_request = {
					.entry_point = "main",
					.shader_path = "src/shaders/stat_reduce_0.comp.glsl",
					.stage = VK_SHADER_STAGE_COMPUTE_BIT,
					.defines = defines,
					.define_count = COUNT_OF(defines),
				},
				.group_counts = { stat_counts[1], 1, 1 },
				.binding_count = 2,
				.bindings = bindings,
			};
			for (uint32_t j = 0; j != reduction_step_count - 1; ++j) {
				dispatch_binding_t* bindings = stat_bindings[reduction_count++];
				bindings[0] = (dispatch_binding_t) { .desc = { .binding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }, .set = 0, .entry = 1 + i * reduction_step_count + j };
				bindings[1] = (dispatch_binding_t) { .desc = { .binding = 1, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }, .set = 0, .entry = 1 + i * reduction_step_count + j + 1 };
				dispatches[dispatch_count++] = (compute_dispatch_t) {
					.shader_request = {
						.entry_point = "main",
						.shader_path = "src/shaders/stat_reduce_1.comp.glsl",
						.stage = VK_SHADER_STAGE_COMPUTE_BIT,
						.defines = &stat_defines[2 * j],
						.define_count = 2,
					},
					.group_counts = { stat_counts[j + 1], 1, 1 },
					.binding_count = 2,
					.bindings = bindings,
				};
			}
		}
	}
	dispatch_binding_t quantization_coeffs_bindings[1 + GRID_ATTRIBUTE_COUNT];
	if (settings->grid_compression == grid_compression_bc4) {
		// Define the dispatch that turns grid statistics into dequantization
		// parameters
		quantization_coeffs_bindings[0] = (dispatch_binding_t) {
			.desc = { .binding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }, .set = 0, .entry = 0,
		};
		for (uint32_t i = 0; i != GRID_ATTRIBUTE_COUNT; ++i)
			quantization_coeffs_bindings[i + 1] = (dispatch_binding_t) {
				.desc = { .binding = i + 1, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }, .set = 0, .entry = 1 + i * reduction_step_count + reduction_step_count - 1,
			};
		dispatches[dispatch_count++] = (compute_dispatch_t) {
			.shader_request = {
				.entry_point = "main",
				.shader_path = "src/shaders/compute_quantization_coeffs.comp.glsl",
				.stage = VK_SHADER_STAGE_COMPUTE_BIT,
				.defines = defines,
				.define_count = COUNT_OF(defines),
			},
			.group_counts = { 1, 1, 1 },
			.bindings = quantization_coeffs_bindings,
			.binding_count = COUNT_OF(quantization_coeffs_bindings),
		};
	}
	else {
		// Use a compute shader to write default dequantization coefficients
		quantization_coeffs_bindings[0] = (dispatch_binding_t) {
			.desc = { .binding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }, .set = 0, .entry = 0,
		};
		dispatches[dispatch_count++] = (compute_dispatch_t) {
			.shader_request = {
				.entry_point = "main",
				.shader_path = "src/shaders/default_quantization_coeffs.comp.glsl",
				.stage = VK_SHADER_STAGE_COMPUTE_BIT,
				.defines = defines,
				.define_count = COUNT_OF(defines),
			},
			.group_counts = { 1, 1, 1 },
			.bindings = quantization_coeffs_bindings,
			.binding_count = 1,
		};
	}
	// Define the dispatches that perform BC4 compression
	dispatch_binding_t bc4_bindings[GRID_ATTRIBUTE_COUNT][3];
	char* bc4_defines[GRID_ATTRIBUTE_COUNT][2] = {
		{ "ROUND_OP=floor", "GRID_ATTRIBUTE_INDEX=0" },
		{ "ROUND_OP=ceil", "GRID_ATTRIBUTE_INDEX=1" },
		{ "ROUND_OP=ceil", "GRID_ATTRIBUTE_INDEX=2" },
		{ "ROUND_OP=ceil", "GRID_ATTRIBUTE_INDEX=3" },
		{ "ROUND_OP=ceil", "GRID_ATTRIBUTE_INDEX=4" },
	};
	if (settings->grid_compression == grid_compression_bc4) {
		compute_dispatch_t bc4_dispatch = {
			.shader_request = {
				.entry_point = "main",
				.shader_path = "src/shaders/bc4_compress.comp.glsl",
				.stage = VK_SHADER_STAGE_COMPUTE_BIT,
				.defines = defines,
				.define_count = COUNT_OF(defines),
			},
			.group_counts = { (padded_brick_counts.width + 15) / 16, (padded_brick_counts.height + 15) / 16, (padded_brick_counts.depth + 3) / 4 },
			.binding_count = COUNT_OF(bc4_bindings[0]),
		};
		for (uint32_t i = 0; i != GRID_ATTRIBUTE_COUNT; ++i) {
			compute_dispatch_t* dispatch = &dispatches[dispatch_count++];
			(*dispatch) = bc4_dispatch;
			bc4_bindings[i][0] = (dispatch_binding_t) { .desc = { .binding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER }, .set = 0, .entry = i };
			bc4_bindings[i][1] = (dispatch_binding_t) { .desc = { .binding = 1, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }, .set = 0, .entry = 0 };
			bc4_bindings[i][2] = (dispatch_binding_t) { .desc = { .binding = 2, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE }, .set = 0, .entry = GRID_ATTRIBUTE_COUNT + i };
			dispatch->shader_request.defines = bc4_defines[i];
			dispatch->shader_request.define_count = COUNT_OF(bc4_defines[i]);
			dispatch->bindings = bc4_bindings[i];
		}
	}
	// Create the compute graph in its entirety
	compute_graph_t graph = {
		.image_set_count = COUNT_OF(image_sets) - ((shaded_volume_count == 0) ? 1 : 0),
		.image_sets = image_sets,
		.buffer_set_count = COUNT_OF(buffer_sets),
		.buffer_sets = buffer_sets,
		.dispatch_count = dispatch_count,
		.dispatches = dispatches,
		.sampler = transfer->sampler,
	};
	int create_result = create_compute_workload(&as->workload, device, &graph);
	for (uint32_t i = 0; i != COUNT_OF(defines); ++i)
		free(defines[i]);
	for (uint32_t i = 0; i != COUNT_OF(stat_defines); ++i)
		free(stat_defines[i]);
	if (create_result) {
		printf("Failed to create a compute workload to generate volume acceleration structures.\n");
		free_volume_acceleration_structure(as, device);
		return 1;
	}
	return 0;
}


void free_volume_acceleration_structure(volume_as_t* as, const device_t* device) {
	free_compute_workload(&as->workload, device);
	free_buffers(&as->grid_props, device);
	for (uint32_t i = 0; i != COUNT_OF(as->bc_views); ++i)
		if (as->bc_views[i])
			vkDestroyImageView(device->device, as->bc_views[i], NULL);
	free_images(&as->shaded_volume, device);
	free_images(&as->grids, device);
	memset(as, 0, sizeof(*as));
}


void write_transfer_function(void* image_data, uint32_t image_index, const VkImageSubresource* subresource, VkDeviceSize buffer_size, const VkImageCreateInfo* image_info, const VkExtent3D* subresource_extent, const void* context) {
	// Just write the first scanline of the loaded image
	memcpy(image_data, context, buffer_size);
}


int create_transfer_function(transfer_function_t* transfer, const device_t* device, const scene_spec_t* spec) {
	memset(transfer, 0, sizeof(*transfer));
	// Use stb to load the transfer function from an image file
	int width, height, channels_in_file;
	char* desired_path = spec->transfer_function_path;
	const char* backup_name = "cividis";
	const char* backup_path = "data/transfer_function/cividis.png";
	stbi_uc* image_data = stbi_load(desired_path, &width, &height, &channels_in_file, 4);
	if (!image_data) {
		printf("Could not load transfer function %s, falling back to default.\n", desired_path);
		image_data = stbi_load(backup_path, &width, &height, &channels_in_file, 4);
		if (!image_data) {
			printf("Failed to load the transfer function from image file %s. Please make sure that the file exists and is valid. The STBI loading error is: %s\n", backup_path, stbi_failure_reason());
			return 1;
		}
		transfer->name = copy_string(backup_name);
	}
	else {
		transfer->found = true;
		transfer->name = copy_string(spec->transfer_function_path);
	}
	// Create an image for the transfer function
	image_request_t image_request = {
		.image_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.arrayLayers = 1,
			.extent = { .width = (uint32_t) width, .height = 1, .depth = 1 },
			.format = VK_FORMAT_R8G8B8A8_SRGB,
			.imageType = VK_IMAGE_TYPE_1D,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.mipLevels = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		},
		.view_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.viewType = VK_IMAGE_VIEW_TYPE_1D,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			},
		},
	};
	if (create_images(&transfer->transfer_function, device, &image_request, 1, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
		printf("Failed to create a 1D texture of size %u for the transfer function.\n", image_request.image_info.extent.width);
		stbi_image_free(image_data);
		return 1;
	}
	// Fill the image with data
	if (fill_images(&transfer->transfer_function, device, &write_transfer_function, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image_data)) {
		printf("Failed to create a 1D texture of size %u for the transfer function.\n", image_request.image_info.extent.width);
		free_transfer_function(transfer, device);
		stbi_image_free(image_data);
		return 1;
	}
	// Tidy up the image
	stbi_image_free(image_data);
	// Create a suitable sampler with clamping and everything
	VkSamplerCreateInfo sampler_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.minFilter = spec->interpolate_transfer_function ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
		.magFilter = spec->interpolate_transfer_function ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
		.anisotropyEnable = VK_FALSE,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	};
	if (vkCreateSampler(device->device, &sampler_info, NULL, &transfer->sampler)) {
		printf("Failed to create a sampler for the transfer function.\n");
		free_transfer_function(transfer, device);
		return 1;
	}
	return 0;
}


void free_transfer_function(transfer_function_t* transfer, const device_t* device) {
	free_images(&transfer->transfer_function, device);
	free(transfer->name);
	if (transfer->sampler) vkDestroySampler(device->device, transfer->sampler, NULL);
	memset(transfer, 0, sizeof(*transfer));
}


//! Writes a light probe to an image
void write_light_probe(void* image_data, uint32_t image_index, const VkImageSubresource* subresource, VkDeviceSize buffer_size, const VkImageCreateInfo* image_info, const VkExtent3D* subresource_extent, const void* source_image_data) {
	memcpy(image_data, source_image_data, buffer_size);
}


//! Constructs an alias table from RGB image data of a light probe and writes
//! it to a buffer
void write_light_probe_alias_table(void* buffer_data, uint32_t buffer_index, VkDeviceSize buffer_size, const void* context) {
	light_probe_t* probe = (light_probe_t*) context;
	// Compute per-pixel luminance, weighted by the sine of the inclination as
	// target density for importance sampling
	uint32_t width = probe->probe.images[0].request.image_info.extent.width;
	uint32_t height = probe->probe.images[0].request.image_info.extent.height;
	float inclination_factor = M_PI / ((float) height);
	float inclination_summand = 0.5f * inclination_factor;
	float* weights = malloc(sizeof(float) * width * height);
	for (uint32_t y = 0; y != height; ++y) {
		for (uint32_t x = 0; x != width; ++x) {
			const float* pixel = &probe->image_data[(y * width + x) * 4];
			float luminance = 0.17697f * pixel[0] + 0.8124f * pixel[1] + 0.01063f * pixel[2];
			float inclination = inclination_factor * ((float) y) + inclination_summand;
			weights[y * width + x] = sinf(inclination) * luminance;
		}
	}
	// Build the alias table
	probe->weight_sum = build_alias_table((uint32_t*) buffer_data, weights, width * height);
	free(weights);
}


int create_light_probe(light_probe_t* probe, const device_t* device, const char* probe_path) {
	// Create a sampler with appropriate wrapping modes
	VkSamplerCreateInfo sampler_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.minFilter = VK_FILTER_LINEAR,
		.magFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_FILTER_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
	};
	if (vkCreateSampler(device->device, &sampler_info, NULL, &probe->sampler)) {
		printf("Failed to create a sampler for sampling light probes.\n");
		free_light_probe(probe, device);
		return 1;
	}
	// Load the HDR image
	int width = 0, height = 0, actual_channel_count = 0;
	probe->image_data = stbi_loadf(probe_path, &width, &height, &actual_channel_count, STBI_rgb_alpha);
	if (!probe->image_data) {
		printf("Failed to load the light probe at path %s. Please check path and permissions and ensure that it is an *.hdr file.\n", probe_path);
		free_light_probe(probe, device);
		return 1;
	}
	// Create an image
	image_request_t probe_request = {
		.image_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.arrayLayers = 1,
			.extent = { width, height, 1 },
			.format = VK_FORMAT_R32G32B32A32_SFLOAT,
			.mipLevels = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		},
		.view_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1 , .levelCount = 1 },
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = VK_FORMAT_R32G32B32A32_SFLOAT,
		},
	};
	if (create_images(&probe->probe, device, &probe_request, 1, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
		printf("Failed to create an image for the light probe at path %s.\n", probe_path);
		free_light_probe(probe, device);
		return 1;
	}
	// Create a buffer for the alias table
	buffer_request_t alias_request = {
		.buffer_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = sizeof(uint32_t) * width * height,
			.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
		},
		.view_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
			.format = VK_FORMAT_R32_UINT,
		},
	};
	if (create_buffers(&probe->alias_table, device, &alias_request, 1, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1)) {
		printf("Failed to create a buffer for an alias table to sample the light probe at path %s.\n", probe_path);
		free_light_probe(probe, device);
		return 1;
	}
	// Copy image data
	if (fill_images(&probe->probe, device, &write_light_probe, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, probe->image_data)) {
		printf("Failed to copy light probe image data to the GPU for the probe at %s.", probe_path);
		free_light_probe(probe, device);
		return 1;
	}
	// Build and copy the alias table
	if (fill_buffers(&probe->alias_table, device, &write_light_probe_alias_table, probe)) {
		printf("Failed to build or copy an alias table for the light probe at %s.", probe_path);
		free_light_probe(probe, device);
		return 1;
	}
	// Free the image data
	free(probe->image_data);
	probe->image_data = NULL;
	return 0;
}


void free_light_probe(light_probe_t* probe, const device_t* device) {
	free(probe->image_data);
	free_images(&probe->probe, device);
	free_buffers(&probe->alias_table, device);
	if (probe->sampler) vkDestroySampler(device->device, probe->sampler, NULL);
	memset(probe, 0, sizeof(*probe));
}


int create_render_pass(render_pass_t* render_pass, const device_t* device, const swapchain_t* swapchain, const render_targets_t* targets) {
	memset(render_pass, 0, sizeof(*render_pass));
	// Define the render pass
	VkAttachmentDescription attachments[] = {
		// 0: The swapchain image
		{
			.format = swapchain->format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		},
		// 1: The depth buffer
		{
			.format = targets->targets.images[render_target_index_depth_buffer].request.view_info.format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		},
		// 2: The HDR radiance
		{
			.format = targets->targets.images[render_target_index_hdr_radiance].request.view_info.format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		},
	};
	VkAttachmentReference swapchain_attachment = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};
	VkAttachmentReference depth_attachment = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};
	VkAttachmentReference hdr_radiance_output_attachment = {
		.attachment = 2,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};
	VkAttachmentReference hdr_radiance_input_attachment = {
		.attachment = 2,
		.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};
	VkSubpassDescription subpasses[] = {
		// 0: scene subpass
		{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.pColorAttachments = &hdr_radiance_output_attachment,
			.colorAttachmentCount = 1,
			.pDepthStencilAttachment = &depth_attachment,
		},
		// 1: gui subpass (also blits the HDR render target)
		{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.pColorAttachments = &swapchain_attachment,
			.colorAttachmentCount = 1,
			.pInputAttachments = &hdr_radiance_input_attachment,
			.inputAttachmentCount = 1,
		},
	};
	VkSubpassDependency dependencies[] = {
		// The swapchain image must have been acquired
		{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		},
		// The HDR radiance render target must have been rendered
		{
			.srcSubpass = 0,
			.dstSubpass = 1,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
		},
	};
	VkRenderPassCreateInfo render_pass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pAttachments = attachments,
		.attachmentCount = COUNT_OF(attachments),
		.pSubpasses = subpasses,
		.subpassCount = COUNT_OF(subpasses),
		.pDependencies = dependencies,
		.dependencyCount = COUNT_OF(dependencies),
	};
	if (vkCreateRenderPass(device->device, &render_pass_info, NULL, &render_pass->render_pass)) {
		printf("Failed to create a render pass.\n");
		free_render_pass(render_pass, device);
		return 1;
	}
	// Create one framebuffer per swapchain image
	render_pass->framebuffer_count = swapchain->image_count;
	render_pass->framebuffers = calloc(render_pass->framebuffer_count, sizeof(VkFramebuffer));
	for (uint32_t i = 0; i != render_pass->framebuffer_count; ++i) {
		VkImageView attachments[] = {
			// 0: The swapchain image
			swapchain->views[i],
			// 1: The depth buffer
			targets->targets.images[render_target_index_depth_buffer].view,
			// 2: The HDR radiance
			targets->targets.images[render_target_index_hdr_radiance].view,
		};
		VkFramebufferCreateInfo framebuffer_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pAttachments = attachments,
			.attachmentCount = COUNT_OF(attachments),
			.width = swapchain->extent.width,
			.height = swapchain->extent.height,
			.layers = 1,
			.renderPass = render_pass->render_pass,
		};
		if (vkCreateFramebuffer(device->device, &framebuffer_info, NULL, &render_pass->framebuffers[i])) {
			printf("Failed to create a framebuffer using swapchain image %u.\n", i);
			free_render_pass(render_pass, device);
			return 1;
		}
	}
	return 0;
}


void free_render_pass(render_pass_t* render_pass, const device_t* device) {
	if (render_pass->framebuffers)
		for (uint32_t i = 0; i != render_pass->framebuffer_count; ++i)
			if (render_pass->framebuffers[i])
				vkDestroyFramebuffer(device->device, render_pass->framebuffers[i], NULL);
	free(render_pass->framebuffers);
	if (render_pass->render_pass) vkDestroyRenderPass(device->device, render_pass->render_pass, NULL);
	memset(render_pass, 0, sizeof(*render_pass));
}


int create_scene_subpass(scene_subpass_t* subpass, const device_t* device, const scene_spec_t* scene_spec, const render_settings_t* settings, const swapchain_t* swapchain, const constant_buffers_t* constant_buffers, const volume_t* volume, const volume_as_t* as, const transfer_function_t* transfer, const light_probe_t* probe, const render_pass_t* render_pass) {
	memset(subpass, 0, sizeof(*subpass));
	// Create a sampler for material textures
	VkSamplerCreateInfo texture_sampler_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.magFilter = VK_FILTER_LINEAR,
		.anisotropyEnable = VK_TRUE,
		.maxAnisotropy = 16,
		.minLod = 0.0f,
		.maxLod = 3.4e38f,
	};
	if (vkCreateSampler(device->device, &texture_sampler_info, NULL, &subpass->texture_sampler)) {
		printf("Failed to create a sampler for 2D textures in the scene subpass.\n");
		free_scene_subpass(subpass, device);
		return 1;
	}
	// And another one for volumes. It always uses linear interpolation but
	// when the active method does not call for that, it will directly fetch
	// texels.
	VkSamplerCreateInfo volume_sampler_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.minFilter = VK_FILTER_LINEAR,
		.magFilter = VK_FILTER_LINEAR,
		.minLod = 0.0f,
		.maxLod = 3.4e38f,
	};
	if (vkCreateSampler(device->device, &volume_sampler_info, NULL, &subpass->volume_sampler)) {
		printf("Failed to create a sampler for volume textures in the scene subpass.\n");
		free_scene_subpass(subpass, device);
		return 1;
	}
	// Create a descriptor set
	#define PROBE_BINDING_START 6
	VkDescriptorSetLayoutBinding bindings[PROBE_BINDING_START + 2] = {
		// The constant buffer
		{ .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .stageFlags = VK_SHADER_STAGE_ALL },
		// The volume texture
		{ .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, },
		// The volume extinction texture
		{ .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, },
		// The volume acceleration structure
		{ .binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = GRID_ATTRIBUTE_COUNT },
		// The volume bound dequantization coefficients
		{ .binding = 4, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
		// The transfer function
		{ .binding = 5, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
		// The light probe and its alias table
		{ .binding = PROBE_BINDING_START + 0, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
		{ .binding = PROBE_BINDING_START + 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER },
	};
	complete_descriptor_set_layout_bindings(bindings, COUNT_OF(bindings), 1, VK_SHADER_STAGE_FRAGMENT_BIT);
	if (create_descriptor_sets(&subpass->descriptor_set, device, bindings, COUNT_OF(bindings), 1)) {
		printf("Failed to create a descriptor set for the scene subpass.\n");
		free_scene_subpass(subpass, device);
		return 1;
	}
	// Write to the descriptor set
	VkDescriptorBufferInfo constant_buffer_info = {
		.buffer = constant_buffers->buffer.buffers[0].buffer,
		.range = VK_WHOLE_SIZE,
	};
	const image_t* volume_values_or_colors = get_volume_colors(as);
	if (!volume_values_or_colors) volume_values_or_colors = get_volume_values(volume, as);
	VkDescriptorImageInfo volume_info = {
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.imageView = volume_values_or_colors->view,
		.sampler = subpass->volume_sampler,
	};
	VkDescriptorImageInfo extinction_info = {
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.imageView = as->grids.images[as->grids.image_count - 1].view,
		.sampler = subpass->volume_sampler,
	};
	if (scene_spec->volume_color_mode != volume_color_mode_transfer_function_with_extinction)
		extinction_info = volume_info;
	VkDescriptorImageInfo transfer_info = {
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.imageView = transfer->transfer_function.images[0].view,
		.sampler = transfer->sampler,
	};
	VkDescriptorImageInfo grid_infos[GRID_ATTRIBUTE_COUNT];
	for (uint32_t i = 0; i != GRID_ATTRIBUTE_COUNT; ++i) {
		grid_infos[i] = (VkDescriptorImageInfo) {
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.imageView = (settings->grid_compression == grid_compression_bc4) ? as->bc_views[i] : as->grids.images[i].view,
			.sampler = subpass->volume_sampler,
		};
	}
	VkDescriptorBufferInfo bound_dequantization_info = {
		.buffer = as->grid_props.buffers[0].buffer,
		.range = VK_WHOLE_SIZE,
	};
	VkDescriptorImageInfo probe_info = {
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.imageView = probe->probe.images[0].view,
		.sampler = probe->sampler,
	};
	VkWriteDescriptorSet writes[COUNT_OF(bindings)] = {
		{ .dstBinding = 0, .pBufferInfo = &constant_buffer_info, },
		{ .dstBinding = 1, .pImageInfo = &volume_info },
		{ .dstBinding = 2, .pImageInfo = &extinction_info },
		{ .dstBinding = 3, .pImageInfo = grid_infos },
		{ .dstBinding = 4, .pBufferInfo = &bound_dequantization_info },
		{ .dstBinding = 5, .pImageInfo = &transfer_info },
		{ .dstBinding = PROBE_BINDING_START + 0, .pImageInfo = &probe_info },
		{ .dstBinding = PROBE_BINDING_START + 1, .pTexelBufferView = &probe->alias_table.buffers[0].view },
	};
	complete_descriptor_set_writes(writes, COUNT_OF(writes), bindings, COUNT_OF(bindings), subpass->descriptor_set.descriptor_sets[0]);
	vkUpdateDescriptorSets(device->device, COUNT_OF(writes), writes, 0, NULL);
	// Compile the shaders and create the shader modules
	char* defines[] = {
		format_uint("JITTER_PRIMARY_RAYS=%u", settings->jitter_primary_rays ? 1 : 0),
		format_uint("GRID_ATTRIBUTE_COUNT=%u", GRID_ATTRIBUTE_COUNT),
		format_uint("USE_AMBIENT_LIGHT=%u", scene_spec->use_ambient_light),
		format_uint("POINT_LIGHT_COUNT=%u", scene_spec->point_light_count),
		format_uint("LIGHT_PROBE_WIDTH=%u", probe->probe.images[0].request.image_info.extent.width),
		format_uint("LIGHT_PROBE_HEIGHT=%u", probe->probe.images[0].request.image_info.extent.height),
		format_uint("VOLUME_COUNT=%u", volume->volume_count),
		format_uint("VOLUME_INTERPOLATION_NEAREST=%u", scene_spec->volume_interpolation == volume_interpolation_nearest),
		format_uint("VOLUME_INTERPOLATION_LINEAR=%u", scene_spec->volume_interpolation == volume_interpolation_linear),
		format_uint("VOLUME_INTERPOLATION_LINEAR_STOCHASTIC=%u", scene_spec->volume_interpolation == volume_interpolation_linear_stochastic),
		format_uint("VOLUME_COLOR_MODE_CONSTANT=%u", scene_spec->volume_color_mode == volume_color_mode_constant),
		format_uint("VOLUME_COLOR_MODE_TRANSFER_FUNCTION=%u", scene_spec->volume_color_mode == volume_color_mode_transfer_function),
		format_uint("VOLUME_COLOR_MODE_TRANSFER_FUNCTION_WITH_EXTINCTION=%u", scene_spec->volume_color_mode == volume_color_mode_transfer_function_with_extinction),
		format_uint("VOLUME_COLOR_MODE_SEPARATE=%u", scene_spec->volume_color_mode == volume_color_mode_separate),
		format_uint("DISTANCE_SAMPLER_RAY_MARCHING=%u", settings->distance_sampler == distance_sampler_ray_marching),
		format_uint("DISTANCE_SAMPLER_REGULAR_TRACKING=%u", settings->distance_sampler == distance_sampler_regular_tracking),
		format_uint("DISTANCE_SAMPLER_DELTA_TRACKING=%u", settings->distance_sampler == distance_sampler_delta_tracking),
		format_uint("OPTICAL_DEPTH_ESTIMATOR_EQUIDISTANT=%u", settings->optical_depth_estimator == optical_depth_estimator_equidistant),
		format_uint("OPTICAL_DEPTH_ESTIMATOR_KETTUNEN=%u", settings->optical_depth_estimator == optical_depth_estimator_kettunen),
		format_uint("OPTICAL_DEPTH_ESTIMATOR_MEAN_SAMPLING=%u", settings->optical_depth_estimator == optical_depth_estimator_mean_sampling),
		format_uint("OPTICAL_DEPTH_ESTIMATOR_MEAN_SAMPLING_FIXED_COUNT=%u", settings->optical_depth_estimator == optical_depth_estimator_mean_sampling_fixed_count),
		format_uint("OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE=%u", settings->optical_depth_estimator == optical_depth_estimator_variance_aware),
		format_uint("OPTICAL_DEPTH_ESTIMATOR_VARIANCE_AWARE_FIXED_COUNT=%u", settings->optical_depth_estimator == optical_depth_estimator_variance_aware_fixed_count),
		format_uint("USE_JITTERED=%u", settings->uniform_jittered ? 0 : 1),
		format_uint("USE_UNIFORM_JITTERED=%u", settings->uniform_jittered ? 1 : 0),
		format_uint("TRANSMITTANCE_ESTIMATOR_DUMMY=%u", settings->transmittance_estimator == transmittance_estimator_dummy),
		format_uint("TRANSMITTANCE_ESTIMATOR_UNBIASED_RAY_MARCHING=%u", settings->transmittance_estimator == transmittance_estimator_unbiased_ray_marching),
		format_uint("TRANSMITTANCE_ESTIMATOR_BIASED_RAY_MARCHING=%u", settings->transmittance_estimator == transmittance_estimator_biased_ray_marching),
		format_uint("TRANSMITTANCE_ESTIMATOR_JACKKNIFE=%u", settings->transmittance_estimator == transmittance_estimator_jackknife),
		format_uint("TRANSMITTANCE_ESTIMATOR_REGULAR_TRACKING=%u", settings->transmittance_estimator == transmittance_estimator_regular_tracking),
		format_uint("TRANSMITTANCE_ESTIMATOR_TRACK_LENGTH=%u", settings->transmittance_estimator == transmittance_estimator_track_length),
		format_uint("TRANSMITTANCE_ESTIMATOR_RATIO_TRACKING=%u", settings->transmittance_estimator == transmittance_estimator_ratio_tracking),
		format_uint("MIS_WEIGHT_ESTIMATOR_DUMMY=%u", settings->mis_weight_estimator == mis_weight_estimator_dummy),
		format_uint("MIS_WEIGHT_ESTIMATOR_UNBIASED_RAY_MARCHING=%u", settings->mis_weight_estimator == mis_weight_estimator_unbiased_ray_marching),
		format_uint("MIS_WEIGHT_ESTIMATOR_BIASED_RAY_MARCHING=%u", settings->mis_weight_estimator == mis_weight_estimator_biased_ray_marching),
		format_uint("MIS_WEIGHT_ESTIMATOR_JACKKNIFE=%u", settings->mis_weight_estimator == mis_weight_estimator_jackknife),
		format_uint("MIS_WEIGHT_ESTIMATOR_REGULAR_TRACKING=%u", settings->mis_weight_estimator == mis_weight_estimator_regular_tracking),
		format_uint("SAMPLING_STRATEGIES_LIGHT=%u", settings->sampling_strategies == sampling_strategies_light),
		format_uint("SAMPLING_STRATEGIES_TRANSMITTANCE=%u", settings->sampling_strategies == sampling_strategies_transmittance),
		format_uint("SAMPLING_STRATEGIES_LIGHT_TRANSMITTANCE_MIS=%u", settings->sampling_strategies == sampling_strategies_light_transmittance_mis),
		format_uint("SAMPLING_STRATEGIES_LIGHT_TRANSMITTANCE_APPROXIMATE_MIS=%u", settings->sampling_strategies == sampling_strategies_light_transmittance_approximate_mis),
		format_uint("SAMPLING_STRATEGIES_LIGHT_TRANSMITTANCE_MILLER_MIS=%u", settings->sampling_strategies == sampling_strategies_light_transmittance_miller_mis),
		format_uint("DISPLAYED_QUANTITY_RADIANCE=%u", settings->displayed_quantity == displayed_quantity_radiance),
		format_uint("DISPLAYED_QUANTITY_LUMINANCE_STATS=%u", settings->displayed_quantity == displayed_quantity_luminance_stats),
		format_uint("DISPLAYED_QUANTITY_TRANSMITTANCE=%u", settings->displayed_quantity == displayed_quantity_transmittance),
		format_uint("DISPLAYED_QUANTITY_TRANSMITTANCE_STATS=%u", settings->displayed_quantity == displayed_quantity_transmittance_stats),
		format_uint("DISPLAYED_QUANTITY_TRANSMITTANCE_SIGN=%u", settings->displayed_quantity == displayed_quantity_transmittance_sign),
		format_uint("DISPLAYED_QUANTITY_MIS_WEIGHT=%u", settings->displayed_quantity == displayed_quantity_mis_weight),
		format_uint("DISPLAYED_QUANTITY_MIS_WEIGHT_STATS=%u", settings->displayed_quantity == displayed_quantity_mis_weight_stats),
	};
	shader_compilation_request_t vert_request = {
		.shader_path = "src/shaders/volume.vert.glsl",
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.entry_point = "main",
		.defines = defines,
		.define_count = COUNT_OF(defines),
	};
	shader_compilation_request_t frag_request = {
		.shader_path = "src/shaders/volume.frag.glsl",
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.entry_point = "main",
		.defines = defines,
		.define_count = COUNT_OF(defines),
	};
	if (compile_and_create_shader_module(&subpass->vert_shader, device, &vert_request, true)
	 || compile_and_create_shader_module(&subpass->frag_shader, device, &frag_request, true)
	) {
		printf("Failed to compile one of the shaders for the scene subpass.\n");
		for (uint32_t i = 0; i != COUNT_OF(defines); ++i)
			free(defines[i]);
		free_scene_subpass(subpass, device);
		return 1;
	}
	for (uint32_t i = 0; i != COUNT_OF(defines); ++i)
		free(defines[i]);
	// Define the graphics pipeline state
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	};
	VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	};
	VkPipelineRasterizationStateCreateInfo raster_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.lineWidth = 1.0f,
	};
	VkPipelineViewportStateCreateInfo viewport_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pScissors = &(VkRect2D) { .extent = swapchain->extent },
		.scissorCount = 1,
		.pViewports = &(VkViewport) {
			.width = (float) swapchain->extent.width,
			.height = (float) swapchain->extent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		},
		.viewportCount = 1,
	};
	VkPipelineColorBlendStateCreateInfo blend_info_discard = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pAttachments = &(VkPipelineColorBlendAttachmentState) {
			.blendEnable = VK_FALSE,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		},
		.attachmentCount = 1,
	};
	VkPipelineColorBlendStateCreateInfo blend_info_accum = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pAttachments = &(VkPipelineColorBlendAttachmentState) {
			.blendEnable = VK_TRUE,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		},
		.attachmentCount = 1,
	};
	VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_FALSE,
		.depthWriteEnable = VK_FALSE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
	};
	VkPipelineMultisampleStateCreateInfo multi_sample_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};
	VkPipelineShaderStageCreateInfo shader_stages[2] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = subpass->vert_shader,
			.pName = vert_request.entry_point,
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = subpass->frag_shader,
			.pName = frag_request.entry_point,
		}
	};
	VkGraphicsPipelineCreateInfo pipeline_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.layout = subpass->descriptor_set.pipeline_layout,
		.pVertexInputState = &vertex_input_info,
		.pInputAssemblyState = &input_assembly_info,
		.pRasterizationState = &raster_info,
		.pViewportState = &viewport_info,
		.pColorBlendState = &blend_info_discard,
		.pMultisampleState = &multi_sample_info,
		.pDepthStencilState = &depth_stencil_info,
		.pStages = shader_stages,
		.stageCount = COUNT_OF(shader_stages),
		.renderPass = render_pass->render_pass,
		.subpass = 0,
	};
	if (vkCreateGraphicsPipelines(device->device, NULL, 1, &pipeline_info, NULL, &subpass->pipeline_discard)) {
		printf("Failed to create a graphics pipeline for the scene subpass.\n");
		free_scene_subpass(subpass, device);
		return 1;
	}
	pipeline_info.pColorBlendState = &blend_info_accum;
	if (vkCreateGraphicsPipelines(device->device, NULL, 1, &pipeline_info, NULL, &subpass->pipeline_accum)) {
		printf("Failed to create a graphics pipeline for the scene subpass.\n");
		free_scene_subpass(subpass, device);
		return 1;
	}
	return 0;
}


void free_scene_subpass(scene_subpass_t* subpass, const device_t* device) {
	if (subpass->pipeline_discard) vkDestroyPipeline(device->device, subpass->pipeline_discard, NULL);
	if (subpass->pipeline_accum) vkDestroyPipeline(device->device, subpass->pipeline_accum, NULL);
	free_descriptor_sets(&subpass->descriptor_set, device);
	if (subpass->vert_shader) vkDestroyShaderModule(device->device, subpass->vert_shader, NULL);
	if (subpass->frag_shader) vkDestroyShaderModule(device->device, subpass->frag_shader, NULL);
	if (subpass->volume_sampler) vkDestroySampler(device->device, subpass->volume_sampler, NULL);
	if (subpass->texture_sampler) vkDestroySampler(device->device, subpass->texture_sampler, NULL);
	memset(subpass, 0, sizeof(*subpass));
}


int create_tonemap_subpass(tonemap_subpass_t* subpass, const device_t* device, const render_targets_t* render_targets, const constant_buffers_t* constant_buffers, const render_pass_t* render_pass, const scene_spec_t* scene_spec) {
	memset(subpass, 0, sizeof(*subpass));
	// Create a descriptor set
	VkDescriptorSetLayoutBinding bindings[] = {
		// The constant buffer
		{ .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT },
		// The HDR radiance buffer
		{ .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT },
	};
	complete_descriptor_set_layout_bindings(bindings, COUNT_OF(bindings), 1, 0);
	if (create_descriptor_sets(&subpass->descriptor_set, device, bindings, COUNT_OF(bindings), 1)) {
		printf("Failed to create a descriptor set for the tonemapping subpass.\n");
		free_tonemap_subpass(subpass, device);
		return 1;
	}
	// Write to the descriptor set
	VkDescriptorBufferInfo constant_buffer_info = {
		.buffer = constant_buffers->buffer.buffers[0].buffer,
		.range = VK_WHOLE_SIZE,
	};
	VkDescriptorImageInfo hdr_radiance_info = {
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.imageView = render_targets->targets.images[render_target_index_hdr_radiance].view,
	};
	VkWriteDescriptorSet writes[] = {
		{ .dstBinding = 0, .pBufferInfo = &constant_buffer_info, },
		{ .dstBinding = 1, .pImageInfo = &hdr_radiance_info },
	};
	complete_descriptor_set_writes(writes, COUNT_OF(writes), bindings, COUNT_OF(bindings), subpass->descriptor_set.descriptor_sets[0]);
	vkUpdateDescriptorSets(device->device, COUNT_OF(writes), writes, 0, NULL);
	// Compile the shaders and create the shader modules
	char* defines[] = {
		format_uint("TONEMAPPER_OP_CLAMP=%u", scene_spec->tonemapper == tonemapper_op_clamp),
		format_uint("TONEMAPPER_OP_ACES=%u", scene_spec->tonemapper == tonemapper_op_aces),
		format_uint("TONEMAPPER_OP_KHRONOS_PBR_NEUTRAL=%u", scene_spec->tonemapper == tonemapper_op_khronos_pbr_neutral),
	};
	shader_compilation_request_t vert_request = {
		.shader_path = "src/shaders/tonemap.vert.glsl",
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.entry_point = "main",
		.defines = defines,
		.define_count = COUNT_OF(defines),
	};
	shader_compilation_request_t frag_request = {
		.shader_path = "src/shaders/tonemap.frag.glsl",
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.entry_point = "main",
		.defines = defines,
		.define_count = COUNT_OF(defines),
	};
	if (compile_and_create_shader_module(&subpass->vert_shader, device, &vert_request, true)
	 || compile_and_create_shader_module(&subpass->frag_shader, device, &frag_request, true)
	) {
		printf("Failed to compile one of the shaders for the tonemapping subpass.\n");
		for (uint32_t i = 0; i != COUNT_OF(defines); ++i)
			free(defines[i]);
		free_tonemap_subpass(subpass, device);
		return 1;
	}
	for (uint32_t i = 0; i != COUNT_OF(defines); ++i)
		free(defines[i]);
	// Define the graphics pipeline state
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	};
	VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	};
	VkPipelineRasterizationStateCreateInfo raster_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.cullMode = VK_CULL_MODE_NONE,
		.lineWidth = 1.0f,
	};
	VkExtent3D resolution = render_targets->targets.images[render_target_index_hdr_radiance].request.image_info.extent;
	VkPipelineViewportStateCreateInfo viewport_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pScissors = &(VkRect2D) { .extent = { .width = resolution.width, .height = resolution.height } },
		.scissorCount = 1,
		.pViewports = &(VkViewport) {
			.width = (float) resolution.width,
			.height = (float) resolution.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		},
		.viewportCount = 1,
	};
	VkPipelineColorBlendStateCreateInfo blend_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pAttachments = &(VkPipelineColorBlendAttachmentState) {
			.blendEnable = VK_FALSE,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		},
		.attachmentCount = 1,
	};
	VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_FALSE,
		.depthWriteEnable = VK_FALSE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
	};
	VkPipelineMultisampleStateCreateInfo multi_sample_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};
	VkPipelineShaderStageCreateInfo shader_stages[2] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = subpass->vert_shader,
			.pName = vert_request.entry_point,
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = subpass->frag_shader,
			.pName = frag_request.entry_point,
		}
	};
	VkGraphicsPipelineCreateInfo pipeline_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.layout = subpass->descriptor_set.pipeline_layout,
		.pVertexInputState = &vertex_input_info,
		.pInputAssemblyState = &input_assembly_info,
		.pRasterizationState = &raster_info,
		.pViewportState = &viewport_info,
		.pColorBlendState = &blend_info,
		.pMultisampleState = &multi_sample_info,
		.pDepthStencilState = &depth_stencil_info,
		.pStages = shader_stages,
		.stageCount = COUNT_OF(shader_stages),
		.renderPass = render_pass->render_pass,
		.subpass = 1,
	};
	if (vkCreateGraphicsPipelines(device->device, NULL, 1, &pipeline_info, NULL, &subpass->pipeline)) {
		printf("Failed to create a graphics pipeline for the tonemapping subpass.\n");
		free_tonemap_subpass(subpass, device);
		return 1;
	}
	return 0;
}


void free_tonemap_subpass(tonemap_subpass_t* subpass, const device_t* device) {
	if (subpass->pipeline) vkDestroyPipeline(device->device, subpass->pipeline, NULL);
	free_descriptor_sets(&subpass->descriptor_set, device);
	if (subpass->vert_shader) vkDestroyShaderModule(device->device, subpass->vert_shader, NULL);
	if (subpass->frag_shader) vkDestroyShaderModule(device->device, subpass->frag_shader, NULL);
	memset(subpass, 0, sizeof(*subpass));
}


int create_gui_subpass(gui_subpass_t* subpass, const device_t* device, const gui_t* gui, const swapchain_t* swapchain, const constant_buffers_t* constant_buffers, const render_pass_t* render_pass) {
	memset(subpass, 0, sizeof(*subpass));
	// Create a sampler for the glyph image
	VkSamplerCreateInfo sampler_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.minFilter = VK_FILTER_NEAREST,
		.magFilter = VK_FILTER_NEAREST,
		.anisotropyEnable = VK_FALSE,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
	};
	if (vkCreateSampler(device->device, &sampler_info, NULL, &subpass->sampler)) {
		printf("Failed to create a sampler for the glyph image in the GUI subpass.\n");
		free_gui_subpass(subpass, device);
		return 1;
	}
	// Create a descriptor set
	VkDescriptorSetLayoutBinding bindings[] = {
		// The constant buffer
		{ .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
		// The glyph image
		{ .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT },
	};
	complete_descriptor_set_layout_bindings(bindings, COUNT_OF(bindings), 1, 0);
	if (create_descriptor_sets(&subpass->descriptor_set, device, bindings, COUNT_OF(bindings), 1)) {
		printf("Failed to create a descriptor set for the GUI subpass.\n");
		free_gui_subpass(subpass, device);
		return 1;
	}
	// Write to the descriptor set
	VkDescriptorBufferInfo constant_buffer_info = {
		.buffer = constant_buffers->buffer.buffers[0].buffer,
		.range = VK_WHOLE_SIZE,
	};
	VkDescriptorImageInfo glyph_image_info = {
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.imageView = gui->glyph_image.images[0].view,
		.sampler = subpass->sampler,
	};
	VkWriteDescriptorSet writes[] = {
		{ .dstBinding = 0, .pBufferInfo = &constant_buffer_info, },
		{ .dstBinding = 1, .pImageInfo = &glyph_image_info },
	};
	complete_descriptor_set_writes(writes, COUNT_OF(writes), bindings, COUNT_OF(bindings), subpass->descriptor_set.descriptor_sets[0]);
	vkUpdateDescriptorSets(device->device, COUNT_OF(writes), writes, 0, NULL);
	// Compile the shaders and create the shader modules
	shader_compilation_request_t vert_request = {
		.shader_path = "src/shaders/gui.vert.glsl",
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.entry_point = "main",
	};
	shader_compilation_request_t frag_request = {
		.shader_path = "src/shaders/gui.frag.glsl",
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.entry_point = "main",
	};
	int result = compile_and_create_shader_module(&subpass->vert_shader, device, &vert_request, true)
				|| compile_and_create_shader_module(&subpass->frag_shader, device, &frag_request, true);
	if (result) {
		printf("Failed to compile one of the shaders for the GUI subpass.\n");
		free_gui_subpass(subpass, device);
		return 1;
	}
	// Define the graphics pipeline state
	VkVertexInputBindingDescription vertex_binding = { .stride = sizeof(gui_vertex_t) };
	VkVertexInputAttributeDescription vertex_attributes[] = {
		{ .location = 0, .offset = NK_OFFSETOF(gui_vertex_t, pos), .format = VK_FORMAT_R32G32_SFLOAT },
		{ .location = 1, .offset = NK_OFFSETOF(gui_vertex_t, tex_coord), .format = VK_FORMAT_R32G32_SFLOAT },
		{ .location = 2, .offset = NK_OFFSETOF(gui_vertex_t, color), .format = VK_FORMAT_R8G8B8A8_UINT },
		{ .location = 3, .offset = NK_OFFSETOF(gui_vertex_t, scissor), .format = VK_FORMAT_R16G16B16A16_SINT },
	};
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pVertexBindingDescriptions = &vertex_binding,
		.vertexBindingDescriptionCount = 1,
		.pVertexAttributeDescriptions = vertex_attributes,
		.vertexAttributeDescriptionCount = COUNT_OF(vertex_attributes),
	};
	VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	};
	VkPipelineRasterizationStateCreateInfo raster_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.cullMode = VK_CULL_MODE_NONE,
		.lineWidth = 1.0f,
	};
	VkPipelineViewportStateCreateInfo viewport_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pScissors = &(VkRect2D) { .extent = swapchain->extent },
		.scissorCount = 1,
		.pViewports = &(VkViewport) {
			.width = (float) swapchain->extent.width,
			.height = (float) swapchain->extent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		},
		.viewportCount = 1,
	};
	VkPipelineColorBlendStateCreateInfo blend_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pAttachments = &(VkPipelineColorBlendAttachmentState) {
			.blendEnable = VK_TRUE,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		},
		.attachmentCount = 1,
	};
	VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_FALSE,
		.depthWriteEnable = VK_FALSE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
	};
	VkPipelineMultisampleStateCreateInfo multi_sample_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};
	VkPipelineShaderStageCreateInfo shader_stages[2] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = subpass->vert_shader,
			.pName = vert_request.entry_point,
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = subpass->frag_shader,
			.pName = frag_request.entry_point,
		}
	};
	VkGraphicsPipelineCreateInfo pipeline_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.layout = subpass->descriptor_set.pipeline_layout,
		.pVertexInputState = &vertex_input_info,
		.pInputAssemblyState = &input_assembly_info,
		.pRasterizationState = &raster_info,
		.pViewportState = &viewport_info,
		.pColorBlendState = &blend_info,
		.pMultisampleState = &multi_sample_info,
		.pDepthStencilState = &depth_stencil_info,
		.pStages = shader_stages,
		.stageCount = COUNT_OF(shader_stages),
		.renderPass = render_pass->render_pass,
		.subpass = 1,
	};
	if (vkCreateGraphicsPipelines(device->device, NULL, 1, &pipeline_info, NULL, &subpass->pipeline)) {
		printf("Failed to create a graphics pipeline for the GUI subpass.\n");
		free_gui_subpass(subpass, device);
		return 1;
	}
	return 0;
}


void free_gui_subpass(gui_subpass_t* subpass, const device_t* device) {
	if (subpass->pipeline) vkDestroyPipeline(device->device, subpass->pipeline, NULL);
	free_descriptor_sets(&subpass->descriptor_set, device);
	if (subpass->vert_shader) vkDestroyShaderModule(device->device, subpass->vert_shader, NULL);
	if (subpass->frag_shader) vkDestroyShaderModule(device->device, subpass->frag_shader, NULL);
	if (subpass->sampler) vkDestroySampler(device->device, subpass->sampler, NULL);
	memset(subpass, 0, sizeof(*subpass));
}


int create_frame_workloads(frame_workloads_t* workloads, const device_t* device) {
	memset(workloads, 0, sizeof(*workloads));
	for (uint32_t i = 0; i != FRAME_IN_FLIGHT_COUNT; ++i) {
		frame_workload_t* frame = &workloads->frames_in_flight[i];
		VkCommandBufferAllocateInfo cmd_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = device->cmd_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};
		if (vkAllocateCommandBuffers(device->device, &cmd_info, &frame->cmd)) {
			printf("Failed to allocate a command buffer for a frame workload.\n");
			free_frame_workloads(workloads, device);
			return 1;
		}
		VkSemaphoreCreateInfo semaphore_info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VkFenceCreateInfo fence_info = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};
		VkQueryPoolCreateInfo query_info = {
			.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
			.queryType = VK_QUERY_TYPE_TIMESTAMP,
			.queryCount = timestamp_index_count,
		};
		if (vkCreateSemaphore(device->device, &semaphore_info, NULL, &frame->image_acquired)
		 || vkCreateSemaphore(device->device, &semaphore_info, NULL, &frame->queue_finished[0])
		 || vkCreateSemaphore(device->device, &semaphore_info, NULL, &frame->queue_finished[1])
		 || vkCreateFence(device->device, &fence_info, NULL, &frame->frame_finished)
		 || vkCreateQueryPool(device->device, &query_info, NULL, &frame->query_pool))
		{
			printf("Failed to create semaphores, fences or a query pool for a frame workload.\n");
			free_frame_workloads(workloads, device);
			return 1;
		}
	}
	return 0;
}


void free_frame_workloads(frame_workloads_t* workloads, const device_t* device) {
	for (uint32_t i = 0; i != FRAME_IN_FLIGHT_COUNT; ++i) {
		frame_workload_t* frame = &workloads->frames_in_flight[i];
		if (frame->cmd) vkFreeCommandBuffers(device->device, device->cmd_pool, 1, &frame->cmd);
		if (frame->query_pool) vkDestroyQueryPool(device->device, frame->query_pool, NULL);
		if (frame->image_acquired) vkDestroySemaphore(device->device, frame->image_acquired, NULL);
		if (frame->queue_finished[0]) vkDestroySemaphore(device->device, frame->queue_finished[0], NULL);
		if (frame->queue_finished[1]) vkDestroySemaphore(device->device, frame->queue_finished[1], NULL);
		if (frame->frame_finished) vkDestroyFence(device->device, frame->frame_finished, NULL);
	}
	memset(workloads, 0, sizeof(*workloads));
}


int create_slideshow(slideshow_t* slideshow, scene_spec_t* scene_spec, render_settings_t* render_settings, app_update_t* update) {
	slideshow->slide_count = create_slides(slideshow->slides);
	if (slideshow->slide_count > MAX_SLIDE_COUNT)
		return 1;
	if (slideshow->slide_end > slideshow->slide_count) {
		printf("There are only %u slides but the slideshow was requested to end at slide %u.\n", slideshow->slide_count, slideshow->slide_end);
		return 1;
	}
	if (slideshow->slide_begin < slideshow->slide_end)
		return show_slide(slideshow, scene_spec, render_settings, update, slideshow->slide_begin);
	return 0;
}


int show_slide(slideshow_t* slideshow, scene_spec_t* scene_spec, render_settings_t* render_settings, app_update_t* update, uint32_t slide_index) {
	if (slide_index >= slideshow->slide_count)
		printf("Slide index %u is past the end of the slideshow with its %u slides.\n", slide_index, slideshow->slide_count);
	slideshow->slide_current = slide_index;
	render_settings_t old_settings = *render_settings;
	scene_spec_t old_spec = copy_scene_spec(scene_spec);
	const slide_t* slide = &slideshow->slides[slideshow->slide_current];
	if (quickload(scene_spec, update, slide->quicksave))
		return 1;
	(*render_settings) = slide->render_settings;
	if (slide->volume_interpolation >= 0 && slide->volume_interpolation < volume_interpolation_count)
		scene_spec->volume_interpolation = slide->volume_interpolation;
	if (slide->tonemapper >= 0 && slide->tonemapper < tonemapper_op_count)
		scene_spec->tonemapper = slide->tonemapper;
	get_scene_spec_updates(update, &old_spec, scene_spec);
	get_render_settings_updates(update, &old_settings, render_settings);
	if (slide->screenshot_path)
		printf("Showing slide %u (%s).\n", slide_index, slide->screenshot_path);
	else
		printf("Showing slide %u.\n", slide_index);
	return 0;
}


void free_slideshow(slideshow_t* slideshow) {
	for (uint32_t i = 0; i != slideshow->slide_count; ++i) {
		free(slideshow->slides[i].quicksave);
		free(slideshow->slides[i].screenshot_path);
	}
	memset(slideshow, 0, sizeof(*slideshow));
}


bool update_needed(const app_update_t* update) {
	app_update_t zero_update;
	memset(&zero_update, 0, sizeof(zero_update));
	return memcmp(update, &zero_update, sizeof(zero_update)) != 0;
}


int update_app(app_t* app, const app_update_t* update, bool recreate) {
	// Early out if there is nothing to do
	if (!update_needed(update))
		return 0;
	app_update_t up = *update;
	// Reset accumulation
	app->render_targets.accum_frame_count = 0;
	// Let all GPU work finish before destroying objects it may rely on
	if (app->device.device)
		vkDeviceWaitIdle(app->device.device);
	// Propagate dependencies
	for (uint32_t i = 0; i != sizeof(app_update_t) / sizeof(bool); ++i) {
		up.gui |= up.device | up.window;		
		up.swapchain |= up.device | up.window;
		up.render_targets |= up.device | up.swapchain;
		up.constant_buffers |= up.device | up.volume;
		up.volume |= up.device;
		up.transfer_function |= up.device;
		up.volume_as |= up.device | up.volume | up.transfer_function | up.constant_buffers;
		up.light_probe |= up.device;
		up.render_pass |= up.device | up.swapchain | up.render_targets;
		up.scene_subpass |= up.device | up.swapchain | up.constant_buffers | up.volume | up.volume_as | up.transfer_function | up.light_probe | up.render_pass;
		up.tonemap_subpass |= up.device | up.render_targets | up.constant_buffers | up.render_pass;
		up.gui_subpass |= up.device | up.gui | up.swapchain | up.constant_buffers | up.render_pass;
		up.frame_workloads |= up.device;
	}
	// Free objects in reversed order
	if (up.frame_workloads) free_frame_workloads(&app->frame_workloads, &app->device);
	if (up.gui_subpass) free_gui_subpass(&app->gui_subpass, &app->device);
	if (up.tonemap_subpass) free_tonemap_subpass(&app->tonemap_subpass, &app->device);
	if (up.scene_subpass) free_scene_subpass(&app->scene_subpass, &app->device);
	if (up.render_pass) free_render_pass(&app->render_pass, &app->device);
	if (up.light_probe) free_light_probe(&app->light_probe, &app->device);
	if (up.volume_as) free_volume_acceleration_structure(&app->volume_as, &app->device);
	if (up.volume) free_volume(&app->volume, &app->device);
	if (up.transfer_function) free_transfer_function(&app->transfer_function, &app->device);
	if (up.constant_buffers) free_constant_buffers(&app->constant_buffers, &app->device);
	if (up.render_targets) free_render_targets(&app->render_targets, &app->device);
	if (up.swapchain) free_swapchain(&app->swapchain, &app->device);
	if (up.gui) free_gui(&app->gui, &app->device, app->window);
	if (up.window) free_window(&app->window);
	if (up.device) free_device(&app->device);
	// Early out if we only wanted to free
	if (!recreate)
		return 0;
	// (Re-)create objects
	int ret = 0;
	if (up.device && (ret = create_device(&app->device, "Volume renderer", 0))
	 || up.window && (ret = create_window(&app->window, &app->params.initial_window_extent))
	 || up.gui && (ret = create_gui(&app->gui, &app->device, app->window))
	 || up.swapchain && (ret = create_swapchain(&app->swapchain, &app->device, app->window, app->params.v_sync))
	 || up.render_targets && (ret = create_render_targets(&app->render_targets, &app->device, &app->swapchain))
	 || up.transfer_function && (ret = create_transfer_function(&app->transfer_function, &app->device, &app->scene_spec))
	 || up.volume && (ret = create_volume(&app->volume, &app->device, &app->scene_spec))
	 || up.constant_buffers && (ret = create_constant_buffers(&app->constant_buffers, &app->device, &app->scene_spec, &app->volume))
	 || up.volume_as && (ret = create_volume_acceleration_structure(&app->volume_as, &app->device, &app->volume, &app->transfer_function, &app->constant_buffers, &app->scene_spec, &app->render_settings))
	 || up.light_probe && (ret = create_light_probe(&app->light_probe, &app->device, app->scene_spec.light_probe_path))
	 || up.render_pass && (ret = create_render_pass(&app->render_pass, &app->device, &app->swapchain, &app->render_targets))
	 || up.scene_subpass && (ret = create_scene_subpass(&app->scene_subpass, &app->device, &app->scene_spec, &app->render_settings, &app->swapchain, &app->constant_buffers, &app->volume, &app->volume_as, &app->transfer_function, &app->light_probe, &app->render_pass))
	 || up.tonemap_subpass && (ret = create_tonemap_subpass(&app->tonemap_subpass, &app->device, &app->render_targets, &app->constant_buffers, &app->render_pass, &app->scene_spec))
	 || up.gui_subpass && (ret = create_gui_subpass(&app->gui_subpass, &app->device, &app->gui, &app->swapchain, &app->constant_buffers, &app->render_pass))
	 || up.frame_workloads && (ret = create_frame_workloads(&app->frame_workloads, &app->device))
	)
	{
		if (ret) {
			printf("Failed to initialize application objects. Error code 0x%08x.\n", ret);
			return ret;
		}
	}
	return 0;
}


int create_app(app_t* app, const app_params_t* app_params, const slideshow_t* slideshow) {
	memset(app, 0, sizeof(*app));
	if (app_params) app->params = (*app_params);
	if (slideshow) app->slideshow = (*slideshow);
	app_update_t dummy;
	if (quickload(&app->scene_spec, &dummy, NULL)) {
		app->scene_spec = get_default_scene_spec(true);
		printf("Using a default scene specification.\n");
	}
	init_render_settings(&app->render_settings);
	if (create_slideshow(&app->slideshow, &app->scene_spec, &app->render_settings, &dummy)) {
		printf("Failed to initialize the slideshow.\n");
		return 1;
	}
	app_update_t update;
	memset(&update, 1, sizeof(update));
	return update_app(app, &update, true);
}


void free_app(app_t* app) {
	app_update_t update;
	memset(&update, 1, sizeof(update));
	update_app(app, &update, VK_FALSE);
	free_scene_spec(&app->scene_spec);
	free_slideshow(&app->slideshow);
}


bool key_pressed(GLFWwindow* window, int key_code) {
	if (key_code < 0 || key_code > GLFW_KEY_LAST)
		return false;
	static int prev_states[GLFW_KEY_LAST + 1] = { GLFW_RELEASE };
	int state = glfwGetKey(window, key_code);
	bool result = (state == GLFW_PRESS && prev_states[key_code] == GLFW_RELEASE);
	prev_states[key_code] = state;
	return result;
}


void get_camera_ray(float out_ray_origin[3], float out_ray_dir[3], const camera_t* camera, const float viewport[4], const float pixel[2]) {
	float x = pixel[0], y = pixel[1];
	float uv[2] = { (x - viewport[0]) / (viewport[1] - viewport[0]), (y - viewport[2]) / (viewport[3] - viewport[2]) };
	float world_to_proj[4 * 4], proj_to_world[4 * 4];
	float aspect = (viewport[1] - viewport[0]) / (viewport[3] - viewport[2]);
	get_world_to_projection_space(world_to_proj, camera, aspect);
	invert_mat4(proj_to_world, world_to_proj);
	get_camera_ray_origin(out_ray_origin, uv, proj_to_world);
	get_camera_ray_direction(out_ray_dir, uv, world_to_proj);
}


int handle_user_input(app_t* app, app_update_t* update) {
	// Check if the framebuffer size has changed
	int new_width, new_height;
	glfwGetFramebufferSize(app->window, &new_width, &new_height);
	if (app->swapchain.extent.width != (uint32_t) new_width || app->swapchain.extent.height != (uint32_t) new_height)
		update->swapchain = true;
	// Keep track of whether the scene specification or render settings change to
	// reset accumulation
	scene_spec_t old_spec = copy_scene_spec(&app->scene_spec);
	render_settings_t old_settings = app->render_settings;
	app_params_t old_params = app->params;
	// A new frame has begun. Record its time.
	record_frame_time();
	++app->scene_spec.frame_index;
	// Toggle display of the GUI
	if (key_pressed(app->window, GLFW_KEY_F1))
		app->params.gui = !app->params.gui;
	// Let the GUI respond to user input (and poll events)
	handle_gui_input(&app->gui, app->window);
	// Define the GUI
	bool disable_camera_controls = false;
	if (app->params.gui) {
		disable_camera_controls = define_gui(&app->gui.context, &app->scene_spec, &app->render_settings, update, &app->params, &app->render_targets, &app->volume, &app->transfer_function, &app->swapchain, app->frame_workloads.timestamps, app->device.physical_device_properties.limits.timestampPeriod);
		// Disable the cursor when properties are being dragged, so that it
		// will not get stuck at the screen boundary
		bool dragging = false;
		for (struct nk_window* win = app->gui.context.begin; win != NULL; win = win->next)
			dragging = dragging || (win->property.state == 2);
		for (uint32_t i = 0; i != MAX_CAMERA_COUNT; ++i)
			if (app->scene_spec.cameras[i].rotation.mouse_active)
				dragging = true;
		if (dragging != app->gui.cursor_disabled) {
			app->gui.cursor_disabled = !app->gui.cursor_disabled;
			glfwSetInputMode(app->window, GLFW_CURSOR, app->gui.cursor_disabled ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
		}
	}
	// Use camera controls and update corresponding constants
	if (!disable_camera_controls) {
		float ports[MAX_CAMERA_COUNT][4];
		compute_camera_viewports(ports, &app->swapchain.extent, &app->scene_spec, &app->params);
		int32_t camera_index = -1;
		for (int32_t i = 0; i != MAX_CAMERA_COUNT; ++i)
			if (app->scene_spec.cameras[i].rotation.mouse_active)
				camera_index = i;
		if (camera_index < 0) {
			double x_double, y_double;
			glfwGetCursorPos(app->window, &x_double, &y_double);
			float x = (float) x_double;
			float y = (float) y_double;
			for (int32_t i = 0; i != MAX_CAMERA_COUNT; ++i) {
				if (x >= ports[i][0] && x < ports[i][1] && y >= ports[i][2] && y < ports[i][3]) {
					camera_index = i;
					// If the middle mouse button is pressed, print the camera ray
					static int mmb_down = 0;
					int new_mmb_down = glfwGetMouseButton(app->window, GLFW_MOUSE_BUTTON_3);
					if (mmb_down == GLFW_RELEASE && new_mmb_down == GLFW_PRESS) {
						float ray_origin[3], ray_dir[3];
						float pixel[] = { x + 0.5f, y + 0.5f };
						get_camera_ray(ray_origin, ray_dir, &app->scene_spec.cameras[camera_index], ports[i], pixel);
						printf("Camera ray (pixel %.1f, %.1f): (%.8f, %.8f, %.8f), (%.8f, %.8f, %.8f)\n", pixel[0], pixel[1], ray_origin[0], ray_origin[1], ray_origin[2], ray_dir[0], ray_dir[1], ray_dir[2]);
					}
					mmb_down = new_mmb_down;
				}
			}
		}
		if (camera_index >= 0)
			control_camera(&app->scene_spec.cameras[camera_index], app->window);
	}
	// Quicksave and quickload
	if (key_pressed(app->window, GLFW_KEY_F2)) {
		app->params.v_sync = !app->params.v_sync;
		update->swapchain = true;
	}
	if (key_pressed(app->window, GLFW_KEY_F3))
		quicksave(&app->scene_spec, NULL);
	if (key_pressed(app->window, GLFW_KEY_F4))
		quickload(&app->scene_spec, update, NULL);
	for (uint32_t i = 0; i != 10; ++i) {
		char* quicksave_path = format_uint("data/quicksaves/%u.jack_save", i);
		if (key_pressed(app->window, GLFW_KEY_0 + i)) {
			if (glfwGetKey(app->window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
				if (glfwGetKey(app->window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
					quicksave(&app->scene_spec, quicksave_path);
				else
					quickload(&app->scene_spec, update, quicksave_path);
			}
		}
		free(quicksave_path);
	}
	// Hot shader reload
	if (key_pressed(app->window, GLFW_KEY_F5))
		update->scene_subpass = update->tonemap_subpass = update->gui_subpass = true;
	// Take screenshots
	if (key_pressed(app->window, GLFW_KEY_F9))
		save_screenshot("data/screenshot.pfm", image_file_format_pfm, &app->device, &app->render_targets, &app->scene_spec);
	if (key_pressed(app->window, GLFW_KEY_F10))
		save_screenshot("data/screenshot.hdr", image_file_format_hdr, &app->device, &app->render_targets, &app->scene_spec);
	if (key_pressed(app->window, GLFW_KEY_F11))
		save_screenshot("data/screenshot.png", image_file_format_png, &app->device, &app->render_targets, &app->scene_spec);
	if (key_pressed(app->window, GLFW_KEY_F12))
		save_screenshot("data/screenshot.jpg", image_file_format_jpg, &app->device, &app->render_targets, &app->scene_spec);
	// Ensure that all render settings are valid
	make_render_settings_valid(&app->render_settings, &app->scene_spec);
	// Slideshow controls
	bool terminate = false;
	if (app->slideshow.slide_begin < app->slideshow.slide_end) {
		uint32_t new_slide = app->slideshow.slide_current;
		if (app->params.slide_screenshots) {
			const slide_t* slide = &app->slideshow.slides[app->slideshow.slide_current];
			if (slide->screenshot_path && slide->screenshot_frame == app->render_targets.accum_frame_count) {
				save_screenshot(slide->screenshot_path, slide->screenshot_format, &app->device, &app->render_targets, &app->scene_spec);
				// Report timings
				frame_time_stats_t stats = get_frame_stats();
				float shading_time = app->frame_workloads.timestamps[timestamp_index_shading_end] - app->frame_workloads.timestamps[timestamp_index_shading_begin];
				shading_time *= 1.0e-6f * app->device.physical_device_properties.limits.timestampPeriod;
				printf("Saved screenshot %s. Shading time: %.3f ms. Frame time stats, mean: %.3f ms, 0%%: %.3f ms, 1%%: %.3f ms, 10%%: %.3f ms, 50%%: %.3f ms, 90%%: %.3f ms, 99%%: %.3f ms, 100%%: %.3f ms\n", slide->screenshot_path, shading_time, stats.mean * 1.0e3f, stats.min * 1.0e3f, stats.percentile_1 * 1.0e3f, stats.percentile_10 * 1.0e3f, stats.median * 1.0e3f, stats.percentile_90 * 1.0e3f, stats.percentile_99 * 1.03f);
				++new_slide;
			}
		}
		if (key_pressed(app->window, GLFW_KEY_LEFT) && new_slide > app->slideshow.slide_begin)
			--new_slide;
		if (key_pressed(app->window, GLFW_KEY_RIGHT))
			++new_slide;
		if (key_pressed(app->window, GLFW_KEY_UP))
			if (new_slide > app->slideshow.slide_begin)
				new_slide = app->slideshow.slide_begin;
			else
				terminate = true;
		if (key_pressed(app->window, GLFW_KEY_DOWN))
			terminate = true;
		if (new_slide >= app->slideshow.slide_end)
			terminate = true;
		if (new_slide != app->slideshow.slide_current && new_slide < app->slideshow.slide_end) {
			if (show_slide(&app->slideshow, &app->scene_spec, &app->render_settings, update, new_slide))
				terminate = true;
			app->render_targets.accum_frame_count = 0;
		}
	}
	// Produce update due to scene and setting changes
	get_scene_spec_updates(update, &old_spec, &app->scene_spec);
	get_render_settings_updates(update, &old_settings, &app->render_settings);
	// Reset accumulation if necessary
	if (glfwGetKey(app->window, GLFW_KEY_F6) == GLFW_PRESS
	 || check_accumulation_reset(&old_spec, &app->scene_spec, &old_settings, &app->render_settings, &old_params, &app->params))
		app->render_targets.accum_frame_count = 0;
	free_scene_spec(&old_spec);
	// Check whether the application should terminate (escape or window closed)
	terminate |= glfwWindowShouldClose(app->window) || glfwGetKey(app->window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
	return terminate ? 1 : 0;
}


void compute_camera_viewports(float viewports[MAX_CAMERA_COUNT][4], const VkExtent2D* swapchain_extent, const scene_spec_t* scene_spec, const app_params_t* app_params) {
	float ports[MAX_CAMERA_COUNT][4] = {
		{ 0.0f, 1.0f, 0.0f, 1.0f },
	};
	float pad = 0.0f;
	float offsets[MAX_CAMERA_COUNT][4] = {
		{ 0.0f, -pad, 0.0f, -pad },
	};
	float left = 0.0f;
	float width = ((float) swapchain_extent->width) - left;
	float top = 0.0f;
	float height = ((float) swapchain_extent->height);
	for (uint32_t i = 0; i != MAX_CAMERA_COUNT; ++i) {
		viewports[i][0] = left + width * ports[i][0] + offsets[i][0];
		viewports[i][1] = left + width * ports[i][1] + offsets[i][1];
		viewports[i][2] = top + height * ports[i][2] + offsets[i][2];
		viewports[i][3] = top + height * ports[i][3] + offsets[i][3];
	}
}


//! Defines 3 axis-aligned orthographic cameras and a perspective camera
//! showing the given volume
void set_default_cameras(scene_spec_t* scene_spec, const volume_t* volume, const VkExtent2D* swapchain_extent, const app_params_t* app_params) {
	// Compute bounding box vertices for the volume
	float box[8][3];
	float center[3] = { 0.0f, 0.0f, 0.0f };
	VkExtent3D extent = volume->volume.images[0].request.image_info.extent;
	uint32_t res[3] = { extent.width, extent.height, extent.depth };
	for (uint32_t i = 0; i != 8; ++i) {
		float point[4];
		for (uint32_t j = 0; j != 3; ++j)
			point[j] = (float) (res[j] * ((i >> j) & 1u));
		point[3] = 1.0f;
		mat_vec_mul(box[i], volume->texel_to_world_space, point, 3, 4);
		for (uint32_t j = 0; j != 3; ++j)
			center[j] += 0.125f * box[i][j];
	}
	// Compute a global scale parameter for clipping planes and such
	float size = 0.0f;
	for (uint32_t i = 0; i != 8; ++i) {
		float dist = 0.0f;
		for (uint32_t j = 0; j != 3; ++j)
			dist += (box[i][j] - center[j]) * (box[i][j] - center[j]);
		dist = sqrtf(dist);
		if (size < dist) size = dist;
	}
	size *= 2.0f;
	float size_rounded = powf(10.0f, floorf(log10f(size)));
	// Figure out what the (new) viewports are
	float viewports[MAX_CAMERA_COUNT][4];
	compute_camera_viewports(viewports, swapchain_extent, scene_spec, app_params);
	// Place the perspective camera
	camera_t cam = {
		.type = camera_type_first_person,
		.near = 1.0e-5f * size_rounded, .far = 5.0f * size_rounded,
		.fov = M_PI * 0.4f,
		.speed = size_rounded,
		.rotation = { .angles = { 0.3f * M_PI, 0.0f, -0.25f * M_PI } },
	};
	float view_to_world[3 * 3];
	rotation_matrix_from_angles(view_to_world, cam.rotation.angles);
	float view_space_offset[3] = { 0.0f, 0.0f, 0.4f * size };
	mat_vec_mul(cam.position, view_to_world, view_space_offset, 3, 3);
	for (uint32_t j = 0; j != 3; ++j)
		cam.position[j] += center[j];
	scene_spec->cameras[0] = cam;
}


//! Utility for Nuklear to manipulate an sRGB color with separate brightness control
void hdr_color_picker(struct nk_context* ctx, hdr_color_spec_t* hdr_color, const char* label) {
	struct nk_colorf col = { .r = hdr_color->srgb_color[0], .g = hdr_color->srgb_color[1], .b = hdr_color->srgb_color[2], .a = 1.0f };
	if (label) {
		nk_layout_row_dynamic(ctx, 30, 1);
		nk_label(ctx, label, NK_TEXT_ALIGN_LEFT);
	}
	nk_layout_row_dynamic(ctx, 30, 2);
	if (nk_combo_begin_color(ctx, nk_rgb_cf(col), nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		col = nk_color_picker(ctx, col, NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		col.r = nk_propertyf(ctx, "#R:", 0, col.r, 1.0f, 0.01f, 0.005f);
		col.g = nk_propertyf(ctx, "#G:", 0, col.g, 1.0f, 0.01f, 0.005f);
		col.b = nk_propertyf(ctx, "#B:", 0, col.b, 1.0f, 0.01f, 0.005f);
		nk_combo_end(ctx);
	}
	memcpy(hdr_color->srgb_color, &col, sizeof(float) * 3);
	nk_property_float(ctx, "#Brightness:", 1.0e-34f, &hdr_color->linear_factor, 1.0e38f, hdr_color->linear_factor * 0.1f, hdr_color->linear_factor * 5.0e-3f);
}


/*! Displays a file selection dialog and returns once it has been closed.
	\param file_path A pointer to the path that is to be updated. The current
		value is used as starting point for file selection (unless it is NULL
		or an empty string). Upon file selection, this string will be freed and
		the pointer updated to a new string that has to be freed by the calling
		side.
	\param save Whether a file is to be opened or saved.
	\param directory Whether a file or a directory is to be selected.
	\param filters Filter strings in the format NAME | PATTERN1 PATTERN2 ...
	\param filter_count Number of filter strings.
	\return true if the user has made a file selection.
	\note This function is not thread-safe and requires a binary for zenity to
		be available.*/
bool file_select_dialog(char** file_path, bool save, bool directory, const char* const* filters, uint32_t filter_count) {
	if (!file_path)
		return false;
	// Take apart the current file path into directory and file name
	char* cwd = NULL;
	char* file_name = "";
	if (*file_path) {
		char* slash = strrchr(*file_path, '/');
		char* back = strrchr(*file_path, '\\');
		if (slash < back) slash = back;
		if (slash) {
			size_t length = 1 + slash - *file_path;
			cwd = malloc(length);
			memcpy(cwd, *file_path, length);
			cwd[length - 1] = '\0';
			file_name = slash + 1;
		}
	}
	const char** pieces = malloc((10 + 3 * filter_count) * sizeof(char*));
	uint32_t cnt = 0;
	if (cwd) {
		pieces[cnt++] = "(cd \"";
		pieces[cnt++] = cwd;
		pieces[cnt++] = "\"; ";
	}
	pieces[cnt++] = "zenity --file-selection ";
	if (cwd && file_name) {
		pieces[cnt++] = "--filename=\"";
		pieces[cnt++] = file_name;
		pieces[cnt++] = "\" ";
	}
	if (save) pieces[cnt++] = "--save ";
	if (directory) pieces[cnt++] = "--directory ";
	for (uint32_t i = 0; i != filter_count; ++i) {
		pieces[cnt++] = "--file-filter=\"";
		pieces[cnt++] = filters[i];
		pieces[cnt++] = "\" ";
	}
	if (cwd)
		pieces[cnt++] = ") ";
	pieces[cnt++] = "> selected_file.txt";
	char* cmd = cat_strings(pieces, cnt);
	free(cwd);
	free(pieces);
	int result = system(cmd);
	free(cmd);
	if (result != 0)
		return false;
	else {
		FILE* file = fopen("selected_file.txt", "r");
		if (!file) return false;
		fseek(file, 0, SEEK_END);
		long file_size = ftell(file);
		fseek(file, 0, SEEK_SET);
		char* path = malloc((file_size + 1) * sizeof(char));
		path[file_size] = '\0';
		if (fread(path, 1, file_size, file) == file_size) {
			for (long i = 1; i != 3; ++i)
				if (file_size > i && (path[file_size - i] == '\r' || path[file_size - i] == '\n'))
					path[file_size - i] = '\0';
			free(*file_path);
			(*file_path) = path;
			return true;
		}
		else {
			free(path);
			return false;
		}
	}
}


/*! A portable alternative to file_select_dialog(), which only lets the user
	choose from a predefined number of options using a drop-down menu.
	\param ctx The Nuklear context.
	\param file_path A pointer to the path that is to be updated. Upon file
		selection, this string will be freed and the pointer updated to a new
		string that has to be freed by the calling side.
	\param path_pattern A format string where %s is replaced by an entry from
		choices to construct the new file_path.
	\param choices Available choices for the file name.
	\param choice_count The number of available choices.
	\param combo_size The size of the dropdown in pixels.
	\return true if the user has changed the file selection.*/
bool file_choice_combo(struct nk_context* ctx, char** file_path, const char* path_fmt, const char** choices, uint32_t choice_count, struct nk_vec2 combo_size) {
	// Figure out what is currently selected
	int current_choice = -1;
	for (uint32_t i = 0; i != choice_count; ++i) {
		char* candidate = format_string(path_fmt, choices[i]);
		if (strcmp(candidate, *file_path) == 0) {
			current_choice = (int) i;
			break;
		}
		free(candidate);
	}
	int new_choice = nk_combo(ctx, choices, (int) choice_count, (current_choice == -1) ? 0 : current_choice, 30, combo_size);
	if (new_choice != current_choice) {
		free(*file_path);
		(*file_path) = format_string(path_fmt, choices[new_choice]);
		return true;
	}
	return false;
}


//! Inserts a little vertical skip to separate GUI elements
void nk_default_skip(struct nk_context* ctx) {
	nk_layout_row_dynamic(ctx, 5, 1);
}


bool define_gui(struct nk_context* ctx, scene_spec_t* scene_spec, render_settings_t* render_settings, app_update_t* update, app_params_t* app_params, const render_targets_t* render_targets, const volume_t* volume, const transfer_function_t* transfer, const swapchain_t* swapchain, uint64_t timestamps[timestamp_index_count], float timestamp_period) {
	bool disable_camera_controls = false;
	struct nk_rect bounds = { .x = 0.0f, .y = 0.0f, .w = 400.0f, .h = 500.0f };
	struct nk_vec2 combo_size = { .x = 375.0f, .y = 320.0f };
	if (nk_begin(ctx, "Volume renderer", bounds, NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE)) {
		// Display the frame rate and an indicator whether the UI is refreshing
		nk_layout_row_dynamic(ctx, 30, 2);
		nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "Frame time: %.2f ms", 1000.0f * get_frame_stats().median);
		const char* indicator[] = {
			" .......",
			". ......",
			".. .....",
			"... ....",
			".... ...",
			"..... ..",
			"...... .",
			"....... ",
		};
		static uint32_t frame_index = 0;
		nk_label(ctx, indicator[frame_index % COUNT_OF(indicator)], NK_TEXT_ALIGN_RIGHT);
		++frame_index;
		nk_layout_row_dynamic(ctx, 30, 1);
		nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "Shading time: %.2f ms", 1.0e-6f * timestamp_period * (float) (timestamps[timestamp_index_shading_end] - timestamps[timestamp_index_shading_begin]));
		nk_layout_row_dynamic(ctx, 30, 1);
		nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "Sample count: %u", render_targets->accum_frame_count);
		if (nk_tree_push(ctx, NK_TREE_TAB, "Tonemapping", NK_MINIMIZED)) {
			// Exposure adjustment
			nk_layout_row_dynamic(ctx, 30, 1);
			nk_property_float(ctx, "Exposure:", 1.0e-34f, &scene_spec->exposure, 1.0e38f, scene_spec->exposure * 0.1f, scene_spec->exposure * 5.0e-3f);
			// Define a drop-down menu to select the tonemapping operator
			nk_layout_row_dynamic(ctx, 30, 2);
			const char* tonemappers[tonemapper_op_count];
			tonemappers[tonemapper_op_clamp] = "Clamp";
			tonemappers[tonemapper_op_aces] = "ACES";
			tonemappers[tonemapper_op_khronos_pbr_neutral] = "Khronos PBR neutral";
			scene_spec->tonemapper = nk_combo(ctx, tonemappers, COUNT_OF(tonemappers), scene_spec->tonemapper, 30, combo_size);
			nk_label(ctx, "Tonemapping operator", NK_TEXT_ALIGN_LEFT);
			nk_tree_pop(ctx);
		}
		nk_default_skip(ctx);
		if (nk_tree_push(ctx, NK_TREE_TAB, "Camera", NK_MINIMIZED)) {
			// A layout with 3 axis-aligned ortho views and a perspective view
			if (nk_button_label(ctx, "Set default camera"))
				set_default_cameras(scene_spec, volume, &swapchain->extent, app_params);
			// Controls for each individual camera
			const char* names[] = { "Main camera" };
			for (uint32_t i = 0; i != MAX_CAMERA_COUNT; ++i) {
				camera_t* cam = &scene_spec->cameras[i];
				if (nk_tree_push(ctx, NK_TREE_TAB, names[i], NK_MAXIMIZED)) {
					nk_layout_row_dynamic(ctx, 30, 1);
					nk_bool ortho = (cam->type == camera_type_ortho);
					nk_checkbox_label(ctx, "Orthographic", &ortho);
					cam->type = ortho ? camera_type_ortho : camera_type_first_person;
					const char* names[] = { "#x:", "#y:", "#z:" };
					nk_layout_row_dynamic(ctx, 30, 3);
					for (uint32_t j = 0; j != 3; ++j)
						nk_property_float(ctx, names[j], -1.0e30f, &cam->position[j], 1.0e30f, fabsf(cam->position[j]) * 0.1f, fabsf(cam->position[j]) * 5.0e-3f);
					nk_layout_row_dynamic(ctx, 30.0f, 1);
					nk_property_float(ctx, "#Speed:", 0.0f, &cam->speed, 1.0e30f, cam->speed * 0.1f, cam->speed * 5.0e-3f);
					const char* rot_names[] = { "#rot x:", "#rot y:", "#rot z:" };
					nk_layout_row_dynamic(ctx, 30, 3);
					for (uint32_t j = 0; j != 3; ++j) {
						float degrees = cam->rotation.angles[j] * (180.0f / M_PI);
						nk_property_float(ctx, rot_names[j], -180.0f, &degrees, 180.0f, 1.0f, 0.1f);
						cam->rotation.angles[j] = degrees * (M_PI / 180.0f);
					}
					nk_layout_row_dynamic(ctx, 30, 2);
					nk_property_float(ctx, "#Near:", 1.0e-5f, &cam->near, cam->far, fabsf(cam->near) * 0.1f, fabsf(cam->near) * 5.0e-3f);
					nk_property_float(ctx, "#Far:", cam->near, &cam->far, 1.0e30f, fabsf(cam->far) * 0.1f, fabsf(cam->far) * 5.0e-3f);
					if (ortho) {
						nk_layout_row_dynamic(ctx, 30, 1);
						nk_property_float(ctx, "#Viewport height:", 0.0f, &cam->height, 1.0e30f, cam->height * 0.1f, cam->height * 5.0e-3f);
					}
					else {
						nk_layout_row_dynamic(ctx, 30, 1);
						float degrees = cam->fov * (180.0f / M_PI);
						nk_property_float(ctx, "#Vertical FOV:", 0.0f, &degrees, 180.0f, 1.0f, 0.1f);
						cam->fov = degrees * (M_PI / 180.0f);
					}
					nk_tree_pop(ctx);
				}
			}
			nk_tree_pop(ctx);
		}
		// Controls for volume rendering
		nk_default_skip(ctx);
		if (nk_tree_push(ctx, NK_TREE_TAB, "Volume properties", NK_MINIMIZED)) {
			// Define drop-down menu to select the volume interpolation
			// method
			nk_layout_row_dynamic(ctx, 30, 2);
			const char* interpolations[volume_interpolation_count];
			interpolations[volume_interpolation_nearest] = "Nearest neighbor";
			interpolations[volume_interpolation_linear] = "Trilinear";
			interpolations[volume_interpolation_linear_stochastic] = "Trilinear (stochastic)";
			scene_spec->volume_interpolation = nk_combo(ctx, interpolations, COUNT_OF(interpolations), scene_spec->volume_interpolation, 30, combo_size);
			nk_label(ctx, "Interpolation", NK_TEXT_ALIGN_LEFT);
			// Define a way to control the extinction factor
			nk_layout_row_dynamic(ctx, 30, 1);
			nk_property_float(ctx, "Extinction factor:", 1.0e-34f, &scene_spec->extinction_factor, 1.0e38f, scene_spec->extinction_factor * 0.1f, scene_spec->extinction_factor * 1.0e-3f);
			// Define a way to control the Mie droplet size
			nk_layout_row_dynamic(ctx, 30, 1);
			nk_property_float(ctx, "Mie droplet size (microns):", 0.0, &scene_spec->droplet_size, 50.0f, 1.0e-2f, 1.0e-2f);
			// Define file selection for a volume file
			nk_layout_row_dynamic(ctx, 30, 1);
			const char* volumes[] = { "bunny_cloud", "intel_cloud", "disney_cloud", "explosion", "fire", "smoke", "smoke2" };
			if (file_choice_combo(ctx, &scene_spec->volume_file_path, "data/volume/%s.blob", volumes, COUNT_OF(volumes), combo_size))
				update->volume = true;
			nk_layout_row_dynamic(ctx, 30, 1);
			if (nk_button_label(ctx, "Reload volume"))
				update->volume = true;
			nk_tree_pop(ctx);
		}
		// Controls for the transfer function
		nk_default_skip(ctx);
		if (nk_tree_push(ctx, NK_TREE_TAB, "Transfer function", NK_MINIMIZED)) {
			// Define a drop-down menu to select how colors and extinctions in
			// the volume are defined
			nk_layout_row_dynamic(ctx, 30, 2);
			const char* volume_color_modes[volume_color_mode_count];
			volume_color_modes[volume_color_mode_constant] = "Constant";
			volume_color_modes[volume_color_mode_transfer_function] = "Transfer function";
			volume_color_modes[volume_color_mode_transfer_function_with_extinction] = "Transfer function with extinction";
			volume_color_modes[volume_color_mode_separate] = "Separate color";
			scene_spec->volume_color_mode = nk_combo(ctx, volume_color_modes, COUNT_OF(volume_color_modes), scene_spec->volume_color_mode, 30, combo_size);
			nk_label(ctx, "Volume color mode", NK_TEXT_ALIGN_LEFT);
			if (scene_spec->volume_color_mode == volume_color_mode_constant)
				// Control the constant volume color
				hdr_color_picker(ctx, &scene_spec->volume_color_constant, "Volume color");
			else if (scene_spec->volume_color_mode == volume_color_mode_transfer_function || scene_spec->volume_color_mode == volume_color_mode_transfer_function_with_extinction) {
				nk_layout_row_dynamic(ctx, 30, 1);
				nk_bool interpolate = (nk_bool) scene_spec->interpolate_transfer_function;
				nk_checkbox_label(ctx, "Interpolate transfer function", &interpolate);
				scene_spec->interpolate_transfer_function = interpolate;
				// The value in the volume texture that maps to the
				// left/rightmost value in the transfer function
				nk_layout_row_dynamic(ctx, 30, 1);
				nk_property_float(ctx, "Transfer min. input:", -1.0e38f, &scene_spec->transfer_min_input, 1.0e38f, 0.1f, 1.0e-3f);
				nk_property_float(ctx, "Transfer max. input:", 1.0e-34f, &scene_spec->transfer_max_input, 1.0e38f, scene_spec->transfer_max_input * 0.1f, scene_spec->transfer_max_input * 1.0e-3f);
				// The path of the transfer function to be used
				nk_layout_row_dynamic(ctx, 30, 1);
				const char* transfer_functions[] = { "Accent", "Accent_r", "Blues", "Blues_r", "BrBG", "BrBG_r", "BuGn", "BuGn_r", "BuPu", "BuPu_r", "CMRmap", "CMRmap_r", "Dark2", "Dark2_r", "GnBu", "GnBu_r", "Grays", "Greens", "Greens_r", "Greys", "Greys_r", "OrRd", "OrRd_r", "Oranges", "Oranges_r", "PRGn", "PRGn_r", "Paired", "Paired_r", "Pastel1", "Pastel1_r", "Pastel2", "Pastel2_r", "PiYG", "PiYG_r", "PuBu", "PuBuGn", "PuBuGn_r", "PuBu_r", "PuOr", "PuOr_r", "PuRd", "PuRd_r", "Purples", "Purples_r", "RdBu", "RdBu_r", "RdGy", "RdGy_r", "RdPu", "RdPu_r", "RdYlBu", "RdYlBu_r", "RdYlGn", "RdYlGn_r", "Reds", "Reds_r", "Set1", "Set1_r", "Set2", "Set2_r", "Set3", "Set3_r", "Spectral", "Spectral_r", "Wistia", "Wistia_r", "YlGn", "YlGnBu", "YlGnBu_r", "YlGn_r", "YlOrBr", "YlOrBr_r", "YlOrRd", "YlOrRd_r", "active", "afmhot", "afmhot_r", "autumn", "autumn_r", "binary", "binary_r", "bone", "bone_r", "brg", "brg_r", "bwr", "bwr_r", "cividis", "cividis_r", "cool", "cool_r", "coolwarm", "coolwarm_r", "copper", "copper_r", "csf_grey_white", "cubehelix", "cubehelix_r", "flag", "flag_r", "gimp_colorful", "gist_earth", "gist_earth_r", "gist_gray", "gist_gray_r", "gist_grey", "gist_heat", "gist_heat_r", "gist_ncar", "gist_ncar_r", "gist_rainbow", "gist_rainbow_r", "gist_stern", "gist_stern_r", "gist_yarg", "gist_yarg_r", "gist_yerg", "gnuplot", "gnuplot2", "gnuplot2_r", "gnuplot_r", "gray", "gray_r", "grey", "hot", "hot_r", "hsv", "hsv_r", "inferno", "inferno_r", "jet", "jet_r", "magma", "magma_r", "narrow_window", "nipy_spectral", "nipy_spectral_r", "noise", "ocean", "ocean_r", "pink", "pink_r", "plasma", "plasma_r", "prism", "prism_r", "rainbow", "rainbow_r", "seismic", "seismic_r", "spring", "spring_r", "summer", "summer_r", "tab10", "tab10_r", "tab20", "tab20_r", "tab20b", "tab20b_r", "tab20c", "tab20c_r", "terrain", "terrain_r", "turbo", "turbo_r", "twilight", "twilight_r", "twilight_shifted", "twilight_shifted_r", "viridis", "viridis_r", "winter", "winter_r" };
				if (file_choice_combo(ctx, &scene_spec->transfer_function_path, "data/transfer_function/%s.png", transfer_functions, COUNT_OF(transfer_functions), combo_size))
					update->transfer_function = true;
				// A button to reload the transfer function
				nk_layout_row_dynamic(ctx, 30, 1);
				if (nk_button_label(ctx, "Reload transfer function"))
					update->transfer_function = true;
			}
			nk_tree_pop(ctx);
		}
		nk_default_skip(ctx);
		if (nk_tree_push(ctx, NK_TREE_TAB, "Render settings", NK_MINIMIZED)) {
			// The sample count
			nk_layout_row_dynamic(ctx, 30, 1);
			nk_property_int(ctx, "Sample count:", 1, (int*) &render_settings->sample_count, 128, 1, 1);
			// Whether primary rays get jittered for antialiasing
			nk_layout_row_dynamic(ctx, 30, 1);
			nk_bool jitter_primary_rays = render_settings->jitter_primary_rays;
			nk_checkbox_label(ctx, "Jitter primary rays (antialiasing)", &jitter_primary_rays);
			render_settings->jitter_primary_rays = jitter_primary_rays;
			// Define a control for the brick size
			nk_layout_row_dynamic(ctx, 30, 1);
			nk_property_int(ctx, "Brick size:", 1, (int*) &render_settings->brick_size, 32, 1, 0.01);
			// Define a drop-down menu to select the grid compression
			nk_layout_row_dynamic(ctx, 30, 2);
			const char* grid_compressions[grid_compression_count];
			grid_compressions[grid_compression_float_16] = "16-bit floats";
			grid_compressions[grid_compression_bc4] = "BC4 (4 bits per brick)";
			render_settings->grid_compression = nk_combo(ctx, grid_compressions, COUNT_OF(grid_compressions), render_settings->grid_compression, 30, combo_size);
			nk_label(ctx, "Grid compression", NK_TEXT_ALIGN_LEFT);
			// Define a drop-down menu to select the distance sampler
			nk_layout_row_dynamic(ctx, 30, 2);
			const char* distance_samplers[distance_sampler_count];
			distance_samplers[distance_sampler_ray_marching] = "Ray marching";
			distance_samplers[distance_sampler_regular_tracking] = "Regular tracking (nearest neighbor only)";
			distance_samplers[distance_sampler_delta_tracking] = "Delta tracking";
			render_settings->distance_sampler = nk_combo(ctx, distance_samplers, COUNT_OF(distance_samplers), render_settings->distance_sampler, 30, combo_size);
			nk_label(ctx, "Distance sampler", NK_TEXT_ALIGN_LEFT);
			// Define a drop-down menu to select the optical depth estimator
			bool using_optical_depth_estimator= false;
			switch (render_settings->transmittance_estimator) {
				case transmittance_estimator_unbiased_ray_marching:
				case transmittance_estimator_biased_ray_marching:
				case transmittance_estimator_jackknife:
					using_optical_depth_estimator = true;
				default:
					break;
			}
			switch (render_settings->mis_weight_estimator) {
				case mis_weight_estimator_unbiased_ray_marching:
				case mis_weight_estimator_biased_ray_marching:
				case mis_weight_estimator_jackknife:
					using_optical_depth_estimator = true;
				default:
					break;
			}
			if (using_optical_depth_estimator) {
				nk_layout_row_dynamic(ctx, 30, 2);
				const char* optical_depth_estimators[optical_depth_estimator_count];
				optical_depth_estimators[optical_depth_estimator_equidistant] = "Equidistant ray marching";
				optical_depth_estimators[optical_depth_estimator_kettunen] = "Adaptive ray marching [Kettunen et al. 2021]";
				optical_depth_estimators[optical_depth_estimator_mean_sampling] = "Mean ray marching (ours)";
				optical_depth_estimators[optical_depth_estimator_mean_sampling_fixed_count] = "Mean ray marching, fixed sample count (ours)";
				optical_depth_estimators[optical_depth_estimator_variance_aware] = "Variance aware ray marching (ours)";
				optical_depth_estimators[optical_depth_estimator_variance_aware_fixed_count] = "Variance aware, fixed sample count (ours)";
				render_settings->optical_depth_estimator = nk_combo(ctx, optical_depth_estimators, COUNT_OF(optical_depth_estimators), render_settings->optical_depth_estimator, 30, combo_size);
				nk_label(ctx, "Optical depth estimator", NK_TEXT_ALIGN_LEFT);
				// A toggle for uniform jittered sampling
				nk_layout_row_dynamic(ctx, 30, 1);
				nk_bool uniform_jittered = render_settings->uniform_jittered;
				nk_checkbox_label(ctx, "Uniform jittered", &uniform_jittered);
				render_settings->uniform_jittered = (bool) uniform_jittered;
			}
			// Define a drop-down menu to select the transmittance estimator
			nk_layout_row_dynamic(ctx, 30, 2);
			const char* transmittance_estimators[transmittance_estimator_count];
			transmittance_estimators[transmittance_estimator_dummy] = "Dummy (one)";
			transmittance_estimators[transmittance_estimator_unbiased_ray_marching] = "Unbiased ray marching";
			transmittance_estimators[transmittance_estimator_biased_ray_marching] = "Biased ray marching";
			transmittance_estimators[transmittance_estimator_jackknife] = "Jackknife ray marching";
			transmittance_estimators[transmittance_estimator_regular_tracking] = "Regular tracking (nearest neighbor only)";
			transmittance_estimators[transmittance_estimator_track_length] = "Track length";
			transmittance_estimators[transmittance_estimator_ratio_tracking] = "Ratio tracking";
			render_settings->transmittance_estimator = nk_combo(ctx, transmittance_estimators, COUNT_OF(transmittance_estimators), render_settings->transmittance_estimator, 30, combo_size);
			nk_label(ctx, "Transmittance estimator", NK_TEXT_ALIGN_LEFT);
			// Define a drop-down menu to select the MIS weight estimator
			nk_layout_row_dynamic(ctx, 30, 2);
			const char* mis_weight_estimators[mis_weight_estimator_count];
			mis_weight_estimators[mis_weight_estimator_dummy] = "Dummy";
			mis_weight_estimators[mis_weight_estimator_unbiased_ray_marching] = "Unbiased ray marching";
			mis_weight_estimators[mis_weight_estimator_biased_ray_marching] = "Biased ray marching";
			mis_weight_estimators[mis_weight_estimator_jackknife] = "Jackknife ray marching";
			mis_weight_estimators[mis_weight_estimator_regular_tracking] = "Regular tracking (nearest neighbor only)";
			render_settings->mis_weight_estimator = nk_combo(ctx, mis_weight_estimators, COUNT_OF(mis_weight_estimators), render_settings->mis_weight_estimator, 30, combo_size);
			nk_label(ctx, "MIS weight estimator", NK_TEXT_ALIGN_LEFT);
			// Define a drop-down menu to select which sampling strategies are
			// used and how they are combined
			nk_layout_row_dynamic(ctx, 30, 2);
			const char* sampling_strategies[sampling_strategies_count];
			sampling_strategies[sampling_strategies_light] = "Light only";
			sampling_strategies[sampling_strategies_transmittance] = "Transmittance-based only";
			sampling_strategies[sampling_strategies_light_transmittance_mis] = "Light+transmittance MIS";
			sampling_strategies[sampling_strategies_light_transmittance_approximate_mis] = "Light+transmittance approximate MIS";
			sampling_strategies[sampling_strategies_light_transmittance_miller_mis] = "Light+transmittance Miller et al. [2019] MIS";
			render_settings->sampling_strategies = nk_combo(ctx, sampling_strategies, COUNT_OF(sampling_strategies), render_settings->sampling_strategies, 30, combo_size);
			nk_label(ctx, "Sampling strategies", NK_TEXT_ALIGN_LEFT);
			// Selection of which quantity should be displayed
			nk_layout_row_dynamic(ctx, 30, 2);
			const char* displayed_quantity[displayed_quantity_count];
			displayed_quantity[displayed_quantity_radiance] = "Radiance";
			displayed_quantity[displayed_quantity_luminance_stats] = "Luminance statistics";
			displayed_quantity[displayed_quantity_transmittance] = "Transmittance";
			displayed_quantity[displayed_quantity_transmittance_stats] = "Transmittance statistics";
			displayed_quantity[displayed_quantity_transmittance_sign] = "Transmittance and sign";
			displayed_quantity[displayed_quantity_mis_weight] = "Generalized MIS weight";
			displayed_quantity[displayed_quantity_mis_weight_stats] = "Generalized MIS weight statistics";
			render_settings->displayed_quantity = nk_combo(ctx, displayed_quantity, COUNT_OF(displayed_quantity), render_settings->displayed_quantity, 30, combo_size);
			nk_label(ctx, "Displayed quantity", NK_TEXT_ALIGN_LEFT);
			// The sample count for ray marching
			if (render_settings->distance_sampler == distance_sampler_ray_marching || render_settings->optical_depth_estimator == optical_depth_estimator_equidistant || render_settings->optical_depth_estimator == optical_depth_estimator_mean_sampling_fixed_count || render_settings->optical_depth_estimator == optical_depth_estimator_variance_aware_fixed_count) {
				nk_layout_row_dynamic(ctx, 30, 1);
				nk_property_int(ctx, "Ray march sample count:", 1, (int*) &render_settings->ray_march_sample_count, 1024, 1, 1);
			}
			// The sample count multiplier for techniques of Kettunen et al.
			if (render_settings->optical_depth_estimator == optical_depth_estimator_kettunen) {
				nk_layout_row_dynamic(ctx, 30, 1);
				nk_property_float(ctx, "Sample count multiplier:", 0.01f, &render_settings->kettunen_sample_multiplier, 100.0f, 0.1f, 0.001f);
			}
			// The parameter that controls the sample count for importance
			// sampling strategies without fixed sample count
			switch (render_settings->optical_depth_estimator) {
				case optical_depth_estimator_mean_sampling:
				case optical_depth_estimator_variance_aware:
					nk_layout_row_dynamic(ctx, 30, 1);
					nk_property_float(ctx, "CDF per sample:", 0.01f, &render_settings->optical_depth_cdf_step, 10.0f, 0.1f, 0.001f);
					break;
				default: break;
			}
			// Controls for MIS weights
			if (render_settings->displayed_quantity == displayed_quantity_mis_weight || render_settings->displayed_quantity == displayed_quantity_mis_weight_stats) {
				nk_layout_row_dynamic(ctx, 30, 2);
				nk_property_float(ctx, "MIS u0", -10.0f, &render_settings->mis_weight_numerator[0], 10.0f, 0.1f, 1.0e-3f);
				nk_property_float(ctx, "MIS u1", -10.0f, &render_settings->mis_weight_numerator[1], 10.0f, 0.1f, 1.0e-3f);
				nk_layout_row_dynamic(ctx, 30, 2);
				nk_property_float(ctx, "MIS v0", -10.0f, &render_settings->mis_weight_denominator[0], 10.0f, 0.1f, 1.0e-3f);
				nk_property_float(ctx, "MIS v1", -10.0f, &render_settings->mis_weight_denominator[1], 10.0f, 0.1f, 1.0e-3f);
			}
			nk_tree_pop(ctx);
		}
		// Ambient light
		nk_default_skip(ctx);
		if (nk_tree_push(ctx, NK_TREE_TAB, "Ambient light", NK_MINIMIZED)) {
			if (nk_checkbox_label(ctx, "Use ambient light", (nk_bool*) &scene_spec->use_ambient_light))
				update->scene_subpass = true;
			nk_layout_row_dynamic(ctx, 30, 1);
			const char* light_probes[] = { "lakeside_night", "lakeside_sunrise", "leadenhall_market", "rosendal_plains_2", "rustig_koppie_puresky", "small_empty_room_3", "studio_small_06", "white" };
			if (file_choice_combo(ctx, &scene_spec->light_probe_path, "data/light_probe/%s_1k.hdr", light_probes, COUNT_OF(light_probes), combo_size))
				update->light_probe = true;
			nk_layout_row_dynamic(ctx, 30, 1);
			nk_property_float(ctx, "Ambient brightness:", 1.0e-34f, &scene_spec->ambient_radiance.linear_factor, 1.0e38f, scene_spec->ambient_radiance.linear_factor * 0.1f, scene_spec->ambient_radiance.linear_factor * 5.0e-3f);
			nk_layout_row_dynamic(ctx, 30, 1);
			nk_property_float(ctx, "Ambient lighting factor:", 0.0f, &scene_spec->ambient_lighting_factor, 1.0e7f, 1.0f, 1.0e-3f);
			nk_tree_pop(ctx);
		}
		nk_default_skip(ctx);
		if (nk_tree_push(ctx, NK_TREE_TAB, "Point lights", NK_MINIMIZED)) {
			for (uint64_t i = 0; i < scene_spec->point_light_count; ++i) {
				point_light_t* light = &scene_spec->point_lights[i];
				nk_layout_row_dynamic(ctx, 30, 3);
				nk_property_float(ctx, "#x:", -1.0e30f, &light->pos[0], 1.0e30f, 1.0e-1f, 2.0e-2f);
				nk_property_float(ctx, "#y:", -1.0e30f, &light->pos[1], 1.0e30f, 1.0e-1f, 2.0e-2f);
				nk_property_float(ctx, "#z:", -1.0e30f, &light->pos[2], 1.0e30f, 1.0e-1f, 2.0e-2f);
				hdr_color_picker(ctx, &light->power, "Power");
				nk_layout_row_dynamic(ctx, 30, 2);
				if (nk_button_label(ctx, "Move to camera"))
					memcpy(light->pos, scene_spec->cameras[0].position, sizeof(light->pos));
				if (nk_button_label(ctx, "Delete")) {
					for (uint64_t j = i; j + 1 < scene_spec->point_light_count; ++j)
						scene_spec->point_lights[j] = scene_spec->point_lights[j + 1];
					--scene_spec->point_light_count;
				}
				nk_default_skip(ctx);
			}
			nk_layout_row_dynamic(ctx, 30, 2);
			if (nk_button_label(ctx, "Add")) {
				point_light_t* new_lights = malloc((scene_spec->point_light_count + 1) * sizeof(point_light_t));
				memcpy(new_lights, scene_spec->point_lights, scene_spec->point_light_count * sizeof(point_light_t));
				new_lights[scene_spec->point_light_count] = (point_light_t) {
					.pos = { 0.0f, 0.0f, 0.0f },
					.power = { .linear_factor = 1.0f, .srgb_color = { 1.0f, 1.0f, 1.0f } },
				};
				free(scene_spec->point_lights);
				scene_spec->point_lights = new_lights;
				++scene_spec->point_light_count;
			}
			if (nk_button_label(ctx, "Delete all")) {
				free(scene_spec->point_lights);
				scene_spec->point_lights = NULL;
				scene_spec->point_light_count = 0;
			}
			nk_tree_pop(ctx);
		}
		// Buttons for quicksave and quickload
		nk_default_skip(ctx);
		nk_layout_row_dynamic(ctx, 30, 2);
		if (nk_button_label(ctx, "Quicksave"))
			quicksave(scene_spec, NULL);
		if (nk_button_label(ctx, "Quickload"))
			quickload(scene_spec, update, NULL);
		#ifndef NDEBUG
		nk_default_skip(ctx);
		if (nk_tree_push(ctx, NK_TREE_TAB, "Shader development", NK_MINIMIZED)) {
			// Sliders for numbers to use for any purpose in shaders
			nk_layout_row_dynamic(ctx, 30, 1);
			const char* param_labels[] = { "Param. 0:", "Param. 1:", "Param. 2:", "Param. 3:" };
			for (uint32_t i = 0; i != COUNT_OF(scene_spec->params); ++i)
				nk_property_float(ctx, param_labels[i], -1.0e38f, &scene_spec->params[i], 1.0e38f, 1.0e-6f, 5.0e-4f);
			// Button for hot shader reload
			nk_layout_row_dynamic(ctx, 30, 1);
			if (nk_button_label(ctx, "Reload shaders"))
				update->scene_subpass = update->gui_subpass = true;
			nk_tree_pop(ctx);
		}
		#endif
	}
	nk_end(ctx);
	return disable_camera_controls;
}


//! Fills a command buffer for rendering a single frame
VkResult record_render_frame_commands(app_t* app, frame_workload_t* frame, uint32_t swapchain_image_index, uint32_t workload_index) {
	// Begin recording into the command buffer anew
	VkCommandBuffer cmd = frame->cmd;
	VkResult ret;
	if (ret = vkResetCommandBuffer(cmd, 0))
		return ret;
	VkCommandBufferBeginInfo cmd_begin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	if (ret = vkBeginCommandBuffer(cmd, &cmd_begin))
		return ret;
	// Reset the query pool
	vkCmdResetQueryPool(frame->cmd, frame->query_pool, 0, timestamp_index_count);
	// Update the staging constant buffer and the device-local version
	if (write_constant_buffer(&app->constant_buffers, app, workload_index))
		return 1;
	VkBufferCopy constant_copy = { .size = app->constant_buffers.buffer.buffers[0].request.buffer_info.size };
	vkCmdCopyBuffer(cmd, app->constant_buffers.staging.buffers[workload_index].buffer, app->constant_buffers.buffer.buffers[0].buffer, 1, &constant_copy);
	VkBufferMemoryBarrier constant_barrier = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.buffer = app->constant_buffers.buffer.buffers[0].buffer,
		.size = VK_WHOLE_SIZE,
		.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
	};
	// Update the staging GUI vertex buffer and the device-local version
	if (write_gui_geometry(&app->gui, &app->device, workload_index))
		return 1;
	VkBufferCopy gui_copy = { .size = app->gui.buffer.buffers[0].request.buffer_info.size };
	vkCmdCopyBuffer(cmd, app->gui.staging.buffers[workload_index].buffer, app->gui.buffer.buffers[0].buffer, 1, &gui_copy);
	VkBufferMemoryBarrier gui_barrier = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.buffer = app->gui.buffer.buffers[0].buffer,
		.size = VK_WHOLE_SIZE,
		.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
	};
	// Place a barrier before these buffers are used
	VkBufferMemoryBarrier buffer_barriers[] = { constant_barrier, gui_barrier };
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, NULL, COUNT_OF(buffer_barriers), buffer_barriers, 0, NULL);
	// Update the volume acceleration structures if necessary
	if ((app->volume_as.transfer_min_input == 0.0 && app->volume_as.transfer_max_input == 0.0)
	 || (app->scene_spec.volume_color_mode == volume_color_mode_transfer_function_with_extinction && (app->volume_as.transfer_min_input != app->scene_spec.transfer_min_input || app->volume_as.transfer_max_input != app->scene_spec.transfer_max_input)))
	{
		// Transition layouts for storage images
		VkImageMemoryBarrier* barriers = calloc(app->volume_as.grids.image_count + app->volume_as.shaded_volume.image_count, sizeof(VkImageMemoryBarrier));
		uint32_t barrier_count = 0;
		for (uint32_t j = 0; j != 2; ++j) {
			images_t* images = (j == 0) ? &app->volume_as.grids : &app->volume_as.shaded_volume;
			for (uint32_t i = 0; i != images->image_count; ++i)
				barriers[barrier_count++] = (VkImageMemoryBarrier) {
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
					.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					.newLayout = VK_IMAGE_LAYOUT_GENERAL,
					.image = images->images[i].image,
					.subresourceRange = (VkImageSubresourceRange) {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.layerCount = images->images[i].request.image_info.arrayLayers,
						.levelCount = images->images[i].request.image_info.mipLevels,
					},
				};
		}
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, barrier_count, barriers);
		free(barriers);
		// Run compute dispatches
		record_compute_graph_commands(cmd, &app->volume_as.workload);
		app->volume_as.transfer_min_input = app->scene_spec.transfer_min_input;
		app->volume_as.transfer_max_input = app->scene_spec.transfer_max_input;
	}
	// Begin the render pass
	VkClearValue clear_values[] = {
		{ .color = { .float32 = { 0.0f, 0.0f, 0.0f, 0.0f } } },
		{ .depthStencil = { .depth = 1.0f } },
		{ .color = { .float32 = { 0.0f, 0.0f, 0.0f, 1.0f } } },
	};
	VkRenderPassBeginInfo pass_begin = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = app->render_pass.render_pass,
		.framebuffer = app->render_pass.framebuffers[swapchain_image_index],
		.pClearValues = clear_values,
		.clearValueCount = COUNT_OF(clear_values),
		.renderArea = { .extent = app->swapchain.extent },
	};
	vkCmdBeginRenderPass(cmd, &pass_begin, VK_SUBPASS_CONTENTS_INLINE);
	// Render the scene with the requested number of samples
	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, frame->query_pool, timestamp_index_shading_begin);
	for (uint32_t i = 0; i != app->render_settings.sample_count; ++i) {
		if (app->render_targets.accum_frame_count == 0) {
			VkClearAttachment clear_attachment = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.colorAttachment = 0,
				.clearValue = clear_values[2],
			};
			VkClearRect clear_rect = { .baseArrayLayer = 0, .layerCount = 1, .rect = { .extent = app->swapchain.extent }, };
			vkCmdClearAttachments(cmd, 1, &clear_attachment, 1, &clear_rect);
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->scene_subpass.pipeline_discard);
		}
		else
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->scene_subpass.pipeline_accum);
		++app->render_targets.accum_frame_count;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->scene_subpass.descriptor_set.pipeline_layout, 0, 1, app->scene_subpass.descriptor_set.descriptor_sets, 0, NULL);
		// We repurpose the instance index to communicate the sample index
		// within the frame (i) to the shader
		vkCmdDraw(cmd, 3, 1, 0, i);
	}
	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, frame->query_pool, timestamp_index_shading_end);
	// Begin the next subpass and perform tonemapping
	vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->tonemap_subpass.pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->tonemap_subpass.descriptor_set.pipeline_layout, 0, 1, app->tonemap_subpass.descriptor_set.descriptor_sets, 0, NULL);
	vkCmdDraw(cmd, 3, 1, 0, 0);
	// Render the GUI
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->gui_subpass.pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->gui_subpass.descriptor_set.pipeline_layout, 0, 1, app->gui_subpass.descriptor_set.descriptor_sets, 0, NULL);
	VkDeviceSize gui_offset = 0;
	vkCmdBindVertexBuffers(cmd, 0, 1, &app->gui.buffer.buffers[0].buffer, &gui_offset);
	vkCmdDraw(cmd, 3 * app->gui.used_triangle_counts[workload_index], 1, 0, 0);
	// End the render pass
	vkCmdEndRenderPass(cmd);
	// End recording
	if (ret = vkEndCommandBuffer(cmd))
		return ret;
	return 0;
}


VkResult render_frame(app_t* app, app_update_t* update) {
	// Figure out what workload to use and which one was used in the previous
	// frame (if any)
	uint32_t workload_index = (uint32_t) (app->frame_workloads.frame_index % FRAME_IN_FLIGHT_COUNT);
	frame_workload_t* frame = &app->frame_workloads.frames_in_flight[workload_index];
	frame_workload_t* prev_frame = NULL;
	if (app->frame_workloads.frame_index > 0)
		prev_frame = &app->frame_workloads.frames_in_flight[(workload_index + FRAME_IN_FLIGHT_COUNT - 1) % FRAME_IN_FLIGHT_COUNT];
	// Wait for the workload to be ready for reuse. This is the point of
	// synchronization between host and GPU.
	VkResult ret = 0;
	device_t* device = &app->device;
	if ((ret = vkWaitForFences(device->device, 1, &frame->frame_finished, VK_TRUE, UINT64_MAX))
	 || (ret = vkResetFences(device->device, 1, &frame->frame_finished)))
	{
		printf("Failed to wait for a fence or to reset it. Vulkan error code %d.\n", ret);
		return ret;
	}
	// Read queries of this workload from its last use
	if (app->frame_workloads.frame_index >= FRAME_IN_FLIGHT_COUNT)
		if (vkGetQueryPoolResults(device->device, frame->query_pool, 0, timestamp_index_count, sizeof(uint64_t) * timestamp_index_count, app->frame_workloads.timestamps, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT))
			printf("Failed to retrieve results of timestamp queries.\n");
	// Acquire an image from the swapchain
	uint32_t swapchain_image_index = 0;
	if (ret = vkAcquireNextImageKHR(device->device, app->swapchain.swapchain, UINT64_MAX, frame->image_acquired, NULL, &swapchain_image_index)) {
		printf("Failed to acquire an image from the swapchain. Vulkan error code %d.\n", ret);
		return ret;
	}
	// Record commands
	if (ret = record_render_frame_commands(app, frame, swapchain_image_index, workload_index)) {
		printf("Failed to record commands for rendering a frame.\n");
		return ret;
	}
	// Submit the command buffer, but wait on the previous frame workload to
	// finish using a semaphore. This way, we do not have to allocate all
	// resources that change each frame redundantly.
	VkPipelineStageFlags wait_dst_stage_masks[2] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	};
	VkSemaphore wait_semaphores[2] = { frame->image_acquired, NULL };
	if (prev_frame) wait_semaphores[1] = prev_frame->queue_finished[1];
	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pWaitDstStageMask = wait_dst_stage_masks,
		.pWaitSemaphores = wait_semaphores,
		.waitSemaphoreCount = (prev_frame != NULL) ? 2 : 1,
		.pSignalSemaphores = frame->queue_finished,
		.signalSemaphoreCount = COUNT_OF(frame->queue_finished),
		.pCommandBuffers = &frame->cmd,
		.commandBufferCount = 1,
	};
	if (ret = vkQueueSubmit(device->queue, 1, &submit_info, frame->frame_finished)) {
		printf("Failed to submit a command buffer to the queue for rendering a frame. Vulkan error code %d.\n", ret);
		return ret;
	}
	// Present the rendered frame
	VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pImageIndices = &swapchain_image_index,
		.pSwapchains = &app->swapchain.swapchain,
		.swapchainCount = 1,
		.pWaitSemaphores = &frame->queue_finished[0],
		.waitSemaphoreCount = 1,
	};
	if (ret = vkQueuePresentKHR(device->queue, &present_info)) {
		printf("Failed to present a frame through the swapchain. Vulkan error code %d.\n", ret);
		return ret;
	}
	// All done
	++app->frame_workloads.frame_index;
	return VK_SUCCESS;
}


int save_screenshot(const char* file_path, image_file_format_t format, const device_t* device, const render_targets_t* render_targets, const scene_spec_t* scene_spec) {
	screenshot_t scrot;
	memset(&scrot, 0, sizeof(scrot));
	// Create a staging image
	const image_t* src = &render_targets->targets.images[render_target_index_hdr_radiance];
	image_request_t request = { .image_info = src->request.image_info, };
	request.image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	request.image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	request.image_info.tiling = VK_IMAGE_TILING_LINEAR;
	if (create_images(&scrot.staging, device, &request, 1, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
		printf("Failed to create a staging image for a screenshot.\n");
		free_screenshot(&scrot, device);
		return 1;
	}
	image_t* dst = &scrot.staging.images[0];
	// Copy the render target into the staging image
	VkExtent3D extent = src->request.image_info.extent;
	copy_image_to_image_t copy = {
		.src = src->image,
		.dst = scrot.staging.images[0].image,
		.src_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.dst_old_layout = VK_IMAGE_LAYOUT_UNDEFINED,
		.dst_new_layout = VK_IMAGE_LAYOUT_GENERAL,
		.copy = {
			.srcSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.layerCount = 1,
			},
			.dstSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.layerCount = 1,
			},
			.extent = extent,
		}
	};
	if (copy_images(device, &copy, 1)) {
		printf("Failed to copy the off-screen render target to a staging image for taking a screenshot.\n");
		free_screenshot(&scrot, device);
		return 1;
	}
	// Figure out row pitches in multiples of the size of one entry
	VkImageSubresource subresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT };
	VkSubresourceLayout subresource_layout;
	vkGetImageSubresourceLayout(device->device, scrot.staging.images[0].image, &subresource, &subresource_layout);
	if (subresource_layout.rowPitch % sizeof(float)) {
		printf("Failed to take a screenshot because the row pitch %lu is not a multiple of sizeof(float).\n", subresource_layout.rowPitch);
		return 1;
	}
	uint32_t src_pitch = (uint32_t) (subresource_layout.rowPitch / sizeof(float));
	uint32_t dst_pitch = extent.width * 3;
	// Map memory of the staging image
	void* staged_data = NULL;
	if (vkMapMemory(device->device, scrot.staging.allocations[dst->allocation_index], dst->memory_offset, dst->memory_size, 0, &staged_data)) {
		printf("Failed to map memory of the staging image whilst taking a screenshot.\n");
		free_screenshot(&scrot, device);
		return 1;
	}
	const float* floats = (float*) staged_data;
	// For HDR images copy the floats
	uint32_t pixel_count = extent.width * extent.height;
	if (format == image_file_format_hdr || format == image_file_format_pfm) {
		scrot.hdr_copy = malloc(pixel_count * 3 * sizeof(float));
		for (uint32_t x = 0; x != extent.width; ++x)
			for (uint32_t y = 0; y != extent.height; ++y)
				for (uint32_t i = 0; i != 3; ++i)
					scrot.hdr_copy[i + x * 3 + y * dst_pitch] = floats[i + x * 4 + y * src_pitch] / ((float) render_targets->accum_frame_count);
		// Store the image
		if ((format == image_file_format_hdr && !stbi_write_hdr(file_path, (int) extent.width, (int) extent.height, 3, scrot.hdr_copy))
		 || (format == image_file_format_pfm && write_pfm(file_path, scrot.hdr_copy, extent.width, extent.height, 3)))
		{
			printf("Failed to save a HDR screenshot to %s. Please check path, permissions and available disk space.\n", file_path);
			free_screenshot(&scrot, device);
			return 1;
		}
	}
	else {
		// For LDR images apply tonemapping and convert float to uint8_t
		scrot.ldr_copy = malloc(pixel_count * 3 * sizeof(uint8_t));
		float factor = scene_spec->exposure / ((float) render_targets->accum_frame_count);
		for (uint32_t x = 0; x != extent.width; ++x) {
			for (uint32_t y = 0; y != extent.height; ++y) {
				const float* pixel = &floats[x * 4 + y * src_pitch];
				float color[3] = { pixel[0] * factor, pixel[1] * factor, pixel[2] * factor };
				float tonemapped[3];
				switch (scene_spec->tonemapper) {
					case tonemapper_op_aces:
						tonemapper_aces(tonemapped, color);
						break;
					case tonemapper_op_khronos_pbr_neutral:
						tonemapper_khronos_pbr_neutral(tonemapped, color);
						break;
					case tonemapper_op_clamp:
					default:
						memcpy(tonemapped, color, sizeof(color));
						break;
				}
				for (uint32_t i = 0; i != 3; ++i) {
					float rgb = tonemapped[i];
					rgb = (rgb >= 0.0f) ? rgb : 0.0f;
					rgb = (rgb <= 1.0f) ? rgb : 1.0f;
					float srgb = (rgb <= 0.0031308f) ? (12.92f * rgb) : (1.055f * powf(rgb, 1.0f / 2.4f) - 0.055f);
					uint8_t ldr = (uint8_t) (srgb * 255.0f + 0.5f);
					scrot.ldr_copy[i + x * 3 + y * dst_pitch] = ldr;
				}
			}
		}
		// Store the image
		if (format == image_file_format_jpg && !stbi_write_jpg(file_path, (int) extent.width, (int) extent.height, 3, scrot.ldr_copy, 90)
		 || format == image_file_format_png && !stbi_write_png(file_path, (int) extent.width, (int) extent.height, 3, scrot.ldr_copy, extent.width * 3 * sizeof(uint8_t)))
		{
			printf("Failed to save a screenshot to %s. Please check path, permissions and available disk space.\n", file_path);
			free_screenshot(&scrot, device);
			return 1;
		}
	}
	free_screenshot(&scrot, device);
	return 0;
}


void free_screenshot(screenshot_t* screenshot, const device_t* device) {
	free_images(&screenshot->staging, device);
	free(screenshot->ldr_copy);
	free(screenshot->hdr_copy);
	memset(screenshot, 0, sizeof(*screenshot));
}


int main(int argc, char** argv) {
	// Define default parameters and parse arguments
	app_params_t app_params = {
		.initial_window_extent = { 1920, 1080 },
		.v_sync = true,
		.slide_screenshots = true,
		.gui = true,
	};
	slideshow_t slideshow = { .slide_begin = 0 };
	for (int i = 1; i < argc; ++i) {
		const char* arg = argv[i];
		if (strcmp(arg, "-no_gui") == 0) app_params.gui = false;
		if (strcmp(arg, "-no_v_sync") == 0) app_params.v_sync = false;
		if (strcmp(arg, "-no_screenshots") == 0) app_params.slide_screenshots = false;
		if (arg[0] == '-' && arg[1] == 'b') sscanf(arg + 2, "%u", &slideshow.slide_begin);
		if (arg[0] == '-' && arg[1] == 'e') sscanf(arg + 2, "%u", &slideshow.slide_end);
		if (arg[0] == '-' && arg[1] == 'w') sscanf(arg + 2, "%u", &app_params.initial_window_extent.width);
		if (arg[0] == '-' && arg[1] == 'h') sscanf(arg + 2, "%u", &app_params.initial_window_extent.height);
	}
	// Create all application objects
	app_t app;
	if (create_app(&app, &app_params, &slideshow)) {
		free_app(&app);
		return 1;
	}
	// Main loop
	while (true) {
		app_update_t update;
		memset(&update, 0, sizeof(update));
		if (handle_user_input(&app, &update))
			break;
		// If an update is needed, the state of app objects is inconsistent
		// with application state. The frame might look odd, so we do not
		// render it.
		if (!update_needed(&update)) {
			VkResult ret;
			if (ret = render_frame(&app, &update)) {
				if (ret == VK_ERROR_OUT_OF_DATE_KHR || ret == VK_ERROR_SURFACE_LOST_KHR || ret == VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)
					update.swapchain = update.frame_workloads = true;
				else
					break;
			}
		}
		// Otherwise, we still let Nuklear write its geometry to avoid that it
		// ends up in an invalid state
		else {
			uint32_t workload_index = (uint32_t) (app.frame_workloads.frame_index % FRAME_IN_FLIGHT_COUNT);
			write_gui_geometry(&app.gui, &app.device, workload_index);
		}
		nk_clear(&app.gui.context);
		if (update_app(&app, &update, true))
			break;
	}
	// Clean up
	free_app(&app);
	return 0;
}
