#include "math_utilities.h"
#include <math.h>
#include <string.h>


uint32_t greatest_common_divisor(uint32_t a, uint32_t b) {
	// Use the Euclidean algorithm
	while (b != 0) {
		uint32_t t = b;
		b = a % b;
		a = t;
	}
	return a;
}


void mat_mat_mul(float* out, const float* lhs, const float* rhs, uint32_t m, uint32_t k, uint32_t n) {
	for (uint32_t i = 0; i != m; ++i) {
		for (uint32_t j = 0; j != n; ++j) {
			float entry = 0.0;
			for (uint32_t l = 0; l != k; ++l) {
				entry += lhs[i * k + l] * rhs[l * n + j];
			}
			out[i * n + j] = entry;
		}
	}
}


bool normalize(float* vec, uint32_t entry_count) {
	float length_sq = 0.0f;
	for (uint32_t i = 0; i != entry_count; ++i)
		length_sq += vec[i] * vec[i];
	if (length_sq != 0.0) {
		float scale = 1.0f / sqrtf(length_sq);
		for (uint32_t i = 0; i != entry_count; ++i)
			vec[i] *= scale;
		return true;
	}
	return false;
}


void rotation_matrix_from_angles(float out_rotation[3 * 3], const float angles[3]) {
	float sins[3] = { sinf(angles[0]), sinf(angles[1]), sinf(angles[2]) };
	float coss[3] = { cosf(angles[0]), cosf(angles[1]), cosf(angles[2]) };
	float rot_x[3 * 3] = {
		1.0f, 0.0f, 0.0f,
		0.0f, coss[0], sins[0],
		0.0f, -sins[0], coss[0],
	};
	float rot_y[3 * 3] = {
		coss[1], 0.0f, sins[1],
		0.0f, 1.0f, 0.0f,
		-sins[1], 0.0f, coss[1],
	};
	float rot_z[3 * 3] = {
		coss[2], sins[2], 0.0f,
		-sins[2], coss[2], 0.0f,
		0.0f, 0.0f, 1.0f,
	};
	float rot_xy[3 * 3];
	mat_mat_mul(rot_xy, rot_y, rot_x, 3, 3, 3);
	mat_mat_mul(out_rotation, rot_z, rot_xy, 3, 3, 3);
}


static inline double determinant_mat4(const double mat[4 * 4]) {
	double result;
	result =  24.0f * mat[0] * mat[5] * mat[10] * mat[15];
	result -= 24.0f * mat[0] * mat[5] * mat[11] * mat[14];
	result -= 24.0f * mat[0] * mat[6] * mat[9] * mat[15];
	result += 24.0f * mat[0] * mat[6] * mat[11] * mat[13];
	result += 24.0f * mat[0] * mat[7] * mat[9] * mat[14];
	result -= 24.0f * mat[0] * mat[7] * mat[10] * mat[13];
	result -= 24.0f * mat[1] * mat[4] * mat[10] * mat[15];
	result += 24.0f * mat[1] * mat[4] * mat[11] * mat[14];
	result += 24.0f * mat[1] * mat[6] * mat[8] * mat[15];
	result -= 24.0f * mat[1] * mat[6] * mat[11] * mat[12];
	result -= 24.0f * mat[1] * mat[7] * mat[8] * mat[14];
	result += 24.0f * mat[1] * mat[7] * mat[10] * mat[12];
	result += 24.0f * mat[2] * mat[4] * mat[9] * mat[15];
	result -= 24.0f * mat[2] * mat[4] * mat[11] * mat[13];
	result -= 24.0f * mat[2] * mat[5] * mat[8] * mat[15];
	result += 24.0f * mat[2] * mat[5] * mat[11] * mat[12];
	result += 24.0f * mat[2] * mat[7] * mat[8] * mat[13];
	result -= 24.0f * mat[2] * mat[7] * mat[9] * mat[12];
	result -= 24.0f * mat[3] * mat[4] * mat[9] * mat[14];
	result += 24.0f * mat[3] * mat[4] * mat[10] * mat[13];
	result += 24.0f * mat[3] * mat[5] * mat[8] * mat[14];
	result -= 24.0f * mat[3] * mat[5] * mat[10] * mat[12];
	result -= 24.0f * mat[3] * mat[6] * mat[8] * mat[13];
	result += 24.0f * mat[3] * mat[6] * mat[9] * mat[12];
	return result;
}


