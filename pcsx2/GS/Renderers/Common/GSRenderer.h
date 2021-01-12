/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2021 PCSX2 Dev Team
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

#include "GS/GS.h"
#include "GS/Window/GSWnd.h"
#include "GS/GSState.h"
#include "GS/GSCapture.h"
#include "IGSRenderer.h"

MULTI_ISA_UNSHARED_START

class GSRenderer : public GSState, public IGSRenderer
{
	GSCapture m_capture;
	std::string m_snapshot;
	int m_shader;

	bool Merge(int field);

	bool m_shift_key;
	bool m_control_key;

protected:
	int m_dithering;
	int m_interlace;
	int m_vsync;
	bool m_aa1;
	bool m_shaderfx;
	bool m_fxaa;
	bool m_shadeboost;
	bool m_texture_shuffle;
	bool m_fmv_switch;
	GSVector2i m_real_size;

	virtual GSTexture* GetOutput(int i, int& y_offset) = 0;
	virtual GSTexture* GetFeedbackOutput() { return nullptr; }

public:
	GSRenderer();
	virtual ~GSRenderer();

	virtual bool CreateDevice(GSDevice* dev);
	virtual void ResetDevice();
	virtual void VSync(int field);
	virtual bool MakeSnapshot(const std::string& path);
	virtual void KeyEvent(GSKeyEventData* e);
	virtual bool CanUpscale() { return false; }
	virtual int GetUpscaleMultiplier() { return 1; }
	virtual GSVector2i GetCustomResolution() { return GSVector2i(0, 0); }
	GSVector2i GetInternalResolution();
	void SetVSync(int vsync);

	__fi bool GetFMVSwitch() const { return m_fmv_switch; }
	__fi void SetFMVSwitch(bool enabled) { m_fmv_switch = enabled; }

	virtual bool BeginCapture(std::string& filename);
	virtual void EndCapture();

	void PurgePool();

	GSVector4i ComputeDrawRectangle(int width, int height) const;

public:
	// Helpers to fulfill IGSRenderer
	void SetRegsMem(uint8* basemem) final { GSState::SetRegsMem(basemem); }
	void SetIrqCallback(void (*irq)()) final { GSState::SetIrqCallback(irq); }
	void SetMultithreaded(bool mt) { GSState::SetMultithreaded(mt); }
	void Reset() { GSState::Reset(); }
	void SoftReset(uint32 mask) { GSState::SoftReset(mask); };
	void WriteCSR(uint32 csr) { GSState::WriteCSR(csr); }
	void InitReadFIFO(uint8* mem, int size) { GSState::InitReadFIFO(mem, size); }
	void ReadFIFO(uint8* mem, int size) { GSState::ReadFIFO(mem, size); }
	void Transfer0(const uint8* mem, uint32 size) { Transfer<0>(mem, size); };
	void Transfer1(const uint8* mem, uint32 size) { Transfer<1>(mem, size); };
	void Transfer2(const uint8* mem, uint32 size) { Transfer<2>(mem, size); };
	void Transfer3(const uint8* mem, uint32 size) { Transfer<3>(mem, size); };
	int Freeze(freezeData* fd, bool sizeonly) { return GSState::Freeze(fd, sizeonly); }
	int Defrost(const freezeData* fd) { return GSState::Defrost(fd); }
	void SetGameCRC(uint32 crc, int options) { return GSState::SetGameCRC(crc, options); }
	void GetLastTag(uint32* tag) { GSState::GetLastTag(tag); }
	void SetFrameSkip(int skip) { GSState::SetFrameSkip(skip); }
};

MULTI_ISA_UNSHARED_END
