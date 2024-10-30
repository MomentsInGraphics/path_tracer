#include <stdint.h>
#include <stdbool.h>


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


//! \return The greatest common divisor of the given two integers
uint32_t greatest_common_divisor(uint32_t a, uint32_t b);


//! \return The least common multiple of the given two integers
static inline uint32_t least_common_multiple(uint32_t a, uint32_t b) {
	uint32_t gcd = greatest_common_divisor(a, b);
	// This formulation avoids overflow
	return (a / gcd) * b;
}


/*! Computes the product of two matrices. All matrices use row-major order.
	\param out An m x n matrix to which the product is written. Must not point
		to the same location as lhs or rhs.
	\param lhs An m x k matrix as left-hand side in the product.
	\param rhs A k x n matrix as right-hand side in the product.*/
void mat_mat_mul(float* out, const float* lhs, const float* rhs, uint32_t m, uint32_t k, uint32_t n);


//! Multiplies an m x n matrix (row-major) by an n-entry column vector
//! to output an m-entry vector (non-overlapping with the inputs).
static inline void mat_vec_mul(float* out, const float* mat, const float* vec, uint32_t m, uint32_t n) {
	mat_mat_mul(out, mat, vec, m, n, 1);
}


//! Normalizes the given vector with respect to the 2-norm in place
//! \return false if the squared length underflowed to zero, true otherwise
bool normalize(float* vec, uint32_t entry_count);


/*! Constructs a 3x3 rotation matrix (row-major, to be multiplied onto a column
	vector from the left) from angles in radians. The rotations are performed
	around x-, y- and z-axis, respectively and are applied in this order.*/
void rotation_matrix_from_angles(float out_rotation[3 * 3], const float angles[3]);


//! Computes the inverse of a 4x4 matrix (row major)
void invert_mat4(float out_inv[4 * 4], float mat[4 * 4]);


//! A helper for half_to_float() to modify floats bit by bit
typedef union {
	uint32_t u;
	float f;
} float32_union_t;


/*! Converts a half-precision float (given as an integer with same binary
	representation) into a single-precision float with identical value.
	Originally half_to_float_fast5() as proposed by Fabian Giesen, see:
	https://fgiesen.wordpress.com/2012/03/28/half-to-float-done-quic/ */
static inline float half_to_float(uint16_t half) {
	static const float32_union_t magic = { (254 - 15) << 23 };
	static const float32_union_t was_infnan = { (127 + 16) << 23 };
	float32_union_t o;
	// Copy exponent and mantissa bits
	o.u = (half & 0x7fff) << 13;
	// Make adjustments to the exponent
	o.f *= magic.f;
	// Make sure that inf/NaN survive
	if (o.f >= was_infnan.f)        
		o.u |= 255 << 23;
	// Copy the sign bit
	o.u |= (half & 0x8000) << 16;
	return o.f;
}
