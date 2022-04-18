//------------------------------------------------------------------------------
//  io/ioserver.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/ioserver.h"
#include "core/ptr.h"
#include "io/fswrapper.h"
#include "util/stack.h"
#include "util/crc.h"
#include "system/systeminfo.h"
#include "io/filestream.h"
#include "io/archfs/archive.h"
#include "io/archfs/archivefilesystem.h"
#include "io/filewatcher.h"
#include <filesystem>

namespace IO
{
__ImplementClass(IO::IoServer, 'IOSV', Core::RefCounted);
__ImplementSingleton(IO::IoServer);

Threading::CriticalSection IoServer::assignCriticalSection;
Threading::CriticalSection IoServer::schemeCriticalSection;
Threading::CriticalSection IoServer::archiveCriticalSection;
Threading::CriticalSection IoServer::watcherCriticalSection;
bool IoServer::StandardArchivesMounted = false;

using namespace Core;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
IoServer::IoServer() :
    archiveFileSystemEnabled(true)
{
    __ConstructSingleton;

    // the first IoServer sets up the global scheme registry
    this->schemeCriticalSection.Enter();
    if (!SchemeRegistry::HasInstance())
    {
        this->schemeRegistry = SchemeRegistry::Create();
        this->schemeRegistry->Setup();
    }
    else
    {
        this->schemeRegistry = SchemeRegistry::Instance();
    }    
    this->schemeCriticalSection.Leave();

    // the first IoServer created sets up the global assign registry
    this->assignCriticalSection.Enter();
    if (!AssignRegistry::HasInstance())
    {
        this->assignRegistry = AssignRegistry::Create();
        this->assignRegistry->Setup();
    }
    else
    {
        // global assign registry already exists
        this->assignRegistry = AssignRegistry::Instance();
    }
    this->assignCriticalSection.Leave();

    this->archiveCriticalSection.Enter();
    if (!ArchiveFileSystem::HasInstance())
    {
        n_assert(!StandardArchivesMounted);
        this->archiveFileSystem = ArchiveFileSystem::Create();
        this->archiveFileSystem->Setup();
    }
    else
    {
        this->archiveFileSystem = ArchiveFileSystem::Instance();
    }
    this->archiveCriticalSection.Leave();

    this->watcherCriticalSection.Enter();
    #ifndef __linux__
    if (!FileWatcher::HasInstance())
    {
        this->watcher = FileWatcher::Create();
        this->watcher->Setup();
    }
    else
    {
        this->watcher = FileWatcher::Instance();
    }
    #endif
    this->watcherCriticalSection.Leave();

    this->httpClientRegistry = Http::HttpClientRegistry::Create();
    this->httpClientRegistry->Setup();
    this->streamCache = StreamCache::Create();
}

//------------------------------------------------------------------------------
/**
*/
IoServer::~IoServer()
{
    this->streamCache = nullptr;
    this->httpClientRegistry->Discard();
    this->httpClientRegistry = nullptr;

    this->watcher = nullptr;
    // unmount standard archives if this is the last instance
    if (StandardArchivesMounted && (this->archiveFileSystem->GetRefCount() == 1))
    {
        this->UnmountStandardArchives();
    }
    this->archiveFileSystem = nullptr;
    this->assignRegistry = nullptr;
    this->schemeRegistry = nullptr;

    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Stream>
IoServer::CreateStream(const URI& uri) const
{
    n_assert(!uri.IsEmpty());
    n_assert(SchemeRegistry::Instance()->IsUriSchemeRegistered(uri.Scheme()));
    SchemeRegistry* schemeRegistry = SchemeRegistry::Instance();

    // check if the URI points into a mounted archive
    if (this->IsArchiveFileSystemEnabled())
    {
        URI newUri = ArchiveFileSystem::Instance()->ConvertFileToArchiveURIIfExists(uri);
        Ptr<Stream> stream = (Stream*) schemeRegistry->GetStreamClassByUriScheme(newUri.Scheme()).Create();
        stream->SetURI(newUri);
        return stream;
    }
    else
    {
        Ptr<Stream> stream = (Stream*) schemeRegistry->GetStreamClassByUriScheme(uri.Scheme()).Create();
        stream->SetURI(uri);
        return stream;
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
IoServer::MountArchive(const URI& uri)
{
    Ptr<Archive> archive = this->archiveFileSystem->Mount(uri);
    return archive.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
void
IoServer::UnmountArchive(const URI& uri)
{
    this->archiveFileSystem->Unmount(uri);
}

//------------------------------------------------------------------------------
/**
*/
bool
IoServer::IsArchiveMounted(const URI& uri) const
{
    return this->archiveFileSystem->IsMounted(uri);
}

//------------------------------------------------------------------------------
/**
*/
void
IoServer::MountStandardArchives()
{
    this->archiveCriticalSection.Enter();
    n_assert(!StandardArchivesMounted);
    StandardArchivesMounted = true;

    // first load the platform specific archives, since platform specific
    // files override any generic files
    // if the archive files don't exist, no archives will be mounted
    System::SystemInfo systemInfo;  
    String platformString = System::SystemInfo::PlatformAsString(systemInfo.GetPlatform());
    String platformArchivePath = "root:export";
    
    // prioritize export folder before zip 
    if (!this->DirectoryExists(platformArchivePath))
    {
        this->MountArchive(platformArchivePath);
    }

    this->archiveCriticalSection.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
IoServer::UnmountStandardArchives()
{
    this->archiveCriticalSection.Enter();
    n_assert(StandardArchivesMounted);
    StandardArchivesMounted = false;

    System::SystemInfo systemInfo;  
    String platformString = System::SystemInfo::PlatformAsString(systemInfo.GetPlatform());
    String platformArchivePath = "root:export";
    if (this->IsArchiveMounted(platformArchivePath))
    {
        this->UnmountArchive(platformArchivePath);
    }

    this->archiveCriticalSection.Leave();
}

//------------------------------------------------------------------------------
/**
    This method creates all missing directories in a path.
*/
bool
IoServer::CreateDirectory(const URI& uri) const
{
    n_assert(uri.Scheme() == "file");
    String path = uri.GetHostAndLocalPath();

    // build stack of non-existing dir components
    Stack<String> pathStack;
    while ((!path.IsEmpty()) && (!this->DirectoryExists(path)))
    {
        pathStack.Push(path);
        String nextPath = path.ExtractDirName();
        nextPath.TrimRight("/\\:");
        if ((nextPath == path) || FSWrapper::IsDeviceName(nextPath))
        {
            break;
        }
        path = nextPath;
    }

    // error in original path?
    if (path.IsEmpty())
    {
        return false;
    }

    // create missing directory components
    while (!pathStack.IsEmpty())
    {
        String curPath = pathStack.Pop();
        if (!FSWrapper::CreateDirectory(curPath))
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    This copies a file to another file.
*/
bool
IoServer::CopyFile(const URI& fromUri, const URI& toUri) const
{
    // transparent archive support...
    URI srcUri;
    if (this->IsArchiveFileSystemEnabled())
    {
        srcUri = ArchiveFileSystem::Instance()->ConvertFileToArchiveURIIfExists(fromUri);
    }
    else
    {
        srcUri = fromUri;
    }

    // if target file exists, remove read-only flag
    if (this->FileExists(toUri))
    {
        this->SetReadOnly(toUri, false);
    }

    // create file streams
    Ptr<Stream> srcStream = this->CreateStream(srcUri);
    srcStream->SetAccessMode(Stream::ReadAccess);
    if (srcStream->Open())
    {
        Ptr<Stream> toStream = this->CreateStream(toUri);
        toStream->SetAccessMode(Stream::WriteAccess);
        if (toStream->Open())
        {
            // allocate a buffer, and copy contents in a loop
            const int bufSize = 1024 * 1024;
            void* buffer = Memory::Alloc(Memory::ScratchHeap, bufSize);
            while (!srcStream->Eof())
            {
                Stream::Size bytesRead = srcStream->Read(buffer, bufSize);
                toStream->Write(buffer, bytesRead);
            }
            Memory::Free(Memory::ScratchHeap, buffer);
            toStream->Close();
        }
        else
        {
            n_error("IO::IoServer::CopyFile(): could not open target file '%s'", toUri.AsString().AsCharPtr());
            return false;
        }
        srcStream->Close();
    }
    else
    {
        n_error("IO::IoServer::CopyFile(): could not open source file '%s'", fromUri.AsString().AsCharPtr());
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    This method computes the CRC checksum for a file.
*/
unsigned int
IoServer::ComputeFileCrc(const URI& uri) const
{
    unsigned int result = 0;

    // create a stream
    Ptr<Stream> stream = this->CreateStream(uri);
    stream->SetAccessMode(Stream::ReadAccess);
    if (stream->Open())
    {
        Crc crc;
        crc.Begin();
        const int bufSize = (1<<16);
        unsigned char* buffer = (unsigned char*) Memory::Alloc(Memory::ScratchHeap, bufSize);
        while (!stream->Eof())
        {
            Stream::Size bytesRead = stream->Read(buffer, bufSize);
            crc.Compute(buffer, bytesRead);
        }
        Memory::Free(Memory::ScratchHeap, buffer);
        crc.End();
        result = crc.GetResult();
        stream->Close();
    }
    else
    {
        n_error("IO::IoServer::ComputeFileCrc(): could not open file '%s'!", uri.AsString().AsCharPtr());
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
bool
IoServer::DeleteFile(const URI& uri) const
{
    n_assert(uri.Scheme() == "file");
    const String path = uri.GetHostAndLocalPath();
    n_assert(path.IsValid());
    return FSWrapper::DeleteFile(path);
}

//------------------------------------------------------------------------------
/**
*/
bool
IoServer::DeleteDirectory(const URI& uri) const
{
    n_assert(uri.Scheme() == "file");
    const String path = uri.GetHostAndLocalPath();
    n_assert(path.IsValid());
    return FSWrapper::DeleteDirectory(path);
}

//------------------------------------------------------------------------------
/**
*/
bool
IoServer::FileExists(const URI& uri) const
{
    // transparent archive support
    if (this->IsArchiveFileSystemEnabled())
    {
        Ptr<Archive> archive = ArchiveFileSystem::Instance()->FindArchiveWithFile(uri);
        if (archive.isvalid())
        {
            return true;
        }
    }

    // doesn't exist as archive, check conventional filesystem
    const String path = uri.GetHostAndLocalPath();
    n_assert(path.IsValid());
    return FSWrapper::FileExists(path);
}

//------------------------------------------------------------------------------
/**
*/
bool
IoServer::DirectoryExists(const URI& uri) const
{
    // transparent archive support
    if (this->IsArchiveFileSystemEnabled())
    {
        if (uri.Scheme() == "file")
        {
            Ptr<Archive> archive = ArchiveFileSystem::Instance()->FindArchiveWithDir(uri);
            if (archive.isvalid())
            {
                return true;
            }
        }
    }

    // doesn't exist as archive, check conventional filesystem
    const String path = uri.GetHostAndLocalPath();
    n_assert(path.IsValid());
    return FSWrapper::DirectoryExists(path);
}

//------------------------------------------------------------------------------
/**
*/
void
IoServer::SetReadOnly(const URI& uri, bool readOnly) const
{
    n_assert(uri.Scheme() == "file");
    const String path = uri.GetHostAndLocalPath();
    n_assert(path.IsValid());
    FSWrapper::SetReadOnly(path, readOnly);
}

//------------------------------------------------------------------------------
/**
*/
bool
IoServer::IsReadOnly(const URI& uri) const
{
    n_assert(uri.Scheme() == "file");

    // transparent archive file system support
    if (this->IsArchiveFileSystemEnabled())
    {
        URI arcUri = ArchiveFileSystem::Instance()->ConvertFileToArchiveURIIfExists(uri);
        if (arcUri.Scheme() != "file")
        {
            // file exists in archive, archives are generally read only
            return true;
        }
    }

    // file not in archive, handle conventionally
    const String path = uri.GetHostAndLocalPath();
    n_assert(path.IsValid());
    return FSWrapper::IsReadOnly(path);
}

//------------------------------------------------------------------------------
/**
*/
void
IoServer::SetFileWriteTime(const URI& uri, FileTime fileTime)
{
    n_assert(uri.Scheme() == "file");
    const String path = uri.GetHostAndLocalPath();
    n_assert(path.IsValid());
    FSWrapper::SetFileWriteTime(path, fileTime);
}

//------------------------------------------------------------------------------
/**
*/
FileTime
IoServer::GetFileWriteTime(const URI& uri) const
{
    n_assert(uri.Scheme() == "file");
    const String path = uri.GetHostAndLocalPath();
    n_assert(path.IsValid());
    return FSWrapper::GetFileWriteTime(path);
}

//------------------------------------------------------------------------------
/**
*/
Array<String>
IoServer::ListFiles(const URI& uri, const String& pattern, bool asFullPath) const
{
    n_assert(pattern.IsValid());
    Array<String> result;

    // transparent archive file system support
    if (this->IsArchiveFileSystemEnabled())
    {
        Ptr<Archive> archive = ArchiveFileSystem::Instance()->FindArchiveWithDir(uri);
        if (archive.isvalid())
        {
            String pathInArchive = archive->ConvertToPathInArchive(uri.LocalPath());
            result = archive->ListFiles(pathInArchive, pattern);
            if (asFullPath)
            {
                result = this->AddPathPrefixToArray(uri.LocalPath(), result);
            }
            return result;
        }
    }

    //FIXME this should be handled more generically
    if (uri.Scheme() != "file")
    {
        Util::String fileList;
        URI listFile = uri;
        listFile.AppendLocalPath("/_files.lst");
        if (IoServer::ReadFile(listFile, fileList))
        {
            Util::Array<Util::String> files = fileList.Tokenize("\n");
            for(auto const& fileName : files)
            {
                if (String::MatchPattern(fileName, pattern))
                {
                    result.Append(fileName);
                }
            }
            return result;
        }
    }

    // fallthrough: not contained in archive, handle conventionally
    result = FSWrapper::ListFiles(uri.GetHostAndLocalPath(), pattern);
    if (asFullPath)
    {
        result = this->AddPathPrefixToArray(uri.LocalPath(), result);
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
Array<String>
IoServer::ListDirectories(const URI& uri, const String& pattern, bool asFullPath) const
{
    n_assert(pattern.IsValid());
    Array<String> result;

    // transparent archive file system support
    if (this->IsArchiveFileSystemEnabled())
    {
        Ptr<Archive> archive = ArchiveFileSystem::Instance()->FindArchiveWithDir(uri);
        if (archive.isvalid())
        {
            String pathInArchive = archive->ConvertToPathInArchive(uri.LocalPath());
            result = archive->ListDirectories(pathInArchive, pattern);
            if (asFullPath)
            {
                result = this->AddPathPrefixToArray(uri.LocalPath(), result);
            }
            return result;
        }
    }
    
    // fallthrough: not contained in archive, handle conventionally
    result = FSWrapper::ListDirectories(uri.GetHostAndLocalPath(), pattern);
    if (asFullPath)
    {
        result = this->AddPathPrefixToArray(uri.LocalPath(), result);
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
Array<String>
IoServer::AddPathPrefixToArray(const String& prefix, const Array<String>& filenames) const
{
    Array<String> result;
    if (!filenames.IsEmpty())
    {
        result.Reserve(filenames.Size());
        IndexT i;
        for (i = 0; i < filenames.Size(); i++)
        {
            String path = prefix;
            path.Append("/");
            path.Append(filenames[i]);
            result.Append(path);
        }
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
bool
IoServer::ReadFile(const URI& path, Util::String& contents)
{
    // create file stream
    Ptr<Stream> stream = IoServer::Instance()->CreateStream(path);

    // open file
    if (stream->Open())
    {
        // read all data
        void* data = stream->Map();
        Stream::Size size = stream->GetSize();

        // map to string
        n_assert(size < INT_MAX);
        contents.Set((char*)data, (SizeT)size);

        // close stream
        stream->Close();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
IoServer::IsLocked(const URI& uri) const
{
    n_assert(uri.Scheme() == "file");
    n_assert(this->FileExists(uri));
    const String path = uri.GetHostAndLocalPath();
    n_assert(path.IsValid());
    return FSWrapper::IsLocked(path);
}

//------------------------------------------------------------------------------
/**
*/
IO::URI
IoServer::CreateTemporaryFilename(const URI& uri) const
{
    n_assert(uri.Scheme() == "file");   
    const String path = uri.GetHostAndLocalPath();
    n_assert(path.IsValid());   
    return URI(FSWrapper::CreateTemporaryFilename(path));
}

//------------------------------------------------------------------------------
/**
*/
Util::String
IoServer::NativePath(const Util::String& path)
{
    std::filesystem::path u8path = std::filesystem::absolute(std::filesystem::u8path(path.AsCharPtr()));
    return u8path.string().c_str();
}

} // namespace IO
