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

#include "PrecompiledHeader.h"
#include "LogOptionsPanels.h"

#include "common/IniInterface.h"
#include "DebugTools/Debug.h"

#include <wx/statline.h>

// Conversion between LogSource levels and our current boolean-based settings

static bool isEnabled(const LogSource& source)
{
	return source.configuredLevel() < LogLevel::Info;
}

static void setEnabled(LogSource& source, bool enabled)
{
	source.setLevel(enabled ? LogLevel::Debug : LogLevel::Info);
}

using namespace pxSizerFlags;

Panels::eeLogOptionsPanel::eeLogOptionsPanel( LogOptionsPanel* parent )
	: BaseCpuLogOptionsPanel( parent, L"EE Logs" )
{
	SetMinWidth( 300 );

	m_miscGroup		= new wxStaticBoxSizer( wxVERTICAL, this, L"General" );

	m_disasmPanel	= new CheckedStaticBox( this, wxVERTICAL, L"Disasm" );
	m_hwPanel		= new CheckedStaticBox( this, wxVERTICAL, L"Registers" );
	m_evtPanel		= new CheckedStaticBox( this, wxVERTICAL, L"Events" );

	wxFlexGridSizer& eeTable( *new wxFlexGridSizer( 2, 5, 0 ) );

	eeTable.AddGrowableCol(0);
	eeTable.AddGrowableCol(1);

	eeTable	+= m_miscGroup		| SubGroup();
	eeTable += m_hwPanel		| SubGroup();
	eeTable += m_evtPanel		| SubGroup();
	eeTable += m_disasmPanel	| SubGroup();

	ThisSizer	+= 4;
	ThisSizer	+= eeTable | pxExpand;

	SetValue( true );
}

Panels::iopLogOptionsPanel::iopLogOptionsPanel( LogOptionsPanel* parent )
	: BaseCpuLogOptionsPanel( parent, L"IOP Logs" )
{
	SetMinWidth( 280 );

	m_miscGroup		= new wxStaticBoxSizer( wxVERTICAL, this, L"General" );

	m_disasmPanel	= new CheckedStaticBox( this, wxVERTICAL, L"Disasm" );
	m_hwPanel		= new CheckedStaticBox( this, wxVERTICAL, L"Registers" );
	m_evtPanel		= new CheckedStaticBox( this, wxVERTICAL, L"Events" );

	wxFlexGridSizer& iopTable( *new wxFlexGridSizer( 2, 5, 0 ) );

	iopTable.AddGrowableCol(0);
	iopTable.AddGrowableCol(1);

	iopTable	+= m_miscGroup		| SubGroup();
	iopTable	+= m_hwPanel		| SubGroup();
	iopTable	+= m_evtPanel		| SubGroup();
	iopTable	+= m_disasmPanel	| SubGroup();

	ThisSizer	+= 4;
	ThisSizer	+= iopTable	| pxExpand;

	SetValue( true );
}

CheckedStaticBox* Panels::eeLogOptionsPanel::GetStaticBox( const wxString& subgroup ) const
{
	if (0 == subgroup.CmpNoCase( L"Disasm" ))		return m_disasmPanel;
	if (0 == subgroup.CmpNoCase( L"Registers" ))	return m_hwPanel;
	if (0 == subgroup.CmpNoCase( L"Events" ))		return m_evtPanel;

	return NULL;
}

CheckedStaticBox* Panels::iopLogOptionsPanel::GetStaticBox( const wxString& subgroup ) const
{
	if (0 == subgroup.CmpNoCase( L"Disasm" ))		return m_disasmPanel;
	if (0 == subgroup.CmpNoCase( L"Registers" ))	return m_hwPanel;
	if (0 == subgroup.CmpNoCase( L"Events" ))		return m_evtPanel;

	return NULL;
}


void Panels::eeLogOptionsPanel::OnSettingsChanged()
{
	SetValue(isEnabled(Log::EE::Base));

	m_disasmPanel->SetValue(isEnabled(Log::EE::Disasm));
	m_evtPanel   ->SetValue(isEnabled(Log::EE::Events));
	m_hwPanel    ->SetValue(isEnabled(Log::EE::Registers));
}

