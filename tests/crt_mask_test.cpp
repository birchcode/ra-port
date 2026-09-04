#include "ra_crt_mask.h"

#include <stdio.h>

static int expect(unsigned int actual, unsigned int expected, char const *message)
{
	if (actual != expected) {
		fprintf(stderr, "FAIL: %s: got %08x expected %08x\n", message, actual, expected);
		return 1;
	}
	return 0;
}

int main(void)
{
	if (expect(RA_CRTBloomPixel(0xFF808080U), 0x00808080U, "ordinary terrain does not bloom")) return 1;
	if (expect(RA_CRTBloomPixel(0xFFFFFFFFU), 0x07FFFFFFU, "only highlights bloom subtly")) return 1;
	RAAspectViewport viewport = {128, 0, 2304, 1440};
	if (expect(RA_CRTMaskPixel(0, 0, viewport, 400), 0xFFFFFFFFU, "pillarbox remains unchanged")) return 1;
	if (expect(RA_CRTMaskPixel(128, 0, viewport, 400), 0xFFF1ECECU, "soft beam edge and red phosphor")) return 1;
	if (expect(RA_CRTMaskPixel(129, 0, viewport, 400), 0xFFECF1ECU, "stationary green phosphor phase")) return 1;
	if (expect(RA_CRTMaskPixel(128, 2, viewport, 400), 0xFFFFFAFAU, "beam center remains bright")) return 1;
	if (expect(RA_CRTMaskPixelStrength(128, 0, viewport, 400, 100), 0xFF8D0000U, "maximum mask keeps beam edges readable")) return 1;
	if (expect(RA_CRTMaskPixelStrength(128, 2, viewport, 400, 100), 0xFFFE0000U, "maximum mask isolates red phosphor")) return 1;
	if (expect(RA_CRTMaskPixelStrength(129, 2, viewport, 400, 100), 0xFF00FE00U, "maximum mask isolates green phosphor")) return 1;
	if (expect(RA_CRTMaskPixelStrength(130, 2, viewport, 400, 100), 0xFF0000FEU, "maximum mask isolates blue phosphor")) return 1;
	return 0;
}
