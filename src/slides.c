#include "main.h"
#include "string_utilities.h"

//! Render settings associated with a name
typedef struct {
	render_settings_t settings;
	const char* name;
} named_render_settings_t;


/*! Creates slides for all combinations of the given scenes and techniques.
	Screenshot names take the form <scene_name>-<setting_name>.<extension>.
	\param slides The array into which slides are written.
	\param slide_index Pointer to the offset at which slides are written. Gets
		incremented for each created slide.
	\param dst_path The directory path (including the trailing slash) to which
		screenshots are written. Must exist already.
	\param save_path The source directory containing quicksave files.
	\param screenshot_frame See slide_t.
	\param screenshot_format See slide_t.
	\param volume_interpolation See slide_t.
	\param tonemapper See slide_t.
	\param scenes File names (without extension) of quicksaves.
	\param scene_count Number of array entries in scenes.
	\param settings Complete render settings, along with a name.
	\param setting_count Number of array entries in settings.*/
void slide_combinations(slide_t* slides, uint32_t* slide_index, const char* dst_path, const char* save_path, uint32_t screenshot_frame, image_file_format_t screenshot_format, volume_interpolation_t volume_interpolation, tonemapper_op_t tonemapper, const char* const* const scenes, uint32_t scene_count, const named_render_settings_t* settings, uint32_t setting_count) {
	const char* ext;
	switch (screenshot_format) {
		case image_file_format_jpg: ext = ".jpg"; break;
		case image_file_format_png: ext = ".png"; break;
		case image_file_format_hdr: ext = ".hdr"; break;
		case image_file_format_pfm: ext = ".pfm"; break;
		default: ext = ""; break;
	}
	for (uint32_t i = 0; i != scene_count; ++i) {
		char* save = cat_strings((const char*[]) { save_path, scenes[i], ".jack_save" }, 3);
		for (uint32_t j = 0; j != setting_count; ++j) {
			slides[(*slide_index)++] = (slide_t) {
				.quicksave = copy_string(save), .screenshot_frame = screenshot_frame, .screenshot_format = screenshot_format,
				.render_settings = settings[j].settings,
				.screenshot_path = cat_strings((const char*[]) { dst_path, scenes[i], "-", settings[j].name, ext }, 5),
				.volume_interpolation = volume_interpolation,
				.tonemapper = tonemapper,
			};
		}
		free(save);
	}
}