static inline void adjoint_mat4(double result[4 * 4], const double mat[4 * 4]) {
	result[15] =  6.0f * mat[0] * mat[5] * mat[10];
	result[11] = -6.0f * mat[0] * mat[5] * mat[11];
	result[15] -= 6.0f * mat[0] * mat[6] * mat[9];
	result[7] =  6.0f * mat[0] * mat[6] * mat[11];
	result[11] += 6.0f * mat[0] * mat[7] * mat[9];
	result[7] -= 6.0f * mat[0] * mat[7] * mat[10];
	result[15] -= 6.0f * mat[1] * mat[4] * mat[10];
	result[11] += 6.0f * mat[1] * mat[4] * mat[11];
	result[15] += 6.0f * mat[1] * mat[6] * mat[8];
	result[3] = -6.0f * mat[1] * mat[6] * mat[11];
	result[11] -= 6.0f * mat[1] * mat[7] * mat[8];
	result[3] += 6.0f * mat[1] * mat[7] * mat[10];
	result[15] += 6.0f * mat[2] * mat[4] * mat[9];
	result[7] -= 6.0f * mat[2] * mat[4] * mat[11];
	result[15] -= 6.0f * mat[2] * mat[5] * mat[8];
	result[3] += 6.0f * mat[2] * mat[5] * mat[11];
	result[7] += 6.0f * mat[2] * mat[7] * mat[8];
	result[3] -= 6.0f * mat[2] * mat[7] * mat[9];
	result[11] -= 6.0f * mat[3] * mat[4] * mat[9];
	result[7] += 6.0f * mat[3] * mat[4] * mat[10];
	result[11] += 6.0f * mat[3] * mat[5] * mat[8];
	result[3] -= 6.0f * mat[3] * mat[5] * mat[10];
	result[7] -= 6.0f * mat[3] * mat[6] * mat[8];
	result[3] += 6.0f * mat[3] * mat[6] * mat[9];
	result[14] = -6.0f * mat[0] * mat[5] * mat[14];
	result[10] =  6.0f * mat[0] * mat[5] * mat[15];
	result[14] += 6.0f * mat[0] * mat[6] * mat[13];
	result[6] = -6.0f * mat[0] * mat[6] * mat[15];
	result[10] -= 6.0f * mat[0] * mat[7] * mat[13];
	result[6] += 6.0f * mat[0] * mat[7] * mat[14];
	result[14] += 6.0f * mat[1] * mat[4] * mat[14];
	result[10] -= 6.0f * mat[1] * mat[4] * mat[15];
	result[14] -= 6.0f * mat[1] * mat[6] * mat[12];
	result[2] =  6.0f * mat[1] * mat[6] * mat[15];
	result[10] += 6.0f * mat[1] * mat[7] * mat[12];
	result[2] -= 6.0f * mat[1] * mat[7] * mat[14];
	result[14] -= 6.0f * mat[2] * mat[4] * mat[13];
	result[6] += 6.0f * mat[2] * mat[4] * mat[15];
	result[14] += 6.0f * mat[2] * mat[5] * mat[12];
	result[2] -= 6.0f * mat[2] * mat[5] * mat[15];
	result[6] -= 6.0f * mat[2] * mat[7] * mat[12];
	result[2] += 6.0f * mat[2] * mat[7] * mat[13];
	result[10] += 6.0f * mat[3] * mat[4] * mat[13];
	result[6] -= 6.0f * mat[3] * mat[4] * mat[14];
	result[10] -= 6.0f * mat[3] * mat[5] * mat[12];
	result[2] += 6.0f * mat[3] * mat[5] * mat[14];
	result[6] += 6.0f * mat[3] * mat[6] * mat[12];
	result[2] -= 6.0f * mat[3] * mat[6] * mat[13];
	result[13] =  6.0f * mat[0] * mat[9] * mat[14];
	result[9] = -6.0f * mat[0] * mat[9] * mat[15];
	result[13] -= 6.0f * mat[0] * mat[10] * mat[13];
	result[5] =  6.0f * mat[0] * mat[10] * mat[15];
	result[9] += 6.0f * mat[0] * mat[11] * mat[13];
	result[5] -= 6.0f * mat[0] * mat[11] * mat[14];
	result[13] -= 6.0f * mat[1] * mat[8] * mat[14];
	result[9] += 6.0f * mat[1] * mat[8] * mat[15];
	result[13] += 6.0f * mat[1] * mat[10] * mat[12];
	result[1] = -6.0f * mat[1] * mat[10] * mat[15];
	result[9] -= 6.0f * mat[1] * mat[11] * mat[12];
	result[1] += 6.0f * mat[1] * mat[11] * mat[14];
	result[13] += 6.0f * mat[2] * mat[8] * mat[13];
	result[5] -= 6.0f * mat[2] * mat[8] * mat[15];
	result[13] -= 6.0f * mat[2] * mat[9] * mat[12];
	result[1] += 6.0f * mat[2] * mat[9] * mat[15];
	result[5] += 6.0f * mat[2] * mat[11] * mat[12];
	result[1] -= 6.0f * mat[2] * mat[11] * mat[13];
	result[9] -= 6.0f * mat[3] * mat[8] * mat[13];
	result[5] += 6.0f * mat[3] * mat[8] * mat[14];
	result[9] += 6.0f * mat[3] * mat[9] * mat[12];
	result[1] -= 6.0f * mat[3] * mat[9] * mat[14];
	result[5] -= 6.0f * mat[3] * mat[10] * mat[12];
	result[1] += 6.0f * mat[3] * mat[10] * mat[13];
	result[12] = -6.0f * mat[4] * mat[9] * mat[14];
	result[8] =  6.0f * mat[4] * mat[9] * mat[15];
	result[12] += 6.0f * mat[4] * mat[10] * mat[13];
	result[4] = -6.0f * mat[4] * mat[10] * mat[15];
	result[8] -= 6.0f * mat[4] * mat[11] * mat[13];
	result[4] += 6.0f * mat[4] * mat[11] * mat[14];
	result[12] += 6.0f * mat[5] * mat[8] * mat[14];
	result[8] -= 6.0f * mat[5] * mat[8] * mat[15];
	result[12] -= 6.0f * mat[5] * mat[10] * mat[12];
	result[0] =  6.0f * mat[5] * mat[10] * mat[15];
	result[8] += 6.0f * mat[5] * mat[11] * mat[12];
	result[0] -= 6.0f * mat[5] * mat[11] * mat[14];
	result[12] -= 6.0f * mat[6] * mat[8] * mat[13];
	result[4] += 6.0f * mat[6] * mat[8] * mat[15];
	result[12] += 6.0f * mat[6] * mat[9] * mat[12];
	result[0] -= 6.0f * mat[6] * mat[9] * mat[15];
	result[4] -= 6.0f * mat[6] * mat[11] * mat[12];
	result[0] += 6.0f * mat[6] * mat[11] * mat[13];
	result[8] += 6.0f * mat[7] * mat[8] * mat[13];
	result[4] -= 6.0f * mat[7] * mat[8] * mat[14];
	result[8] -= 6.0f * mat[7] * mat[9] * mat[12];
	result[0] += 6.0f * mat[7] * mat[9] * mat[14];
	result[4] += 6.0f * mat[7] * mat[10] * mat[12];
	result[0] -= 6.0f * mat[7] * mat[10] * mat[13];
}


void invert_mat4(float out_inv[4 * 4], const float mat[4 * 4]) {
	double copy[4 * 4];
	for (uint32_t i = 0; i != 4 * 4; ++i)
		copy[i] = mat[i];
	double det = determinant_mat4(copy);
	double inv[4 * 4];
	adjoint_mat4(inv, copy);
	double scale = 4.0f / det;
	for (uint32_t i = 0; i != 4 * 4; ++i)
		out_inv[i] = (float) (inv[i] * scale);
}


void pad_mat3x4_to_mat4(float out_padded[3 * 4], const float mat[3 * 4]) {
	memcpy(out_padded, mat, sizeof(float) * 3 * 4);
	out_padded[12] = out_padded[13] = out_padded[14] = 0.0f;
	out_padded[15] = 1.0f;
}
