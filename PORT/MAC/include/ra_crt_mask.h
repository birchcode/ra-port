#ifndef RA_CRT_MASK_H
#define RA_CRT_MASK_H

#include "ra_aspect_viewport.h"

static inline unsigned int RA_CRTBloomPixel(unsigned int argb)
{
	unsigned int red = (argb >> 16) & 255U;
	unsigned int green = (argb >> 8) & 255U;
	unsigned int blue = argb & 255U;
	unsigned int luminance = (54U * red + 183U * green + 19U * blue) >> 8;
	unsigned int alpha = luminance > 224U ? (luminance - 224U) / 4U : 0U;
	return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

static inline unsigned int RA_CRTMaskPixel(int x, int y, RAAspectViewport viewport, int logical_height)
{
	if (logical_height <= 0 || x < viewport.x || x >= viewport.x + viewport.w ||
		y < viewport.y || y >= viewport.y + viewport.h) {
		return 0xFFFFFFFFU;
	}

	int phase = (int)(((long long)(y - viewport.y) * logical_height * 256) / viewport.h) & 255;
	int distance = phase > 128 ? phase - 128 : 128 - phase;
	int scan = 255 - (distance * distance * 14 / (128 * 128));
	int dim = scan * 250 / 255;
	int red = dim;
	int green = dim;
	int blue = dim;
	switch ((x - viewport.x) % 3) {
		case 0: red = scan; break;
		case 1: green = scan; break;
		default: blue = scan; break;
	}
	return 0xFF000000U | ((unsigned int)red << 16) | ((unsigned int)green << 8) | (unsigned int)blue;
}

#endif
