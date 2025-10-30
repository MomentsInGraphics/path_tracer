#include "phase_functions.h"
#include <math.h>

void fit_mie_phase_function(float out_fit_params[4], float droplet_size) {

	// These are the parameters to be fitted
	float g_HG, g_D, alpha, w_D;
	// We use different pre-optimized fits from the paper for different parts
	// of the domain
	float d = droplet_size;
	if (d <= 0.0f) {
		g_HG = 0.0f;
		g_D = 0.0f;
		alpha = 0.0f;
		w_D = 0.0f;
	}
	else if (d <= 0.1f) {
		g_HG = 13.8f * d * d;
		g_D = 1.1456f * d * sinf(9.29044f * d);
		alpha = 250.0f;
		w_D = 0.252977f - 312.983f * powf(d, 4.3f);
	}
	else if (d < 1.5f) {
		float log_d = logf(d);
		g_HG = 0.862f - 0.143f * log_d * log_d;
		g_D = 0.379685f * cosf(1.19692f * cosf(((log_d - 0.238604f) * (log_d + 1.00667f)) / (0.507522f - 0.15677f * log_d)) + 1.37932f * log_d + 0.0625835f) + 0.344213f;
		alpha = 250.0f;
		w_D = 0.146209f * cosf(3.38707f * log_d + 2.11193f) + 0.316072f + 0.0778917f * log_d;
	}
	else if (d < 5.0f) {
		float log_d = logf(d);
		float log_log_d = logf(log_d);
		g_HG = 0.0604931f * log_log_d + 0.940256f;
		g_D = 0.500411f - 0.081287f / (-2.0f * log_d + tanf(log_d) + 1.27551f);
		alpha = 7.30354f * log_d + 6.31675f;
		w_D = 0.026914f * (log_d - cosf(5.68947f * (log_log_d - 0.0292149f))) + 0.376475f;
	}
	else {
		g_HG = expf(-0.0990567f / (d - 1.67154f));
		g_D = expf(-2.20679f / (d + 3.91029f) - 0.428934f);
		alpha = expf(3.62489f - 8.29288f / (d + 5.52825f));
		w_D = expf(-0.599085f / (d - 0.641583f) - 0.665888f);
	}
	// Output the result
	out_fit_params[0] = g_HG;
	out_fit_params[1] = g_D;
	out_fit_params[2] = alpha;
	out_fit_params[3] = w_D;
}
