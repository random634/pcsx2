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

#include "FileSystem.h"
#include "Assertions.h"
#include "Console.h"
#include "StringUtil.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <stdlib.h>
#include <sys/param.h>
#else
#include <malloc.h>
#endif

#ifdef __FreeBSD__
#include <sys/sysctl.h>
#endif

#if defined(_WIN32)
#include <shlobj.h>

#if defined(_UWP)
#include <fcntl.h>
#include <io.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.FileProperties.h>
#include <winrt/Windows.Storage.Search.h>
#include <winrt/Windows.Storage.h>
#endif

#else
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef _WIN32
static std::time_t ConvertFileTimeToUnixTime(const FILETIME& ft)
{
	// based off https://stackoverflow.com/a/6161842
	static constexpr s64 WINDOWS_TICK = 10000000;
	static constexpr s64 SEC_TO_UNIX_EPOCH = 11644473600LL;

	const s64 full = static_cast<s64>((static_cast<u64>(ft.dwHighDateTime) << 32) | static_cast<u64>(ft.dwLowDateTime));
	return static_cast<std::time_t>(full / WINDOWS_TICK - SEC_TO_UNIX_EPOCH);
}
#endif

static inline bool FileSystemCharacterIsSane(char c, bool StripSlashes)
{
	if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9') && c != ' ' && c != ' ' &&
		c != '_' && c != '-' && c != '.')
	{
		if (!StripSlashes && (c == '/' || c == '\\'))
			return true;

		return false;
	}

	return true;
}

void FileSystem::SanitizeFileName(char* Destination, u32 cbDestination, const char* FileName, bool StripSlashes /* = true */)
{
	u32 i;
	u32 fileNameLength = static_cast<u32>(std::strlen(FileName));

	if (FileName == Destination)
	{
		for (i = 0; i < fileNameLength; i++)
		{
			if (!FileSystemCharacterIsSane(FileName[i], StripSlashes))
				Destination[i] = '_';
		}
	}
	else
	{
		for (i = 0; i < fileNameLength && i < cbDestination; i++)
		{
			if (FileSystemCharacterIsSane(FileName[i], StripSlashes))
				Destination[i] = FileName[i];
			else
				Destination[i] = '_';
		}
	}
}

void FileSystem::SanitizeFileName(std::string& Destination, bool StripSlashes /* = true*/)
{
	const std::size_t len = Destination.length();
	for (std::size_t i = 0; i < len; i++)
	{
		if (!FileSystemCharacterIsSane(Destination[i], StripSlashes))
			Destination[i] = '_';
	}
}

bool FileSystem::IsAbsolutePath(const std::string_view& path)
{
#ifdef _WIN32
	return (path.length() >= 3 && ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) &&
			path[1] == ':' && (path[2] == '/' || path[2] == '\\'));
#else
	return (path.length() >= 1 && path[0] == '/');
#endif
}

std::string_view FileSystem::GetExtension(const std::string_view& path)
{
	std::string_view::size_type pos = path.rfind('.');
	if (pos == std::string::npos)
		return path;

	return path.substr(pos + 1);
}

std::string_view FileSystem::StripExtension(const std::string_view& path)
{
	std::string_view::size_type pos = path.rfind('.');
	if (pos == std::string::npos)
		return path;

	return path.substr(0, pos);
}

std::string FileSystem::ReplaceExtension(const std::string_view& path, const std::string_view& new_extension)
{
	std::string_view::size_type pos = path.rfind('.');
	if (pos == std::string::npos)
		return std::string(path);

	std::string ret(path, 0, pos + 1);
	ret.append(new_extension);
	return ret;
}

static std::string_view::size_type GetLastSeperatorPosition(const std::string_view& filename, bool include_separator)
{
	std::string_view::size_type last_separator = filename.rfind('/');
	if (include_separator && last_separator != std::string_view::npos)
		last_separator++;

#if defined(_WIN32)
	std::string_view::size_type other_last_separator = filename.rfind('\\');
	if (other_last_separator != std::string_view::npos)
	{
		if (include_separator)
			other_last_separator++;
		if (last_separator == std::string_view::npos || other_last_separator > last_separator)
			last_separator = other_last_separator;
	}
#endif

	return last_separator;
}

std::string FileSystem::GetDisplayNameFromPath(const std::string_view& path)
{
	return std::string(GetFileNameFromPath(path));
}

std::string_view FileSystem::GetPathDirectory(const std::string_view& path)
{
	std::string::size_type pos = GetLastSeperatorPosition(path, false);
	if (pos == std::string_view::npos)
		return {};

	return path.substr(0, pos);
}

std::string_view FileSystem::GetFileNameFromPath(const std::string_view& path)
{
	std::string_view::size_type pos = GetLastSeperatorPosition(path, true);
	if (pos == std::string_view::npos)
		return path;

	return path.substr(pos);
}

std::string_view FileSystem::GetFileTitleFromPath(const std::string_view& path)
{
	std::string_view filename(GetFileNameFromPath(path));
	std::string::size_type pos = filename.rfind('.');
	if (pos == std::string_view::npos)
		return filename;

	return filename.substr(0, pos);
}

std::vector<std::string> FileSystem::GetRootDirectoryList()
{
	std::vector<std::string> results;

#if defined(_WIN32) && !defined(_UWP)
	char buf[256];
	if (GetLogicalDriveStringsA(sizeof(buf), buf) != 0)
	{
		const char* ptr = buf;
		while (*ptr != '\0')
		{
			const std::size_t len = std::strlen(ptr);
			results.emplace_back(ptr, len);
			ptr += len + 1u;
		}
	}
#elif defined(_UWP)
	if (const auto install_location = winrt::Windows::ApplicationModel::Package::Current().InstalledLocation();
		install_location)
	{
		if (const auto path = install_location.Path(); !path.empty())
			results.push_back(StringUtil::WideStringToUTF8String(path));
	}

	if (const auto local_location = winrt::Windows::Storage::ApplicationData::Current().LocalFolder(); local_location)
	{
		if (const auto path = local_location.Path(); !path.empty())
			results.push_back(StringUtil::WideStringToUTF8String(path));
	}

	const auto devices = winrt::Windows::Storage::KnownFolders::RemovableDevices();
	const auto folders_task(devices.GetFoldersAsync());
	for (const auto& storage_folder : folders_task.get())
	{
		const auto path = storage_folder.Path();
		if (!path.empty())
			results.push_back(StringUtil::WideStringToUTF8String(path));
	}
#else
	const char* home_path = std::getenv("HOME");
	if (home_path)
		results.push_back(home_path);

	results.push_back("/");
#endif

	return results;
}

