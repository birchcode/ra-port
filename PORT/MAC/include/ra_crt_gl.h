#ifndef RA_CRT_GL_H
#define RA_CRT_GL_H

#include "ra_aspect_viewport.h"

#include <SDL.h>
#include <stdint.h>

struct RACRTGLConfig {
	int strength;
	int split;
	int subpixel_mask;
	int bgr_panel;
	int test_pattern;
};

bool RA_CRTGL_Init(SDL_Window *window);
void RA_CRTGL_Shutdown(void);
bool RA_CRTGL_IsReady(void);
void RA_CRTGL_GetOutputSize(int *width, int *height);
bool RA_CRTGL_Present(
	uint32_t const *pixels,
	int texture_width,
	int texture_height,
	int content_width,
	RAAspectViewport viewport,
	RACRTGLConfig config);
bool RA_CRTGL_Capture(char const *path);

#endif
