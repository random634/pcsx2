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

#include "common/TraceLog.h"
#include "Memory.h"

extern FILE *emuLog;
extern wxString emuLogName;

extern char* disVU0MicroUF(u32 code, u32 pc);
extern char* disVU0MicroLF(u32 code, u32 pc);
extern char* disVU1MicroUF(u32 code, u32 pc);
extern char* disVU1MicroLF(u32 code, u32 pc);

namespace R5900
{
	void disR5900Fasm( std::string& output, u32 code, u32 pc, bool simplify = false);

	extern const char * const GPR_REG[32];
	extern const char * const COP0_REG[32];
	extern const char * const COP1_REG_FP[32];
	extern const char * const COP1_REG_FCR[32];
	extern const char * const COP2_REG_FP[32];
	extern const char * const COP2_REG_CTL[32];
	extern const char * const COP2_VFnames[4];
	extern const char * const GS_REG_PRIV[19];
	extern const u32 GS_REG_PRIV_ADDR[19];
}

namespace R3000A
{
	extern void (*IOP_DEBUG_BSC[64])(char *buf);

	extern const char * const disRNameGPR[];
	extern char* disR3000AF(u32 code, u32 pc);
}