std::string FileSystem::BuildRelativePath(const std::string_view& filename, const std::string_view& new_filename)
{
	std::string new_string;

	std::string_view::size_type pos = GetLastSeperatorPosition(filename, true);
	if (pos != std::string_view::npos)
		new_string.assign(filename, 0, pos);
	new_string.append(new_filename);
	return new_string;
}

#ifdef _UWP
static std::FILE* OpenCFileUWP(const wchar_t* wfilename, const wchar_t* mode)
{
	DWORD access = 0;
	DWORD share = 0;
	DWORD disposition = 0;

	int flags = 0;
	const wchar_t* tmode = mode;
	while (*tmode)
	{
		if (*tmode == L'r' && *(tmode + 1) == L'+')
		{
			access = GENERIC_READ | GENERIC_WRITE;
			share = 0;
			disposition = OPEN_EXISTING;
			flags |= _O_RDWR;
			tmode += 2;
		}
		else if (*tmode == L'w' && *(tmode + 1) == L'+')
		{
			access = GENERIC_READ | GENERIC_WRITE;
			share = 0;
			disposition = CREATE_ALWAYS;
			flags |= _O_RDWR | _O_CREAT | _O_TRUNC;
			tmode += 2;
		}
		else if (*tmode == L'a' && *(tmode + 1) == L'+')
		{
			access = GENERIC_READ | GENERIC_WRITE;
			share = 0;
			disposition = CREATE_ALWAYS;
			flags |= _O_RDWR | _O_APPEND | _O_CREAT | _O_TRUNC;
			tmode += 2;
		}
		else if (*tmode == L'r')
		{
			access = GENERIC_READ;
			share = 0;
			disposition = OPEN_EXISTING;
			flags |= _O_RDONLY;
			tmode++;
		}
		else if (*tmode == L'w')
		{
			access = GENERIC_WRITE;
			share = 0;
			disposition = CREATE_ALWAYS;
			flags |= _O_WRONLY | _O_CREAT | _O_TRUNC;
			tmode++;
		}
		else if (*tmode == L'a')
		{
			access = GENERIC_READ | GENERIC_WRITE;
			share = 0;
			disposition = CREATE_ALWAYS;
			flags |= _O_WRONLY | _O_APPEND | _O_CREAT | _O_TRUNC;
			tmode++;
		}
		else if (*tmode == L'b')
		{
			flags |= _O_BINARY;
			tmode++;
		}
		else if (*tmode == L'S')
		{
			flags |= _O_SEQUENTIAL;
			tmode++;
		}
		else if (*tmode == L'R')
		{
			flags |= _O_RANDOM;
			tmode++;
		}
		else
		{
			Log_ErrorPrintf("Unknown mode flags: '%s'", StringUtil::WideStringToUTF8String(mode).c_str());
			return nullptr;
		}
	}

	HANDLE hFile = CreateFileFromAppW(wfilename, access, share, nullptr, disposition, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return nullptr;

	if (flags & _O_APPEND && !SetFilePointerEx(hFile, LARGE_INTEGER{}, nullptr, FILE_END))
	{
		Log_ErrorPrintf("SetFilePointerEx() failed: %08X", GetLastError());
		CloseHandle(hFile);
		return nullptr;
	}

	int fd = _open_osfhandle(reinterpret_cast<intptr_t>(hFile), flags);
	if (fd < 0)
	{
		CloseHandle(hFile);
		return nullptr;
	}

	std::FILE* fp = _wfdopen(fd, mode);
	if (!fp)
	{
		_close(fd);
		return nullptr;
	}

	return fp;
}
#endif // _UWP

std::FILE* FileSystem::OpenCFile(const char* filename, const char* mode)
{
#ifdef _WIN32
	int filename_len = static_cast<int>(std::strlen(filename));
	int mode_len = static_cast<int>(std::strlen(mode));
	int wlen = MultiByteToWideChar(CP_UTF8, 0, filename, filename_len, nullptr, 0);
	int wmodelen = MultiByteToWideChar(CP_UTF8, 0, mode, mode_len, nullptr, 0);
	if (wlen > 0 && wmodelen > 0)
	{
		wchar_t* wfilename = static_cast<wchar_t*>(alloca(sizeof(wchar_t) * (wlen + 1)));
		wchar_t* wmode = static_cast<wchar_t*>(alloca(sizeof(wchar_t) * (wmodelen + 1)));
		wlen = MultiByteToWideChar(CP_UTF8, 0, filename, filename_len, wfilename, wlen);
		wmodelen = MultiByteToWideChar(CP_UTF8, 0, mode, mode_len, wmode, wmodelen);
		if (wlen > 0 && wmodelen > 0)
		{
			wfilename[wlen] = 0;
			wmode[wmodelen] = 0;

			std::FILE* fp;
			if (_wfopen_s(&fp, wfilename, wmode) != 0)
			{
#ifdef _UWP
				return OpenCFileUWP(wfilename, wmode);
#else
				return nullptr;
#endif
			}

			return fp;
		}
	}

	std::FILE* fp;
	if (fopen_s(&fp, filename, mode) != 0)
		return nullptr;

	return fp;
#else
	return std::fopen(filename, mode);
#endif
}

int FileSystem::OpenFDFile(const char* filename, int mode)
{
#ifdef _WIN32
	int filename_len = static_cast<int>(std::strlen(filename));
	int wlen = MultiByteToWideChar(CP_UTF8, 0, filename, filename_len, nullptr, 0);
	if (wlen > 0)
	{
		wchar_t* wfilename = static_cast<wchar_t*>(alloca(sizeof(wchar_t) * (wlen + 1)));
		wlen = MultiByteToWideChar(CP_UTF8, 0, filename, filename_len, wfilename, wlen);
		if (wlen > 0)
		{
			wfilename[wlen] = 0;

			// TODO: UWP
			return _wopen(wfilename, mode);
		}
	}

	return -1;
#else
	return open(filename, mode);
#endif
}

FileSystem::ManagedCFilePtr FileSystem::OpenManagedCFile(const char* filename, const char* mode)
{
	return ManagedCFilePtr(OpenCFile(filename, mode), [](std::FILE* fp) { std::fclose(fp); });
}

int FileSystem::FSeek64(std::FILE* fp, s64 offset, int whence)
{
#ifdef _WIN32
	return _fseeki64(fp, offset, whence);
#else
	// Prevent truncation on platforms which don't have a 64-bit off_t (Android 32-bit).
	if constexpr (sizeof(off_t) != sizeof(s64))
	{
		if (offset < std::numeric_limits<off_t>::min() || offset > std::numeric_limits<off_t>::max())
			return -1;
	}

	return fseeko(fp, static_cast<off_t>(offset), whence);
#endif
}

s64 FileSystem::FTell64(std::FILE* fp)
{
#ifdef _WIN32
	return static_cast<s64>(_ftelli64(fp));
#else
	return static_cast<s64>(ftello(fp));
#endif
}

s64 FileSystem::FSize64(std::FILE* fp)
{
	const s64 pos = FTell64(fp);
	if (pos >= 0)
	{
		if (FSeek64(fp, 0, SEEK_END) == 0)
		{
			const s64 size = FTell64(fp);
			if (FSeek64(fp, pos, SEEK_SET) == 0)
				return size;
		}
	}

	return -1;
}

s64 FileSystem::GetPathFileSize(const char* Path)
{
	FILESYSTEM_STAT_DATA sd;
	if (!StatFile(Path, &sd))
		return -1;

	return sd.Size;
}

std::optional<std::vector<u8>> FileSystem::ReadBinaryFile(const char* filename)
{
	ManagedCFilePtr fp = OpenManagedCFile(filename, "rb");
	if (!fp)
		return std::nullopt;

	return ReadBinaryFile(fp.get());
}

std::optional<std::vector<u8>> FileSystem::ReadBinaryFile(std::FILE* fp)
{
	std::fseek(fp, 0, SEEK_END);
	long size = std::ftell(fp);
	std::fseek(fp, 0, SEEK_SET);
	if (size < 0)
		return std::nullopt;

	std::vector<u8> res(static_cast<size_t>(size));
	if (size > 0 && std::fread(res.data(), 1u, static_cast<size_t>(size), fp) != static_cast<size_t>(size))
		return std::nullopt;

	return res;
}

std::optional<std::string> FileSystem::ReadFileToString(const char* filename)
{
	ManagedCFilePtr fp = OpenManagedCFile(filename, "rb");
	if (!fp)
		return std::nullopt;

	return ReadFileToString(fp.get());
}

std::optional<std::string> FileSystem::ReadFileToString(std::FILE* fp)
{
	std::fseek(fp, 0, SEEK_END);
	long size = std::ftell(fp);
	std::fseek(fp, 0, SEEK_SET);
	if (size < 0)
		return std::nullopt;

	std::string res;
	res.resize(static_cast<size_t>(size));
	if (size > 0 && std::fread(res.data(), 1u, static_cast<size_t>(size), fp) != static_cast<size_t>(size))
		return std::nullopt;

	return res;
}

bool FileSystem::WriteBinaryFile(const char* filename, const void* data, size_t data_length)
{
	ManagedCFilePtr fp = OpenManagedCFile(filename, "wb");
	if (!fp)
		return false;

	if (data_length > 0 && std::fwrite(data, 1u, data_length, fp.get()) != data_length)
		return false;

	return true;
}

bool FileSystem::WriteFileToString(const char* filename, const std::string_view& sv)
{
	ManagedCFilePtr fp = OpenManagedCFile(filename, "wb");
	if (!fp)
		return false;

	if (sv.length() > 0 && std::fwrite(sv.data(), 1u, sv.length(), fp.get()) != sv.length())
		return false;

	return true;
}

#ifdef _WIN32

static u32 TranslateWin32Attributes(u32 Win32Attributes)
{
	u32 r = 0;

	if (Win32Attributes & FILE_ATTRIBUTE_DIRECTORY)
		r |= FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY;
	if (Win32Attributes & FILE_ATTRIBUTE_READONLY)
		r |= FILESYSTEM_FILE_ATTRIBUTE_READ_ONLY;
	if (Win32Attributes & FILE_ATTRIBUTE_COMPRESSED)
		r |= FILESYSTEM_FILE_ATTRIBUTE_COMPRESSED;

	return r;
}

static DWORD WrapGetFileAttributes(const wchar_t* path)
{
#ifndef _UWP
	return GetFileAttributesW(path);
#else
	WIN32_FILE_ATTRIBUTE_DATA fad;
	if (!GetFileAttributesExFromAppW(path, GetFileExInfoStandard, &fad))
		return INVALID_FILE_ATTRIBUTES;

	return fad.dwFileAttributes;
#endif
}

static u32 RecursiveFindFiles(const char* OriginPath, const char* ParentPath, const char* Path, const char* Pattern,
	u32 Flags, FileSystem::FindResultsArray* pResults)
{
	std::string tempStr;
	if (Path)
	{
		if (ParentPath)
			tempStr = StringUtil::StdStringFromFormat("%s\\%s\\%s\\*", OriginPath, ParentPath, Path);
		else
			tempStr = StringUtil::StdStringFromFormat("%s\\%s\\*", OriginPath, Path);
	}
	else
	{
		tempStr = StringUtil::StdStringFromFormat("%s\\*", OriginPath);
	}

	// holder for utf-8 conversion
	WIN32_FIND_DATAW wfd;
	std::string utf8_filename;
	utf8_filename.reserve((sizeof(wfd.cFileName) / sizeof(wfd.cFileName[0])) * 2);

#ifndef _UWP
	HANDLE hFind = FindFirstFileW(StringUtil::UTF8StringToWideString(tempStr).c_str(), &wfd);
#else
	HANDLE hFind = FindFirstFileExFromAppW(StringUtil::UTF8StringToWideString(tempStr).c_str(), FindExInfoBasic, &wfd,
		FindExSearchNameMatch, nullptr, 0);
#endif

	if (hFind == INVALID_HANDLE_VALUE)
		return 0;

	// small speed optimization for '*' case
	bool hasWildCards = false;
	bool wildCardMatchAll = false;
	u32 nFiles = 0;
	if (std::strpbrk(Pattern, "*?") != nullptr)
	{
		hasWildCards = true;
		wildCardMatchAll = !(std::strcmp(Pattern, "*"));
	}

	// iterate results
	do
	{
		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN && !(Flags & FILESYSTEM_FIND_HIDDEN_FILES))
			continue;

		if (wfd.cFileName[0] == L'.')
		{
			if (wfd.cFileName[1] == L'\0' || (wfd.cFileName[1] == L'.' && wfd.cFileName[2] == L'\0'))
				continue;
		}

		if (!StringUtil::WideStringToUTF8String(utf8_filename, wfd.cFileName))
			continue;

		FILESYSTEM_FIND_DATA outData;
		outData.Attributes = 0;

		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (Flags & FILESYSTEM_FIND_RECURSIVE)
			{
				// recurse into this directory
				if (ParentPath != nullptr)
				{
					const std::string recurseDir = StringUtil::StdStringFromFormat("%s\\%s", ParentPath, Path);
					nFiles += RecursiveFindFiles(OriginPath, recurseDir.c_str(), utf8_filename.c_str(), Pattern, Flags, pResults);
				}
				else
				{
					nFiles += RecursiveFindFiles(OriginPath, Path, utf8_filename.c_str(), Pattern, Flags, pResults);
				}
			}

			if (!(Flags & FILESYSTEM_FIND_FOLDERS))
				continue;

			outData.Attributes |= FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY;
		}
		else
		{
			if (!(Flags & FILESYSTEM_FIND_FILES))
				continue;
		}

		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
			outData.Attributes |= FILESYSTEM_FILE_ATTRIBUTE_READ_ONLY;

		// match the filename
		if (hasWildCards)
		{
			if (!wildCardMatchAll && !StringUtil::WildcardMatch(utf8_filename.c_str(), Pattern))
				continue;
		}
		else
		{
			if (std::strcmp(utf8_filename.c_str(), Pattern) != 0)
				continue;
		}

		// add file to list
		// TODO string formatter, clean this mess..
		if (!(Flags & FILESYSTEM_FIND_RELATIVE_PATHS))
		{
			if (ParentPath != nullptr)
				outData.FileName =
					StringUtil::StdStringFromFormat("%s\\%s\\%s\\%s", OriginPath, ParentPath, Path, utf8_filename.c_str());
			else if (Path != nullptr)
				outData.FileName = StringUtil::StdStringFromFormat("%s\\%s\\%s", OriginPath, Path, utf8_filename.c_str());
			else
				outData.FileName = StringUtil::StdStringFromFormat("%s\\%s", OriginPath, utf8_filename.c_str());
		}
		else
		{
			if (ParentPath != nullptr)
				outData.FileName = StringUtil::StdStringFromFormat("%s\\%s\\%s", ParentPath, Path, utf8_filename.c_str());
			else if (Path != nullptr)
				outData.FileName = StringUtil::StdStringFromFormat("%s\\%s", Path, utf8_filename.c_str());
			else
				outData.FileName = utf8_filename;
		}

		outData.ModificationTime = ConvertFileTimeToUnixTime(wfd.ftLastWriteTime);
		outData.Size = (static_cast<u64>(wfd.nFileSizeHigh) << 32) | static_cast<u64>(wfd.nFileSizeLow);

		nFiles++;
		pResults->push_back(std::move(outData));
	} while (FindNextFileW(hFind, &wfd) == TRUE);
	FindClose(hFind);

	return nFiles;
}

