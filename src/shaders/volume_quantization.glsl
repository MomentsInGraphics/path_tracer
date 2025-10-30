/*! Turns quantization coefficients into dequantization coefficients a, b, c
	(returned in this order). The formula to get from quantized values y in
	[0,1] to volume values x uses those. If a==0, the formula is x = b*y + c.
	Otherwise, it is a * log(b*y + c).*/
vec3 get_dequantization_coeffs(vec3 quantization_coeffs) {
	float exponent_factor = quantization_coeffs[0];
	float factor = quantization_coeffs[1];
	float summand = quantization_coeffs[2];
	float a = (exponent_factor == 0.0) ? 0.0 : (1.0 / exponent_factor);
	float b = 1.0 / factor;
	float c = -summand * b;
	return vec3(a, b, c);
}


//! Inverse of get_dequantization_coeffs()
vec3 get_quantization_coeffs(vec3 dequantization_coeffs) {
	// The function happens to be self-inverse
	return get_dequantization_coeffs(dequantization_coeffs);
}


//! Applies the dequantization transform to a quantized value obtained from the
//! volume texture
float dequantize_volume(float quantized, vec3 dequantization_coeffs) {
	float mapped = fma(quantized, dequantization_coeffs[1], dequantization_coeffs[2]);
	return (dequantization_coeffs[0] == 0.0) ? mapped : (dequantization_coeffs[0] * log(mapped));
}


//! Applies the quantization transform to a volume value
float quantize_volume(float dequantized, vec3 quantization_coeffs) {
	float mapped = (quantization_coeffs[0] == 0.0) ? dequantized : exp(quantization_coeffs[0] * dequantized);
	return quantization_coeffs[1] * mapped + quantization_coeffs[2];	
}
