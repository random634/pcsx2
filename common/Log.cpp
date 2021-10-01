/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2021  PCSX2 Dev Team
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

#include "Log.h"
#include "LogSink.h"

LogSource::LogSource(const char* name, LogStyle style, LogSource* parent, LogLevelInheritance levelInherit, LogLevel baseLevel)
	: m_cachedLevel(LogLevel::Unset)
	, m_localLevel(baseLevel)
	, m_style(style)
	, m_levelInheritance(levelInherit)
	, m_indent(0)
	, m_cachedSink(nullptr)
	, m_localSink(nullptr)
	, m_name(name)
	, m_parent(parent)
{
	if (parent)
		parent->m_children.push_back(this);
	updateCachedSink();
	updateCachedLevel();
}

void LogSource::updateCachedLevel()
{
	LogLevel newLevel;
	switch (m_levelInheritance)
	{
		case LogLevelInheritance::Override:
			if (m_localLevel != LogLevel::Unset)
				newLevel = m_localLevel;
			else if (m_parent)
				newLevel = m_parent->m_cachedLevel;
			else
				newLevel = LogLevel::Trace;
			break;

		case LogLevelInheritance::Maximum:
			newLevel = std::max(m_localLevel, LogLevel::Trace);
			if (m_parent)
				newLevel = std::max(newLevel, m_parent->m_cachedLevel);
			break;
	}

	if (newLevel != m_cachedLevel)
	{
		m_cachedLevel = newLevel;
		for (LogSource* child : m_children)
			child->updateCachedLevel();
	}
}

void LogSource::updateCachedSink()
{
	LogSink* newSink;
	if (m_localSink)
		newSink = m_localSink;
	else if (m_parent)
		newSink = m_parent->m_cachedSink;
	else
		newSink = &defaultLogSink;

	if (newSink != m_cachedSink)
	{
		m_cachedSink = newSink;
		for (LogSource* child : m_children)
			child->updateCachedSink();
	}
}

void LogSource::setLevel(LogLevel level)
{
	m_localLevel = level;
	updateCachedLevel();
}

void LogSource::setSink(LogSink* sink)
{
	m_localSink = sink;
	updateCachedSink();
}

void LogSource::logString(LogLevel level, std::string_view string) const
{
	m_cachedSink->log(level, getStyle(level), *this, string);
}

void LogSource::logFormat(LogLevel level, std::string_view string, fmt::format_args args) const
{
	std::string msg;
	try
	{
		msg = fmt::vformat(string, args);
	}
	catch (const std::exception& err)
	{
		const char* devmsg = IsDevBuild ? "Break on the catch in LogSource::logFormat for details!\n" : "";
		msg = fmt::format("Failed to format the following string for logging: {:s}\n{:s}", err.what(), devmsg);
		m_cachedSink->log(LogLevel::Error, LogStyle::Error, Log::Logging, msg);
		ScopedLogIndent indent(Log::Logging);
		m_cachedSink->log(LogLevel::Error, LogStyle::Error, Log::Logging, string);
		return;
	}
	m_cachedSink->log(level, getStyle(level), *this, msg);
}

void LogSource::logFormatStyle(LogLevel level, std::string_view string, fmt::format_args args, LogStyle style) const
{
	std::string msg;
	try
	{
		msg = fmt::vformat(string, args);
	}
	catch (const std::exception& err)
	{
		const char* devmsg = IsDevBuild ? "Break on the catch in LogSource::logFormatStyle for details!\n" : "";
		msg = fmt::format("Failed to format the following string for logging: {:s}\n{:s}", err.what(), devmsg);
		m_cachedSink->log(LogLevel::Error, LogStyle::Error, Log::Logging, msg);
		ScopedLogIndent indent(Log::Logging);
		m_cachedSink->log(LogLevel::Error, LogStyle::Error, Log::Logging, string);
		return;
	}
	m_cachedSink->log(level, style, *this, msg);
}

