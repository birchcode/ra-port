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
	RAAspectViewport viewport = {128, 0, 2304, 1440};
	if (expect(RA_CRTMaskPixel(0, 0, viewport, 400), 0xFFFFFFFFU, "pillarbox remains unchanged")) return 1;
	if (expect(RA_CRTMaskPixel(128, 0, viewport, 400), 0xFFE0D1D1U, "scanline edge and red phosphor")) return 1;
	if (expect(RA_CRTMaskPixel(129, 0, viewport, 400), 0xFFD1E0D1U, "green phosphor phase")) return 1;
	return 0;
}
