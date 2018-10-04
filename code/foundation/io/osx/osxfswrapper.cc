//------------------------------------------------------------------------------
//  osxfswrapper.cc
//  (C) 2010 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "osxfswrapper.h"
#include "core/sysfunc.h"

namespace OSX
{
using namespace Util;
using namespace Core;
using namespace IO;
    
//------------------------------------------------------------------------------
/**
*/
OSXFSWrapper::Handle
OSXFSWrapper::OpenFile(const String& path, Stream::AccessMode accessMode, Stream::AccessPattern accessPattern)
{
    n_error("OSXFSWrapper::OpenFile(): IMPLEMENT ME!\n");
    return 0;
}
    
//------------------------------------------------------------------------------
/**
*/
void
OSXFSWrapper::CloseFile(Handle handle)
{
    n_error("OSXFSWrapper::CloseFile(): IMPLEMENT ME!\n");
}
    
//------------------------------------------------------------------------------
/**
*/
void
OSXFSWrapper::Write(Handle handle, const void* buf, Stream::Size numBytes)
{
    n_error("OSXFSWrapper::Write(): IMPLEMENT ME!\n");
}
    
//------------------------------------------------------------------------------
/**
*/
Stream::Size
OSXFSWrapper::Read(Handle handle, void* buf, Stream::Size numBytes)
{
    n_error("OSXFSWrapper::Read(): IMPLEMENT ME!\n");
    return 0;
}
    
//------------------------------------------------------------------------------
/**
*/
void
OSXFSWrapper::Seek(Handle handle, Stream::Offset offset, Stream::SeekOrigin orig)
{
    n_error("OSXFSWrapper::Seek(): IMPLEMENT ME!\n");
}
    
//------------------------------------------------------------------------------
/**
*/
Stream::Position
OSXFSWrapper::Tell(Handle handle)
{
    n_error("OSXFSWrapper::Tell(): IMPLEMENT ME!\n");
    return 0;
}
    
//------------------------------------------------------------------------------
/**
*/
void
OSXFSWrapper::Flush(Handle handle)
{
    n_error("OSXFSWrapper::Flush(): IMPLEMENT ME!\n");
}
    
//------------------------------------------------------------------------------
/**
*/
bool
OSXFSWrapper::Eof(Handle handle)
{
    n_error("OSXFSWrapper::Eof(): IMPLEMENT ME!\n");
    return false;
}
    
//------------------------------------------------------------------------------
/**
*/
Stream::Size
OSXFSWrapper::GetFileSize(Handle handle)
{
    n_error("OSXFSWrapper::GetFileSize(): IMPLEMENT ME!\n");
    return 0;
}
    
//------------------------------------------------------------------------------
/**
*/
void
OSXFSWrapper::SetReadOnly(const String& path, bool readOnly)
{
    n_error("OSXFSWrapper::SetReadOnly(): IMPLEMENT ME!\n");
}

//------------------------------------------------------------------------------
/**
*/
bool
OSXFSWrapper::IsReadOnly(const String& path)
{
    n_error("OSXFSWrapper::IsReadOnly(): IMPLEMENT ME!\n");
    return false;
}
    
//------------------------------------------------------------------------------
/**
 */
bool
OSXFSWrapper::DeleteFile(const String& path)
{
    n_error("OSXFSWrapper::DeleteFile(): IMPLEMENT ME!\n");
    return false;
}
    
//------------------------------------------------------------------------------
/**
*/
bool
OSXFSWrapper::DeleteDirectory(const String& path)
{
    n_error("OSXFSWrapper::DeleteDirectory(): IMPLEMENT ME!\n");
    return false;
}
    
//------------------------------------------------------------------------------
/**
*/
bool
OSXFSWrapper::FileExists(const String& path)
{
    n_error("OSXFSWrapper::FileExists(): IMPLEMENT ME!\n");
    return false;
}
    
//------------------------------------------------------------------------------
/**
*/
bool
OSXFSWrapper::DirectoryExists(const String& path)
{
    n_error("OSXFSWrapper::DirectoryExists(): IMPLEMENT ME!\n");
    return false;
}
    
//------------------------------------------------------------------------------
/**
*/
void
OSXFSWrapper::SetFileWriteTime(const String& path, FileTime fileTime)
{
    n_error("OSXFSWrapper::SetFileWriteTime() not implemented!");
}
    
//------------------------------------------------------------------------------
/**
*/
FileTime
OSXFSWrapper::GetFileWriteTime(const String& path)
{
    n_error("OSXFSWrapper::GetFileWriteTime() not implemented!");
    return FileTime();
}
    
//------------------------------------------------------------------------------
/**
*/
bool
OSXFSWrapper::CreateDirectory(const String& path)
{
    n_error("OSXFSWrapper::CreateDirectory(): NOT IMPLEMENTED!");
    return false;
}
    
//------------------------------------------------------------------------------
/**
*/
Array<String>
OSXFSWrapper::ListFiles(const String& path, const String& pattern)
{
    n_error("OSXFSWrapper::ListFiles(): NOT IMPLEMENTED!");
    return Array<String>();
}
    
//------------------------------------------------------------------------------
/**
 */
Array<String>
OSXFSWrapper::ListDirectories(const String& path, const String& pattern)
{
    n_error("OSXFSWrapper::ListDirectories(): NOT IMPLEMENTED!");
    return Array<String>();
}
    
//------------------------------------------------------------------------------
/**
 */
String
OSXFSWrapper::GetUserDirectory()
{
    n_error("OSXFSWrapper::GetUserDirectory(): NOT IMPLEMENTED!");
    return "";
}
    
//------------------------------------------------------------------------------
/**
 */
String
OSXFSWrapper::GetAppDataDirectory()
{  
    n_error("OSXFSWrapper::GetAppDataDirectory(): NOT IMPLEMENTED!");
    return "";
}
    
//------------------------------------------------------------------------------
/**
 */
String 
OSXFSWrapper::GetProgramsDirectory()
{  
    n_error("OSXFSWrapper::GetProgramsDirectory(): NOT IMPLEMENTED!");
    return "";
}
    
//------------------------------------------------------------------------------
/**
 */
String
OSXFSWrapper::GetTempDirectory()
{  
    n_error("OSXFSWrapper::GetTempDirectory(): NOT IMPLEMENTED!");
    return "";
}
    
//------------------------------------------------------------------------------
/**
 */
String
OSXFSWrapper::GetBinDirectory()
{
    n_error("OSXFSWrapper::GetBinDirectory(): NOT IMPLEMENTED!");
    return "";
}
    
//------------------------------------------------------------------------------
/**
 */
String
OSXFSWrapper::GetHomeDirectory()
{
    n_error("OSXFSWrapper::GetHomeDirectory(): NOT IMPLEMENTED!");
    return "";
}
    
//------------------------------------------------------------------------------
/**
 */
bool
OSXFSWrapper::IsDeviceName(const Util::String& str)
{
    if (str == "OSX") return true;
    else return false;
}
    
//------------------------------------------------------------------------------
/**
 */
const char*
OSXFSWrapper::ConvertPath(const Util::String& str)
{
    const char* ptr = str.AsCharPtr();
    n_assert((ptr[0] == 'O') && (ptr[1] == 'S') && (ptr[2] == 'X') && (ptr[3] == ':'));
    return &(ptr[4]);
}

} // namespace OSX