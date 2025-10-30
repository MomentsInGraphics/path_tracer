//! Maintains the state of the random number generation for a single thread
//! (usually a pixel).
struct random_generator {
	//! The seed for PCG2D
	uvec2 seed;
};


//! Produces a uniform random number in [0,1) using PCG (1D) and updates the
//! seed
float pcg(inout uint v) {
	v = v * 747796405u + 2891336453u;
	v = ((v >> ((v >> 28u) + 4u)) ^ v) * 277803737u;
	v = (v >> 22u) ^ v;
	return v * pow(0.5, 32.0);
}


//! Produces 3 independent uniform random numbers in [0,1) using PCG 3D and
//! updates the seed
vec3 pcg_3d(inout uvec3 v) {
	// PCG3D, as described here: http://www.jcgt.org/published/0009/03/02/
	v = v * 1664525u + 1013904223u;
	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	v ^= v >> 16u;
	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	return vec3(v) * pow(0.5, 32.0);
}


/*! Generates a pair of pseudo-random numbers.
	\param seed Integers that change with each invocation. They get updated so
		that you can reuse them.
	\return A uniform, pseudo-random point in [0,1)^2.*/
vec2 get_random_numbers(inout random_generator rand) {
	uvec2 s = rand.seed;
	// PCG2D, as described here: https://jcgt.org/published/0009/03/02/
	s = 1664525u * s + 1013904223u;
	s.x += 1664525u * s.y;
	s.y += 1664525u * s.x;
	s ^= (s >> 16u);
	s.x += 1664525u * s.y;
	s.y += 1664525u * s.x;
	s ^= (s >> 16u);
	// Multiply by 2^-32 to get floats
	rand.seed = s;
	return vec2(s) * 2.32830643654e-10;
}
