// Exercise the real multiplayer dialog wait, including SDL input and cursor timers.
#include "../CODE/CONQUER.CPP"
// SDL pulls in the C math y1(), which collides with a legacy game global.
#define y1 menu_test_bessel_y1
#include "mac_sdl.h"
#undef y1
#include <mmsystem.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>

static int cursor_updates = 0;
static void CALLBACK cursor_tick(UINT, UINT, DWORD, DWORD, DWORD)
{
	POINT point;
	GetCursorPos(&point);
	if (point.x == 200 && point.y == 100) ++cursor_updates;
}

int main()
{
	SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
	SDL_setenv("RA_CRT", "0", 1);
	SDL_setenv("RA_FULLSCREEN", "0", 1);
	assert(MacSDL_SetMode(640, 400));
	MacSDL_PumpEvents();
	WinTimerClass clock(60);
	// No assets or network peer are needed to exercise the wait itself.
	SampleType = (Sample_Type)0;
	Session.Type = GAME_NORMAL;
	Options.IsPaletteScroll = false;
	SpecialDialog = SDLG_SPECIAL;
	MMRESULT timer = timeSetEvent(16, 1, cursor_tick, 0, TIME_PERIODIC);
	assert(timer);
	SDL_Event motion = {};
	motion.type = SDL_MOUSEMOTION;
	motion.motion.x = 200;
	motion.motion.y = 100;
	assert(SDL_PushEvent(&motion) == 1);
	SDL_Event key = {};
	key.type = SDL_KEYDOWN;
	key.key.keysym.sym = SDLK_ESCAPE;
	assert(SDL_PushEvent(&key) == 1);
	long frame = Frame;
	FrameTimer = 12; // 200 ms, deliberately slower than cursor refresh.
	DWORD start = GetTickCount();
	Sync_Delay();
	DWORD elapsed = GetTickCount() - start;
	timeKillEvent(timer);
	printf("Dialog wait: %lu ms, %d cursor updates, simulation frame delta %ld\n",
		(unsigned long)elapsed, cursor_updates, (long)Frame - frame);
	fflush(stdout);
	assert(cursor_updates >= 5);
	assert(cursor_updates <= (int)(elapsed / 16 + 1));
	assert(elapsed >= 150 && elapsed < 2000);
	assert(Frame == frame);
	// Pumping must leave the key queued for the dialog to consume.
	MSG message;
	bool escape_queued = false;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
		if (message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE) escape_queued = true;
	}
	assert(escape_queued);
	MacSDL_Shutdown();
	return 0;
}