bool FileSystem::FindFiles(const char* Path, const char* Pattern, u32 Flags, FindResultsArray* pResults)
{
	// has a path
	if (Path[0] == '\0')
		return false;

	// clear result array
	if (!(Flags & FILESYSTEM_FIND_KEEP_ARRAY))
		pResults->clear();

	// enter the recursive function
	return (RecursiveFindFiles(Path, nullptr, nullptr, Pattern, Flags, pResults) > 0);
}

bool FileSystem::StatFile(const char* path, FILESYSTEM_STAT_DATA* pStatData)
{
	// has a path
	if (path[0] == '\0')
		return false;

	// convert to wide string
	int len = static_cast<int>(std::strlen(path));
	int wlen = MultiByteToWideChar(CP_UTF8, 0, path, len, nullptr, 0);
	if (wlen <= 0)
		return false;

	wchar_t* wpath = static_cast<wchar_t*>(alloca(sizeof(wchar_t) * (wlen + 1)));
	wlen = MultiByteToWideChar(CP_UTF8, 0, path, len, wpath, wlen);
	if (wlen <= 0)
		return false;

	wpath[wlen] = 0;

#ifndef _UWP
	// determine attributes for the path. if it's a directory, things have to be handled differently..
	DWORD fileAttributes = GetFileAttributesW(wpath);
	if (fileAttributes == INVALID_FILE_ATTRIBUTES)
		return false;

	// test if it is a directory
	HANDLE hFile;
	if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		hFile = CreateFileW(wpath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
			OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	}
	else
	{
		hFile = CreateFileW(wpath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
			OPEN_EXISTING, 0, nullptr);
	}

	// createfile succeded?
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	// use GetFileInformationByHandle
	BY_HANDLE_FILE_INFORMATION bhfi;
	if (GetFileInformationByHandle(hFile, &bhfi) == FALSE)
	{
		CloseHandle(hFile);
		return false;
	}

	// close handle
	CloseHandle(hFile);

	// fill in the stat data
	pStatData->Attributes = TranslateWin32Attributes(bhfi.dwFileAttributes);
	pStatData->ModificationTime = ConvertFileTimeToUnixTime(bhfi.ftLastWriteTime);
	pStatData->Size = static_cast<s64>(((u64)bhfi.nFileSizeHigh) << 32 | (u64)bhfi.nFileSizeLow);
	return true;
#else
	WIN32_FILE_ATTRIBUTE_DATA fad;
	if (!GetFileAttributesExFromAppW(wpath, GetFileExInfoStandard, &fad))
		return false;

	pStatData->Attributes = TranslateWin32Attributes(fad.dwFileAttributes);
	pStatData->ModificationTime = ConvertFileTimeToUnixTime(fad.ftLastWriteTime);
	pStatData->Size = static_cast<s64>(((u64)fad.nFileSizeHigh) << 32 | (u64)fad.nFileSizeLow);
	return true;
#endif
}

