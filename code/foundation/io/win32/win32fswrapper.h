#pragma once
//------------------------------------------------------------------------------
/**
    @class Win32::Win32FSWrapper

    Internal filesystem wrapper for Win32. All paths must be native
    paths (i.e. not contain Nebula assigns).

    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"
#include "util/array.h"
#include "io/stream.h"
#include "io/filetime.h"

//------------------------------------------------------------------------------
namespace Win32
{
class Win32FSWrapper
{
public:
    typedef HANDLE Handle;

    /// open a file
    static Handle OpenFile(const Util::String& path, IO::Stream::AccessMode accessMode, IO::Stream::AccessPattern accessPattern, DWORD flagsAndAttributes = 0);
    /// close a file
    static void CloseFile(Handle h);
    /// write to a file
    static void Write(Handle h, const void* buf, IO::Stream::Size numBytes);
    /// read from a file
    static IO::Stream::Size Read(Handle h, void* buf, IO::Stream::Size numBytes);
    /// map file to virtual memory
    static char* Map(Handle h, IO::Stream::AccessMode accessMode, Handle& mappedHandle);
    /// unmap file
    static void Unmap(Handle mapHandle, char* buf);
    /// seek in a file
    static void Seek(Handle h, IO::Stream::Offset offset, IO::Stream::SeekOrigin orig);
    /// get position in file
    static IO::Stream::Position Tell(Handle h);
    /// flush a file
    static void Flush(Handle h);
    /// return true if at end-of-file
    static bool Eof(Handle h);
    /// get size of a file in bytes
    static IO::Stream::Size GetFileSize(Handle h);
    /// set read-only status of a file
    static void SetReadOnly(const Util::String& path, bool readOnly);
    /// get read-only status of a file
    static bool IsReadOnly(const Util::String& path);
    /// get locked status of file
    static bool IsLocked(const Util::String& path);
    /// delete a file
    static bool DeleteFile(const Util::String& path);
    /// rename a file
    static bool ReplaceFile(const Util::String& source, const Util::String& target);
    /// delete an empty directory
    static bool DeleteDirectory(const Util::String& path);
    /// return true if a file exists
    static bool FileExists(const Util::String& path);
    /// return true if a directory exists
    static bool DirectoryExists(const Util::String& path);
    /// set the write-access time stamp of a file
    static void SetFileWriteTime(const Util::String& path, IO::FileTime fileTime);
    /// get the last write-access time stamp of a file
    static IO::FileTime GetFileWriteTime(const Util::String& path);
    /// create a directory
    static bool CreateDirectory(const Util::String& path);
    /// create a temporary file name
    static Util::String CreateTemporaryFilename(const Util::String& path);
    /// list all files in a directory
    static Util::Array<Util::String> ListFiles(const Util::String& dirPath, const Util::String& pattern);
    /// list all subdirectories in a directory
    static Util::Array<Util::String> ListDirectories(const Util::String& dirPath, const Util::String& pattern);
    /// get path to the current user's home directory (for user: standard assign)
    static Util::String GetUserDirectory();
    /// get path to the current user's appdata directory (for appdata: standard assign)
    static Util::String GetAppDataDirectory();
    /// get path to the current user's temp directory (for temp: standard assign)
    static Util::String GetTempDirectory();
    /// get path to the current application directory (for home: standard assign)
    static Util::String GetHomeDirectory();
    /// get path to the current bin directory (for bin: standard assign)
    static Util::String GetBinDirectory();
    /// get path to the "c:/program files" directory
    static Util::String GetProgramsDirectory();
    /// get current working directory
    static Util::String GetCurrentDirectory();
    /// return true when the string is a device name (e.g. "C:")
    static bool IsDeviceName(const Util::String& str);

private:

    /// convert UTF8 path name to WideChar path name
    static void ConvertToWidePath(const Util::String& src, ushort* dst, SizeT dstNumBytes);
};

}; // namespace Win32
//------------------------------------------------------------------------------
