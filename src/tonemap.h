#pragma once

/*! Applies the Khronos PBR neutral tone mapper to the given Rec. 709 color
	(a.k.a. linear sRGB) and returns a tonemapped Rec. 709 color. For details
	see:
	https://github.com/KhronosGroup/ToneMapping/blob/main/PBR_Neutral/README.md */
void tonemapper_khronos_pbr_neutral(float* out_color, const float* in_color);


/*! An implementation of the ACES tonemapper using an approximation derived by
	Stephen Hill and found here:
	https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl */
void tonemapper_aces(float* out_color, const float* color);