bool FileSystem::StatFile(std::FILE* fp, FILESYSTEM_STAT_DATA* pStatData)
{
	const int fd = _fileno(fp);
	if (fd < 0)
		return false;

	struct _stat64 st;
	if (_fstati64(fd, &st) != 0)
		return false;

	// parse attributes
	pStatData->ModificationTime = st.st_mtime;
	pStatData->Attributes = 0;
	if ((st.st_mode & _S_IFMT) == _S_IFDIR)
		pStatData->Attributes |= FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY;

	// parse size
	if ((st.st_mode & _S_IFMT) == _S_IFREG)
		pStatData->Size = st.st_size;
	else
		pStatData->Size = 0;

	return true;
}

bool FileSystem::FileExists(const char* path)
{
	// has a path
	if (path[0] == '\0')
		return false;

	// convert to wide string
	int len = static_cast<int>(std::strlen(path));
	int wlen = MultiByteToWideChar(CP_UTF8, 0, path, len, nullptr, 0);
	if (wlen <= 0)
		return false;

	wchar_t* wpath = static_cast<wchar_t*>(alloca(sizeof(wchar_t) * (wlen + 1)));
	wlen = MultiByteToWideChar(CP_UTF8, 0, path, len, wpath, wlen);
	if (wlen <= 0)
		return false;

	wpath[wlen] = 0;

	// determine attributes for the path. if it's a directory, things have to be handled differently..
	DWORD fileAttributes = WrapGetFileAttributes(wpath);
	if (fileAttributes == INVALID_FILE_ATTRIBUTES)
		return false;

	if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return false;
	else
		return true;
}

