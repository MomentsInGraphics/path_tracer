#version 460
#extension GL_EXT_control_flow_attributes : enable
#extension GL_GOOGLE_include_directive : enable
#include "volume_quantization.glsl"


#ifndef ROUND_OP
	//! The function used to round values to representable values. Setting this
	//! to floor() or ceil() gives conservative compression.
	#define ROUND_OP round
#endif


//! The volume texture to which BC4 compression should be applied
layout (binding = 0) uniform sampler3D g_uncompressed_volume;
//! The buffer holding coefficients for dequantization of BC4-compressed values
layout (binding = 1) uniform dequantization_coeff_buffer {
	vec3 g_dequantization_coeffs[GRID_ATTRIBUTE_INDEX + 1];
};

//! A view onto the BC4-compressed output volume texture. Each block is viewed
//! as two 32-bit uints.
layout (binding = 2) uniform writeonly uimage3D g_compressed_volume;

//! We process 4x4x4 blocks per thread group
layout (local_size_x = 4, local_size_y = 4, local_size_z = 4) in;


void main() {
	const ivec2 block_size = ivec2(4, 4);
	const int texel_bit_count = 3;
	// Grab the quantization coefficients
	vec3 quantization_coeffs = get_quantization_coeffs(g_dequantization_coeffs[GRID_ATTRIBUTE_INDEX]);
	// Figure out the location of the output block and its left top in the
	// source array
	ivec3 block_pos = ivec3(gl_GlobalInvocationID);
	ivec3 left_top = ivec3(block_pos.xy * block_size, block_pos.z);
	// Read all values that make up the block to determine minimum and maximum
	float minimum = 1.0e38, maximum = -1.0e38;
	[[unroll]]
	for (int x = 0; x != block_size.x; ++x) {
		[[unroll]]
		for (int y = 0; y != block_size.y; ++y) {
			float value = texelFetch(g_uncompressed_volume, left_top + ivec3(x, y, 0), 0).r;
			value = quantize_volume(value, quantization_coeffs);
			minimum = min(minimum, value);
			maximum = max(maximum, value);
		}
	}
	// Figure out how to quantize for BC4
	uint quantized_min = uint(ROUND_OP(255.0 * clamp(minimum, 0.0, 1.0)));
	uint quantized_max = uint(ROUND_OP(255.0 * clamp(maximum, 0.0, 1.0)));
	minimum = float(quantized_min) * (1.0 / 255.0);
	maximum = float(quantized_max) * (1.0 / 255.0);
	float factor = 7.0 / (maximum - minimum);
	float summand = -minimum * factor;
	uvec2 block = uvec2(0, 0);
	block[0] = bitfieldInsert(block[0], quantized_max, 0, 8);
	block[0] = bitfieldInsert(block[0], quantized_min, 8, 8);
	// Read all values again to represent them in the block
	[[unroll]]
	for (int x = 0; x != block_size.x; ++x) {
		[[unroll]]
		for (int y = 0; y != block_size.y; ++y) {
			float value = texelFetch(g_uncompressed_volume, left_top + ivec3(x, y, 0), 0).r;
			value = quantize_volume(value, quantization_coeffs);
			int quantized = int(ROUND_OP(value * factor + summand));
			uint mapped = (quantized >= 7) ? 0 : ((quantized <= 0) ? 1 : uint(8 - quantized));
			int offset = 16 + texel_bit_count * x + (texel_bit_count * block_size.x) * y;
			[[flatten]]
			if (offset + texel_bit_count <= 32)
				block[0] = bitfieldInsert(block[0], mapped, offset, texel_bit_count);
			else if (offset >= 32)
				block[1] = bitfieldInsert(block[1], mapped, offset - 32, texel_bit_count);
			else {
				block[0] = bitfieldInsert(block[0], mapped, offset, 32 - offset);
				block[1] |= bitfieldInsert(block[1], bitfieldExtract(mapped, 32 - offset, texel_bit_count), 0, texel_bit_count - (32 - offset));
			}
		}
	}
	// Write the block
	imageStore(g_compressed_volume, block_pos, uvec4(block, 0, 0));
}
