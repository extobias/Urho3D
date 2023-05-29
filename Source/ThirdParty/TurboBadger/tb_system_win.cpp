// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2014, Emil Segerås      ==
// ==                     See tb_core.h for more information.                    ==
// ================================================================================

#include "tb_system.h"

#ifdef TB_SYSTEM_WINDOWS

#include <Windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <math.h>

#ifdef TB_RUNTIME_DEBUG_INFO

void TBDebugOut(const char *str)
{
	OutputDebugString(str);
}

#endif // TB_RUNTIME_DEBUG_INFO

#define MM_TO_INCH 1.0f / 25.4f

namespace tb {

// == TBSystem ========================================

double TBSystem::GetTimeMS()
{
	return timeGetTime();
}

// Implementation currently done in port_glfw.cpp.
// Windows timer suck. Glut timers suck too (can't be canceled) but that will do for now.
void TBSystem::RescheduleTimer(double fire_time)
{
}

int TBSystem::GetLongClickDelayMS()
{
	return 500;
}

int TBSystem::GetPanThreshold()
{
	return 5 * GetDPI() / 96;
}

int TBSystem::GetPixelsPerLine()
{
	return 40 * GetDPI() / 96;
}

int TBSystem::GetDPI()
{
	HDC hdc = GetDC(nullptr);
	int DPI_x = GetDeviceCaps(hdc, LOGPIXELSX);

	int width_mm = GetDeviceCaps(hdc, HORZSIZE);
	int height_mm = GetDeviceCaps(hdc, VERTSIZE);

	int width_res = GetDeviceCaps(hdc, HORZRES);
	int height_res = GetDeviceCaps(hdc, VERTRES);

	int diag_pixels = sqrt(width_res * width_res + height_res * height_res);
	int diag_inches = MM_TO_INCH * sqrt(width_mm * width_mm + height_mm * height_mm);

	TBDebugPrint("TBSystem::GetDPI: mm <%i, %i> res <%i, %i> dpi <%i>\n", 
	 		width_mm, height_mm, width_res, height_res, (int)diag_pixels / diag_inches);
	ReleaseDC(nullptr, hdc);
#if 0 // TEST CODE!
	DPI_x *= 2;
#endif

	return (int)diag_pixels / diag_inches;
}

} // namespace tb

#endif // TB_SYSTEM_WINDOWS
