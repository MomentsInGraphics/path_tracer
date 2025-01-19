#include "main.h"
#include "string_utilities.h"
#include "math_utilities.h"
#include "timer.h"
#include "stb_image_write.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>


int get_scene_file(scene_file_t scene_file, const char** scene_name, const char** scene_file_path, const char** texture_path, const char** light_path, const char** quicksave_path) {
	const char* name = NULL;
	const char* file = NULL;
	const char* textures = NULL;
	const char* lights = NULL;
	const char* save = NULL;
	switch (scene_file) {
		case scene_file_arcade:
			name = "Arcade";
			file = "data/Arcade.vks";
			textures = "data/Arcade_textures";
			lights = "data/Arcade.lights";
			save = "data/saves/Arcade/default.rt_save";
			break;
		case scene_file_attic:
			name = "Attic";
			file = "data/attic.vks";
			textures = "data/attic_textures";
			lights = "data/attic.lights";
			save = "data/saves/attic/default.rt_save";
			break;
		case scene_file_bistro_inside:
			name = "Bistro inside";
			file = "data/Bistro_inside.vks";
			textures = "data/Bistro_textures";
			lights = "data/Bistro_inside.lights";
			save = "data/saves/bistro/inside.rt_save";
			break;
		case scene_file_bistro_outside:
			name = "Bistro outside";
			file = "data/Bistro_outside.vks";
			textures = "data/Bistro_textures";
			lights = "data/Bistro_outside.lights";
			save = "data/saves/bistro/outside.rt_save";
			break;
		case scene_file_cornell_box:
			name = "Cornell box";
			file = "data/cornell_box.vks";
			textures = "data/cornell_box_textures";
			lights = "data/cornell_box.lights";
			save = "data/saves/cornell_box/default.rt_save";
			break;
		case scene_file_living_room_day:
			name = "Living room day";
			file = "data/living_room_day.vks";
			textures = "data/living_room_textures";
			lights = "data/living_room_day.lights";
			save = "data/saves/living_room/day.rt_save";
			break;
		case scene_file_living_room_night:
			name = "Living room night";
			file = "data/living_room_night.vks";
			textures = "data/living_room_textures";
			lights = "data/living_room_night.lights";
			save = "data/saves/living_room/night.rt_save";
			break;
		default:
			break;
	}
	if (scene_name) (*scene_name) = name;
	if (scene_file_path) (*scene_file_path) = file;
	if (texture_path) (*texture_path) = textures;
	if (light_path) (*light_path) = lights;
	if (quicksave_path) (*quicksave_path) = save;
	return !name || !file || !textures || !lights || !save;
}



int quicksave(const scene_spec_t* spec) {
	FILE* file = fopen("data/quicksave.rt_save", "wb");
	if (file) {
		fwrite(spec, sizeof(*spec), 1, file);
		printf("Quicksave.\n");
		fclose(file);
		return 0;
	}
	return 1;
}


int quickload(scene_spec_t* spec, app_update_t* update, const char* save_path) {
	if (save_path == NULL)
		save_path = "data/quicksave.rt_save";
	FILE* file = fopen(save_path, "rb");
	scene_spec_t old = *spec;
	if (file) {
		if (fread(spec, sizeof(*spec), 1, file) == 1) {
			printf("Quickload from %s.\n", save_path);
			get_scene_spec_updates(update, &old, spec);
			fclose(file);
			return 0;
		}
		fclose(file);
	}
	printf("Could not open the quicksave file %s.\n", save_path);
	return 1;
}


void get_scene_spec_updates(app_update_t* update, const scene_spec_t* old_spec, const scene_spec_t* new_spec) {
	if (old_spec->scene_file != new_spec->scene_file)
		update->lit_scene = update->scene_subpass = true;
	if (old_spec->tonemapper != new_spec->tonemapper)
		update->tonemap_subpass = true;
}


void init_scene_spec(scene_spec_t* spec) {
	app_update_t dummy;
	if (!quickload(spec, &dummy, NULL))
		return;
	camera_t cornell_box_camera = {
		.type = camera_type_first_person,
		.near = 0.01f,
		.far = 1.0e4f,
		.fov = M_PI * 0.4f,
		.height = 10.0f,
		.speed = 2.0f,
		.rotation.angles = { 1.570796f, 0.0f, 0.0f },
		.position = { 0.278f, 0.379f, 0.2744f },
	};
	scene_spec_t default_spec = {
		.camera = cornell_box_camera,
		.tonemapper = tonemapper_aces,
		.exposure = 1.0f,
		.sky_color = { 0.2f, 0.4f, 1.0f },
		.sky_strength = 0.0f,
		.emission_material_color = { 1.0f, 0.9f, 0.6f },
		.emission_material_strength = 20.0f,
		.scene_file = scene_file_cornell_box,
	};
	(*spec) = default_spec;
	printf("Using a default scene specification.\n");
}