bool FileSystem::DirectoryExists(const char* path)
{
	// has a path
	if (path[0] == '\0')
		return false;

	// convert to wide string
	int len = static_cast<int>(std::strlen(path));
	int wlen = MultiByteToWideChar(CP_UTF8, 0, path, len, nullptr, 0);
	if (wlen <= 0)
		return false;

	wchar_t* wpath = static_cast<wchar_t*>(alloca(sizeof(wchar_t) * (wlen + 1)));
	wlen = MultiByteToWideChar(CP_UTF8, 0, path, len, wpath, wlen);
	if (wlen <= 0)
		return false;

	wpath[wlen] = 0;

	// determine attributes for the path. if it's a directory, things have to be handled differently..
	DWORD fileAttributes = WrapGetFileAttributes(wpath);
	if (fileAttributes == INVALID_FILE_ATTRIBUTES)
		return false;

	if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return true;
	else
		return false;
}

bool FileSystem::CreateDirectoryPath(const char* Path, bool Recursive)
{
	std::wstring wpath(StringUtil::UTF8StringToWideString(Path));

	// has a path
	if (wpath[0] == L'\0')
		return false;

		// try just flat-out, might work if there's no other segments that have to be made
#ifndef _UWP
	if (CreateDirectoryW(wpath.c_str(), nullptr))
		return true;
#else
	if (CreateDirectoryFromAppW(wpath.c_str(), nullptr))
		return true;
#endif

	// check error
	DWORD lastError = GetLastError();
	if (lastError == ERROR_ALREADY_EXISTS)
	{
		// check the attributes
		u32 Attributes = WrapGetFileAttributes(wpath.c_str());
		if (Attributes != INVALID_FILE_ATTRIBUTES && Attributes & FILE_ATTRIBUTE_DIRECTORY)
			return true;
		else
			return false;
	}
	else if (lastError == ERROR_PATH_NOT_FOUND)
	{
		// part of the path does not exist, so we'll create the parent folders, then
		// the full path again. allocate another buffer with the same length
		u32 pathLength = static_cast<u32>(wpath.size());
		wchar_t* tempStr = (wchar_t*)alloca(sizeof(wchar_t) * (pathLength + 1));

		// create directories along the path
		for (u32 i = 0; i < pathLength; i++)
		{
			if (wpath[i] == L'\\' || wpath[i] == L'/')
			{
				tempStr[i] = L'\0';

#ifndef _UWP
				const BOOL result = CreateDirectoryW(tempStr, nullptr);
#else
				const BOOL result = CreateDirectoryFromAppW(tempStr, nullptr);
#endif
				if (!result)
				{
					lastError = GetLastError();
					if (lastError != ERROR_ALREADY_EXISTS) // fine, continue to next path segment
						return false;
				}
			}

			tempStr[i] = wpath[i];
		}

		// re-create the end if it's not a separator, check / as well because windows can interpret them
		if (wpath[pathLength - 1] != L'\\' && wpath[pathLength - 1] != L'/')
		{
#ifndef _UWP
			const BOOL result = CreateDirectoryW(wpath.c_str(), nullptr);
#else
			const BOOL result = CreateDirectoryFromAppW(wpath.c_str(), nullptr);
#endif
			if (!result)
			{
				lastError = GetLastError();
				if (lastError != ERROR_ALREADY_EXISTS)
					return false;
			}
		}

		// ok
		return true;
	}
	else
	{
		// unhandled error
		return false;
	}
}