void Panels::iopLogOptionsPanel::OnSettingsChanged()
{
	SetValue(isEnabled(Log::IOP::Base));

	m_disasmPanel->SetValue(isEnabled(Log::IOP::Disasm));
	m_evtPanel   ->SetValue(isEnabled(Log::IOP::Events));
	m_hwPanel    ->SetValue(isEnabled(Log::IOP::Registers));
}

struct LogSourceDescriptor
{
	/// The actual log source
	LogSource& source;
	/// Standard UI name for this log source.  Used in menus, options dialogs.
	const wxChar* name;
	/// Long description for use as a tooltip or menu item description.
	const wxChar* description;
};

struct LogSourceCategory
{
	/// The log source that represents this category
	LogSource& source;
	/// Standard UI name for this category
	const wxChar* name;
};

static const LogSourceDescriptor traceLogSourceList[] =
{
	{Log::SIF,            L"SIF (EE ↔︎ IOP)",   L""},

	{Log::EE::Bios,       L"Bios",             pxDt("SYSCALL and DECI2 activity.")},
	{Log::EE::Memory,     L"Memory",           pxDt("Direct memory accesses to unknown or unmapped EE memory space.")},

	{Log::EE::R5900,      L"R5900 Core",       pxDt("Disasm of executing core instructions (excluding COPs and CACHE).")},
	{Log::EE::COP0,       L"COP0",             pxDt("Disasm of COP0 instructions (MMU, cpu and dma status, etc).")},
	{Log::EE::COP1,       L"COP1/FPU",         pxDt("Disasm of the EE's floating point unit (FPU) only.")},
	{Log::EE::COP2,       L"COP2/VUmacro",     pxDt("Disasm of the EE's VU0macro co-processor instructions.")},
	{Log::EE::Cache,      L"Cache",            pxDt("Execution of EE cache instructions.")},

	{Log::EE::KnownHW,    L"Hardware Regs",    pxDt("All known hardware register accesses (very slow!); not including sub filter options below.")},
	{Log::EE::UnknownHW,  L"Unknown Regs",     pxDt("Logs only unknown, unmapped, or unimplemented register accesses.")},
	{Log::EE::DMAHW,      L"DMA Regs",         pxDt("Logs only DMA-related registers.")},
	{Log::EE::IPU,        L"IPU",              pxDt("IPU activity: hardware registers, decoding operations, DMA status, etc.")},
	{Log::EE::GIFtag,     L"GIFtags",          pxDt("All GIFtag parse activity; path index, tag type, etc.")},
	{Log::EE::VIFcode,    L"VIFcodes",         pxDt("All VIFcode processing; command, tag style, interrupts.")},
	{Log::EE::MSKPATH3,   L"MSKPATH3",         pxDt("All processing involved in Path3 Masking.")},

	{Log::EE::DMAC,       L"DMA Controller",   pxDt("Actual data transfer logs, bus right arbitration, stalls, etc.")},
	{Log::EE::Counters,   L"Counters",         pxDt("Tracks all EE counters events and some counter register activity.")},
	{Log::EE::SPR,        L"Scratchpad MFIFO", pxDt("Scratchpad's MFIFO activity.")},
	{Log::EE::VIF,        L"VIF",              pxDt("Dumps various VIF and VIFcode processing data.")},
	{Log::EE::GIF,        L"GIF",              pxDt("Dumps various GIF and GIFtag parsing data.")},

	{Log::IOP::Bios,      L"Bios",             pxDt("SYSCALL and IRX activity.")},
	{Log::IOP::Memcards,  L"Memory Cards",     pxDt("Memory card reads, writes, erases, terminators, and other processing.")},
	{Log::IOP::PAD,       L"Pad",              pxDt("Gamepad activity on the SIO.")},

	{Log::IOP::R3000A,    L"R3000A Core",      pxDt("Disasm of executing core instructions (excluding COPs and CACHE).")},
	{Log::IOP::COP2,      L"COP2",             pxDt("Disasm of the IOP's GPU co-processor instructions.")},
	{Log::IOP::Memory,    L"Memory",           pxDt("Direct memory accesses to unknown or unmapped IOP memory space.")},

	{Log::IOP::KnownHW,   L"Hardware Regs",    pxDt("All known hardware register accesses, not including the sub-filters below.")},
	{Log::IOP::UnknownHW, L"Unknown Regs",     pxDt("Logs only unknown, unmapped, or unimplemented register accesses.")},
	{Log::IOP::DMAHW,     L"DMA Regs",         pxDt("Logs only DMA-related registers.")},
	{Log::IOP::DMAC,      L"DMA Controller",   pxDt("Actual DMA event processing and data transfer logs.")},
	{Log::IOP::Counters,  L"Counters",         pxDt("Tracks all IOP counters events and some counter register activity.")},
	{Log::IOP::CDVD,      L"CDVD",             pxDt("Detailed logging of CDVD hardware.")},
	{Log::IOP::MDEC,      L"MDEC",             pxDt("Detailed logging of the Motion (FMV) Decoder hardware unit.")},
};

