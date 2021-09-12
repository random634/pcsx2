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
	std::string msg = fmt::vformat(string, args);
	m_cachedSink->log(level, getStyle(level), *this, msg);
}

void LogSource::logFormatStyle(LogLevel level, std::string_view string, fmt::format_args args, LogStyle style) const
{
	std::string msg = fmt::vformat(string, args);
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
