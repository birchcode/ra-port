// Real engine geometry, picking and offscreen rendering without game assets.
#include "function.h"
#include "mac_sdl_runtime.h"
#define y1 camera_test_bessel_y1
#include "mac_sdl.h"
#undef y1
#undef NDEBUG
#include <assert.h>
#include <stdio.h>

int main()
{
	SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
	SDL_setenv("RA_CRT", "0", 1);
	SDL_setenv("RA_FULLSCREEN", "0", 1);
	assert(MacSDL_SetMode(640, 400));
	MacSDL_PumpEvents();
	ScreenWidth = 640; ScreenHeight = 400;
	VisiblePage.Init(640, 400, 0, 0, GBC_NONE);
	HiddenPage.Init(640, 400, 0, 0, GBC_NONE);
	SeenBuff.Attach(&VisiblePage, 0, 0, 640, 400);
	HidPage.Attach(&HiddenPage, 0, 0, 640, 400);
	Set_Logic_Page(HidPage);
	Map.Size = MAP_CELL_TOTAL;
	Map.Alloc_Cells();
	Map.Init_Cells();
	Map.CellRedraw.Resize(MAP_CELL_TOTAL);
	Map.MapCellX = Map.MapCellY = 1;
	Map.MapCellWidth = Map.MapCellHeight = 100;
	ScenarioInit = 1;
	Map.Set_View_Dimensions(0, 16, 20, 16);
	Map.Set_Tactical_Position(Cell_Coord(XY_Cell(10, 10)));
	ScenarioInit = 0;
	GameActive = true; GameInFocus = true;
	SpecialDialog = SDLG_NONE;
	MacSDL_SetCameraInput(true);
	int width = Map.TacLeptonWidth, height = Map.TacLeptonHeight;
	CELL distant = XY_Cell(35, 30);
	assert(!Map.In_View(distant));
	SDL_WarpMouseInWindow(SDL_GetWindowFromID(1), 200, 200);
	MacSDL_PumpEvents();
	SDL_Event wheel = {};
	wheel.type = SDL_MOUSEWHEEL; wheel.wheel.y = 100;
#if SDL_VERSION_ATLEAST(2, 0, 18)
	wheel.wheel.preciseY = 100;
#endif
	assert(SDL_PushEvent(&wheel) == 1);
	MacSDL_PumpEvents();
	int x, y;
	assert(MacSDL_ConsumeCameraZoom(&x, &y));
	Map.Apply_Camera_Zoom();
	assert(Map.TacLeptonWidth == width * 2 && Map.TacLeptonHeight == height * 2);
	assert(Map.In_View(distant));
	assert(WindowList[WINDOW_TACTICAL][WINDOWWIDTH] == 480);
	assert(Map.Coord_To_Pixel(Cell_Coord(distant), x, y));
	assert(Map.Click_Cell_Calc(x, y + 16) == distant);
	assert(Coord_Cell(Map.Pixel_To_Coord(x, y + 16)) == distant);
	assert(Map.Pixel_To_Coord(480, 100) == 0); // Sidebar is outside the battlefield.

	// Tooltip cleanup can cover more than the legacy 36-cell scratch buffer
	// when zoomed out. Exercise the same Help_Text -> Refresh_Cells crash path.
	int overlap_counts[] = {0, 35, 36, 58};
	for (int sidebar = 0; sidebar <= 1; ++sidebar) {
		for (unsigned test = 0; test < sizeof(overlap_counts) / sizeof(overlap_counts[0]); ++test) {
			int count = overlap_counts[test];
			Map.CellRedraw.Reset();
			if (sidebar) Map.OverlapList[0] = REFRESH_SIDEBAR;
			for (int i = 0; i < count; ++i) Map.OverlapList[i + sidebar] = (i / 20) * MAP_CELL_W + i % 20;
			Map.OverlapList[count + sidebar] = REFRESH_EOL;
			Map.HelpClass::Text = 1;
			Map.Help_Text(TXT_NONE, 0, 0);
			for (int i = 0; i < count; ++i)
				assert(Map.Is_Cell_Flagged(Coord_Cell(Map.TacticalCoord) + Map.OverlapList[i + sidebar]));
		}
	}

	// A selection rectangle beyond the original view must render after reduction.
	HidPage.Clear(17);
	Map.IsRubberBand = true;
	Map.BandX = 600; Map.BandY = 300; Map.NewX = 700; Map.NewY = 400;
	Map.DisplayClass::Draw_It(true);
	assert(HidPage.Get_Width() == 640 && HidPage.Get_Height() == 400);
	assert(WindowList[WINDOW_TACTICAL][WINDOWWIDTH] == 480);
	assert(HidPage.Get_Pixel(300, 166) == WHITE);
	assert(HidPage.Get_Pixel(550, 200) == 17); // Sidebar is unchanged.
	Map.IsRubberBand = false;

	wheel.wheel.direction = SDL_MOUSEWHEEL_FLIPPED;
	assert(SDL_PushEvent(&wheel) == 1);
	MacSDL_PumpEvents();
	assert(MacSDL_ConsumeCameraZoom(&x, &y));
	Map.Apply_Camera_Zoom();
	assert(MacSDL_GetCameraZoom() == 1);
	assert(Map.TacLeptonWidth == width && Map.TacLeptonHeight == height);
	assert(!Map.In_View(distant));
	// A small map limits zoom before the view can extend beyond its boundaries.
	Map.MapCellWidth = 30; Map.MapCellHeight = 24;
	wheel.wheel.direction = SDL_MOUSEWHEEL_NORMAL;
	assert(SDL_PushEvent(&wheel) == 1);
	MacSDL_PumpEvents();
	assert(MacSDL_ConsumeCameraZoom(&x, &y));
	Map.Apply_Camera_Zoom();
	assert(MacSDL_GetCameraZoom() > 0.5);
	assert(Map.TacLeptonWidth <= Cell_To_Lepton(Map.MapCellWidth));
	assert(Map.TacLeptonHeight <= Cell_To_Lepton(Map.MapCellHeight));
	Map.Set_Tactical_Position(Cell_Coord(XY_Cell(100, 100)));
	assert(Coord_X(Map.DesiredTacticalCoord) + Map.TacLeptonWidth <= Cell_To_Lepton(31));
	assert(Coord_Y(Map.DesiredTacticalCoord) + Map.TacLeptonHeight <= Cell_To_Lepton(25));
	Map.TacticalCoord = Map.DesiredTacticalCoord;
	COORDINATE center = Coord_Add(Map.TacticalCoord, XY_Coord(CELL_LEPTON_W * 5 + CELL_LEPTON_W / 2,
		CELL_LEPTON_H * 5 + CELL_LEPTON_H / 2));
	assert(Map.Coord_To_Pixel(center, x, y));
	assert(Coord_Cell(Map.Pixel_To_Coord(x, y + 16)) == Coord_Cell(center));
	Map.DisplayClass::Draw_It(true);
	assert(HidPage.Get_Pixel(550, 200) == 17);
	printf("Camera zoom: expanded map visibility, picking, reduced rendering, tooltip cleanup, sidebar and reset passed.\n");
	MacSDL_Shutdown();
	return 0;
}
