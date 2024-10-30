// Applies the non-linearity that maps linear RGB to sRGB
float linear_to_srgb(float linear) {
	return (linear <= 0.0031308) ? (12.92 * linear) : (1.055 * pow(linear, 1.0 / 2.4) - 0.055);
}

// Inverse of linear_to_srgb()
float srgb_to_linear(float non_linear) {
	return (non_linear <= 0.04045) ? ((1.0 / 12.92) * non_linear) : pow(non_linear * (1.0 / 1.055) + 0.055 / 1.055, 2.4);
}

// Turns a linear RGB color (i.e. rec. 709) into sRGB
vec3 linear_rgb_to_srgb(vec3 linear) {
	return vec3(linear_to_srgb(linear.r), linear_to_srgb(linear.g), linear_to_srgb(linear.b));
}

// Inverse of linear_rgb_to_srgb()
vec3 srgb_to_linear_rgb(vec3 srgb) {
	return vec3(srgb_to_linear(srgb.r), srgb_to_linear(srgb.g), srgb_to_linear(srgb.b));
}
