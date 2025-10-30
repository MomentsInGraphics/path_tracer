#version 460


//! The buffer to which quantization coefficients are stored
layout (binding = 0) buffer writeonly quantization_coeff_output {
	vec4 g_dequantization_coeffs[3];
};


//! This shader runs briefly, but with very few threads
layout (local_size_x = GRID_ATTRIBUTE_COUNT, local_size_y = 1, local_size_z = 1) in;


void main() {
	g_dequantization_coeffs[gl_GlobalInvocationID.x] = vec4(0.0, 1.0, 0.0, 0.0);
}