bool FileSystem::DeleteFilePath(const char* Path)
{
	if (Path[0] == '\0')
		return false;

	const std::wstring wpath(StringUtil::UTF8StringToWideString(Path));
	const DWORD fileAttributes = WrapGetFileAttributes(wpath.c_str());
	if (fileAttributes == INVALID_FILE_ATTRIBUTES || fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return false;

#ifndef _UWP
	return (DeleteFileW(wpath.c_str()) == TRUE);
#else
	return (DeleteFileFromAppW(wpath.c_str()) == TRUE);
#endif
}

bool FileSystem::RenamePath(const char* OldPath, const char* NewPath)
{
	const std::wstring old_wpath(StringUtil::UTF8StringToWideString(OldPath));
	const std::wstring new_wpath(StringUtil::UTF8StringToWideString(NewPath));

#ifndef _UWP
	if (!MoveFileExW(old_wpath.c_str(), new_wpath.c_str(), MOVEFILE_REPLACE_EXISTING))
	{
		Console.Error("MoveFileEx('%s', '%s') failed: %08X", OldPath, NewPath, GetLastError());
		return false;
	}
#else
	// try moving if it doesn't exist, since ReplaceFile fails on non-existing destinations
	if (WrapGetFileAttributes(new_wpath.c_str()) != INVALID_FILE_ATTRIBUTES)
	{
		if (!DeleteFileFromAppW(new_wpath.c_str()))
		{
			Log_ErrorPrintf("DeleteFileFromAppW('%s') failed: %08X", new_wpath.c_str(), GetLastError());
			return false;
		}
	}

	if (!MoveFileFromAppW(old_wpath.c_str(), new_wpath.c_str()))
	{
		Log_ErrorPrintf("MoveFileFromAppW('%s', '%s') failed: %08X", OldPath, NewPath, GetLastError());
		return false;
	}
#endif

	return true;
}

static bool RecursiveDeleteDirectory(const std::wstring& wpath, bool Recursive)
{
	// ensure it exists
	const DWORD fileAttributes = WrapGetFileAttributes(wpath.c_str());
	if (fileAttributes == INVALID_FILE_ATTRIBUTES || fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return false;

	// non-recursive case just try removing the directory
	if (!Recursive)
	{
#ifndef _UWP
		return (RemoveDirectoryW(wpath.c_str()) == TRUE);
#else
		return (RemoveDirectoryFromAppW(wpath.c_str()) == TRUE);
#endif
	}

	// doing a recursive delete
	std::wstring fileName = wpath;
	fileName += L"\\*";

	// is there any files?
	WIN32_FIND_DATAW findData;
#ifndef _UWP
	HANDLE hFind = FindFirstFileW(fileName.c_str(), &findData);
#else
	HANDLE hFind =
		FindFirstFileExFromAppW(fileName.c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, 0);
#endif
	if (hFind == INVALID_HANDLE_VALUE)
		return false;

	// search through files
	do
	{
		// skip . and ..
		if (findData.cFileName[0] == L'.')
		{
			if ((findData.cFileName[1] == L'\0') || (findData.cFileName[1] == L'.' && findData.cFileName[2] == L'\0'))
			{
				continue;
			}
		}

		// found a directory?
		fileName = wpath;
		fileName += L"\\";
		fileName += findData.cFileName;
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// recurse into that
			if (!RecursiveDeleteDirectory(fileName, true))
			{
				FindClose(hFind);
				return false;
			}
		}
		else
		{
			// found a file, so delete it
#ifndef _UWP
			const BOOL result = DeleteFileW(fileName.c_str());
#else
			const BOOL result = DeleteFileFromAppW(fileName.c_str());
#endif
			if (!result)
			{
				FindClose(hFind);
				return false;
			}
		}
	} while (FindNextFileW(hFind, &findData));
	FindClose(hFind);

	// nuke the directory itself
#ifndef _UWP
	const BOOL result = RemoveDirectoryW(wpath.c_str());
#else
	const BOOL result = RemoveDirectoryFromAppW(wpath.c_str());
#endif
	if (!result)
		return false;

	// done
	return true;
}

bool FileSystem::DeleteDirectoryPath(const char* Path, bool Recursive)
{
	const std::wstring wpath(StringUtil::UTF8StringToWideString(Path));
	return RecursiveDeleteDirectory(wpath, Recursive);
}

std::string FileSystem::GetProgramPath()
{
	std::wstring buffer;
	buffer.resize(MAX_PATH);

	// Fall back to the main module if this fails.
	HMODULE module = nullptr;
#ifndef _UWP
	GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		reinterpret_cast<LPCWSTR>(&GetProgramPath), &module);
#endif

	for (;;)
	{
		DWORD nChars = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
		if (nChars == static_cast<DWORD>(buffer.size()) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			buffer.resize(buffer.size() * 2);
			continue;
		}

		buffer.resize(nChars);
		break;
	}

	return StringUtil::WideStringToUTF8String(buffer);
}

std::string FileSystem::GetWorkingDirectory()
{
	DWORD required_size = GetCurrentDirectoryW(0, nullptr);
	if (!required_size)
		return {};

	std::wstring buffer;
	buffer.resize(required_size - 1);

	if (!GetCurrentDirectoryW(static_cast<DWORD>(buffer.size() + 1), buffer.data()))
		return {};

	return StringUtil::WideStringToUTF8String(buffer);
}

bool FileSystem::SetWorkingDirectory(const char* path)
{
	const std::wstring wpath(StringUtil::UTF8StringToWideString(path));
	return (SetCurrentDirectoryW(wpath.c_str()) == TRUE);
}

#else

