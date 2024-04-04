//------------------------------------------------------------------------------
//  win32fswrapper.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/win32/win32fswrapper.h"
#include "system/nebulasettings.h"
#if __WIN32__
#include "util/win32/win32stringconverter.h"
#endif
#include <direct.h>

namespace Win32
{
using namespace Util;
using namespace Core;
using namespace IO;

//------------------------------------------------------------------------------
/**
    Open a file using the function CreateFile(). Returns a handle
    to the file which must be passed to the other Win32FSWrapper file methods.
    If opening the file fails, the function will return 0. The filename
    must be a native win32 path (no assigns, etc...).
*/
Win32FSWrapper::Handle
Win32FSWrapper::OpenFile(const String& path, Stream::AccessMode accessMode, Stream::AccessPattern accessPattern, DWORD flagsAndAttributes)
{
    ushort widePath[1024];
    Win32::Win32StringConverter::UTF8ToWide(path, widePath, sizeof(widePath));

    DWORD access = 0;
    DWORD disposition = 0;
    DWORD shareMode = 0;
    switch (accessMode)
    {
        case Stream::ReadAccess:
            access = GENERIC_READ;            
            disposition = OPEN_EXISTING;
            shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
            break;

        case Stream::WriteAccess:
            access = GENERIC_WRITE;
            disposition = CREATE_ALWAYS;
            shareMode = FILE_SHARE_READ;
            break;

        case Stream::ReadWriteAccess:
        case Stream::AppendAccess:
            access = GENERIC_READ | GENERIC_WRITE;
            disposition = OPEN_ALWAYS;
            shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
            break;
    }
    switch (accessPattern)
    {
        case Stream::Random:
            flagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;
            break;

        case Stream::Sequential:
            flagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;
            break;
    }

    // open/create the file
    Handle handle = CreateFileW((LPCWSTR)widePath,       // lpFileName
                                access,                  // dwDesiredAccess
                                shareMode,               // dwShareMode
                                0,                       // lpSecurityAttributes
                                disposition,             // dwCreationDisposition,
                                flagsAndAttributes,      // dwFlagsAndAttributes
                                NULL);                   // hTemplateFile

    if (handle != INVALID_HANDLE_VALUE)
    {
        // in append mode, we need to seek to the end of the file
        if (Stream::AppendAccess == accessMode)
        {
            SetFilePointer(handle, 0, NULL, FILE_END);
        }
        return handle;
    }
    else
    {
        return 0;
    }
}

//------------------------------------------------------------------------------
/**
    Closes a file opened by Win32FSWrapper::OpenFile().
*/
void
Win32FSWrapper::CloseFile(Handle handle)
{
    n_assert(0 != handle);
    CloseHandle(handle);
}

//------------------------------------------------------------------------------
/**
    Write data to a file.
*/
void
Win32FSWrapper::Write(Handle handle, const void* buf, Stream::Size numBytes)
{
    n_assert(0 != handle);
    n_assert(buf != 0);
    n_assert(numBytes > 0);
    n_assert(numBytes < LLONG_MAX);
    DWORD bytesWritten;
    BOOL result = WriteFile(handle, buf, (DWORD)numBytes, &bytesWritten, NULL);
    if ((0 == result) || ((DWORD)numBytes != bytesWritten))
    {
        n_error("Win32FSWrapper: WriteFile() failed!");
    }
}

//------------------------------------------------------------------------------
/**
    Read data from a file, returns number of bytes read.
*/
Stream::Size
Win32FSWrapper::Read(Handle handle, void* buf, Stream::Size numBytes)
{
    n_assert(0 != handle);
    n_assert(buf != 0);
    n_assert(numBytes > 0);
    n_assert(numBytes < LLONG_MAX);
    DWORD bytesRead;
    BOOL result = ReadFile(handle, buf, (DWORD)numBytes, &bytesRead, NULL);
    if (0 == result)
    {
        n_error("Win32FSWrapper: ReadFile() failed!");
    }
    return bytesRead;
}

//------------------------------------------------------------------------------
/**
*/
char*
Win32FSWrapper::Map(Handle h, IO::Stream::AccessMode accessMode, Handle& mappedHandle)
{
    DWORD access = PAGE_READONLY;
    DWORD mapAccess = FILE_MAP_READ;
    switch (accessMode)
    {
    case Stream::ReadAccess:
        access = PAGE_READONLY;
        mapAccess = FILE_MAP_READ;
        break;

    case Stream::WriteAccess:
        n_error("Can't map file with only write access");
        access = PAGE_WRITECOPY;
        mapAccess = FILE_MAP_WRITE;
        break;

    case Stream::ReadWriteAccess:
    case Stream::AppendAccess:
        access = PAGE_READWRITE;
        mapAccess = FILE_MAP_ALL_ACCESS;
        break;
    }
    mappedHandle = CreateFileMappingW(h, nullptr, access, 0, 0, nullptr);
    n_assert(mappedHandle != nullptr);

    // map the file
    char* ret = (char*)MapViewOfFile(mappedHandle, mapAccess, 0, 0, 0);
    n_assert(ret != nullptr);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
Win32FSWrapper::Unmap(Handle mapHandle, char* buf)
{
    UnmapViewOfFile(buf);
    CloseHandle(mapHandle);
}

//------------------------------------------------------------------------------
/**
    Seek in a file.
*/
void
Win32FSWrapper::Seek(Handle handle, Stream::Offset offset, Stream::SeekOrigin orig)
{
    n_assert(0 != handle);
    n_assert(offset < INT_MAX);
    DWORD moveMethod;
    switch (orig)
    {
        case Stream::Begin:
            moveMethod = FILE_BEGIN;
            break;
        case Stream::Current:
            moveMethod = FILE_CURRENT;
            break;
        case Stream::End:
            moveMethod = FILE_END;
            break;
        default:
            // can't happen
            moveMethod = FILE_BEGIN;
            break;
    }
    SetFilePointer(handle, (LONG)offset, NULL, moveMethod);
}

//------------------------------------------------------------------------------
/**
    Get current position in file.
*/
Stream::Position
Win32FSWrapper::Tell(Handle handle)
{
    n_assert(0 != handle);
    return SetFilePointer(handle, 0, NULL, FILE_CURRENT);
}

//------------------------------------------------------------------------------
/**
    Flush unwritten data to file.
*/
void
Win32FSWrapper::Flush(Handle handle)
{
    n_assert(0 != handle);
    FlushFileBuffers(handle);
}

//------------------------------------------------------------------------------
/**
    Returns true if current position is at end of file.
*/
bool
Win32FSWrapper::Eof(Handle handle)
{
    n_assert(0 != handle);
    DWORD fpos = SetFilePointer(handle, 0, NULL, FILE_CURRENT);
    DWORD size = ::GetFileSize(handle, NULL);

    // NOTE: THE '>=' IS NOT A BUG!!!
    return fpos >= size;
}

//------------------------------------------------------------------------------
/**
    Returns the size of a file in bytes.
*/
Stream::Size
Win32FSWrapper::GetFileSize(Handle handle)
{
    n_assert(0 != handle);
    return ::GetFileSize(handle, NULL);
}

//------------------------------------------------------------------------------
/**
    Set the read-only status of a file. 
*/
void
Win32FSWrapper::SetReadOnly(const String& path, bool readOnly)
{
    n_assert(path.IsValid());
    ushort widePath[1024];
    Win32::Win32StringConverter::UTF8ToWide(path, widePath, sizeof(widePath));
    DWORD fileAttrs = GetFileAttributesW((LPCWSTR)widePath);
    if (readOnly)
    {
        fileAttrs |= FILE_ATTRIBUTE_READONLY;
    }
    else
    {
        fileAttrs &= ~FILE_ATTRIBUTE_READONLY;
    }
    SetFileAttributes(path.AsCharPtr(), fileAttrs);
}

//------------------------------------------------------------------------------
/**
    Get the read-only status of a file.
*/
bool
Win32FSWrapper::IsReadOnly(const String& path)
{
    n_assert(path.IsValid());
    ushort widePath[1024];
    Win32::Win32StringConverter::UTF8ToWide(path, widePath, sizeof(widePath));
    DWORD fileAttrs = GetFileAttributesW((LPCWSTR)widePath);
    return (fileAttrs & FILE_ATTRIBUTE_READONLY);
}

//------------------------------------------------------------------------------
/**
    try to check for a lock by trying to lock the file. inherently racey, but
    good enough in some situations
*/
bool
Win32FSWrapper::IsLocked(const Util::String& path)
{
    n_assert(path.IsValid());
    Handle h = Win32FSWrapper::OpenFile(path, Stream::ReadWriteAccess, Stream::Sequential); 
    OVERLAPPED overlap = { 0 };
    bool locked = true;
    if (::LockFileEx(h, LOCKFILE_EXCLUSIVE_LOCK| LOCKFILE_FAIL_IMMEDIATELY, 0, MAXDWORD, MAXDWORD, &overlap))
    {
        BOOL ret = ::UnlockFileEx(h, 0, MAXDWORD, MAXDWORD, &overlap);      
        locked = false;
    }
    Win32FSWrapper::CloseFile(h);
    return locked;
}

//------------------------------------------------------------------------------
/**
    Deletes a file. Returns true if the operation was successful. The delete
    will fail if the fail doesn't exist or the file is read-only.
*/
bool
Win32FSWrapper::DeleteFile(const String& path)
{
    n_assert(path.IsValid());
    ushort widePath[1024];
    Win32::Win32StringConverter::UTF8ToWide(path, widePath, sizeof(widePath));
    return (0 != ::DeleteFileW((LPCWSTR)widePath));
}

//------------------------------------------------------------------------------
/**
*/
bool
Win32FSWrapper::ReplaceFile(const Util::String& source, const Util::String& target)
{
    ushort wideSourcePath[1024];
    ushort wideTargetPath[1024];
    Win32::Win32StringConverter::UTF8ToWide(source, wideSourcePath, sizeof(wideSourcePath));
    Win32::Win32StringConverter::UTF8ToWide(target, wideTargetPath, sizeof(wideTargetPath));
    return (0 != ::ReplaceFileW((LPCWSTR)wideTargetPath, (LPCWSTR)wideSourcePath, NULL, 0, 0, 0));
}

//------------------------------------------------------------------------------
/**
    Delete an empty directory. Returns true if the operation was successful.
*/
bool
Win32FSWrapper::DeleteDirectory(const String& path)
{
    n_assert(path.IsValid());

    ushort widePath[1024];
    Win32::Win32StringConverter::UTF8ToWide(path, widePath, sizeof(widePath));
    return (0 != ::RemoveDirectoryW((LPCWSTR)widePath));
}

//------------------------------------------------------------------------------
/**
    Return true if a file exists.
*/
bool
Win32FSWrapper::FileExists(const String& path)
{
    n_assert(path.IsValid());

    ushort widePath[1024];
    Win32::Win32StringConverter::UTF8ToWide(path, widePath, sizeof(widePath));
    DWORD fileAttrs = GetFileAttributesW((LPCWSTR)widePath);

    if ((-1 != fileAttrs) && (0 == (FILE_ATTRIBUTE_DIRECTORY & fileAttrs)))
    {
        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    Return true if a directory exists.
*/
bool
Win32FSWrapper::DirectoryExists(const String& path)
{
    n_assert(path.IsValid());
    
    ushort widePath[1024];
    Win32::Win32StringConverter::UTF8ToWide(path, widePath, sizeof(widePath));
    DWORD fileAttrs = GetFileAttributesW((LPCWSTR)widePath);

    if ((-1 != fileAttrs) && (0 != (FILE_ATTRIBUTE_DIRECTORY & fileAttrs)))
    {
        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    Set the write-access time stamp of a file.
*/
void
Win32FSWrapper::SetFileWriteTime(const String& path, FileTime fileTime)
{
    n_assert(path.IsValid());
    Handle h = Win32FSWrapper::OpenFile(path, Stream::ReadWriteAccess, Stream::Sequential);
    if (0 != h)
    {
        SetFileTime(h, NULL, NULL, &fileTime.time);
        Win32FSWrapper::CloseFile(h);
    }
    else
    {
        n_error("Win32FSWrapper::SetFileWriteTime(): failed to open file '%s'!", path.AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
    Return the last write-access time to a file.
*/
FileTime
Win32FSWrapper::GetFileWriteTime(const String& path)
{
    n_assert(path.IsValid());
    FileTime fileTime;
    Handle h = Win32FSWrapper::OpenFile(path, Stream::ReadAccess, Stream::Sequential);
    if (0 != h)
    {
        GetFileTime(h, NULL, NULL, &fileTime.time);
        Win32FSWrapper::CloseFile(h);
    }
    else
    {
        // do not fail hard if file does not exist
        // n_printf("Win32FSWrapper::GetFileWriteTime(): failed to open file '%s'!", path.AsCharPtr());
    }
    return fileTime;
}

//------------------------------------------------------------------------------
/**
    Creates a new directory.
*/
bool
Win32FSWrapper::CreateDirectory(const String& path)
{
    n_assert(path.IsValid());
    const String& nativePath = path;
    ushort widePath[1024];
    Win32::Win32StringConverter::UTF8ToWide(path, widePath, sizeof(widePath));
    return (0 != ::CreateDirectoryW((LPCWSTR)widePath, NULL));
}

//------------------------------------------------------------------------------
/**
*/
Util::String
Win32FSWrapper::CreateTemporaryFilename(const Util::String& path)
{
    n_assert(path.IsValid());
    const String& nativePath = path;
    ushort widePath[1024];
    Win32::Win32StringConverter::UTF8ToWide(path, widePath, sizeof(widePath));
    const wchar_t* prefix = L"NEB";
    wchar_t name[MAX_PATH];
    n_assert(GetTempFileNameW((LPCWSTR)widePath, prefix, 0, name));
    return Win32::Win32StringConverter::WideToUTF8((ushort*)name);
}

//------------------------------------------------------------------------------
/**
    Lists all files in a directory, filtered by a pattern.
*/
Array<String>
Win32FSWrapper::ListFiles(const String& dirPath, const String& pattern)
{
    n_assert(dirPath.IsValid());
    n_assert(pattern.IsValid());
    
    String pathWithPattern = dirPath + "/" + pattern;
    ushort widePath[1024];
    Win32::Win32StringConverter::UTF8ToWide(pathWithPattern, widePath, sizeof(widePath));
    
    Array<String> result;
    HANDLE hFind;
    WIN32_FIND_DATAW findFileData;
    hFind = FindFirstFileW((LPCWSTR)widePath, &findFileData);  
    if (INVALID_HANDLE_VALUE != hFind) 
    {
        do
        {
            if (0 == (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                result.Append(Win32::Win32StringConverter::WideToUTF8((ushort*)findFileData.cFileName));
            }
        }
        while (FindNextFileW(hFind, &findFileData) != 0);
        FindClose(hFind);
    }
    return result;
}

//------------------------------------------------------------------------------
/**
    Lists all subdirectories in a directory, filtered by a pattern. This will
    not return the special directories ".." and ".".
*/
Array<String>
Win32FSWrapper::ListDirectories(const String& dirPath, const String& pattern)
{
    n_assert(dirPath.IsValid());
    n_assert(pattern.IsValid());
    
    String pathWithPattern = dirPath + "/" + pattern;
    ushort widePath[1024];
    Win32::Win32StringConverter::UTF8ToWide(pathWithPattern, widePath, sizeof(widePath));

    Array<String> result;
    HANDLE hFind;
    
    WIN32_FIND_DATAW findFileData;
    hFind = FindFirstFileW((LPCWSTR)widePath, &findFileData);  
    if (INVALID_HANDLE_VALUE != hFind) 
    {
        do
        {
            String fileName = Win32::Win32StringConverter::WideToUTF8((ushort*)findFileData.cFileName);
            if ((0 != (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) &&
                (fileName != "..") && (fileName != "."))
            {
                result.Append(fileName);
            }
        }
        while (FindNextFileW(hFind, &findFileData) != 0);
        FindClose(hFind);
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
String
Win32FSWrapper::GetUserDirectory()
{
    ushort wideBuffer[NEBULA_MAXPATH] = { 0 };
    HRESULT hr = SHGetFolderPathW(NULL, 
                                    CSIDL_PERSONAL | CSIDL_FLAG_CREATE, 
                                    NULL, 
                                    0,
                                    (LPWSTR)wideBuffer);
    n_assert(SUCCEEDED(hr));
    String result = Win32::Win32StringConverter::WideToUTF8(wideBuffer);
    result.ConvertBackslashes();
    return String("file:///") + result;
}

//------------------------------------------------------------------------------
/**
*/
String
Win32FSWrapper::GetAppDataDirectory()
{    
    ushort wideBuffer[NEBULA_MAXPATH] = { 0 };
    HRESULT hr = SHGetFolderPathW(NULL, 
                                    CSIDL_APPDATA | CSIDL_FLAG_CREATE, 
                                    NULL, 
                                    0,
                                    (LPWSTR)wideBuffer);
    n_assert(SUCCEEDED(hr));
    String result = Win32::Win32StringConverter::WideToUTF8(wideBuffer);
    result.ConvertBackslashes();
    return String("file:///") + result;
}

//------------------------------------------------------------------------------
/**
*/
String 
Win32FSWrapper::GetProgramsDirectory()
{

    ushort wideBuffer[NEBULA_MAXPATH] = { 0 };
    HRESULT hr = SHGetFolderPathW(NULL,
                                    CSIDL_PROGRAM_FILES,
                                    NULL,
                                    0,
                                    (LPWSTR)wideBuffer);
    n_assert(SUCCEEDED(hr));
    String result = Win32::Win32StringConverter::WideToUTF8(wideBuffer);
    result.ConvertBackslashes();
    return String("file:///") + result;
}

//------------------------------------------------------------------------------
/**
*/
String
Win32FSWrapper::GetCurrentDirectory()
{
    char buffer[NEBULA_MAXPATH] = { 0 };
    n_assert(_getcwd(buffer, NEBULA_MAXPATH));
    String result = buffer;
    result.ConvertBackslashes();
    return String("file:///") + result;    
}

//------------------------------------------------------------------------------
/**
*/
String
Win32FSWrapper::GetTempDirectory()
{
    ushort wideBuffer[NEBULA_MAXPATH] = { 0 };
    GetTempPathW(sizeof(wideBuffer) / 2, (LPWSTR)wideBuffer);
    String result = Win32::Win32StringConverter::WideToUTF8(wideBuffer);
    result.ConvertBackslashes();
    result.TrimRight("/");
    return String("file:///") + result;
}

//------------------------------------------------------------------------------
/**
    This method sould return the directory where the application executable
    is located.
*/
String
Win32FSWrapper::GetBinDirectory()
{
    ushort wideBuffer[NEBULA_MAXPATH];
    DWORD res = GetModuleFileNameW(NULL, (LPWSTR)wideBuffer, sizeof(wideBuffer) / 2);
    n_assert(0 != res);
    String result = Win32::Win32StringConverter::WideToUTF8(wideBuffer);
    result.ConvertBackslashes();
    result = result.ExtractDirName();
    result.TrimRight("/");
    return String("file:///") + result;
}

//------------------------------------------------------------------------------
/**
    This method should return the installation directory of the
    application.
*/
String
Win32FSWrapper::GetHomeDirectory()
{

    ushort wideBuffer[NEBULA_MAXPATH];
    DWORD res = GetModuleFileNameW(NULL, (LPWSTR)wideBuffer, sizeof(wideBuffer) / 2);
    n_assert(0 != res);

    String pathToExe = Win32::Win32StringConverter::WideToUTF8(wideBuffer);
    pathToExe.ConvertBackslashes();
#if PUBLIC_BUILD
    // check if executable resides in a win32 directory
    String pathToDir = pathToExe.ExtractLastDirName();
    if (n_stricmp(pathToDir.AsCharPtr(), "win32") == 0)
    {
        // normal home:bin/win32 directory structure
        // strip bin/win32
        String homePath = pathToExe.ExtractDirName();
        homePath = homePath.ExtractDirName();
        homePath = homePath.ExtractDirName();
        homePath.TrimRight("/");
        return String("file:///") + homePath;
    }
    else
    {
        // not in normal home:bin/win32 directory structure, 
        // use the exe's directory as home path
        String homePath = pathToExe.ExtractDirName();
        return String("file:///") + homePath;
    }            
#else
    Util::String root;
    if( System::NebulaSettings::Exists("gscept", "ToolkitShared", "workdir"))
    {
        root = System::NebulaSettings::ReadString("gscept", "ToolkitShared", "workdir");    
    }    
    if (root.IsEmpty())
    {
        String homePath = pathToExe.ExtractDirName();
        return String("file:///") + homePath;
    }
    else
    {
        return String("file:///") + root;
    }
#endif
}

//------------------------------------------------------------------------------
/**
    Return true if the provided string is a device name.
*/
bool
Win32FSWrapper::IsDeviceName(const Util::String& str)
{
    if (str.Length() == 1)
    {
        uchar c = str[0];
        if (((c >= 'A') && (c <= 'Z')) ||
            ((c >= 'a') && (c <= 'z')))
        {
            return true;
        }
    }
    return false;
}

} // namespace IO