uint32_t create_slides(slide_t* slides) {
	uint32_t n = 0;
	const char* src = "data/quicksaves/jackknife_paper/";
	const char* dst = "data/jackknife_screenshots/";
	printf("Slide ranges start at: ");
	// Define presentation slides
	volume_interpolation_t interp = volume_interpolation_linear_stochastic;
	render_settings_t our_jackknife_settings = {
		.optical_depth_cdf_step = 0.6,
		.optical_depth_estimator = optical_depth_estimator_variance_aware_fixed_count,
		.ray_march_sample_count = 10,
		.transmittance_estimator = transmittance_estimator_jackknife,
		.uniform_jittered = false,
		.brick_size = 16,
		.sample_count = 1,
		.jitter_primary_rays = true,
		.distance_sampler = distance_sampler_delta_tracking,
		.grid_compression = grid_compression_bc4,
		.sampling_strategies = sampling_strategies_light_transmittance_mis,
		.displayed_quantity = displayed_quantity_radiance,
		.mis_weight_estimator = mis_weight_estimator_dummy,
		.kettunen_sample_multiplier = 1.0f,
	};
	printf("%u: Presentation  ", n);
	slides[n++] = (slide_t) {
		.quicksave = copy_string("data/quicksaves/jackknife_paper/intel_cloud.jack_save"),
		.render_settings = our_jackknife_settings,
		.screenshot_format = image_file_format_png,
		.screenshot_frame = 64,
		.screenshot_path = copy_string("data/jackknife_screenshots/teaser_slide.png"),
		.volume_interpolation = interp,
		.tonemapper = tonemapper_op_aces,
	};
	render_settings_t dummy_settings = our_jackknife_settings;
	dummy_settings.transmittance_estimator = transmittance_estimator_dummy;
	slides[n++] = (slide_t) {
		.quicksave = copy_string("data/quicksaves/jackknife_paper/intel_cloud.jack_save"),
		.render_settings = dummy_settings,
		.screenshot_format = image_file_format_png,
		.screenshot_frame = 64,
		.screenshot_path = copy_string("data/jackknife_screenshots/teaser_slide_no_shadow.png"),
		.volume_interpolation = interp,
		.tonemapper = tonemapper_op_aces,
	};
	// Define a bunch of transmittance estimation techniques for systematic
	// comparisons
	named_render_settings_t trans_tecs[] = {
		{
			{
				.optical_depth_cdf_step = 0.6,
				.optical_depth_estimator = optical_depth_estimator_variance_aware,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_jackknife,
				.uniform_jittered = false,
			},
			"jackknife_va0.6"
		},
		{
			{
				.optical_depth_cdf_step = 0.6,
				.optical_depth_estimator = optical_depth_estimator_variance_aware_fixed_count,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_jackknife,
				.uniform_jittered = false,
			},
			"jackknife_vafc10"
		},
		{
			{
				.optical_depth_cdf_step = 0.6,
				.optical_depth_estimator = optical_depth_estimator_variance_aware_fixed_count,
				.ray_march_sample_count = 20,
				.transmittance_estimator = transmittance_estimator_jackknife,
				.uniform_jittered = false,
			},
			"jackknife_vafc20"
		},
		{
			{
				.optical_depth_cdf_step = 0.15,
				.optical_depth_estimator = optical_depth_estimator_mean_sampling_fixed_count,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_jackknife,
				.uniform_jittered = false,
			},
			"jackknife_meanfc10"
		},
		{
			{
				.optical_depth_cdf_step = 0.3,
				.optical_depth_estimator = optical_depth_estimator_variance_aware,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_biased_ray_marching,
				.uniform_jittered = true,
			},
			"biased_va0.3"
		},
		{
			{
				.optical_depth_cdf_step = 0.3,
				.optical_depth_estimator = optical_depth_estimator_variance_aware_fixed_count,
				.ray_march_sample_count = 20,
				.transmittance_estimator = transmittance_estimator_biased_ray_marching,
				.uniform_jittered = true,
			},
			"biased_vafc20"
		},
		{
			{
				.optical_depth_cdf_step = 0.3,
				.optical_depth_estimator = optical_depth_estimator_variance_aware_fixed_count,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_biased_ray_marching,
				.uniform_jittered = false,
			},
			"biased_vafc10_stratified"
		},
		{
			{
				.optical_depth_cdf_step = 0.15,
				.optical_depth_estimator = optical_depth_estimator_mean_sampling,
				.ray_march_sample_count = 20,
				.transmittance_estimator = transmittance_estimator_biased_ray_marching,
				.uniform_jittered = true,
			},
			"biased_mean0.15"
		},
		{
			{
				.optical_depth_cdf_step = 0.3,
				.optical_depth_estimator = optical_depth_estimator_equidistant,
				.ray_march_sample_count = 50,
				.transmittance_estimator = transmittance_estimator_biased_ray_marching,
				.uniform_jittered = true,
			},
			"biased_equi50"
		},
		{
			{
				.optical_depth_cdf_step = 0.3,
				.optical_depth_estimator = optical_depth_estimator_equidistant,
				.ray_march_sample_count = 25,
				.transmittance_estimator = transmittance_estimator_jackknife,
				.uniform_jittered = false,
			},
			"jackknife_equi25"
		},
		{
			{
				.optical_depth_cdf_step = 0.3,
				.optical_depth_estimator = optical_depth_estimator_kettunen,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_biased_ray_marching,
				.uniform_jittered = true,
				.kettunen_sample_multiplier = 1.0f,
			},
			"biased_kettunen_mean"
		},
		{
			{
				.optical_depth_cdf_step = 0.3,
				.optical_depth_estimator = optical_depth_estimator_kettunen,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_biased_ray_marching,
				.uniform_jittered = true,
				.kettunen_sample_multiplier = 2.0f,
			},
			"biased_kettunen_mean_x2"
		},
		{
			{
				.optical_depth_cdf_step = 0.3,
				.optical_depth_estimator = optical_depth_estimator_kettunen,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_unbiased_ray_marching,
				.uniform_jittered = true,
				.kettunen_sample_multiplier = 1.0f,
			},
			"unbiased_kettunen_mean"
		},
		{
			{
				.optical_depth_cdf_step = 0.3,
				.optical_depth_estimator = optical_depth_estimator_kettunen,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_unbiased_ray_marching,
				.uniform_jittered = true,
				.kettunen_sample_multiplier = 2.0f,
			},
			"unbiased_kettunen_mean_x2"
		},
		{
			{
				.optical_depth_cdf_step = 0.3,
				.optical_depth_estimator = optical_depth_estimator_equidistant,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_track_length,
				.uniform_jittered = true,
			},
			"track_length"
		},
		{
			{
				.optical_depth_cdf_step = 0.3,
				.optical_depth_estimator = optical_depth_estimator_equidistant,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_ratio_tracking,
				.uniform_jittered = true,
			},
			"ratio_tracking"
		},
		{
			{
				.optical_depth_cdf_step = 0.3,
				.optical_depth_estimator = optical_depth_estimator_equidistant,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_regular_tracking,
				.uniform_jittered = true,
			},
			"regular_tracking"
		},
	};
	// Set a few parameters that are always kept equal for transmittance
	// estimation experiments
	for (uint32_t i = 0; i != COUNT_OF(trans_tecs); ++i) {
		render_settings_t* s = &trans_tecs[i].settings;
		s->jitter_primary_rays = true;
		s->brick_size = 16;
		s->distance_sampler = distance_sampler_delta_tracking;
		s->grid_compression = grid_compression_bc4;
		s->sample_count = 1;
		s->sampling_strategies = sampling_strategies_light_transmittance_mis;
		s->displayed_quantity = displayed_quantity_transmittance;
		s->mis_weight_estimator = mis_weight_estimator_dummy;
		if (s->kettunen_sample_multiplier == 0.0f) s->kettunen_sample_multiplier = 1.0f;
	}
	// Test scenes for transmittance estimation, rendered at 1spp
	const char* transmittance_scenes[] = { "bunny_cloud", "disney_cloud", "explosion_vdb", "fire", "intel_cloud", "smoke", "smoke2" };
	printf("%u: LDR transmittance  ", n);
	slide_combinations(slides, &n, dst, src, 1, image_file_format_png, interp, tonemapper_op_clamp, transmittance_scenes, COUNT_OF(transmittance_scenes), trans_tecs, COUNT_OF(trans_tecs));
	// Another variant of these slides, which stores stats of transmittance
	for (uint32_t i = 0; i != COUNT_OF(trans_tecs); ++i) {
		trans_tecs[i].settings.displayed_quantity = displayed_quantity_transmittance_stats;
		trans_tecs[i].settings.sample_count = 32;
	}
	printf("%u: HDR transmittance  ", n);
	slide_combinations(slides, &n, dst, src, 128 * 2048, image_file_format_pfm, interp, tonemapper_op_clamp, transmittance_scenes, COUNT_OF(transmittance_scenes), trans_tecs, COUNT_OF(trans_tecs) - 1);
	//printf("%u: HDR transmittance reference  ", n);
	//slide_combinations(slides, &n, dst, src, 128, image_file_format_pfm, interp, transmittance_scenes, COUNT_OF(transmittance_scenes), &trans_tecs[COUNT_OF(trans_tecs) - 1], 1);
	// Yet another variant using only one technique to analyze how often the
	// transmittance estimates have a negative sign
	for (uint32_t i = 0; i != COUNT_OF(trans_tecs); ++i) {
		trans_tecs[i].settings.displayed_quantity = displayed_quantity_transmittance_sign;
		trans_tecs[i].settings.sample_count = 32;
	}
	printf("%u: Transmittance sign  ", n);
	slide_combinations(slides, &n, "data/jackknife_screenshots/sign_", src, 32 * 2048, image_file_format_pfm, interp, tonemapper_op_clamp, transmittance_scenes, COUNT_OF(transmittance_scenes), &trans_tecs[1], 1);
	// Teaser
	for (uint32_t i = 0; i != COUNT_OF(trans_tecs); ++i) {
		render_settings_t* s = &trans_tecs[i].settings;
		s->displayed_quantity = displayed_quantity_radiance;
		s->jitter_primary_rays = true;
		s->sample_count = 32;
	}
	const char* teaser_scene = "intel_cloud";
	printf("%u: Teaser  ", n);
	slide_combinations(slides, &n, "data/jackknife_screenshots/teaser_", src, 32, image_file_format_png, interp, tonemapper_op_count, &teaser_scene, 1, trans_tecs, COUNT_OF(trans_tecs) - 1);
	// Another variant of the teaser slide, which stores stats of luminance
	for (uint32_t i = 0; i != COUNT_OF(trans_tecs); ++i)
		trans_tecs[i].settings.displayed_quantity = displayed_quantity_luminance_stats;
	printf("%u: HDR teaser  ", n);
	slide_combinations(slides, &n, "data/jackknife_screenshots/teaser_", src, 32 * 2048, image_file_format_pfm, interp, tonemapper_op_count, &teaser_scene, 1, trans_tecs, COUNT_OF(trans_tecs) - 1);

	// Define tests for MIS in the same manner
	named_render_settings_t mis_tecs[] = {
		{
			{
				.optical_depth_cdf_step = 0.6,
				.optical_depth_estimator = optical_depth_estimator_variance_aware_fixed_count,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_jackknife,
				.mis_weight_estimator = mis_weight_estimator_jackknife,
				.uniform_jittered = false,
				.sampling_strategies = sampling_strategies_light,
			},
			"light_only"
		},
		{
			{
				.optical_depth_cdf_step = 0.6,
				.optical_depth_estimator = optical_depth_estimator_variance_aware_fixed_count,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_jackknife,
				.mis_weight_estimator = mis_weight_estimator_jackknife,
				.uniform_jittered = false,
				.sampling_strategies = sampling_strategies_transmittance,
			},
			"transmittance_only"
		},
		{
			{
				.optical_depth_cdf_step = 0.6,
				.optical_depth_estimator = optical_depth_estimator_variance_aware_fixed_count,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_jackknife,
				.mis_weight_estimator = mis_weight_estimator_biased_ray_marching,
				.uniform_jittered = false,
				.sampling_strategies = sampling_strategies_light_transmittance_mis,
			},
			"mis_naive"
		},
		{
			{
				.optical_depth_cdf_step = 0.6,
				.optical_depth_estimator = optical_depth_estimator_variance_aware_fixed_count,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_jackknife,
				.mis_weight_estimator = mis_weight_estimator_biased_ray_marching,
				.uniform_jittered = false,
				.sampling_strategies = sampling_strategies_light_transmittance_approximate_mis,
			},
			"mis_approximate_naive"
		},
		{
			{
				.optical_depth_cdf_step = 0.6,
				.optical_depth_estimator = optical_depth_estimator_variance_aware_fixed_count,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_jackknife,
				.mis_weight_estimator = mis_weight_estimator_jackknife,
				.uniform_jittered = false,
				.sampling_strategies = sampling_strategies_light_transmittance_mis,
			},
			"mis_ours"
		},
		{
			{
				.optical_depth_cdf_step = 0.6,
				.optical_depth_estimator = optical_depth_estimator_variance_aware_fixed_count,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_jackknife,
				.mis_weight_estimator = mis_weight_estimator_jackknife,
				.uniform_jittered = false,
				.sampling_strategies = sampling_strategies_light_transmittance_approximate_mis,
			},
			"mis_approximate_ours"
		},
		{
			{
				.optical_depth_cdf_step = 0.6,
				.optical_depth_estimator = optical_depth_estimator_variance_aware_fixed_count,
				.ray_march_sample_count = 10,
				.transmittance_estimator = transmittance_estimator_jackknife,
				.mis_weight_estimator = mis_weight_estimator_jackknife,
				.uniform_jittered = false,
				.sampling_strategies = sampling_strategies_light_transmittance_miller_mis,
			},
			"mis_miller"
		},
	};
	for (uint32_t i = 0; i != COUNT_OF(mis_tecs); ++i) {
		render_settings_t* s = &mis_tecs[i].settings;
		s->jitter_primary_rays = true;
		s->brick_size = 16;
		s->distance_sampler = distance_sampler_delta_tracking;
		s->grid_compression = grid_compression_bc4;
		s->sample_count = 16;
	}
	const char* mis_scenes[] = { "disney_cloud_mis" };
	volume_interpolation_t mis_interp = volume_interpolation_nearest;
	printf("%u: MIS comparisons (LDR, low spp)  ", n);
	slide_combinations(slides, &n, dst, src, 16, image_file_format_png, mis_interp, tonemapper_op_clamp, mis_scenes, COUNT_OF(mis_scenes), mis_tecs, COUNT_OF(mis_tecs));
	printf("%u: MIS comparisons (high spp)  ", n);
	slide_combinations(slides, &n, dst, src, 128 * 2048, image_file_format_pfm, mis_interp, tonemapper_op_clamp, mis_scenes, COUNT_OF(mis_scenes), mis_tecs, COUNT_OF(mis_tecs));
	const char* mis_scenes_hdr[] = { "disney_cloud_mis_hdr" };
	printf("%u: MIS comparisons (HDR, low spp)  ", n);
	slide_combinations(slides, &n, dst, src, 16, image_file_format_pfm, mis_interp, tonemapper_op_clamp, mis_scenes_hdr, COUNT_OF(mis_scenes_hdr), mis_tecs, COUNT_OF(mis_tecs));

	// Report results
	printf("\n");
	printf("Defined %u slides.\n", n);
	if (n > MAX_SLIDE_COUNT)
		printf("WARNING: Wrote %u slides but MAX_SLIDE_COUNT is %u. Increase it.\n", n, MAX_SLIDE_COUNT);
	// Optionally, print the full list of screenshot IDs
	if (false)
		for (uint32_t i = 0; i != n; ++i)
			printf("%03u: %s\n", i, slides[i].screenshot_path);
	return n;
}
