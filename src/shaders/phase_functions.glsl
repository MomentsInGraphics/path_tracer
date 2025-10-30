/*! Evaluates an approximation to a Mie-scattering phase function as described
	in: https://doi.org/10.1145/3587421.3595409
	\param in_dir Normalized incoming light direction (pointing away from the
		shading point).
	\param out_dir Normalized outgoing light direction (pointing away from the
		shading point).
	\param fit_params Parameters of the fit. Using notations from the paper,
		they are g_HG, g_D, alpha, w_D, in this order. These should be computed
		using C-code based on the droplet size.
	\return The value of the phase function.*/
float mie_phase_function(vec3 in_dir, vec3 out_dir, vec4 fit_params) {
	float g_HG = fit_params[0], g_D = fit_params[1], alpha = fit_params[2], w_D = fit_params[3];
	// Evaluate the Henyey-Greenstein phase function
	float u = -dot(in_dir, out_dir);
	float denom_HG = (1.0 + g_HG * g_HG) - 2.0 * g_HG * u;
	denom_HG *= sqrt(denom_HG);
	float phi_HG = (1.0 - g_HG * g_HG) / denom_HG;
	// Evaluate Draine's phase function
	float denom_D = (1.0 + g_D * g_D) - 2.0 * g_D * u;
	denom_D *= sqrt(denom_D);
	float phi_D = (1.0 - g_D * g_D) * (1.0 + alpha * u * u) / (denom_D * (1.0 + alpha * (1.0 / 3.0 + 2.0 / 3.0 * g_D * g_D)));
	// Combine them
	float phi = mix(phi_HG, phi_D, w_D);
	// Apply a shared factor that was left out above
	const float pi = 3.14159265358;
	phi *= 0.25 / pi;
	return phi;
}


//! Constructs a special orthogonal matrix where the given normalized normal
//! vector is the third column
mat3 get_shading_space(vec3 n) {
	// This implementation is from: https://www.jcgt.org/published/0006/01/01/
	float s = (n.z > 0.0) ? 1.0 : -1.0;
	float a = -1.0 / (s + n.z);
	float b = n.x * n.y * a;
	vec3 b1 = vec3(1.0 + s * n.x * n.x * a, s * b, -s * n.x);
	vec3 b2 = vec3(b, s + n.y * n.y * a, -n.y);
	return mat3(b1, b2, n);
}


//! Helper for sample_mie_phase_function(). Samples dot(-in_dir, out_dir).
float sample_mie_phase_function_dot(float rand, vec4 fit_params) {
	float g_HG = fit_params[0], g_D = fit_params[1], alpha = fit_params[2], w_D = fit_params[3];
	// Sample Draine
	if (rand < w_D) {
		// Reuse the random number
		rand /= w_D;
		// Just evaluate the formulas from the paper
		float g = g_D;
		float g2 = g * g;
		float g4 = g2 * g2;
		float t0 = alpha - alpha * g2;
		float t1 = alpha * g4 - alpha;
		float t2 = -3.0 * (4.0 * (g4 - g2) + t1 * (1.0 + g2));
		float t3 = g * (2.0 * rand - 1.0);
		float t4 = 3.0 * g2 * (1.0 + t3) + alpha * (2.0 + g2 * (1.0 + (1.0 + 2.0 * g2) * t3));
		float t5 = t0 * (t1 * t2 + t4 * t4) + t1 * t1 * t1;
		float t6 = t0 * 4.0 * (g4 - g2);
		float t7 = pow(t5 + sqrt(t5 * t5 - t6 * t6 * t6), 1.0 / 3.0);
		float t8 = 2.0 * (t1 + t6 / t7 + t7) / t0;
		float t9 = sqrt(6.0 * (1.0 + g2) + t8);
		float t10 = sqrt(6.0 * (1.0 + g2) - t8 + 8.0 * t4 / (t0 * t9)) - t9;
		return 0.5 * g + (0.5 / g - 0.125 / g * t10 * t10);
	}
	// Or sample Henyey-Greenstein
	else {
		// Reuse the random number
		rand = (rand - w_D) / (1.0 - w_D);
		float t = (1.0 - g_HG * g_HG) / (1.0 - g_HG + 2.0 * g_HG * rand);
		float sample_dot = (1.0 + g_HG * g_HG - t * t) / (2.0 * g_HG);
		return g_HG == 0.0 ? (rand * 2.0 - 1.0) : sample_dot;
	}
}


/*! Given uniform random numbers in [0,1)^2 and an incoming light direction,
	this function produces a sampled outgoing light direction with a density
	matching mie_phase_function(). Direction vectors are normalized.*/
vec3 sample_mie_phase_function(vec3 in_dir, vec2 rands, vec4 fit_params) {
	// Sample in a local coordinate frame where -in_dir is the z-axis
	float z = sample_mie_phase_function_dot(rands[1], fit_params);
	float r = sqrt(max(0.0, 1.0 - z * z));
	const float pi = 3.14159265358;
	float phi = rands[0] * (2.0 * pi) - pi;
	vec3 dir = vec3(cos(phi) * r, sin(phi) * r, z);
	// Rotate to global coordinates
	return get_shading_space(-in_dir) * dir;
}
