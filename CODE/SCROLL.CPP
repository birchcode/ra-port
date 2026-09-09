/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#include	"function.h"
#if defined(RA_GAME_RUNTIME)
#include "mac_sdl_runtime.h"

bool MacSDL_CameraBounds(int *x, int *y, int *width, int *height)
{
	if (!GameActive || !GameInFocus || InMovie || ScenarioInit || SpecialDialog != SDLG_NONE) return false;
	*x = WindowList[WINDOW_TACTICAL][WINDOWX];
	*y = WindowList[WINDOW_TACTICAL][WINDOWY];
	*width = WindowList[WINDOW_TACTICAL][WINDOWWIDTH];
	*height = WindowList[WINDOW_TACTICAL][WINDOWHEIGHT];
	return *width > 0 && *height > 0;
}
#endif

#ifdef WIN32
#define	SCROLL_DELAY	1
#else
#define	SCROLL_DELAY	2
#endif

CDTimerClass<SystemTimerClass> ScrollClass::Counter;


/***********************************************************************************************
 * ScrollClass::ScrollClass -- Constructor for the scroll class object.                        *
 *                                                                                             *
 *    This is the constructor for the scroll class object.                                     *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/10/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
ScrollClass::ScrollClass(void) :
	IsAutoScroll(true)
{
	Counter = SCROLL_DELAY;
	Inertia = 0;
}


/***********************************************************************************************
 * ScrollClass::AI -- Handles scroll AI processing.                                            *
 *                                                                                             *
 *    This routine is called every game frame for purposes of input processing.                *
 *                                                                                             *
 * INPUT:   input    -- Reference to the keyboard/mouse event that just occurred.              *
 *                                                                                             *
 *          x,y      -- The mouse coordinates.                                                 *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/10/1995 JLB : Created.                                                                 *
 *   08/10/1995 JLB : Revamped for free smooth scrolling.                                      *
 *   08/25/1995 JLB : Handles new scrolling option.                                            *
 *=============================================================================================*/
