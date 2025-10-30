#define NK_IMPLEMENTATION
#include "string_utilities.h"
#include <math.h>
#define NK_DTOA double_to_string
#define NK_STRTOD string_to_double
#define NK_COS cosf
#define NK_SIN sinf
static inline float nuklear_inv_sqrtf(float x) { return 1.0f / sqrtf(x); }
#define NK_INV_SQRT nuklear_inv_sqrtf
#include "nuklear.h"