static u32 RecursiveFindFiles(const char* OriginPath, const char* ParentPath, const char* Path, const char* Pattern,
	u32 Flags, FileSystem::FindResultsArray* pResults)
{
	std::string tempStr;
	if (Path)
	{
		if (ParentPath)
			tempStr = StringUtil::StdStringFromFormat("%s/%s/%s", OriginPath, ParentPath, Path);
		else
			tempStr = StringUtil::StdStringFromFormat("%s/%s", OriginPath, Path);
	}
	else
	{
		tempStr = StringUtil::StdStringFromFormat("%s", OriginPath);
	}

	DIR* pDir = opendir(tempStr.c_str());
	if (pDir == nullptr)
		return 0;

	// small speed optimization for '*' case
	bool hasWildCards = false;
	bool wildCardMatchAll = false;
	u32 nFiles = 0;
	if (std::strpbrk(Pattern, "*?"))
	{
		hasWildCards = true;
		wildCardMatchAll = (std::strcmp(Pattern, "*") == 0);
	}

	// iterate results
	struct dirent* pDirEnt;
	while ((pDirEnt = readdir(pDir)) != nullptr)
	{
		if (pDirEnt->d_name[0] == '.')
		{
			if (pDirEnt->d_name[1] == '\0' || (pDirEnt->d_name[1] == '.' && pDirEnt->d_name[2] == '\0'))
				continue;

			if (!(Flags & FILESYSTEM_FIND_HIDDEN_FILES))
				continue;
		}

		std::string full_path;
		if (ParentPath != nullptr)
			full_path = StringUtil::StdStringFromFormat("%s/%s/%s/%s", OriginPath, ParentPath, Path, pDirEnt->d_name);
		else if (Path != nullptr)
			full_path = StringUtil::StdStringFromFormat("%s/%s/%s", OriginPath, Path, pDirEnt->d_name);
		else
			full_path = StringUtil::StdStringFromFormat("%s/%s", OriginPath, pDirEnt->d_name);

		FILESYSTEM_FIND_DATA outData;
		outData.Attributes = 0;

#if defined(__HAIKU__) || defined(__APPLE__) || defined(__FreeBSD__)
		struct stat sDir;
		if (stat(full_path.c_str(), &sDir) < 0)
			continue;

#else
		struct stat64 sDir;
		if (stat64(full_path.c_str(), &sDir) < 0)
			continue;
#endif

		if (S_ISDIR(sDir.st_mode))
		{
			if (Flags & FILESYSTEM_FIND_RECURSIVE)
			{
				// recurse into this directory
				if (ParentPath != nullptr)
				{
					std::string recursiveDir = StringUtil::StdStringFromFormat("%s/%s", ParentPath, Path);
					nFiles += RecursiveFindFiles(OriginPath, recursiveDir.c_str(), pDirEnt->d_name, Pattern, Flags, pResults);
				}
				else
				{
					nFiles += RecursiveFindFiles(OriginPath, Path, pDirEnt->d_name, Pattern, Flags, pResults);
				}
			}

			if (!(Flags & FILESYSTEM_FIND_FOLDERS))
				continue;

			outData.Attributes |= FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY;
		}
		else
		{
			if (!(Flags & FILESYSTEM_FIND_FILES))
				continue;
		}

		outData.Size = static_cast<u64>(sDir.st_size);
		outData.ModificationTime = sDir.st_mtime;

		// match the filename
		if (hasWildCards)
		{
			if (!wildCardMatchAll && !StringUtil::WildcardMatch(pDirEnt->d_name, Pattern))
				continue;
		}
		else
		{
			if (std::strcmp(pDirEnt->d_name, Pattern) != 0)
				continue;
		}

		// add file to list
		// TODO string formatter, clean this mess..
		if (!(Flags & FILESYSTEM_FIND_RELATIVE_PATHS))
		{
			outData.FileName = std::move(full_path);
		}
		else
		{
			if (ParentPath != nullptr)
				outData.FileName = StringUtil::StdStringFromFormat("%s/%s/%s", ParentPath, Path, pDirEnt->d_name);
			else if (Path != nullptr)
				outData.FileName = StringUtil::StdStringFromFormat("%s/%s", Path, pDirEnt->d_name);
			else
				outData.FileName = pDirEnt->d_name;
		}

		nFiles++;
		pResults->push_back(std::move(outData));
	}

	closedir(pDir);
	return nFiles;
}

bool FileSystem::FindFiles(const char* Path, const char* Pattern, u32 Flags, FindResultsArray* pResults)
{
	// has a path
	if (Path[0] == '\0')
		return false;

	// clear result array
	if (!(Flags & FILESYSTEM_FIND_KEEP_ARRAY))
		pResults->clear();

	// enter the recursive function
	return (RecursiveFindFiles(Path, nullptr, nullptr, Pattern, Flags, pResults) > 0);
}

bool FileSystem::StatFile(const char* Path, FILESYSTEM_STAT_DATA* pStatData)
{
	// has a path
	if (Path[0] == '\0')
		return false;

		// stat file
#if defined(__HAIKU__) || defined(__APPLE__) || defined(__FreeBSD__)
	struct stat sysStatData;
	if (stat(Path, &sysStatData) < 0)
#else
	struct stat64 sysStatData;
	if (stat64(Path, &sysStatData) < 0)
#endif
		return false;

	// parse attributes
	pStatData->ModificationTime = sysStatData.st_mtime;
	pStatData->Attributes = 0;
	if (S_ISDIR(sysStatData.st_mode))
		pStatData->Attributes |= FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY;

	// parse size
	if (S_ISREG(sysStatData.st_mode))
		pStatData->Size = sysStatData.st_size;
	else
		pStatData->Size = 0;

	// ok
	return true;
}

bool FileSystem::StatFile(std::FILE* fp, FILESYSTEM_STAT_DATA* pStatData)
{
	int fd = fileno(fp);
	if (fd < 0)
		return false;

		// stat file
#if defined(__HAIKU__) || defined(__APPLE__) || defined(__FreeBSD__)
	struct stat sysStatData;
	if (fstat(fd, &sysStatData) < 0)
#else
	struct stat64 sysStatData;
	if (fstat64(fd, &sysStatData) < 0)
#endif
		return false;

	// parse attributes
	pStatData->ModificationTime = sysStatData.st_mtime;
	pStatData->Attributes = 0;
	if (S_ISDIR(sysStatData.st_mode))
		pStatData->Attributes |= FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY;

	// parse size
	if (S_ISREG(sysStatData.st_mode))
		pStatData->Size = sysStatData.st_size;
	else
		pStatData->Size = 0;

	// ok
	return true;
}

