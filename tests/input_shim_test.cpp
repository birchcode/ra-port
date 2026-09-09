#include <assert.h>
#include <string.h>

#include "mac_sdl.h"
#include "windows.h"
#include "mac_sdl_runtime.h"

LRESULT FAR PASCAL _export Windows_Procedure(HWND, UINT, WPARAM, LPARAM)
{
	return 0;
}

static bool camera_available = true;
bool MacSDL_CameraBounds(int *x, int *y, int *w, int *h)
{
	*x = 0; *y = 16; *w = 480; *h = 384;
	return camera_available;
}

static void send_event(SDL_Event event)
{
	assert(SDL_PushEvent(&event) == 1);
	MacSDL_PumpEvents();
}

static void pan_is(int x, int y, int sidebar, bool active = false)
{
	int dx, dy, steps;
	bool panning = MacSDL_ConsumeCameraPan(&dx, &dy, &steps);
	assert(dx == x && dy == y && steps == sidebar);
	assert(panning == (active || x || y || sidebar));
}

static void camera_input_test()
{
	SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
	SDL_setenv("RA_CRT", "0", 1);
	SDL_setenv("RA_FULLSCREEN", "0", 1);
	SDL_setenv("RA_PAN_SPEED", "1", 1);
	SDL_setenv("RA_PAN_REVERSE", "0", 1);
	assert(MacSDL_SetMode(640, 400));
	SDL_Window *window = SDL_GetWindowFromID(1);
	assert(window);
	MacSDL_PumpEvents();
	MacSDL_SetCameraInput(true);
	SDL_WarpMouseInWindow(window, 200, 200);
	MacSDL_PumpEvents();

	SDL_Event wheel;
	memset(&wheel, 0, sizeof(wheel));
	wheel.type = SDL_MOUSEWHEEL;
	wheel.wheel.y = 1;
#if SDL_VERSION_ATLEAST(2, 0, 18)
	wheel.wheel.preciseY = 1;
#endif
	send_event(wheel); pan_is(0, -24, 0);
	SDL_SetModState(KMOD_SHIFT);
	send_event(wheel); pan_is(-24, 0, 0);
	SDL_SetModState(KMOD_NONE);
	wheel.wheel.direction = SDL_MOUSEWHEEL_FLIPPED;
	send_event(wheel); pan_is(0, 24, 0);
	wheel.wheel.direction = SDL_MOUSEWHEEL_NORMAL;
#if SDL_VERSION_ATLEAST(2, 0, 18)
	wheel.wheel.preciseX = 0.125f; wheel.wheel.preciseY = 0.125f;
	send_event(wheel); pan_is(3, -3, 0);
	wheel.wheel.preciseX = 0; wheel.wheel.preciseY = 0.015625f;
	send_event(wheel); pan_is(0, 0, 0);
	send_event(wheel); pan_is(0, 0, 0);
	send_event(wheel); pan_is(0, -1, 0);
	MacSDL_SetCameraInput(false); MacSDL_SetCameraInput(true);
	wheel.wheel.preciseY = 1;
#endif
	SDL_WarpMouseInWindow(window, 550, 200);
	send_event(wheel); pan_is(0, 0, -1);
	SDL_WarpMouseInWindow(window, 200, 5);
	send_event(wheel); pan_is(0, 0, 0);
	SDL_WarpMouseInWindow(window, 200, 200);
	MacSDL_SetCameraInput(false);
	send_event(wheel); pan_is(0, 0, 0);
	MacSDL_SetCameraInput(true);
	camera_available = false;
	send_event(wheel); pan_is(0, 0, 0);
	camera_available = true;

	SDL_Event button;
	memset(&button, 0, sizeof(button));
	button.type = SDL_MOUSEBUTTONDOWN;
	button.button.button = SDL_BUTTON_MIDDLE;
	button.button.x = 200; button.button.y = 200;
	send_event(button); pan_is(0, 0, 0, true);
	SDL_Event motion;
	memset(&motion, 0, sizeof(motion));
	motion.type = SDL_MOUSEMOTION;
	motion.motion.x = 212; motion.motion.y = 207;
	motion.motion.xrel = 12; motion.motion.yrel = 7;
	send_event(motion); pan_is(-12, -7, 0, true);
	send_event(wheel); pan_is(0, 0, 0, true);
	button.type = SDL_MOUSEBUTTONUP;
	send_event(button); pan_is(0, 0, 0);
	send_event(motion); pan_is(0, 0, 0);

	// Focus loss cancels drag and pending movement; it cannot resume on return.
	button.type = SDL_MOUSEBUTTONDOWN;
	send_event(button);
	SDL_Event focus;
	memset(&focus, 0, sizeof(focus));
	focus.type = SDL_WINDOWEVENT;
	focus.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
	send_event(focus); pan_is(0, 0, 0);
	send_event(motion); pan_is(0, 0, 0);

	focus.window.event = SDL_WINDOWEVENT_FOCUS_GAINED;
	send_event(focus);
	// Left selection blocks camera input, including attempts to begin a drag.
	button.button.button = SDL_BUTTON_LEFT;
	send_event(button);
	send_event(wheel); pan_is(0, 0, 0);
	button.button.button = SDL_BUTTON_MIDDLE;
	send_event(button); pan_is(0, 0, 0);
	button.button.button = SDL_BUTTON_LEFT; button.type = SDL_MOUSEBUTTONUP;
	send_event(button);
	// Escape and changing input context discard an unfinished drag.
	button.button.button = SDL_BUTTON_MIDDLE; button.type = SDL_MOUSEBUTTONDOWN;
	send_event(button); send_event(motion);
	SDL_Event escape;
	memset(&escape, 0, sizeof(escape));
	escape.type = SDL_KEYDOWN; escape.key.keysym.sym = SDLK_ESCAPE;
	send_event(escape); pan_is(0, 0, 0);
	send_event(button); send_event(motion);
	MacSDL_SetCameraInput(false); MacSDL_SetCameraInput(true);
	pan_is(0, 0, 0);

	// Relative drag distances follow the scaled viewport, not window pixels.
	SDL_SetWindowSize(window, 1280, 800);
	MacSDL_PumpEvents();
	button.button.x = 400; button.button.y = 400;
	send_event(button);
	motion.motion.xrel = 20; motion.motion.yrel = -10;
	send_event(motion); pan_is(-10, 5, 0, true);
	MacSDL_Shutdown();
}

static char to_ascii(UINT vk, bool shift, bool caps)
{
	BYTE state[256];
	memset(state, 0, sizeof(state));
	if (shift) state[VK_SHIFT] = 0x80;
	if (caps) state[VK_CAPITAL] = 0x01;

	WORD out = 0;
	int result = ToAscii(vk, MapVirtualKey(vk, 0), state, &out, 0);
	assert(result == 1);
	return (char)(out & 0xFF);
}

int main()
{
	assert(to_ascii('A', false, false) == 'a');
	assert(to_ascii('A', true, false) == 'A');
	assert(to_ascii('A', false, true) == 'A');
	assert(to_ascii('A', true, true) == 'a');

	assert(to_ascii('1', false, false) == '1');
	assert(to_ascii('1', true, false) == '!');
	assert(to_ascii(VK_SPACE, false, false) == ' ');
	assert(to_ascii(VK_NONE_BD, false, false) == '-');
	assert(to_ascii(VK_NONE_BD, true, false) == '_');
	assert(to_ascii(VK_NONE_DE, false, false) == '\'');
	assert(to_ascii(VK_NONE_DE, true, false) == '"');

	camera_input_test();

	return 0;
}
