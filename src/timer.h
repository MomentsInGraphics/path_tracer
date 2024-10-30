#pragma once
#define RECORDED_FRAME_COUNT 101


//! Various statistics about frame times observed in the most recent
//! RECORDED_FRAME_COUNT - 1 frames (all in seconds)
typedef struct {
	//! The most recently measured frame time, as produced by get_frame_delta()
	float last;
	//! The arithmetic mean frame time
	float mean;
	//! The median frame time
	float median;
	//! Various percentiles of frame times (percentile_1 is smallest)
	float percentile_1, percentile_10, percentile_90, percentile_99;
	//! The minimal and maximal frame time
	float min, max;
} frame_time_stats_t;


//! Records the current time. Called exactly once per frame to indicate that a
//! new frame has begun.
void record_frame_time();


//! \return The time in seconds elapsed between the two most recent invocations
//!		of record_frame_time() or 0.0f if there were less than two invocations
float get_frame_delta();


//! \return Statistics about the frame times observed in the most recent
//!		RECORDED_FRAME_COUNT frames or an all 0 object if less than two frames
//!		have been recorded.
frame_time_stats_t get_frame_stats();
