/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2020  PCSX2 Dev Team
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

#include "Global.h"
#include "Utilities/mt_queue.h"

enum PadOptions
{
	PADOPTION_FORCEFEEDBACK = 1 << 0,
	PADOPTION_REVERSELX     = 1 << 1,
	PADOPTION_REVERSELY     = 1 << 2,
	PADOPTION_REVERSERX     = 1 << 3,
	PADOPTION_REVERSERY     = 1 << 4,
	PADOPTION_MOUSE_L       = 1 << 5,
	PADOPTION_MOUSE_R       = 1 << 6,
};

extern FILE* padLog;
extern void initLogging();

extern keyEvent event;
extern MtQueue<keyEvent> g_ev_fifo;

s32 _PADopen(void* pDsp);
void _PADclose();
void PADsetMode(int pad, int mode);

void SysMessage(char* fmt, ...);

s32 PADinit();
void PADshutdown();
s32 PADopen(void* pDsp);
void PADsetLogDir(const char* dir);
void PADclose();
s32 PADsetSlot(u8 port, u8 slot);
s32 PADfreeze(int mode, freezeData* data);
u8 PADstartPoll(int pad);
u8 PADpoll(u8 value);
keyEvent* PADkeyEvent();
void PADupdate(int pad);
void PADconfigure();

#if defined(__unix__)
void PADWriteEvent(keyEvent& evt);
#endif