static const LogSourceCategory traceLogCategoriesList[] =
{
	{Log::EE::Base,       L"EE"},
	{Log::EE::Disasm,     L"Disasm"},
	{Log::EE::Registers,  L"Registers"},
	{Log::EE::Events,     L"Events"},
	{Log::IOP::Base,      L"IOP"},
	{Log::IOP::Disasm,    L"Disasm"},
	{Log::IOP::Registers, L"Registers"},
	{Log::IOP::Events,    L"Events"},
};

static const LogSourceCategory* getCategory(LogSource* source)
{
	for (const LogSourceCategory& category : traceLogCategoriesList)
	{
		if (&category.source == source)
			return &category;
	}
	return nullptr;
}

static void LogSourceEntry(IniInterface& ini, const wxString& name, LogSource& source)
{
	int level = static_cast<int>(source.configuredLevel());
	ini.Entry(name, level, false);
	source.setLevel(static_cast<LogLevel>(level));
}

static void SysTraceLog_LoadSaveCategories(IniInterface& ini)
{
	ScopedIniGroup path(ini, L"TraceLog");
	LogSourceEntry(ini, L"Enabled", Log::Trace);
	for (const LogSourceCategory& category : traceLogCategoriesList)
	{
		LogSourceEntry(ini, wxString::FromUTF8(category.source.name()), category.source);
	}
}

void SysTraceLog_LoadSaveSettings( IniInterface& ini )
{
	SysTraceLog_LoadSaveCategories(ini);
	ScopedIniGroup path(ini, L"TraceLogSources");

	for (const LogSourceDescriptor& log : traceLogSourceList)
	{
		pxAssertMsg(log.source.name(), "Trace log without a name!");
		LogSourceEntry(ini, wxString::FromUTF8(log.source.name()), log.source);
	}
}

