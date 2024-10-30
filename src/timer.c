#include "timer.h"
#include "string_utilities.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


//! A ring-buffer used to record times at which record_frame_time() was invoked
typedef struct {
	//! All the recorded times in seconds as produced by glfwGetTime()
	double times[RECORDED_FRAME_COUNT];
	//! The number of times which have been recorded
	uint64_t frame_count;
	//! The index of the next entry in times that will be overwritten
	uint64_t time_index;
} frame_record_t;


//! The only instance of frame_record_t
static frame_record_t record = {
	.frame_count = 0,
	.time_index = 0,
};


void record_frame_time() {
	record.times[record.time_index] = glfwGetTime();
	record.time_index = (record.time_index + 1) % RECORDED_FRAME_COUNT;
	++record.frame_count;
}


float get_frame_delta() {
	if (record.frame_count < 2)
		return 0.0f;
	else
		return (float) (
			record.times[(record.time_index + RECORDED_FRAME_COUNT - 1) % RECORDED_FRAME_COUNT] -
			record.times[(record.time_index + RECORDED_FRAME_COUNT - 2) % RECORDED_FRAME_COUNT]);
}


//! Comparison function for qsort
int cmp_floats(const void* raw_lhs, const void* raw_rhs) {
	float lhs = *(const float*) raw_lhs;
	float rhs = *(const float*) raw_rhs;
	return (lhs < rhs) ? -1 : ((lhs > rhs) ? 1 : 0);
}


frame_time_stats_t get_frame_stats() {
	frame_time_stats_t stats;
	memset(&stats, 0, sizeof(stats));
	if (record.frame_count < 2)
		return stats;
	stats.last = get_frame_delta();
	// Gather all recorded frame times
	float frame_times[RECORDED_FRAME_COUNT];
	memset(frame_times, 0, sizeof(frame_times));
	uint64_t frame_time_count = record.frame_count - 1;
	if (frame_time_count > RECORDED_FRAME_COUNT - 1)
		frame_time_count = RECORDED_FRAME_COUNT - 1;
	for (uint64_t i = 0; i != frame_time_count; ++i)
		frame_times[i] = (float) (
			record.times[(record.time_index + RECORDED_FRAME_COUNT - i - 1) % RECORDED_FRAME_COUNT] -
			record.times[(record.time_index + RECORDED_FRAME_COUNT - i - 2) % RECORDED_FRAME_COUNT]);
	// Compute the mean
	float frame_time_sum = 0.0;
	for (uint64_t i = 0; i != frame_time_count; ++i)
		frame_time_sum += frame_times[i];
	stats.mean = frame_time_sum / ((float) frame_time_count);
	// Sort the frame times
	qsort(frame_times, frame_time_count, sizeof(float), &cmp_floats);
	// Compute percentiles
	float percents[] = { 0.0f, 1.0f, 10.0f, 50.0f, 90.0f, 99.0f, 100.0f };
	float* percentiles[] = { &stats.min, &stats.percentile_1, &stats.percentile_10, &stats.median, &stats.percentile_90, &stats.percentile_99, &stats.max };
	for (uint32_t i = 0; i != COUNT_OF(percents); ++i) {
		// Perform linear interpolation between the two adjacent entries
		float float_index = 0.01 * percents[i] * ((float) (frame_time_count - 1));
		uint64_t left = (uint64_t) float_index;
		float lerp = float_index - (float) left;
		uint64_t right = left + 1;
		(*percentiles[i]) = (1.0f - lerp) * frame_times[left] + lerp * frame_times[right];
	}
	return stats;
}