void init_render_settings(render_settings_t* settings) {	
	render_settings_t default_settings = {
		.sampling_strategy = sampling_strategy_nee,
		.path_length = 4,
	};
	(*settings) = default_settings;
}


int create_window(GLFWwindow** window, const VkExtent2D* extent) {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	(*window) = glfwCreateWindow(extent->width, extent->height, "Path tracer", NULL, NULL);
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
	// Feed mouse input
	double x, y;
	glfwGetCursorPos(window, &x, &y);
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


//! \see constant_buffers_t
int create_constant_buffers(constant_buffers_t* constant_buffers, const device_t* device) {
	memset(constant_buffers, 0, sizeof(*constant_buffers));
	// Create one staging buffer per frame in flight
	buffer_request_t staging_request = {
		.buffer_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = sizeof(constants_t),
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
			.size = sizeof(constants_t),
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
	const camera_t* camera = &app->scene_spec.camera;
	constants_t cts = {
		.viewport_size = { (float) viewport.width, (float) viewport.height },
		.inv_viewport_size = { 1.0f / (float) viewport.width, 1.0f / (float) viewport.height },
		.camera_type = (int) camera->type,
		.exposure = app->scene_spec.exposure,
		.frame_index = app->scene_spec.frame_index,
		.accum_frame_count = app->render_targets.accum_frame_count + 1,
	};
	memcpy(cts.camera_pos, camera->position, sizeof(cts.camera_pos));
	float world_to_view[4 * 4];
	get_world_to_view_space(world_to_view, camera);
	cts.hemispherical_camera_normal[0] = world_to_view[2 * 4 + 0];
	cts.hemispherical_camera_normal[1] = world_to_view[2 * 4 + 1];
	cts.hemispherical_camera_normal[2] = world_to_view[2 * 4 + 2];
	const scene_t* scene = &app->lit_scene.scene;
	memcpy(cts.dequantization_factor, scene->header.dequantization_factor, sizeof(cts.dequantization_factor));
	memcpy(cts.dequantization_summand, scene->header.dequantization_summand, sizeof(cts.dequantization_summand));
	for (uint32_t i = 0; i != 3; ++i) {
		cts.sky_radiance[i] = app->scene_spec.sky_color[i] * app->scene_spec.sky_strength;
		cts.emission_material_radiance[i] = app->scene_spec.emission_material_color[i] * app->scene_spec.emission_material_strength;
	}
	memcpy(cts.params, app->scene_spec.params, sizeof(cts.params));
	memcpy(cts.spherical_lights, app->lit_scene.spherical_lights, sizeof(cts.spherical_lights));
	float aspect = ((float) app->swapchain.extent.width) / ((float) app->swapchain.extent.height);
	get_world_to_projection_space(cts.world_to_projection_space, &app->scene_spec.camera, aspect);
	invert_mat4(cts.projection_to_world_space, cts.world_to_projection_space);
	// Update the staging buffer
	memcpy((uint8_t*) constant_buffers->staging_data + constant_buffers->staging.buffers[buffer_index].memory_offset, &cts, sizeof(cts));
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


int create_lit_scene(lit_scene_t* lit_scene, const device_t* device, const scene_spec_t* scene_spec) {
	const char* scene_path;
	const char* textures_path;
	const char* lights_path;
	if (get_scene_file(scene_spec->scene_file, NULL, &scene_path, &textures_path, &lights_path, NULL)) {
		printf("Failed to load the scene, because the requested scene file is unknown.\n");
		return 1;
	}
	// Load the spherical light file
	FILE* file = fopen(lights_path, "rb");
	if (file) {
		fread(&lit_scene->spherical_light_count, sizeof(uint32_t), 1, file);
		if (lit_scene->spherical_light_count > MAX_SPHERICAL_LIGHT_COUNT) {
			printf("Warning: At most %u spherical lights are supported but %u were found in the file. Dropping some of them.\n", MAX_SPHERICAL_LIGHT_COUNT, lit_scene->spherical_light_count);
			lit_scene->spherical_light_count = MAX_SPHERICAL_LIGHT_COUNT;
		}
		lit_scene->spherical_light_count = (uint32_t) fread(&lit_scene->spherical_lights, sizeof(float) * 4, lit_scene->spherical_light_count, file);
		printf("Loaded %u spherical lights from %s.\n", lit_scene->spherical_light_count, lights_path);
		fclose(file);
	}
	// Load the scene
	int result = load_scene(&lit_scene->scene, device, scene_path, textures_path);
	if (!result)
		printf("Loaded %lu triangles and %lu materials from %s.\n", lit_scene->scene.header.triangle_count, lit_scene->scene.header.material_count, scene_path);
	return result;
}


void free_lit_scene(lit_scene_t* lit_scene, const device_t* device) {
	free_scene(&lit_scene->scene, device);
	memset(lit_scene, 0, sizeof(*lit_scene));
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


int create_scene_subpass(scene_subpass_t* subpass, const device_t* device, const scene_spec_t* scene_spec, const render_settings_t* render_settings, const swapchain_t* swapchain, const constant_buffers_t* constant_buffers, const lit_scene_t* lit_scene, const render_pass_t* render_pass) {
	memset(subpass, 0, sizeof(*subpass));
	const scene_t* scene = &lit_scene->scene;
	// Create a sampler for material textures
	VkSamplerCreateInfo sampler_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.magFilter = VK_FILTER_LINEAR,
		.anisotropyEnable = VK_TRUE,
		.maxAnisotropy = 16,
		.minLod = 0.0f,
		.maxLod = 3.4e38f,
	};
	if (vkCreateSampler(device->device, &sampler_info, NULL, &subpass->sampler)) {
		printf("Failed to create a sampler for material textures in the scene subpass.\n");
		free_scene_subpass(subpass, device);
		return 1;
	}
	// Create a descriptor set
	#define MESH_BINDING_START 3
	VkDescriptorSetLayoutBinding bindings[MESH_BINDING_START + mesh_buffer_type_count] = {
		// The constant buffer
		{ .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
		// All material textures
		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = material_texture_type_count * scene->header.material_count,
		},
		// The acceleration structure (BVH) containing all scene geometry
		{ .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR },
	};
	// The mesh buffers (made available to the fragment shader)
	for (uint32_t i = 0; i != mesh_buffer_type_count; ++i) {
		VkDescriptorSetLayoutBinding* binding = &bindings[MESH_BINDING_START + i];
		binding->binding = MESH_BINDING_START + i;
		binding->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
	}
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
	VkDescriptorImageInfo* image_infos = calloc(scene->textures.image_count, sizeof(VkDescriptorImageInfo));
	for (uint32_t i = 0; i != scene->textures.image_count; ++i) {
		image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_infos[i].imageView = scene->textures.images[i].view;
		image_infos[i].sampler = subpass->sampler;
	}
	VkWriteDescriptorSetAccelerationStructureKHR bvh_info = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
		.pAccelerationStructures = &scene->bvhs.bvhs[bvh_level_top],
		.accelerationStructureCount = 1,
	};
	VkWriteDescriptorSet writes[MESH_BINDING_START + mesh_buffer_type_count] = {
		{ .dstBinding = 0, .pBufferInfo = &constant_buffer_info, },
		{ .dstBinding = 1, .pImageInfo = image_infos, },
		{ .dstBinding = 2, .pNext = &bvh_info, },
	};
	for (uint32_t i = 0; i != mesh_buffer_type_count; ++i) {
		VkWriteDescriptorSet* write = &writes[MESH_BINDING_START + i];
		write->dstBinding = MESH_BINDING_START + i;
		write->pTexelBufferView = &scene->mesh_buffers.buffers[i].view;
	}
	complete_descriptor_set_writes(writes, COUNT_OF(writes), bindings, COUNT_OF(bindings), subpass->descriptor_set.descriptor_sets[0]);
	vkUpdateDescriptorSets(device->device, COUNT_OF(writes), writes, 0, NULL);
	free(image_infos);
	image_infos = NULL;
	// Compile the shaders and create the shader modules
	uint32_t emission_material_index = 0;
	for (uint32_t i = 0; i != scene->header.material_count; ++i)
		if (strcmp(scene->header.material_names[i], "_emission") == 0)
			emission_material_index = i;
	char* defines[] = {
		format_uint("MATERIAL_COUNT=%u", scene->header.material_count),
		format_uint("EMISSION_MATERIAL_INDEX=%u", emission_material_index),
		format_uint("SPHERICAL_LIGHT_COUNT=%u", lit_scene->spherical_light_count),
		format_uint("PATH_LENGTH=%u", render_settings->path_length),
		format_uint("SAMPLING_STRATEGY_SPHERICAL=%u", render_settings->sampling_strategy == sampling_strategy_spherical),
		format_uint("SAMPLING_STRATEGY_PSA=%u", render_settings->sampling_strategy == sampling_strategy_psa),
		format_uint("SAMPLING_STRATEGY_BRDF=%u", render_settings->sampling_strategy == sampling_strategy_brdf),
		format_uint("SAMPLING_STRATEGY_NEE=%u", render_settings->sampling_strategy == sampling_strategy_nee),
	};
	shader_compilation_request_t vert_request = {
		.shader_path = "src/shaders/pathtrace.vert.glsl",
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.entry_point = "main",
		.defines = defines,
		.define_count = COUNT_OF(defines),
	};
	shader_compilation_request_t frag_request = {
		.shader_path = "src/shaders/pathtrace.frag.glsl",
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
	VkVertexInputBindingDescription vertex_binding = { .stride = sizeof(float) * 2 };
	VkVertexInputAttributeDescription vertex_attributes[] = {
		{ .location = 0, .offset = 0, .format = VK_FORMAT_R32G32_SFLOAT },
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
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
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
	if (subpass->sampler) vkDestroySampler(device->device, subpass->sampler, NULL);
	memset(subpass, 0, sizeof(*subpass));
}


//! Callback for fill_buffer() to write a screen-filling triangle
void write_screen_filling_triangle(void* buffer_data, uint32_t buffer_index, VkDeviceSize buffer_size, const void* context) {
	float vertex_buffer[] = { -1.5f, -1.5f,  -1.5f, 5.0f,  5.0f, -1.5f };
	memcpy(buffer_data, vertex_buffer, sizeof(vertex_buffer));
}


int create_tonemap_subpass(tonemap_subpass_t* subpass, const device_t* device, const render_targets_t* render_targets, const constant_buffers_t* constant_buffers, const render_pass_t* render_pass, const scene_spec_t* scene_spec) {
	memset(subpass, 0, sizeof(*subpass));
	// Create a buffer containing a single triangle
	buffer_request_t triangle_request = {
		.buffer_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = sizeof(float) * 2 * 3,
			.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		},
	};
	if (create_buffers(&subpass->triangle_buffer, device, &triangle_request, 1, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1)) {
		printf("Failed to create a vertex buffer for a screen-filling triangle.\n");
		free_tonemap_subpass(subpass, device);
		return 1;
	}
	if (fill_buffers(&subpass->triangle_buffer, device, &write_screen_filling_triangle, NULL)) {
		printf("Failed to write a vertex buffer for a screen-filling triangle to the GPU.\n");
		free_tonemap_subpass(subpass, device);
		return 1;
	}
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
		format_uint("TONEMAPPER_CLAMP=%u", scene_spec->tonemapper == tonemapper_clamp),
		format_uint("TONEMAPPER_ACES=%u", scene_spec->tonemapper == tonemapper_aces),
		format_uint("TONEMAPPER_KHRONOS_PBR_NEUTRAL=%u", scene_spec->tonemapper == tonemapper_khronos_pbr_neutral),
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
	VkVertexInputBindingDescription vertex_binding = { .stride = sizeof(float) * 2 };
	VkVertexInputAttributeDescription vertex_attributes[] = {
		{ .location = 0, .offset = 0, .format = VK_FORMAT_R32G32_SFLOAT },
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
	free_buffers(&subpass->triangle_buffer, device);
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
		{ .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },
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
	if (compile_and_create_shader_module(&subpass->vert_shader, device, &vert_request, true)
	 || compile_and_create_shader_module(&subpass->frag_shader, device, &frag_request, true)
	) {
		printf("Failed to compile one of the shaders for the GUI subpass.\n");
		free_gui_subpass(subpass, device);
		return 1;
	}
	// Define the graphics pipeline state
	VkVertexInputBindingDescription vertex_binding = { .stride = sizeof(gui_vertex_t) };
	VkVertexInputAttributeDescription vertex_attributes[] = {
		{ .location = 0, .offset = NK_OFFSETOF(gui_vertex_t, pos), .format = VK_FORMAT_R32G32_SFLOAT },
		{ .location = 1, .offset = NK_OFFSETOF(gui_vertex_t, tex_coord), .format = VK_FORMAT_R32G32_SFLOAT },
		{ .location = 2, .offset = NK_OFFSETOF(gui_vertex_t, color), .format = VK_FORMAT_R8G8B8A8_UNORM },
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
	if (quickload(scene_spec, update, slideshow->slides[slideshow->slide_current].quicksave))
		return 1;
	(*render_settings) = slideshow->slides[slideshow->slide_current].render_settings;
	if (memcmp(&old_settings, render_settings, sizeof(old_settings)) != 0)
		update->scene_subpass = true;
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
	app_update_t up = *update;
	if (!update_needed(update))
		return 0;
	// Reset accmulation
	app->render_targets.accum_frame_count = 0;
	// Let all GPU work finish before destroying objects it may rely on
	if (app->device.device)
		vkDeviceWaitIdle(app->device.device);
	// Propagate dependencies
	for (uint32_t i = 0; i != sizeof(app_update_t) / sizeof(bool); ++i) {
		up.gui |= up.device | up.window;		
		up.swapchain |= up.device | up.window;
		up.render_targets |= up.device | up.swapchain;
		up.constant_buffers |= up.device;
		up.lit_scene |= up.device;
		up.render_pass |= up.device | up.swapchain | up.render_targets;
		up.scene_subpass |= up.device | up.swapchain | up.constant_buffers | up.lit_scene | up.render_pass;
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
	if (up.lit_scene) free_lit_scene(&app->lit_scene, &app->device);
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
	if (up.device && (ret = create_device(&app->device, "Path tracer", 0))
	 || up.window && (ret = create_window(&app->window, &app->params.initial_window_extent))
	 || up.gui && (ret = create_gui(&app->gui, &app->device, app->window))
	 || up.swapchain && (ret = create_swapchain(&app->swapchain, &app->device, app->window, app->params.v_sync))
	 || up.render_targets && (ret = create_render_targets(&app->render_targets, &app->device, &app->swapchain))
	 || up.constant_buffers && (ret = create_constant_buffers(&app->constant_buffers, &app->device))
	 || up.lit_scene && (ret = create_lit_scene(&app->lit_scene, &app->device, &app->scene_spec))
	 || up.render_pass && (ret = create_render_pass(&app->render_pass, &app->device, &app->swapchain, &app->render_targets))
	 || up.scene_subpass && (ret = create_scene_subpass(&app->scene_subpass, &app->device, &app->scene_spec, &app->render_settings, &app->swapchain, &app->constant_buffers, &app->lit_scene, &app->render_pass))
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
	init_scene_spec(&app->scene_spec);
	init_render_settings(&app->render_settings);
	app_update_t dummy;
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


int handle_user_input(app_t* app, app_update_t* update) {
	// Keep track of whether the scene specification changes to reset
	// accumulation
	scene_spec_t old_spec = app->scene_spec;
	// A new frame has begun. Record its time.
	record_frame_time();
	++app->scene_spec.frame_index;
	// Toggle display of the GUI
	if (key_pressed(app->window, GLFW_KEY_F1))
		app->params.gui = !app->params.gui;
	// Let the GUI respond to user input (and poll events)
	handle_gui_input(&app->gui, app->window);
	// Define the GUI
	if (app->params.gui)
		define_gui(&app->gui.context, &app->scene_spec, &app->render_settings, update, &app->render_targets, app->frame_workloads.timestamps, app->device.physical_device_properties.limits.timestampPeriod);
	// Use camera controls and update corresponding constants
	control_camera(&app->scene_spec.camera, app->window);
	// Quicksave and quickload
	if (key_pressed(app->window, GLFW_KEY_F2)) {
		app->params.v_sync = !app->params.v_sync;
		update->swapchain = true;
	}
	if (key_pressed(app->window, GLFW_KEY_F3))
		quicksave(&app->scene_spec);
	if (key_pressed(app->window, GLFW_KEY_F4))
		quickload(&app->scene_spec, update, NULL);
	// Hot shader reload
	if (key_pressed(app->window, GLFW_KEY_F5)) {
		update->scene_subpass = update->gui_subpass = true;
		app->render_targets.accum_frame_count = 0;
	}
	// Take screenshots
	if (key_pressed(app->window, GLFW_KEY_F10))
		save_screenshot("data/screenshot.hdr", image_file_format_hdr, &app->device, &app->render_targets, &app->scene_spec);
	if (key_pressed(app->window, GLFW_KEY_F11))
		save_screenshot("data/screenshot.png", image_file_format_png, &app->device, &app->render_targets, &app->scene_spec);
	if (key_pressed(app->window, GLFW_KEY_F12))
		save_screenshot("data/screenshot.jpg", image_file_format_jpg, &app->device, &app->render_targets, &app->scene_spec);
	// Slideshow controls
	bool terminate = false;
	if (app->slideshow.slide_begin < app->slideshow.slide_end) {
		uint32_t new_slide = app->slideshow.slide_current;
		if (app->params.slide_screenshots) {
			const slide_t* slide = &app->slideshow.slides[app->slideshow.slide_current];
			if (slide->screenshot_path && slide->screenshot_frame == app->render_targets.accum_frame_count) {
				save_screenshot(slide->screenshot_path, slide->screenshot_format, &app->device, &app->render_targets, &app->scene_spec);
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
	// Produce update due to scene changes
	get_scene_spec_updates(update, &old_spec, &app->scene_spec);
	// Reset accumulation if necessary
	old_spec.exposure = app->scene_spec.exposure;
	old_spec.frame_index = app->scene_spec.frame_index;
	if (memcmp(&old_spec, &app->scene_spec, sizeof(old_spec)))
		app->render_targets.accum_frame_count = 0;
	if (glfwGetKey(app->window, GLFW_KEY_F6) == GLFW_PRESS)
		app->render_targets.accum_frame_count = 0;
	// Check whether the application should terminate (escape or window closed)
	terminate |= glfwWindowShouldClose(app->window) || glfwGetKey(app->window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
	return terminate ? 1 : 0;
}


//! Utility for Nuklear to manipulate an RGB color
void color_picker(struct nk_context* ctx, float* rgb_color) {
	struct nk_colorf col = { .r = rgb_color[0], .g = rgb_color[1], .b = rgb_color[2], .a = 1.0f };
	if (nk_combo_begin_color(ctx, nk_rgb_cf(col), nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		col = nk_color_picker(ctx, col, NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		col.r = nk_propertyf(ctx, "#R:", 0, col.r, 1.0f, 0.01f, 0.005f);
		col.g = nk_propertyf(ctx, "#G:", 0, col.g, 1.0f, 0.01f, 0.005f);
		col.b = nk_propertyf(ctx, "#B:", 0, col.b, 1.0f, 0.01f, 0.005f);
		nk_combo_end(ctx);
	}
	memcpy(rgb_color, &col, sizeof(float) * 3);
}


void define_gui(struct nk_context* ctx, scene_spec_t* scene_spec, render_settings_t* render_settings, app_update_t* update, const render_targets_t* render_targets, uint64_t timestamps[timestamp_index_count], float timestamp_period) {
	struct nk_rect bounds = { .x = 20.0f, .y = 20.0f, .w = 400.0f, .h = 380.0f };
	if (nk_begin(ctx, "Path tracer", bounds, NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE)) {
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
		// Display the sample count
		nk_layout_row_dynamic(ctx, 30, 1);
		nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "Sample count: %u", render_targets->accum_frame_count);
		// Define a drop-down menu to select the scene
		nk_layout_row_dynamic(ctx, 30, 2);
		const char* scene_names[scene_file_count];
		for (uint32_t i = 0; i != scene_file_count; ++i)
			get_scene_file(i, &scene_names[i], NULL, NULL, NULL, NULL);
		scene_file_t new_scene_file = nk_combo(ctx, scene_names, COUNT_OF(scene_names), scene_spec->scene_file, 30, (struct nk_vec2) { .x = 200.0f, .y = 300.0f });
		nk_label(ctx, "Scene file", NK_TEXT_ALIGN_LEFT);
		if (scene_spec->scene_file != new_scene_file) {
			const char* save_path = NULL;
			get_scene_file(new_scene_file, NULL, NULL, NULL, NULL, &save_path);
			quickload(scene_spec, update, save_path);
		}
		scene_spec->scene_file = new_scene_file;
		// Define a drop-down menu to select the tonemapping operator
		nk_layout_row_dynamic(ctx, 30, 2);
		const char* tonemappers[tonemapper_count];
		tonemappers[tonemapper_clamp] = "Clamp";
		tonemappers[tonemapper_aces] = "ACES";
		tonemappers[tonemapper_khronos_pbr_neutral] = "Khronos PBR neutral";
		tonemapper_t new_tonemapper = nk_combo(ctx, tonemappers, COUNT_OF(tonemappers), scene_spec->tonemapper, 30, (struct nk_vec2) { .x = 240.0f, .y = 180.0f });
		nk_label(ctx, "Tonemapper", NK_TEXT_ALIGN_LEFT);
		if (scene_spec->tonemapper != new_tonemapper)
			update->tonemap_subpass = true;
		scene_spec->tonemapper = new_tonemapper;
		// Illumination settings
		nk_layout_row_dynamic(ctx, 30, 1);
		nk_property_float(ctx, "Sky emission:", 0.0f, &scene_spec->sky_strength, 40.0f, 0.01f, 1.0e-2f);
		color_picker(ctx, scene_spec->sky_color);
		nk_property_float(ctx, "Light emission:", 0.0f, &scene_spec->emission_material_strength, 1.0e5f, 0.01f, 1.0e-2f);
		color_picker(ctx, scene_spec->emission_material_color);
		// Exposure adjustment
		nk_layout_row_dynamic(ctx, 30, 1);
		nk_property_float(ctx, "Exposure:", 1.0e-34f, &scene_spec->exposure, 1.0e38f, scene_spec->exposure * 0.1f, scene_spec->exposure * 5.0e-3f);
		// Camera settings
		const char* camera_types[camera_type_count];
		camera_types[camera_type_first_person] = "First-person";
		camera_types[camera_type_ortho] = "Orthographic";
		camera_types[camera_type_hemispherical] = "Hemispherical";
		camera_types[camera_type_spherical] = "Spherical";
		nk_layout_row_dynamic(ctx, 30, 2);
		scene_spec->camera.type = nk_combo(ctx, camera_types, COUNT_OF(camera_types), scene_spec->camera.type, 30, (struct nk_vec2) { .x = 240.0f, .y = 180.0f });
		nk_label(ctx, "Camera type", NK_TEXT_ALIGN_LEFT);
		// Path length
		nk_layout_row_dynamic(ctx, 15, 0);
		nk_layout_row_dynamic(ctx, 30, 1);
		int new_path_length = (int) render_settings->path_length;
		nk_property_int(ctx, "Path length:", 0, &new_path_length, 10, 1, 0.001f);
		if (((int) render_settings->path_length) != new_path_length)
			update->scene_subpass = true;
		render_settings->path_length = (uint32_t) new_path_length;
		// Sampling strategies for path tracing
		nk_layout_row_dynamic(ctx, 30, 2);
		const char* sampling_strategies[sampling_strategy_count];
		sampling_strategies[sampling_strategy_spherical] = "Spherical";
		sampling_strategies[sampling_strategy_psa] = "Projected solid angle";
		sampling_strategies[sampling_strategy_brdf] = "BRDF";
		sampling_strategies[sampling_strategy_nee] = "Next event estimation";
		sampling_strategy_t new_sampling_strategy = nk_combo(ctx, sampling_strategies, COUNT_OF(sampling_strategies), render_settings->sampling_strategy, 30, (struct nk_vec2) { .x = 240.0f, .y = 180.0f });
		nk_label(ctx, "Sampling strategy", NK_TEXT_ALIGN_LEFT);
		if (render_settings->sampling_strategy != new_sampling_strategy)
			update->scene_subpass = true;
		render_settings->sampling_strategy = new_sampling_strategy;
		// Buttons quicksave, quickload and shader reload
		nk_layout_row_dynamic(ctx, 15, 1);
		nk_layout_row_dynamic(ctx, 30, 2);
		if (nk_button_label(ctx, "Quicksave"))
			quicksave(scene_spec);
		if (nk_button_label(ctx, "Quickload"))
			quickload(scene_spec, update, NULL);
		nk_layout_row_dynamic(ctx, 30, 1);
		if (nk_button_label(ctx, "Reload shaders"))
			update->scene_subpass = update->gui_subpass = true;
		#ifndef NDEBUG
		// Sliders for numbers to use for any purpose in shaders
		nk_layout_row_dynamic(ctx, 15, 1);
		nk_layout_row_dynamic(ctx, 30, 1);
		const char* param_labels[] = { "Param. 0:", "Param. 1:", "Param. 2:", "Param. 3:" };
		for (uint32_t i = 0; i != COUNT_OF(scene_spec->params); ++i)
			nk_property_float(ctx, param_labels[i], -1.0f, &scene_spec->params[i], 1.0f, 1.0e-6f, 5.0e-4f);
		#endif
	}
	nk_end(ctx);
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
	// Begin the render pass
	VkClearValue clear_values[] = {
		{ .color = { .float32 = { 0.0f, 0.0f, 0.0f, 0.0f } } },
		{ .depthStencil = { .depth = 1.0f } },
		{ .color = { .float32 = { 0.6f, 0.8f, 1.0f, 1.0f } } },
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
	// Render the scene
	if (app->render_targets.accum_frame_count == 0)
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->scene_subpass.pipeline_discard);
	else
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->scene_subpass.pipeline_accum);
	++app->render_targets.accum_frame_count;
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->scene_subpass.descriptor_set.pipeline_layout, 0, 1, app->scene_subpass.descriptor_set.descriptor_sets, 0, NULL);
	vkCmdBindVertexBuffers(cmd, 0, 1, &app->tonemap_subpass.triangle_buffer.buffers[0].buffer, &(VkDeviceSize) { 0 });
	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, frame->query_pool, timestamp_index_shading_begin);
	vkCmdDraw(cmd, 3, 1, 0, 0);
	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, frame->query_pool, timestamp_index_shading_end);
	// Begin the next subpass and perform tonemapping
	vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->tonemap_subpass.pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->tonemap_subpass.descriptor_set.pipeline_layout, 0, 1, app->tonemap_subpass.descriptor_set.descriptor_sets, 0, NULL);
	vkCmdBindVertexBuffers(cmd, 0, 1, &app->tonemap_subpass.triangle_buffer.buffers[0].buffer, &(VkDeviceSize) { 0 });
	vkCmdDraw(cmd, 3, 1, 0, 0);
	// Render the GUI
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->gui_subpass.pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->gui_subpass.descriptor_set.pipeline_layout, 0, 1, app->gui_subpass.descriptor_set.descriptor_sets, 0, NULL);
	vkCmdBindVertexBuffers(cmd, 0, 1, &app->gui.buffer.buffers[0].buffer, &(VkDeviceSize) { 0 });
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
	if (vkMapMemory(device->device, scrot.staging.allocations[dst->allocation_index], dst->memory_offset, VK_WHOLE_SIZE, 0, &staged_data)) {
		printf("Failed to map memory of the staging image whilst taking a screenshot.\n");
		free_screenshot(&scrot, device);
		return 1;
	}
	const float* floats = (float*) staged_data;
	// For HDR images convert half to float
	uint32_t pixel_count = extent.width * extent.height;
	if (format == image_file_format_hdr) {
		scrot.hdr_copy = malloc(pixel_count * 3 * sizeof(float));
		for (uint32_t x = 0; x != extent.width; ++x)
			for (uint32_t y = 0; y != extent.height; ++y)
				for (uint32_t i = 0; i != 3; ++i)
					scrot.hdr_copy[i + x * 3 + y * dst_pitch] = floats[i + x * 4 + y * src_pitch] / ((float) render_targets->accum_frame_count);
		// Store the image
		if (!stbi_write_hdr(file_path, (int) extent.width, (int) extent.height, 3, scrot.hdr_copy)) {
			printf("Failed to save a HDR screenshot to %s. Please check path, permissions and available disk space.\n", file_path);
			free_screenshot(&scrot, device);
			return 1;
		}
	}
	else {
		// For LDR images convert float to uint8_t using tone mapping
		scrot.ldr_copy = malloc(pixel_count * 3 * sizeof(uint8_t));
		float factor = scene_spec->exposure / ((float) render_targets->accum_frame_count);
		for (uint32_t x = 0; x != extent.width; ++x) {
			for (uint32_t y = 0; y != extent.height; ++y) {
				for (uint32_t i = 0; i != 3; ++i) {
					float rgb = floats[i + x * 4 + y * src_pitch] * factor;
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
		.initial_window_extent = { 1440, 1080 },
		.slide_screenshots = true,
		.v_sync = true,
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
		app_update_t update = { false };
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
