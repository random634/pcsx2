/*
 *	Copyright (C) 2002-2021 PCSX2 Dev Team
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GS/GS.h"

class GSDevice;
class GSWnd;

/// GSRenderer but with mostly vtable stuff
/// Used so outwards interfaces can use GSRenderer without worrying about its split ISA stuff
class IGSRenderer
{
public:
	std::shared_ptr<GSWnd> m_wnd;
	GSDevice* m_dev;

	std::mutex m_pGSsetTitle_Crit;
	char m_GStitleInfoBuffer[128];
public:

	IGSRenderer();
	virtual ~IGSRenderer();
	virtual void ResetDevice() = 0;
	virtual void SetRegsMem(uint8* basemem) = 0;
	virtual void SetIrqCallback(void (*irq)()) = 0;
	virtual void SetVSync(int vsync) = 0;
	virtual void SetMultithreaded(bool mt) = 0;
	virtual bool CreateDevice(GSDevice* dev) = 0;
	virtual void Reset() = 0;
	virtual void SoftReset(uint32 mask) = 0;
	virtual void WriteCSR(uint32 csr) = 0;
	virtual void InitReadFIFO(uint8* mem, int len) = 0;
	virtual void ReadFIFO(uint8* mem, int size) = 0;
	virtual void Transfer0(const uint8* mem, uint32 size) = 0;
	virtual void Transfer1(const uint8* mem, uint32 size) = 0;
	virtual void Transfer2(const uint8* mem, uint32 size) = 0;
	virtual void Transfer3(const uint8* mem, uint32 size) = 0;
	virtual void VSync(int field) = 0;
	virtual bool MakeSnapshot(const std::string& path) = 0;
	virtual void KeyEvent(GSKeyEventData* e) = 0;
	virtual int Freeze(freezeData* fd, bool sizeonly) = 0;
	virtual int Defrost(const freezeData* fd) = 0;
	virtual bool BeginCapture(std::string& filename) = 0;
	virtual void EndCapture() = 0;
	virtual void SetGameCRC(uint32 crc, int options) = 0;
	virtual void GetLastTag(uint32* tag) = 0;
	virtual void SetFrameSkip(int skip) = 0;
	virtual bool GetFMVSwitch() const = 0;
	virtual void SetFMVSwitch(bool enabled) = 0;
};