// --------------------------------------------------------------------------------------
//  LogOptionsPanel Implementations
// --------------------------------------------------------------------------------------
Panels::LogOptionsPanel::LogOptionsPanel(wxWindow* parent )
	: BaseApplicableConfigPanel( parent )
	, m_checks( new pxCheckBox*[std::size(traceLogSourceList)] )
{
	m_miscSection = new wxStaticBoxSizer( wxHORIZONTAL, this, L"Misc" );

	m_eeSection		= new eeLogOptionsPanel( this );
	m_iopSection	= new iopLogOptionsPanel( this );

	for (uint i = 0; i < std::size(traceLogSourceList); ++i)
	{
		const LogSourceDescriptor& item = traceLogSourceList[i];

		pxAssertMsg(item.source.name(), "Trace log without a name!" );

		BaseCpuLogOptionsPanel* cpu = nullptr;
		CheckedStaticBox* group = nullptr;
		if (auto* parent = getCategory(item.source.parent()))
		{
			if (auto* grandparent = getCategory(parent->source.parent()))
			{
				cpu = GetCpuPanel(grandparent->name);
				if (cpu)
					group = cpu->GetStaticBox(parent->name);
			}
			else
			{
				cpu = GetCpuPanel(parent->name);
			}
		}

		wxSizer* addsizer = NULL;
		wxWindow* addparent = NULL;

		if (cpu && group)
		{
			addsizer = &group->ThisSizer;
			addparent = group;
		}
		else if (cpu)
		{
			addsizer = cpu->GetMiscGroup();
			addparent = cpu;
		}
		else
		{
			addsizer = m_miscSection;
			addparent = m_miscSection->GetStaticBox();
		}

		*addsizer += m_checks[i] = new pxCheckBox(addparent, item.name);
 		if (m_checks[i] && item.description)
			m_checks[i]->SetToolTip(pxGetTranslation(item.description));
	}

	m_masterEnabler = new pxCheckBox( this, _("Enable Trace Logging"),
		_("Trace logs are all written to emulog.txt.  Toggle trace logging at any time using F10.") );
	m_masterEnabler->SetToolTip( _("Warning: Trace logging is typically very slow, and is a leading cause of 'What happened to my FPS?' problems. :)") );

	wxFlexGridSizer& topSizer = *new wxFlexGridSizer( 2 );

	topSizer.AddGrowableCol(0);
	topSizer.AddGrowableCol(1);

	topSizer	+= m_eeSection		| StdExpand();
	topSizer	+= m_iopSection		| StdExpand();

	*this		+= m_masterEnabler				| StdExpand();
	*this		+= new wxStaticLine( this )		| StdExpand().Border(wxLEFT | wxRIGHT, 20);
	*this		+= 5;
	*this		+= topSizer						| StdExpand();
	*this		+= m_miscSection				| StdSpace().Centre();

	Bind(wxEVT_CHECKBOX, &LogOptionsPanel::OnCheckBoxClicked, this, m_masterEnabler->GetWxPtr()->GetId());
}

Panels::BaseCpuLogOptionsPanel* Panels::LogOptionsPanel::GetCpuPanel( const wxString& token ) const
{
	if( token == L"EE" )	return m_eeSection;
	if( token == L"IOP" )	return m_iopSection;

	return NULL;
}

void Panels::LogOptionsPanel::AppStatusEvent_OnSettingsApplied()
{
	m_masterEnabler->SetValue(isEnabled(Log::Trace));

	m_eeSection->OnSettingsChanged();
	m_iopSection->OnSettingsChanged();

	for (uint i = 0; i < std::size(traceLogSourceList); ++i)
	{
		if (m_checks[i])
			m_checks[i]->SetValue(isEnabled(traceLogSourceList[i].source));
	}
	OnUpdateEnableAll();
}

void Panels::LogOptionsPanel::OnUpdateEnableAll()
{
	bool enabled( m_masterEnabler->GetValue() );

	m_eeSection->Enable( enabled );
	m_iopSection->Enable( enabled );
	m_miscSection->GetStaticBox()->Enable( enabled );
}

void Panels::LogOptionsPanel::OnCheckBoxClicked(wxCommandEvent &evt)
{
	OnUpdateEnableAll();
	evt.Skip();
}

void Panels::LogOptionsPanel::Apply()
{
	setEnabled(Log::Trace, m_masterEnabler->GetValue());

	m_eeSection->Apply();
	m_iopSection->Apply();

	for (uint i = 0; i < std::size(traceLogSourceList); ++i)
	{
		if (m_checks[i])
			setEnabled(traceLogSourceList[i].source, m_checks[i]->IsChecked());
	}
}

#define GetSet( cpu, name )		SysTrace.cpu.name.Enabled	= m_##name->GetValue()

void Panels::eeLogOptionsPanel::Apply()
{
	setEnabled(Log::EE::Base, GetValue());
	setEnabled(Log::EE::Disasm, m_disasmPanel->GetValue());
	setEnabled(Log::EE::Registers, m_hwPanel->GetValue());
	setEnabled(Log::EE::Events, m_evtPanel->GetValue());
}

void Panels::iopLogOptionsPanel::Apply()
{
	setEnabled(Log::IOP::Base, GetValue());
	setEnabled(Log::IOP::Disasm, m_disasmPanel->GetValue());
	setEnabled(Log::IOP::Registers, m_hwPanel->GetValue());
	setEnabled(Log::IOP::Events, m_evtPanel->GetValue());
}
