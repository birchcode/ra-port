#pragma once

#include <windows.h>

bool MacSDL_SetMode(int width, int height);
bool MacSDL_SetGameMode(int *width, int *height);
bool MacSDL_SetLegacyViewport(bool enabled);
bool MacSDL_SetMovieViewport(bool enabled);
bool MacSDL_SetTitleBackground(unsigned char const *pixels, int width, int height);
bool MacSDL_SetFullscreen(bool enabled);
bool MacSDL_GetFullscreen(void);
void MacSDL_Shutdown(void);
void MacSDL_SetPalette(PALETTEENTRY const *entries, int count);
void MacSDL_Present8(unsigned char const *pixels, int width, int height, int pitch);
void MacSDL_PumpEvents(void);
bool MacSDL_QuitRequested(void);
bool MacSDL_TouchCursorHidden(void);
int MacSDL_ConsumeMobilePointerDrag(int *x, int *y);

void MacSDL_SetCameraInput(bool enabled);
bool MacSDL_ConsumeCameraPan(int *dx, int *dy, int *sidebar);
bool MacSDL_CameraBounds(int *x, int *y, int *width, int *height);

// Full-screen setup screens own their viewport through every return path.
class MacSDLFullMenuViewport {
	bool previous;
public:
	MacSDLFullMenuViewport() : previous(MacSDL_SetLegacyViewport(false)) { MacSDL_SetCameraInput(false); }
	~MacSDLFullMenuViewport() { MacSDL_SetLegacyViewport(previous); }
private:
	MacSDLFullMenuViewport(MacSDLFullMenuViewport const &);
	MacSDLFullMenuViewport &operator=(MacSDLFullMenuViewport const &);
};
bool MacSDL_IsLegacyViewport(void);
