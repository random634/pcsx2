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

#include "LogSink.h"
#ifdef __POSIX__
	#include <unistd.h>
#else
	#include <debugapi.h>
#endif

static std::string_view stringify(LogLevel level)
{
	switch (level) {
		case LogLevel::Trace:    return "Trace";
		case LogLevel::Debug:    return "Debug";
		case LogLevel::Info:     return "Info";
		case LogLevel::Warning:  return "Warn";
		case LogLevel::Error:    return "Error";
		case LogLevel::Critical: return "Crit";
		default: return "Unk";
	}
}

void TextLogSink::logOnThread(LogLevel level, LogStyle style, u8 indent, const LogSource& source, std::string_view msg)
{
	if (msg.empty())
		return;

	std::string title;
	if (!m_headersEnabled)
		title.clear();
	else if (m_supportsColor)
		title = fmt::format("[{:^8}] ", source.name());
	else
		title = fmt::format("[{:^8}][{:<5}] ", source.name(), stringify(level));
	if (indent > 0)
		title.resize(title.size() + indent * 4, ' ');

	if (!m_currentLineSource)
	{
		outputText(source.style(), title);
	}
	else if (m_currentLineSource != &source || m_currentLineLevel != level)
	{
		outputNewline();
		outputText(source.style(), title);
	}

	size_t pos;
	while ((pos = msg.find('\n')) != msg.npos)
	{
		if (pos > 0)
			outputText(style, msg.substr(0, pos));
		outputNewline();
		msg.remove_prefix(pos + 1);
		if (!msg.empty())
			outputText(source.style(), title);
	}

	if (msg.empty())
	{
		m_currentLineSource = nullptr;
		m_currentLineLevel = LogLevel::Unset;
	}
	else
	{
		outputText(style, msg);
		m_currentLineSource = &source;
		m_currentLineLevel = level;
	}
}

static bool checkSupportsColor(FILE* file)
{
#ifdef __POSIX__
	if (!isatty(fileno(file)))
		return false;
	char* term = getenv("TERM");
	if (!term || (0 == strcmp(term, "dumb")))
		return false;
	return true; // Probably supports color
#else
	return false;
#endif
}

FileLogSink::FileLogSink(FILE* file)
	: TextLogSink(checkSupportsColor(file))
	, m_file(file)
{
}

void FileLogSink::log(LogLevel level, LogStyle style, const LogSource& source, std::string_view msg)
{
	std::lock_guard<std::mutex> l(m_mtx);
	logOnThread(level, style, source.currentIndent(), source, msg);
	fflush(m_file);
}

void FileLogSink::outputNewline()
{
#ifdef __POSIX__
	fputc('\n', m_file);
#else
	fputs("\r\n", m_file);
#endif
}

static const char* styleToTerminalFormat(LogStyle style)
{
	switch (style)
	{
		case LogStyle::General:  return nullptr;
		case LogStyle::Special:  return "\033[1m\033[32m"; // Bold Green
		case LogStyle::Header:   return "\033[1m\033[34m"; // Bold Blue
		case LogStyle::GameLog:  return "\033[2m";         // Light
		case LogStyle::Emulator: return "\033[34m";        // Blue
		case LogStyle::Trace:    return "\033[2m\033[34m"; // Light Blue
		case LogStyle::Warning:  return "\033[1m\033[35m"; // Bold Magenta
		case LogStyle::Error:    return "\033[1m\033[31m"; // Bold Red
	}
}

void FileLogSink::outputText(LogStyle style, std::string_view msg)
{
#ifdef __POSIX__
	const char* escape;
	if (m_supportsColor && (escape = styleToTerminalFormat(style)))
	{
		fputs(escape, m_file);
		fwrite(msg.data(), 1, msg.size(), m_file);
		fputs("\033[0m", m_file);
	}
	else
#endif
	{
		fwrite(msg.data(), 1, msg.size(), m_file);
	}
}

#ifdef __POSIX__
FileLogSink defaultLogSink(stderr);
#else
OutputDebugStringLogSink::OutputDebugStringLogSink()
	: FileLogSink(false)
{
}

void OutputDebugStringLogSink::outputNewline()
{
	OutputDebugStringW(L"\r\n");
}

void OutputDebugStringLogSink::outputText(LogStyle style, std::string_view msg)
{
	// TODO: Don't use wx
	wxString str = wxString::FromUTF8(msg.data(), msg.size());
	OutputDebugStringW(str.wx_str());
}

void log(LogLevel level, LogStyle style, const LogSource& source, std::string_view msg)
{
	if (!IsDebuggerPresent())
		return;
	std::lock_guard<std::mutex> l(m_mtx);
	logOnThread(level, style, source.currentIndent(), source, msg);
}
#endif

MultiOutputLogSink::MultiOutputLogSink(std::vector<LogSink*> outputs)
	: m_outputs(outputs)
{
}

void MultiOutputLogSink::log(LogLevel level, LogStyle style, const LogSource& source, std::string_view msg)
{
	for (LogSink* output : m_outputs)
	{
		output->log(level, style, source, msg);
	}
}
