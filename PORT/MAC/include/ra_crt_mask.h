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

static inline int RA_CRTBrightnessBoostAlpha(int mask_strength)
{
	int strength = RA_ClampInt(mask_strength, 0, 100);
	int inactive = 250 - strength * 250 / 100;
	int average = 255 + 2 * inactive;
	return RA_ClampInt((765 * 255 / average) - 255 + strength * 40 / 100, 0, 510);
}

static inline unsigned int RA_CRTMaskPixelStrength(
	int x,
	int y,
	RAAspectViewport viewport,
	int logical_height,
	int mask_strength)
{
	if (logical_height <= 0 || x < viewport.x || x >= viewport.x + viewport.w ||
		y < viewport.y || y >= viewport.y + viewport.h) {
		return 0xFFFFFFFFU;
	}

	int triad = (x - viewport.x) / 3;
	int phase = ((int)(((long long)(y - viewport.y) * logical_height * 256) / viewport.h) +
		((triad & 1) ? 128 : 0)) & 255;
	int strength = RA_ClampInt(mask_strength, 0, 100);
	int distance = phase > 128 ? phase - 128 : 128 - phase;
	int scan_loss = 8 + strength / 3;
	int scan = 255 - (distance * distance * scan_loss / (128 * 128));
	if (distance > 96) scan -= 8 + strength / 5;
	int loss = 5 + strength * 250 / 100;
	int dim = scan * (255 - loss) / 255;
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

static inline unsigned int RA_CRTMaskPixel(int x, int y, RAAspectViewport viewport, int logical_height)
{
	return RA_CRTMaskPixelStrength(x, y, viewport, logical_height, 0);
}

#endif
