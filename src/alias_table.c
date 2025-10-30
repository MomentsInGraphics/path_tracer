#include "alias_table.h"
#include <stdbool.h>


float build_alias_table(uint32_t* out_table, const float* weights, uint32_t count) {
	if (count == 0)
		return 0.0f;
	// Compute the sum of weights
	double weight_sum = 0.0;
	for (uint32_t i = 0; i != count; ++i)
		weight_sum += (double) weights[i];
	double mean = weight_sum / ((double) count);
	double inv_mean = 1.0 / mean;
	// Iterate over large and small elements. Small means less than the mean,
	// fake small is an element that used to be large.
	double large_weight = -1.0, small_weight = 3.4e+38;
	int32_t large = -1, real_small = -1, fake_small = -1;
	int32_t int_count = (int32_t) count;
	while (true) {
		// Advance to the next large or small element if weights are marked to
		// indicate that that is necessary
		while (large_weight <= mean && large + 1 < int_count)
			large_weight = weights[++large];
		while (small_weight > mean && fake_small < 0 && real_small + 1 < int_count)
			small_weight = weights[++real_small];
		// Check if we ran out of large or small weights
		if ((large == int_count - 1 && large_weight <= mean) || (real_small == int_count - 1 && small_weight > mean))
			break;
		// Make an entry for the alias table
		float prob = (float) (small_weight * inv_mean);
		uint32_t alias = (uint32_t) large;
		int32_t used_small = (fake_small >= 0) ? fake_small : real_small;
		out_table[used_small] = quantize_alias_table(prob, alias, count);
		// Update the probability for the large element
		large_weight += small_weight - mean;
		// If it has become small, make it our fake small element
		if (large_weight <= mean) {
			small_weight = large_weight;
			fake_small = large;
			// Remember that we need a new large element now
			large_weight = -1.0;
		}
		// Otherwise, remember that we have to seek a new small element
		else {
			fake_small = -1;
			small_weight = 3.4e+38;
		}
	}
	// Fill up the remainder of the alias table with entries that never use the
	// alias
	uint32_t no_alias = quantize_alias_table(1.0f, 0, int_count);
	if ((large == int_count - 1 && large_weight <= mean) && !(real_small == int_count - 1 && small_weight > mean)) {
		while (real_small < int_count) {
			out_table[real_small++] = no_alias;
			while (real_small < int_count && weights[real_small] > mean)
				++real_small;
		}
	}
	else if (!(large == int_count - 1 && large_weight <= mean) && (real_small == int_count - 1 && small_weight > mean)) {
		while (large < int_count) {
			out_table[large++] = no_alias;
			while (large < int_count && weights[large] <= mean)
				++large;
		}
	}
	return (float) weight_sum;
}
