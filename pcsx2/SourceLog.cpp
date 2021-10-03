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

// --------------------------------------------------------------------------------------
//  Source / Tracre Logging  (high volume logging facilities)
// --------------------------------------------------------------------------------------
// This module defines functions for performing high-volume diagnostic trace logging.
// Only ASCII versions of these logging functions are provided.  Translated messages are
// not supported, and typically all logs are written to disk (ASCII), thus making the
// ASCII versions the more efficient option.

#include "PrecompiledHeader.h"

#ifndef _WIN32
#include <sys/time.h>
#endif

#include <cstdarg>
#include <ctype.h>

#include "R3000A.h"
#include "iR5900.h"
#include "System.h"
#include "DebugTools/Debug.h"

using namespace R5900;

FILE* emuLog;
wxString emuLogName;

typedef void Fntype_SrcLogPrefix(FastFormatAscii& dest);

// writes text directly to the logfile, no newlines appended.
void __Log(const char* fmt, ...)
{
	va_list list;
	va_start(list, fmt);

	if (emuLog != NULL)
	{
		fputs(FastFormatAscii().WriteV(fmt, list), emuLog);
		fputs("\n", emuLog);
		fflush(emuLog);
	}

	va_end(list);
}

/*
// TODO: Support in new logger

void SysTraceLog_EE::ApplyPrefix(FastFormatAscii& ascii) const
{
	ascii.Write("%-4s(%8.8lx %8.8lx): ", ((SysTraceLogDescriptor*)m_Descriptor)->Prefix, cpuRegs.pc, cpuRegs.cycle);
}

void SysTraceLog_IOP::ApplyPrefix(FastFormatAscii& ascii) const
{
	ascii.Write("%-4s(%8.8lx %8.8lx): ", ((SysTraceLogDescriptor*)m_Descriptor)->Prefix, psxRegs.pc, psxRegs.cycle);
}

void SysTraceLog_VIFcode::ApplyPrefix(FastFormatAscii& ascii) const
{
	_parent::ApplyPrefix(ascii);
	ascii.Write("vifCode_");
}
*/
