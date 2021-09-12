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

#include "Pcsx2Defs.h"
#include <fmt/core.h>
#include <vector>

/// Levels to log at.
///
/// LogSources can filter logs to only display logs with levels greater than or equal to a value
enum class LogLevel : u8
{
	Unset    = 0, ///< Inherit log level from parent
	Trace    = 1, ///< Super spammy logs
	Debug    = 2, ///< Logs used for debugging
	Info     = 3, ///< General information
	Warning  = 4, ///< Warnings
	Error    = 5, ///< Errors
	Critical = 6, ///< Help, the emulator's about to explode
};

static bool operator<(LogLevel a, LogLevel b)
{
	return static_cast<u8>(a) < static_cast<u8>(b);
}

static bool operator<=(LogLevel a, LogLevel b)
{
	return static_cast<u8>(a) <= static_cast<u8>(b);
}

static bool operator>(LogLevel a, LogLevel b)
{
	return static_cast<u8>(a) > static_cast<u8>(b);
}

static bool operator>=(LogLevel a, LogLevel b)
{
	return static_cast<u8>(a) >= static_cast<u8>(b);
}

/// Styles for text
///
/// These intentionally don't specify colors, so each output can choose its own color set based on what colors are available, etc
/// Maybe we'll let users customize the mapping in the future too
enum class LogStyle : u8
{
	General,  ///< General information
	Special,  ///< Infrequent state information... or something like that
	Header,   ///< Header for a block of information
	GameLog,  ///< Logs that come from the game itself
	Emulator, ///< Logs from emulation of emulator components
	Trace,    ///< Trace level logs
	Warning,  ///< Warnings
	Error,    ///< Error / Critical
};

class LogSource;

/// A place for log messages to be sent
/// See `LogSink.h` for conforming implementations
class LogSink
{
public:
	/// Output a log
	/// Must be threadsafe
	virtual void log(LogLevel level, LogStyle style, const LogSource& source, std::string_view msg) = 0;
};

/// ----------------------------------------------------------------------------------------
///  LogSource -- For printing messages to the console.
/// ----------------------------------------------------------------------------------------
/// General ConsoleWrite Threading Guideline:
///   PCSX2 is a threaded environment and multiple threads can write to the console asynchronously.
///   Individual calls to a LogSource will be written in atomic fashion, however "partial" logs may end up interrupted by logs on other threads.
///   In cases where you want to print multi-line blocks of uninterrupted logs, compound the entire log into a single large string and issue that in one call.
///   A `MultiPieceLog` can assist in doing this.
class LogSource
{
	LogLevel m_cachedLevel; ///< The log level to be used for deciding whether to log
	LogLevel m_localLevel;  ///< The log level assigned to this source (Unset means inherit)
	LogStyle m_style;       ///< The style to display normal priority logs from this source
	u8       m_indent;      ///< Current indent level
	LogSink* m_cachedSink;  ///< The sink to be used for logging
	LogSink* m_localSink;   ///< The sink assigned to this source (null means inherit)
	const char* m_name;     ///< This source's name
	LogSource* m_parent;    ///< This source's parent (if any)
	std::vector<LogSource*> m_children; ///< All sources that have this source set as their parent

	/// Recalculate cached level based on local and parent values
	void updateCachedLevel();
	/// Recalculate cached sink based on local and parent values
	void updateCachedSink();

	/// Log a string without formatting
	void logString(LogLevel level, std::string_view string) const;
	/// Log a formatted string
	void logFormat(LogLevel level, std::string_view string, fmt::format_args args) const;
	/// Log a formatted, styled string
	void logFormatStyle(LogLevel level, std::string_view string, fmt::format_args args, LogStyle style) const;

	/// Log the given formatted string
	template <typename S, typename... Args>
	void doLogStyle(LogLevel level, LogStyle style, const S& format, const Args&... args) const
	{
		const auto& vargs = fmt::make_args_checked<Args...>(format, args...);
		logFormatStyle(level, format, vargs, style);
	}

	/// Log the given string
	template <typename S>
	void doLog(LogLevel level, const S& string) const
	{
		fmt::detail::check_format_string<>(string);
		logString(level, string);
	}

	/// Log the given formatted string
	template <typename S, typename Arg1, typename... Args>
	void doLog(LogLevel level, const S& format, const Arg1& arg1, const Args&... args) const
	{
		const auto& vargs = fmt::make_args_checked<Arg1, Args...>(format, arg1, args...);
		logFormat(level, format, vargs);
	}

public:
	/// Can't move due to pointers used for child/parent relationship tracking
	LogSource(LogSource&&) = delete;
	/// Create a LogSource
	LogSource(const char* name, LogStyle style, LogSource* parent, LogLevel baseLevel = LogLevel::Unset);

