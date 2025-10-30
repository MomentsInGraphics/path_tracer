#include <stdint.h>
#include <math.h>


/*! Constructs an alias table for importance sampling of a discrete
	distribution.
	\param out_table A pointer to an array of count elements to which the alias
		table is written. It is packed using quantize_alias_table().
	\param weights An array of count non-negative weights.
	\param count The number of elements to sample.
	\return The sum of all given weights.
	\note For count in excess of ca. 2^23, quantization errors in probabilities
		become significant and sample quality deteriorates. This method
		implements the following paper, including optimizations to avoid the
		need for memory allocations: https://doi.org/10.1109/32.92917 */
float build_alias_table(uint32_t* out_table, const float* weights, uint32_t count);


//! Packs a probability and alias (i.e. one entry of an alias table with count
//! entries) into 32 bits and returns the result.
static inline uint32_t quantize_alias_table(float probability, uint32_t alias, uint32_t count) {
	uint32_t prob_count = 0xfffffffful / count;
	if (probability < 0.0f) probability = 0.0f;
	if (probability > 1.0f) probability = 1.0f;
	uint32_t quantized_prob = (uint32_t) roundf(probability * ((float) (prob_count - 1)));
	return quantized_prob * count + alias;
}


//! Extracts the probability from a quantized alias table entry
static inline float dequantize_alias_table_probability(uint32_t quantized, uint32_t count) {
	uint32_t prob_count = 0xfffffffful / count;
	uint32_t quantized_prob = quantized / count;
	return ((float) quantized_prob) / ((float) (prob_count - 1));
}


//! Extracts the alias from a quantized alias table entry
static inline uint32_t dequantize_alias_table_alias(uint32_t quantized, uint32_t count) {
	return quantized % count;
}