bool FileSystem::FileExists(const char* Path)
{
	// has a path
	if (Path[0] == '\0')
		return false;

		// stat file
#if defined(__HAIKU__) || defined(__APPLE__) || defined(__FreeBSD__)
	struct stat sysStatData;
	if (stat(Path, &sysStatData) < 0)
#else
	struct stat64 sysStatData;
	if (stat64(Path, &sysStatData) < 0)
#endif
		return false;

	if (S_ISDIR(sysStatData.st_mode))
		return false;
	else
		return true;
}

bool FileSystem::DirectoryExists(const char* Path)
{
	// has a path
	if (Path[0] == '\0')
		return false;

		// stat file
#if defined(__HAIKU__) || defined(__APPLE__) || defined(__FreeBSD__)
	struct stat sysStatData;
	if (stat(Path, &sysStatData) < 0)
#else
	struct stat64 sysStatData;
	if (stat64(Path, &sysStatData) < 0)
#endif
		return false;

	if (S_ISDIR(sysStatData.st_mode))
		return true;
	else
		return false;
}

bool FileSystem::CreateDirectoryPath(const char* Path, bool Recursive)
{
	u32 i;
	int lastError;

	// has a path
	if (Path[0] == '\0')
		return false;

	// try just flat-out, might work if there's no other segments that have to be made
	if (mkdir(Path, 0777) == 0)
		return true;

	// check error
	lastError = errno;
	if (lastError == EEXIST)
	{
		// check the attributes
		struct stat sysStatData;
		if (stat(Path, &sysStatData) == 0 && S_ISDIR(sysStatData.st_mode))
			return true;
		else
			return false;
	}
	else if (lastError == ENOENT)
	{
		// part of the path does not exist, so we'll create the parent folders, then
		// the full path again. allocate another buffer with the same length
		u32 pathLength = static_cast<u32>(std::strlen(Path));
		char* tempStr = (char*)alloca(pathLength + 1);

		// create directories along the path
		for (i = 0; i < pathLength; i++)
		{
			if (Path[i] == '/')
			{
				tempStr[i] = '\0';
				if (mkdir(tempStr, 0777) < 0)
				{
					lastError = errno;
					if (lastError != EEXIST) // fine, continue to next path segment
						return false;
				}
			}

			tempStr[i] = Path[i];
		}

		// re-create the end if it's not a separator, check / as well because windows can interpret them
		if (Path[pathLength - 1] != '/')
		{
			if (mkdir(Path, 0777) < 0)
			{
				lastError = errno;
				if (lastError != EEXIST)
					return false;
			}
		}

		// ok
		return true;
	}
	else
	{
		// unhandled error
		return false;
	}
}

bool FileSystem::DeleteFilePath(const char* Path)
{
	if (Path[0] == '\0')
		return false;

	struct stat sysStatData;
	if (stat(Path, &sysStatData) != 0 || S_ISDIR(sysStatData.st_mode))
		return false;

	return (unlink(Path) == 0);
}

bool FileSystem::RenamePath(const char* OldPath, const char* NewPath)
{
	if (OldPath[0] == '\0' || NewPath[0] == '\0')
		return false;

	if (rename(OldPath, NewPath) != 0)
	{
		Console.Error("rename('%s', '%s') failed: %d", OldPath, NewPath, errno);
		return false;
	}

	return true;
}

bool FileSystem::DeleteDirectoryPath(const char* Path, bool Recursive)
{
	Console.Error("FileSystem::DeleteDirectory(%s) not implemented", Path);
	return false;
}

std::string FileSystem::GetProgramPath()
{
#if defined(__linux__)
	static const char* exeFileName = "/proc/self/exe";

	int curSize = PATH_MAX;
	char* buffer = static_cast<char*>(std::realloc(nullptr, curSize));
	for (;;)
	{
		int len = readlink(exeFileName, buffer, curSize);
		if (len < 0)
		{
			std::free(buffer);
			return {};
		}
		else if (len < curSize)
		{
			buffer[len] = '\0';
			std::string ret(buffer, len);
			std::free(buffer);
			return ret;
		}

		curSize *= 2;
		buffer = static_cast<char*>(std::realloc(buffer, curSize));
	}

#elif defined(__APPLE__)

	int curSize = PATH_MAX;
	char* buffer = static_cast<char*>(std::realloc(nullptr, curSize));
	for (;;)
	{
		u32 nChars = curSize - 1;
		int res = _NSGetExecutablePath(buffer, &nChars);
		if (res == 0)
		{
			buffer[nChars] = 0;

			char* resolvedBuffer = realpath(buffer, nullptr);
			if (resolvedBuffer == nullptr)
			{
				std::free(buffer);
				return {};
			}

			std::string ret(buffer);
			std::free(buffer);
			return ret;
		}

		curSize *= 2;
		buffer = static_cast<char*>(std::realloc(buffer, curSize + 1));
	}

#elif defined(__FreeBSD__)
	int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
	char buffer[PATH_MAX];
	size_t cb = sizeof(buffer) - 1;
	int res = sysctl(mib, countof(mib), buffer, &cb, nullptr, 0);
	if (res != 0)
		return {};

	buffer[cb] = '\0';
	return buffer;
#else
	return {};
#endif
}

std::string FileSystem::GetWorkingDirectory()
{
	std::string buffer;
	buffer.resize(PATH_MAX);
	while (!getcwd(buffer.data(), buffer.size()))
	{
		if (errno != ERANGE)
			return {};

		buffer.resize(buffer.size() * 2);
	}

	return buffer;
}

bool FileSystem::SetWorkingDirectory(const char* path)
{
	return (chdir(path) == 0);
}

#endif