#define	EVA_WIDTH		80
void ScrollClass::AI(KeyNumType &input, int x, int y)
{
	bool		player_scrolled=false;
	static 	DirType	direction;
	static unsigned long last_scroll_tick = 0xffffffffUL;
	int		rate;
	bool camera_panning = false;
	bool edge_scroll = true;
#if defined(RA_GAME_RUNTIME)
	char const *edge_setting = getenv("RA_EDGE_SCROLL");
	edge_scroll = !edge_setting || strcmp(edge_setting, "0") != 0;
	if (!edge_scroll) Inertia = 0;
	int pan_x, pan_y, sidebar;
	camera_panning = MacSDL_ConsumeCameraPan(&pan_x, &pan_y, &sidebar);
	if (camera_panning && !IsRubberBand && !IsTentative) {
		Inertia = 0;
		if (pan_x || pan_y) {
			// Both axes must use the pending position: Scroll_Map uses the last rendered
			// position, so two cardinal calls would discard the first axis.
			int next_x = Coord_X(DesiredTacticalCoord) + (pan_x < 0 ? -1 : 1) * Pixel_To_Lepton(abs(pan_x));
			int next_y = Coord_Y(DesiredTacticalCoord) + (pan_y < 0 ? -1 : 1) * Pixel_To_Lepton(abs(pan_y));
			next_x = Bound(next_x, Cell_To_Lepton(MapCellX), Cell_To_Lepton(MapCellX + MapCellWidth) - TacLeptonWidth);
			next_y = Bound(next_y, Cell_To_Lepton(MapCellY), Cell_To_Lepton(MapCellY + MapCellHeight) - TacLeptonHeight);
			Set_Tactical_Position(XY_Coord(next_x, next_y));
			// Wheel/drag jumps can exceed the legacy renderer's two-cell dirty strip.
			Flag_To_Redraw(true);
		}
		for (int step = 0; step < min(abs(sidebar), 8); ++step) {
			Map.SidebarClass::Scroll(sidebar < 0, -1);
		}
	}
#endif


	/*
	**	If rubber band mode is in progress, then don't allow scrolling of the tactical map.
	*/
	if (!IsRubberBand && !camera_panning && edge_scroll /*&& !IsTentative*/) {

		/*
		**	Special check to not scroll within the special no-scroll regions.
		*/
		bool noscroll = false;

		if (!noscroll) {
			int screen_width = SeenBuff.Get_Width();
			int screen_height = SeenBuff.Get_Height();
			bool at_screen_edge = (y == 0 || x == 0 || x >= SeenBuff.Get_Width()-1 || y >= SeenBuff.Get_Height()-1);

			/*
			**	Verify that the mouse is over a scroll region.
			*/
			if (Inertia || at_screen_edge) {
				if (at_screen_edge) {

					player_scrolled=true;

					/*
					**	Adjust the mouse coordinates to emphasize the
					**	cardinal directions over the diagonals.
					*/
					int altx = x;
					if (altx < 50 * RESFACTOR) altx -= ((50 * RESFACTOR)-altx);
					altx = max(altx, 0);
					if (altx > screen_width-(50 * RESFACTOR)) altx += altx-(screen_width-(50 * RESFACTOR));
					altx = min(altx, screen_width);
					if (altx > (50 * RESFACTOR) && altx < screen_width-(50 * RESFACTOR)) {
						altx += ((screen_width/2)-altx)/2;
					}

					int alty = y;
					if (alty < (50 * RESFACTOR)) alty -= (50 * RESFACTOR)-alty;
					alty = max(alty, 0);
					if (alty > screen_height-(50 * RESFACTOR)) alty += alty-(screen_height-(50 * RESFACTOR));
					alty = min(alty, screen_height);

					direction = (DirType)Desired_Facing256(screen_width/2, screen_height/2, altx, alty);
				}

				int control = Dir_Facing(direction);

				/*
				**	The mouse is over a scroll region so set the mouse shape accordingly if the map
				**	can be scrolled in the direction indicated.
				*/
				static int _rate[9] = {
					0x00E0*RESFACTOR,
					0x00C0*RESFACTOR,
					0x00A0*RESFACTOR,
					0x0080*RESFACTOR,
					0x0060*RESFACTOR,
					0x0040*RESFACTOR,
					0x0020*RESFACTOR,
					0x0010*RESFACTOR,
					0x0008*RESFACTOR
				};
				if (Debug_Map) {
					rate = Options.ScrollRate+1;
				} else {
					rate = 8-Inertia;
				}

				if (rate < Options.ScrollRate+1) {
					rate = Options.ScrollRate+1;
					Inertia = 8-rate;
				}

				/*
				**	Increase the scroll rate if the mouse button is held down.
				*/
	//			if (Keyboard->Down(KN_LMOUSE)) {
	//				rate = Bound(rate-3, 0, 4);
	//			}
				if (Keyboard->Down(KN_RMOUSE)) {
					rate = Bound(rate+1, 4, (int)(sizeof(_rate)/sizeof(_rate[0]))-1);
				}

				/*
				**	If options indicate that scrolling should be forced to
				**	one of the 8 facings, then adjust the direction value
				**	accordingly.
				*/
				direction = Facing_Dir(Dir_Facing(direction));

				int distance = _rate[rate]/2;

				if (!Scroll_Map(direction, distance, false)) {
					Override_Mouse_Shape((MouseType)(MOUSE_NO_N+control), false);
				} else {
					Override_Mouse_Shape((MouseType)(MOUSE_N+control), false);

					/*
					**	If the mouse button is pressed or auto scrolling is active, then scroll
					**	the map if the delay counter indicates.
					*/
					if (Keyboard->Down(KN_LMOUSE) || IsAutoScroll) {
						distance = _rate[rate];
						bool scrolled_this_tick = false;

						if (Debug_Map) {
							if (last_scroll_tick != TickCount) {
								scrolled_this_tick = Scroll_Map(direction, distance, true);
								last_scroll_tick = TickCount;
							}
							Counter = SCROLL_DELAY;
						} else {
							distance = _rate[rate];
							if (last_scroll_tick != TickCount) {
								scrolled_this_tick = Scroll_Map(direction, distance, true);
								last_scroll_tick = TickCount;
							}

							if (Counter == 0 && player_scrolled && scrolled_this_tick) {
								Counter = SCROLL_DELAY;
								Inertia++;
							}
						}
					}
				}

			}

			if (!Debug_Map && !player_scrolled) {
				if (!Counter) {
					Inertia--;
					if (Inertia<0) Inertia++;
					Counter = SCROLL_DELAY;
				}
			}

		}
	}

	HelpClass::AI(input, x, y);
}


/***********************************************************************************************
 * ScrollClass::Set_Autoscroll -- Turns autoscrolling on or off.                               *
 *                                                                                             *
 *    This routine controls the autoscrolling setting. Autoscroll, when active, will cause the *
 *    map to scroll if the mouse is held over the scroll region. This is regardless of whether *
 *    any mouse button is held down or not.                                                    *
 *                                                                                             *
 * INPUT:   control  -- Should the autoscroll be turned on?                                    *
 *                      0  = turn off                                                          *
 *                      1  = turn on                                                           *
 *                      -1 = toggle current setting                                            *
 *                                                                                             *
 * OUTPUT:  Returns with the old setting of the autoscroll flag.                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/10/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
bool ScrollClass::Set_Autoscroll(int control)
{
	bool old = IsAutoScroll;

	switch (control) {
		case -1:
			IsAutoScroll = !IsAutoScroll;
			break;

		default:
			IsAutoScroll = control;
			break;
	}
	return(old);
}
