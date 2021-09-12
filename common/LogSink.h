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

#pragma once

#include "Log.h"
#include <mutex>

class TextLogSink : public LogSink
{
	const LogSource* m_currentLineSource = nullptr;
	LogLevel m_currentLineLevel = LogLevel::Unset;
public:
	bool m_supportsColor;
	bool m_headersEnabled;
	TextLogSink(bool supportsColor, bool headersEnabled = true)
		: m_supportsColor(supportsColor)
		, m_headersEnabled(headersEnabled)
	{
	}
	/// Like LogSink::log but *not thread safe*.  Use some sort of locking mechanism to ensure thread safety before calling this.
	void logOnThread(LogLevel level, LogStyle style, u8 indent, const LogSource& source, std::string_view msg);
	virtual void outputNewline() = 0;
	virtual void outputText(LogStyle style, std::string_view msg) = 0;
};

class FileLogSink final : public TextLogSink
{
public:
	/// File to output to
	FILE* m_file;
	/// Lock
	std::mutex m_mtx;
	FileLogSink(FILE* file);
	void outputNewline() override;
	void outputText(LogStyle style, std::string_view msg) override;
	void log(LogLevel level, LogStyle style, const LogSource& source, std::string_view msg) override;
};

#ifdef __POSIX__
extern FileLogSink defaultLogSink;
#else
class OutputDebugStringLogSink final : public TextLogSink
{
	/// Lock
	std::mutex m_mtx;
public:
	OutputDebugStringLogSink();
	void outputNewline() override;
	void outputText(LogStyle style, std::string_view msg) override;
	void log(LogLevel level, LogStyle style, const LogSource& source, std::string_view msg) override;
};

extern OutputDebugStringLogSink defaultLogSink;
#endif
