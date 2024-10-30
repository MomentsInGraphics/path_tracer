//! Given a quantized 64-bit position as used in the *.vks scene file format
//! and corresponding dequantization constants, this function produces a
//! floating-point world-space position.
vec3 dequantize_position(uvec2 quantized_position, vec3 dequantization_factor, vec3 dequantization_summand) {
	vec3 position = vec3(
		bitfieldExtract(quantized_position[0], 0, 21),
		bitfieldExtract(quantized_position[0], 21, 11) | ((quantized_position[1] & 0x3FF) << 11),
		bitfieldExtract(quantized_position[1], 10, 21)
	);
	return fma(position, dequantization_factor, dequantization_summand);
}


//! Given two coordinates of a normal vector between 0 and 1, this function
//! produces a normalized floating-point world-space normal
vec3 dequantize_normal(vec2 octahedral_normal) {
	// The *.vks format specifies normals in such a way that the coordinate
	// zero can be represented exactly. Because of that, -1 corresponds to the
	// second-smallest fixed-point number.
	const float factor = 2.0 * (65534.0 / 65535.0);
	const float summand = -(32768.0 / 65535.0) * factor;
	octahedral_normal = fma(octahedral_normal, vec2(factor), vec2(summand));
	// Undo the octahedral map
	vec3 normal = vec3(octahedral_normal.xy, 1.0 - abs(octahedral_normal.x) - abs(octahedral_normal.y));
	vec2 non_zero_sign = vec2(
		(octahedral_normal.x >= 0.0) ? 1.0 : -1.0,
		(octahedral_normal.y >= 0.0) ? 1.0 : -1.0
	);
	normal.xy = (normal.z < 0.0) ? ((1.0 - abs(normal.yx)) * non_zero_sign) : normal.xy;
	return normalize(normal);
}