LogStyle LogSource::getStyle(LogLevel level) const
{
	if (level <= LogLevel::Trace)
		return LogStyle::Trace;
	else if (level == LogLevel::Warning)
		return LogStyle::Warning;
	else if (level > LogLevel::Warning)
		return LogStyle::Error;
	else
		return m_style;
}

LogSource Log::PCSX2     ("PCSX2",       LogStyle::General,  nullptr);
LogSource Log::Console   ("Console",     LogStyle::General,  &PCSX2);
LogSource Log::Dev       ("DevCon",      LogStyle::General,  &PCSX2);
LogSource Log::Logging   ("Logging",     LogStyle::General,  &Console);
LogSource Log::Trace     ("Trace",       LogStyle::General,  &PCSX2);

LogSource Log::SIF       ("SIF",         LogStyle::Emulator, &Trace, LogLevelInheritance::Maximum);

LogSource Log::pxEvt     ("pxEvent",     LogStyle::General,  &PCSX2);
LogSource Log::pxThread  ("pxThread",    LogStyle::General,  &PCSX2);
LogSource Log::Patches   ("Patches",     LogStyle::General,  &Console, LogLevelInheritance::Override, LogLevel::Trace);

LogSource Log::Recording ("Recording",   LogStyle::General,  &PCSX2);
LogSource Log::RecControl("Rec.Control", LogStyle::General,  &PCSX2);
LogSource Log::EERecPerf ("EERecPerf",   LogStyle::Emulator, &PCSX2);

LogSource Log::EE::Base     ("EE",          LogStyle::Emulator, &Trace, LogLevelInheritance::Maximum);
LogSource Log::EE::Disasm   ("EE.Disasm",   LogStyle::Emulator, &Base,  LogLevelInheritance::Maximum);
LogSource Log::EE::Registers("EE.Regs",     LogStyle::Emulator, &Base,  LogLevelInheritance::Maximum);
LogSource Log::EE::Events   ("EE.Events",   LogStyle::Emulator, &Base,  LogLevelInheritance::Maximum);

LogSource Log::EE::Bios     ("EE.BIOS",     LogStyle::Emulator, &Base,      LogLevelInheritance::Maximum);
LogSource Log::EE::Memory   ("EE.Mem",      LogStyle::Emulator, &Base,      LogLevelInheritance::Maximum);
LogSource Log::EE::GIFtag   ("EE.GIFtag",   LogStyle::Emulator, &Base,      LogLevelInheritance::Maximum);
LogSource Log::EE::VIFcode  ("EE.VIFcode",  LogStyle::Emulator, &Base,      LogLevelInheritance::Maximum);
LogSource Log::EE::MSKPATH3 ("EE.MSKPath3", LogStyle::Emulator, &Base,      LogLevelInheritance::Maximum);
LogSource Log::EE::R5900    ("EE.R5900",    LogStyle::Emulator, &Disasm,    LogLevelInheritance::Maximum);
LogSource Log::EE::COP0     ("EE.COP0",     LogStyle::Emulator, &Disasm,    LogLevelInheritance::Maximum);
LogSource Log::EE::COP1     ("EE.COP1",     LogStyle::Emulator, &Disasm,    LogLevelInheritance::Maximum);
LogSource Log::EE::COP2     ("EE.COP2",     LogStyle::Emulator, &Disasm,    LogLevelInheritance::Maximum);
LogSource Log::EE::Cache    ("EE.Cache",    LogStyle::Emulator, &Disasm,    LogLevelInheritance::Maximum);
LogSource Log::EE::KnownHW  ("EE.KnownHW",  LogStyle::Emulator, &Registers, LogLevelInheritance::Maximum);
LogSource Log::EE::UnknownHW("EE.UnkHW",    LogStyle::Emulator, &Registers, LogLevelInheritance::Maximum);
LogSource Log::EE::DMAHW    ("EE.DMA",      LogStyle::Emulator, &Registers, LogLevelInheritance::Maximum);
LogSource Log::EE::IPU      ("EE.IPU",      LogStyle::Emulator, &Registers, LogLevelInheritance::Maximum);
LogSource Log::EE::DMAC     ("EE.DMAC",     LogStyle::Emulator, &Events,    LogLevelInheritance::Maximum);
LogSource Log::EE::Counters ("EE.Counters", LogStyle::Emulator, &Events,    LogLevelInheritance::Maximum);
LogSource Log::EE::SPR      ("EE.SPR",      LogStyle::Emulator, &Events,    LogLevelInheritance::Maximum);
LogSource Log::EE::VIF      ("EE.VIF",      LogStyle::Emulator, &Events,    LogLevelInheritance::Maximum);
LogSource Log::EE::GIF      ("EE.GIF",      LogStyle::Emulator, &Events,    LogLevelInheritance::Maximum);

