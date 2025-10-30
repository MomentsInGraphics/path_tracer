#pragma once


/*! Produces parameters for the fit to the Mie-scattering phase function
	described in https://doi.org/10.1145/3587421.3595409 using the given
	droplet diameter in micrometers (at most 50). A droplet size of zero or
	less gives parameters for a constant phase function.*/
void fit_mie_phase_function(float out_fit_params[4], float droplet_size);
