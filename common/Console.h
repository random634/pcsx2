/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "common/StringHelpers.h"

enum ConsoleColors
{
	Color_Current = -1,

	Color_Default = 0,

	Color_Black,
	Color_Green,
	Color_Red,
	Color_Blue,
	Color_Magenta,
	Color_Orange,
	Color_Gray,

	Color_Cyan, // faint visibility, intended for logging PS2/IOP output
	Color_Yellow, // faint visibility, intended for logging PS2/IOP output
	Color_White, // faint visibility, intended for logging PS2/IOP output

	// Strong text *may* result in mis-aligned text in the console, depending on the
	// font and the platform, so use these with caution.
	Color_StrongBlack,
	Color_StrongRed, // intended for errors
	Color_StrongGreen, // intended for infrequent state information
	Color_StrongBlue, // intended for block headings
	Color_StrongMagenta,
	Color_StrongOrange, // intended for warnings
	Color_StrongGray,

	Color_StrongCyan,
	Color_StrongYellow,
	Color_StrongWhite,

	ConsoleColors_Count
};

static const ConsoleColors DefaultConsoleColor = Color_Default;

extern void SetConsoleTitle(const wxString& title);
