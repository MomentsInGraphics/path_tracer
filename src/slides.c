#include "main.h"
#include "string_utilities.h"


uint32_t create_slides(slide_t* slides) {
	uint32_t n = 0;
	render_settings_t quality = {
		.path_length = 4,
		.sampling_strategy = sampling_strategy_nee,
	};
	// 0: A pretty view of the bistro with quality path tracing
	slides[n++] = (slide_t) {
		.quicksave = copy_string("data/saves/bistro/pretty.rt_save"),
		.render_settings = quality,
		.screenshot_frame = 2048,
		.screenshot_path = copy_string("/home/cpeters/projects/cg_lectures/graphics/renders/pretty_bistro.png"),
	};
	// 1: Same view with emission only
	slides[n++] = (slide_t) {
		.quicksave = copy_string("data/saves/bistro/pretty.rt_save"),
		.render_settings = { .sampling_strategy = sampling_strategy_nee, .path_length = 1 },
		.screenshot_frame = 1,
		.screenshot_path = copy_string("/home/cpeters/projects/cg_lectures/graphics/renders/pretty_bistro_emission.png"),
	};
	// 2: An overview of the bistro to illustrate path construction
	slides[n++] = (slide_t) {
		.quicksave = copy_string("data/saves/bistro/path_overview.rt_save"),
		.render_settings = quality,
		.screenshot_frame = 2048,
		.screenshot_path = copy_string("/home/cpeters/projects/cg_lectures/graphics/renders/path_overview_bistro.png"),
	};
	// 3: The pretty view with spherical sampling
	slides[n++] = (slide_t) {
		.quicksave = copy_string("data/saves/bistro/pretty.rt_save"),
		.render_settings = { .sampling_strategy = sampling_strategy_spherical, .path_length = 4 },
		.screenshot_frame = 128,
		.screenshot_path = copy_string("/home/cpeters/projects/cg_lectures/graphics/renders/pretty_bistro_spherical.png"),
	};
	// 4: Same thing during the day
	slides[n++] = (slide_t) {
		.quicksave = copy_string("data/saves/bistro/pretty_day.rt_save"),
		.render_settings = { .sampling_strategy = sampling_strategy_spherical, .path_length = 4 },
		.screenshot_frame = 128,
		.screenshot_path = copy_string("/home/cpeters/projects/cg_lectures/graphics/renders/pretty_bistro_spherical_day.png"),
	};
	// 5: Mirror closeup with spherical sampling
	slides[n++] = (slide_t) {
		.quicksave = copy_string("data/saves/bistro/mirror.rt_save"),
		.render_settings = { .sampling_strategy = sampling_strategy_spherical, .path_length = 4 },
		.screenshot_frame = 4,
		.screenshot_path = copy_string("/home/cpeters/projects/cg_lectures/graphics/renders/mirror_spherical.png"),
	};
	// 6: Mirror closeup with PSA sampling
	slides[n++] = (slide_t) {
		.quicksave = copy_string("data/saves/bistro/mirror.rt_save"),
		.render_settings = { .sampling_strategy = sampling_strategy_psa, .path_length = 4 },
		.screenshot_frame = 4,
		.screenshot_path = copy_string("/home/cpeters/projects/cg_lectures/graphics/renders/mirror_psa.png"),
	};
	// 7: Mirror closeup with BRDF sampling
	slides[n++] = (slide_t) {
		.quicksave = copy_string("data/saves/bistro/mirror.rt_save"),
		.render_settings = { .sampling_strategy = sampling_strategy_brdf, .path_length = 4 },
		.screenshot_frame = 4,
		.screenshot_path = copy_string("/home/cpeters/projects/cg_lectures/graphics/renders/mirror_brdf.png"),
	};
	// 8: Spherical view from a shading point in the bistro
	slides[n++] = (slide_t) {
		.quicksave = copy_string("data/saves/bistro/shading_point_day.rt_save"),
		.render_settings = quality,
		.screenshot_frame = 2048,
		.screenshot_path = copy_string("/home/cpeters/projects/cg_lectures/graphics/renders/shading_point_incoming.png"),
	};
	// 9: The pretty view with BRDF sampling during day light
	slides[n++] = (slide_t) {
		.quicksave = copy_string("data/saves/bistro/pretty_day.rt_save"),
		.render_settings = { .sampling_strategy = sampling_strategy_brdf, .path_length = 4 },
		.screenshot_frame = 16,
		.screenshot_path = copy_string("/home/cpeters/projects/cg_lectures/graphics/renders/pretty_bistro_brdf_day.png"),
	};
	// 10: Same thing at night
	slides[n++] = (slide_t) {
		.quicksave = copy_string("data/saves/bistro/pretty.rt_save"),
		.render_settings = { .sampling_strategy = sampling_strategy_brdf, .path_length = 4 },
		.screenshot_frame = 16,
		.screenshot_path = copy_string("/home/cpeters/projects/cg_lectures/graphics/renders/pretty_bistro_brdf.png"),
	};
	// 11: Night-time pretty bistro with NEE
	slides[n++] = (slide_t) {
		.quicksave = copy_string("data/saves/bistro/pretty.rt_save"),
		.render_settings = { .sampling_strategy = sampling_strategy_nee, .path_length = 4 },
		.screenshot_frame = 16,
		.screenshot_path = copy_string("/home/cpeters/projects/cg_lectures/graphics/renders/pretty_bistro_nee.png"),
	};
	// 12: Spherical view from a shading point in the bistro at night with
	// reduced exposure
	slides[n++] = (slide_t) {
		.quicksave = copy_string("data/saves/bistro/shading_point_dark.rt_save"),
		.render_settings = quality,
		.screenshot_frame = 2048,
		.screenshot_path = copy_string("/home/cpeters/projects/cg_lectures/graphics/renders/shading_point_dark_incoming.png"),
	};
	// 13: Single sample render of the bistro as HDR image
	slides[n++] = (slide_t) {
		.quicksave = copy_string("data/saves/bistro/pretty.rt_save"),
		.render_settings = quality,
		.screenshot_frame = 1,
		.screenshot_path = copy_string("/home/cpeters/projects/cg_lectures/graphics/renders/pretty_bistro_1_spp.hdr"),
		.screenshot_format = image_file_format_hdr,
	};
	// Different path lengths for the pretty view of the bistro
	for (uint32_t i = 0; i != 10; ++i) {
		slides[n++] = (slide_t) {
			.quicksave = copy_string("data/saves/bistro/pretty.rt_save"),
			.render_settings = { .sampling_strategy = sampling_strategy_nee, .path_length = i },
			.screenshot_frame = 16 * 2048,
			.screenshot_path = format_uint("/home/cpeters/projects/cg_lectures/graphics/renders/bistro_path_lengths/%u.png", i),
		};
	}
	// Different path lengths for the Cornell box
	for (uint32_t i = 0; i != 10; ++i) {
		slides[n++] = (slide_t) {
			.quicksave = copy_string("data/saves/cornell_box/default.rt_save"),
			.render_settings = { .sampling_strategy = sampling_strategy_nee, .path_length = i },
			.screenshot_frame = 2048,
			.screenshot_path = format_uint("/home/cpeters/projects/cg_lectures/graphics/renders/cornell_box_path_lengths/%u.png", i),
		};
	}
	// Different path lengths for the living room
	for (uint32_t i = 0; i != 10; ++i) {
		slides[n++] = (slide_t) {
			.quicksave = copy_string("data/saves/living_room/day.rt_save"),
			.render_settings = { .sampling_strategy = sampling_strategy_nee, .path_length = i },
			.screenshot_frame = 16 * 2048,
			.screenshot_path = format_uint("/home/cpeters/projects/cg_lectures/graphics/renders/living_room_path_lengths/%u.png", i),
		};
	}
	printf("Defined %u slides.\n", n);
	if (n > MAX_SLIDE_COUNT)
		printf("WARNING: Wrote %u slides but MAX_SLIDE_COUNT is %u. Increase it.\n", n, MAX_SLIDE_COUNT);
	return n;
}