	/// Should a message with the given level be logged?
	bool shouldLog(LogLevel level) const
	{
		return level >= m_cachedLevel;
	}

	/// Log the given message with the given level using a custom style
	template <typename S, typename... Args>
	void logStyle(LogLevel level, LogStyle style, const S& format, const Args&... args) const
	{
		if (shouldLog(level))
		{
			doLogStyle(level, style, format, args...);
		}
	}

	/// Log the given message with the given level
	template <typename S, typename... Args>
	void log(LogLevel level, const S& format, const Args&... args) const
	{
		// Not neccessarily unlikely, but we expect logging to be fairly slow vs not logging
		if (unlikely(shouldLog(level)))
		{
			doLog(level, format, args...);
		}
	}

	/// Log the given message at the trace level
	template <typename S, typename... Args>
	void trace(const S& format, const Args&... args) const
	{
		log(LogLevel::Trace, format, args...);
	}

	/// Log the given message at the debug level
	template <typename S, typename... Args>
	void debug(const S& format, const Args&... args) const
	{
		log(LogLevel::Debug, format, args...);
	}

	/// Log the given message at the info level
	template <typename S, typename... Args>
	void info(const S& format, const Args&... args) const
	{
		log(LogLevel::Info, format, args...);
	}

	/// Log the given message at the warning level
	template <typename S, typename... Args>
	void warning(const S& format, const Args&... args)
	{
		log(LogLevel::Warning, format, args...);
	}

	/// Log the given message at the error level
	template <typename S, typename... Args>
	void error(const S& format, const Args&... args) const
	{
		log(LogLevel::Error, format, args...);
	}

	/// Log the given message at the critical level
	template <typename S, typename... Args>
	void critical(const S& format, const Args&... args) const
	{
		log(LogLevel::Critical, format, args...);
	}

	///  The style to display normal priority logs from this source
	LogStyle style() const
	{
		return m_style;
	}

	/// The source's current logging level
	LogLevel level() const
	{
		return m_cachedLevel;
	}

	/// The source's name
	const char* name() const
	{
		return m_name;
	}

	/// The source's current indent level
	u8 currentIndent() const
	{
		return m_indent;
	}

	/// Increase the source's indent level
	void increaseIndent(u8 amt)
	{
		m_indent += amt;
	}

	/// Decrease the source's indent level
	void decreaseIndent(u8 amt)
	{
		m_indent -= amt;
	}

	/// Get the default style for the given log level
	LogStyle getStyle(LogLevel level) const;

	/// Change the level of this source
	/// Supply Unset to make the source inherit its parent's level
	void setLevel(LogLevel level);

	/// Change the output of this source
	/// Supply a nullptr to make the source inherit its parent's output
	void setSink(LogSink* sink);
};

/// Gather multiple logs to print atomically
class MultiPieceLog
{
	LogSource& m_source;
	LogLevel m_level;
	std::string m_log;
public:
	MultiPieceLog(LogSource& source, LogLevel level): m_source(source) , m_level(level) {}

	template <typename S>
	void add(const S& str)
	{
		if (!m_source.shouldLog(m_level))
			return;
		fmt::detail::check_format_string<>(str);
		m_log += std::string_view(str);
	}

	template <typename S, typename Arg1, typename... Args>
	void add(const S& str, const Arg1& arg1, const Args&... args)
	{
		if (!m_source.shouldLog(m_level))
			return;
		fmt::format_to(std::back_inserter(m_log), arg1, args...);
	}

	template <typename S, typename... Args>
	void addLine(const S& s, const Args&... args)
	{
		if (!m_source.shouldLog(m_level))
			return;
		add(s, args...);
		m_log.push_back('\n');
	}

	void flush()
	{
		if (m_log.empty())
			return;
		m_source.log(m_level, m_log);
		m_log.clear();
	}

	~MultiPieceLog()
	{
		flush();
	}
};

/// Increase a source's indent level until the end of the current scope
class ScopedLogIndent
{
	LogSource& m_source;
	u8 m_amt;
public:
	ScopedLogIndent(LogSource& source, u8 amt = 1)
		: m_source(source)
		, m_amt(amt)
	{
		m_source.increaseIndent(m_amt);
	}

	~ScopedLogIndent()
	{
		m_source.decreaseIndent(m_amt);
	}
};
