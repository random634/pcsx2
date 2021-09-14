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

LogSource::LogSource(const char* name, LogStyle style, LogSource* parent, LogLevel baseLevel)
	: m_cachedLevel(LogLevel::Unset)
	, m_localLevel(baseLevel)
	, m_style(style)
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
	if (m_localLevel != LogLevel::Unset)
		newLevel = m_localLevel;
	else if (m_parent)
		newLevel = m_parent->m_cachedLevel;
	else
		newLevel = LogLevel::Trace;

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
LogSource Log::SIF       ("SIF",         LogStyle::Emulator, &PCSX2);
LogSource Log::Recording ("Recording",   LogStyle::General,  &PCSX2);
LogSource Log::RecControl("Rec.Control", LogStyle::General,  &PCSX2);
LogSource Log::EERecPerf ("EERecPerf",   LogStyle::Emulator, &PCSX2);

LogSource Log::EE::Base     ("EE",          LogStyle::Emulator, &PCSX2);
LogSource Log::EE::Bios     ("EE.BIOS",     LogStyle::Emulator, &Base);
LogSource Log::EE::Memory   ("EE.Mem",      LogStyle::Emulator, &Base);
LogSource Log::EE::GIFtag   ("EE.GIFtag",   LogStyle::Emulator, &Base);
LogSource Log::EE::VIFcode  ("EE.VIFcode",  LogStyle::Emulator, &Base);
LogSource Log::EE::MSKPATH3 ("EE.MSKPath3", LogStyle::Emulator, &Base);
LogSource Log::EE::R5900    ("EE.R5900",    LogStyle::Emulator, &Base);
LogSource Log::EE::COP0     ("EE.COP0",     LogStyle::Emulator, &Base);
LogSource Log::EE::COP1     ("EE.COP1",     LogStyle::Emulator, &Base);
LogSource Log::EE::COP2     ("EE.COP2",     LogStyle::Emulator, &Base);
LogSource Log::EE::Cache    ("EE.Cache",    LogStyle::Emulator, &Base);
LogSource Log::EE::KnownHW  ("EE.KnownHW",  LogStyle::Emulator, &Base);
LogSource Log::EE::UnknownHW("EE.UnkHW",    LogStyle::Emulator, &Base);
LogSource Log::EE::DMAHW    ("EE.DMA",      LogStyle::Emulator, &Base);
LogSource Log::EE::IPU      ("EE.IPU",      LogStyle::Emulator, &Base);
LogSource Log::EE::DMAC     ("EE.DMAC",     LogStyle::Emulator, &Base);
LogSource Log::EE::Counters ("EE.Counters", LogStyle::Emulator, &Base);
LogSource Log::EE::SPR      ("EE.SPR",      LogStyle::Emulator, &Base);
LogSource Log::EE::VIF      ("EE.VIF",      LogStyle::Emulator, &Base);
LogSource Log::EE::GIF      ("EE.GIF",      LogStyle::Emulator, &Base);

LogSource Log::IOP::Base     ("IOP",          LogStyle::Emulator, &PCSX2);
LogSource Log::IOP::Bios     ("IOP.BIOS",     LogStyle::Emulator, &Base);
LogSource Log::IOP::Memcards ("IOP.Memcards", LogStyle::Emulator, &Base);
LogSource Log::IOP::PAD      ("IOP.PAD",      LogStyle::Emulator, &Base);
LogSource Log::IOP::R3000A   ("IOP.R3000A",   LogStyle::Emulator, &Base);
LogSource Log::IOP::COP2     ("IOP.COP2",     LogStyle::Emulator, &Base);
LogSource Log::IOP::Memory   ("IOP.Memory",   LogStyle::Emulator, &Base);
LogSource Log::IOP::KnownHW  ("IOP.KnownHW",  LogStyle::Emulator, &Base);
LogSource Log::IOP::UnknownHW("IOP.UnkHW",    LogStyle::Emulator, &Base);
LogSource Log::IOP::DMAHW    ("IOP.DMA",      LogStyle::Emulator, &Base);
LogSource Log::IOP::SPU2     ("IOP.SPU2",     LogStyle::Emulator, &Base);
LogSource Log::IOP::USB      ("IOP.USB",      LogStyle::Emulator, &Base);
LogSource Log::IOP::FW       ("IOP.FW",       LogStyle::Emulator, &Base);
LogSource Log::IOP::DMAC     ("IOP.DMAC",     LogStyle::Emulator, &Base);
LogSource Log::IOP::Counters ("IOP.Counters", LogStyle::Emulator, &Base);
LogSource Log::IOP::CDVD     ("IOP.CDVD",     LogStyle::Emulator, &Base);
LogSource Log::IOP::MDEC     ("IOP.MDEC",     LogStyle::Emulator, &Base);

LogSource Log::SysCon::Base  ("Sys",       LogStyle::GameLog,  &PCSX2);
LogSource Log::SysCon::ELF   ("Sys.ELF",   LogStyle::GameLog,  &Base);
LogSource Log::SysCon::EE    ("Sys.EE",    LogStyle::GameLog,  &Base);
LogSource Log::SysCon::DECI2 ("Sys.DECI2", LogStyle::GameLog,  &Base);
LogSource Log::SysCon::IOP   ("Sys.IOP",   LogStyle::GameLog,  &Base);
LogSource Log::SysCon::SysOut("Sys.Out",   LogStyle::GameLog,  &Base);
LogSource Log::SysCon::PGIF  ("Sys.PGIF",  LogStyle::GameLog,  &Base);