LogSource Log::IOP::Base     ("IOP",          LogStyle::Emulator, &Trace, LogLevelInheritance::Maximum);
LogSource Log::IOP::Disasm   ("IOP.Disasm",   LogStyle::Emulator, &Base,  LogLevelInheritance::Maximum);
LogSource Log::IOP::Registers("IOP.Regs",     LogStyle::Emulator, &Base,  LogLevelInheritance::Maximum);
LogSource Log::IOP::Events   ("IOP.Events",   LogStyle::Emulator, &Base,  LogLevelInheritance::Maximum);

LogSource Log::IOP::Bios     ("IOP.BIOS",     LogStyle::Emulator, &Base,      LogLevelInheritance::Maximum);
LogSource Log::IOP::Memcards ("IOP.Memcards", LogStyle::Emulator, &Base,      LogLevelInheritance::Maximum);
LogSource Log::IOP::PAD      ("IOP.PAD",      LogStyle::Emulator, &Base,      LogLevelInheritance::Maximum);
LogSource Log::IOP::R3000A   ("IOP.R3000A",   LogStyle::Emulator, &Disasm,    LogLevelInheritance::Maximum);
LogSource Log::IOP::COP2     ("IOP.COP2",     LogStyle::Emulator, &Disasm,    LogLevelInheritance::Maximum);
LogSource Log::IOP::Memory   ("IOP.Memory",   LogStyle::Emulator, &Disasm,    LogLevelInheritance::Maximum);
LogSource Log::IOP::KnownHW  ("IOP.KnownHW",  LogStyle::Emulator, &Registers, LogLevelInheritance::Maximum);
LogSource Log::IOP::UnknownHW("IOP.UnkHW",    LogStyle::Emulator, &Registers, LogLevelInheritance::Maximum);
LogSource Log::IOP::DMAHW    ("IOP.DMA",      LogStyle::Emulator, &Registers, LogLevelInheritance::Maximum);
LogSource Log::IOP::DMAC     ("IOP.DMAC",     LogStyle::Emulator, &Events,    LogLevelInheritance::Maximum);
LogSource Log::IOP::Counters ("IOP.Counters", LogStyle::Emulator, &Events,    LogLevelInheritance::Maximum);
LogSource Log::IOP::CDVD     ("IOP.CDVD",     LogStyle::Emulator, &Events,    LogLevelInheritance::Maximum);
LogSource Log::IOP::MDEC     ("IOP.MDEC",     LogStyle::Emulator, &Events,    LogLevelInheritance::Maximum);

LogSource Log::SysCon::Base  ("Sys",       LogStyle::GameLog,  &PCSX2);
LogSource Log::SysCon::ELF   ("Sys.ELF",   LogStyle::GameLog,  &Base);
LogSource Log::SysCon::EE    ("Sys.EE",    LogStyle::GameLog,  &Base);
LogSource Log::SysCon::DECI2 ("Sys.DECI2", LogStyle::GameLog,  &EE);
LogSource Log::SysCon::IOP   ("Sys.IOP",   LogStyle::GameLog,  &Base);
LogSource Log::SysCon::SysOut("Sys.Out",   LogStyle::GameLog,  &Base);
LogSource Log::SysCon::PGIF  ("Sys.PGIF",  LogStyle::GameLog,  &Base);
