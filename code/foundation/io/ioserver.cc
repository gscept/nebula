//------------------------------------------------------------------------------
//  io/ioserver.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/ioserver.h"
#include "core/ptr.h"
#include "io/fswrapper.h"
#include "util/stack.h"
#include "util/crc.h"
#include "system/systeminfo.h"
#include "io/assignregistry.h"
#include "io/archfs/archive.h"
#include "io/archfs/archivefilesystem.h"
#include "io/filewatcher.h"
#include "io/filestream.h"
#include <filesystem>
#include "http/httpclientregistry.h"


namespace IO
{

using namespace Core;
using namespace Util;

struct
{
    bool archiveFileSystemEnabled = true;
    Ptr<ArchiveFileSystem> archiveFileSystem;
    Threading::CriticalSection archiveCriticalSection;
    bool StandardArchivesMounted = false;

    Ptr<Http::HttpClientRegistry> httpClientRegistry;
    Ptr<AssignRegistry> assignRegistry;
    Ptr<SchemeRegistry> schemeRegistry;
    Ptr<FileWatcher> watcher;
    Ptr<StreamCache> streamCache;
    Threading::CriticalSection assignCriticalSection;
    Threading::CriticalSection schemeCriticalSection;
    Threading::CriticalSection watcherCriticalSection;
} state;


//------------------------------------------------------------------------------
/**
*/
void
StartupFilesystem(const FilesystemStartupInfo& info)
{
    // the first IoServer sets up the global scheme registry
    state.schemeCriticalSection.Enter();
    if (!SchemeRegistry::HasInstance())
    {
        state.schemeRegistry = SchemeRegistry::Create();
        state.schemeRegistry->Setup();
    }
    else
    {
        state.schemeRegistry = SchemeRegistry::Instance();
    }
    state.schemeCriticalSection.Leave();

    state.archiveCriticalSection.Enter();
    if (!ArchiveFileSystem::HasInstance())
    {
        n_assert(!state.StandardArchivesMounted);
        state.archiveFileSystem = ArchiveFileSystem::Create();
        state.archiveFileSystem->Setup();
    }
    else
    {
        state.archiveFileSystem = ArchiveFileSystem::Instance();
    }
    state.archiveCriticalSection.Leave();

    // the first IoServer created sets up the global assign registry
    state.assignCriticalSection.Enter();
    if (!AssignRegistry::HasInstance())
    {
        state.assignRegistry = AssignRegistry::Create();
        state.assignRegistry->Setup();
    }
    else
    {
        // global assign registry already exists
        state.assignRegistry = AssignRegistry::Instance();
    }
    state.assignCriticalSection.Leave();

    state.watcherCriticalSection.Enter();

    if (!FileWatcher::HasInstance())
    {
        state.watcher = FileWatcher::Create();
        state.watcher->Setup();
    }
    else
    {
        state.watcher = FileWatcher::Instance();
    }

    state.watcherCriticalSection.Leave();

    state.httpClientRegistry = Http::HttpClientRegistry::Create();
    state.httpClientRegistry->Setup();
    state.streamCache = StreamCache::Create();
}


//------------------------------------------------------------------------------
/**
*/
void
ShutdownFilesystem()
{
    state.streamCache = nullptr;
    state.httpClientRegistry->Discard();
    state.httpClientRegistry = nullptr;

    state.watcher = nullptr;
    // unmount standard archives if this is the last instance
    if (state.StandardArchivesMounted && (state.archiveFileSystem->GetRefCount() == 1))
    {
        UnmountStandardArchives();
    }
    state.archiveFileSystem = nullptr;
    state.assignRegistry = nullptr;
    state.schemeRegistry = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Stream>
CreateStream(const URI& uri)
{
    n_assert(!uri.IsEmpty());
    n_assert(SchemeRegistry::Instance()->IsUriSchemeRegistered(uri.Scheme()));
    const Core::Rtti& streamClass = SchemeRegistry::Instance()->GetStreamClassByUriScheme(uri.Scheme());

#if !PUBLIC_BUILD
    if (streamClass == FileStream::RTTI)
    {
        const String path = uri.GetHostAndLocalPath();
        n_assert(path.IsValid());
        if (FSWrapper::FileExists(path))
        {
            Ptr<Stream> stream = (Stream*)streamClass.Create();
            stream->SetURI(uri);
            return stream;
        }
    }
#endif
    // check if the URI points into a mounted archive
    if (IsArchiveFileSystemEnabled())
    {
        URI newUri = ArchiveFileSystem::Instance()->ConvertFileToArchiveURIIfExists(uri);
        Ptr<Stream> stream = (Stream*)state.schemeRegistry->GetStreamClassByUriScheme(newUri.Scheme()).Create();
        stream->SetURI(newUri);
        return stream;
    }
    else
    {
        Ptr<Stream> stream = (Stream*)state.schemeRegistry->GetStreamClassByUriScheme(uri.Scheme()).Create();
        stream->SetURI(uri);
        return stream;
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
MountArchive(const URI& uri)
{
    Ptr<Archive> archive = state.archiveFileSystem->Mount(uri);
    return archive.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
bool
MountEmbeddedArchive(const URI& uri)
{
    Ptr<Archive> archive = state.archiveFileSystem->MountEmbedded(uri, "root:");
    return archive.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
void
UnmountArchive(const URI& uri)
{
    state.archiveFileSystem->Unmount(uri);
}

//------------------------------------------------------------------------------
/**
*/
bool
IsArchiveMounted(const URI& uri)
{
    return state.archiveFileSystem->IsMounted(uri);
}

//------------------------------------------------------------------------------
/**
*/
void
SetArchiveFileSystemEnabled(bool b)
{
    state.archiveFileSystemEnabled = b;
}

//------------------------------------------------------------------------------
/**
*/
bool
IsArchiveFileSystemEnabled()
{
    return state.archiveFileSystemEnabled && state.archiveFileSystem->HasArchives();
}

//------------------------------------------------------------------------------
/**
*/
void
MountStandardArchives()
{
    state.archiveCriticalSection.Enter();
    n_assert(!state.StandardArchivesMounted);
    state.StandardArchivesMounted = true;

    // first load the platform specific archives, since platform specific
    // files override any generic files
    // if the archive files don't exist, no archives will be mounted
    System::SystemInfo systemInfo;
    String platformString = System::PlatformTypeAsString(System::Platform);
    String platformArchivePath = "root:export";

    // prioritize export folder before zip
    if (!DirectoryExists(platformArchivePath))
    {
        MountArchive(platformArchivePath);
    }

    state.archiveCriticalSection.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
UnmountStandardArchives()
{
    state.archiveCriticalSection.Enter();
    n_assert(state.StandardArchivesMounted);
    state.StandardArchivesMounted = false;

    System::SystemInfo systemInfo;
    String platformString = System::PlatformTypeAsString(System::Platform);
    String platformArchivePath = "root:export";
    if (IsArchiveMounted(platformArchivePath))
    {
        UnmountArchive(platformArchivePath);
    }

    state.archiveCriticalSection.Leave();
}

//------------------------------------------------------------------------------
/**
    This method creates all missing directories in a path.
*/
bool
CreateDirectory(const URI& uri)
{
    n_assert(uri.Scheme() == "file");
    String path = uri.GetHostAndLocalPath();

    // build stack of non-existing dir components
    Stack<String> pathStack;
    while ((!path.IsEmpty()) && (!DirectoryExists(path)))
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
*/
bool
EnsureDirectoriesForFile(const URI& uri)
{
    n_assert(uri.Scheme() == "file");
    String path = uri.GetHostAndLocalPath();

    String folder = path.ExtractToLastSlash();
    return CreateDirectory(folder);
}

//------------------------------------------------------------------------------
/**
    This copies a file to another file.
*/
bool
CopyFile(const URI& fromUri, const URI& toUri)
{
    // transparent archive support...
    URI srcUri;
    if (IsArchiveFileSystemEnabled())
    {
        srcUri = state.archiveFileSystem->ConvertFileToArchiveURIIfExists(fromUri);
    }
    else
    {
        srcUri = fromUri;
    }

    // if target file exists, remove read-only flag
    if (FileExists(toUri))
    {
        SetReadOnly(toUri, false);
    }

    // create file streams
    Ptr<Stream> srcStream = CreateStream(srcUri);
    srcStream->SetAccessMode(Stream::ReadAccess);
    if (srcStream->Open())
    {
        Ptr<Stream> toStream = CreateStream(toUri);
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
ComputeFileCrc(const URI& uri)
{
    unsigned int result = 0;

    // create a stream
    Ptr<Stream> stream = CreateStream(uri);
    stream->SetAccessMode(Stream::ReadAccess);
    if (stream->Open())
    {
        Crc crc;
        crc.Begin();
        const int bufSize = (1 << 16);
        unsigned char* buffer = (unsigned char*)Memory::Alloc(Memory::ScratchHeap, bufSize);
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
    Renames a file or a directory on the conventional filesystem. The target must not exist as the
    exact name, so a rename never replaces anything. The name is looked up in the directory listing
    rather than through FileExists(), which is case-insensitive on Windows, so a name that differs
    only in case counts as a new name on both platforms.
*/
bool
RenameFile(const URI& fromUri, const URI& toUri)
{
    n_assert(fromUri.Scheme() == "file");
    n_assert(toUri.Scheme() == "file");
    const String fromPath = fromUri.GetHostAndLocalPath();
    const String toPath = toUri.GetHostAndLocalPath();
    n_assert(fromPath.IsValid());
    n_assert(toPath.IsValid());

    String toDir = toPath.ExtractToLastSlash();
    if (toDir.IsEmpty())
        toDir = ".";
    const String toName = toPath.ExtractFileName();

    for (const String& name : FSWrapper::ListFiles(toDir, "*"))
    {
        if (name == toName)
            return false;
    }
    for (const String& name : FSWrapper::ListDirectories(toDir, "*"))
    {
        if (name == toName)
            return false;
    }

    return FSWrapper::RenameFile(fromPath, toPath);
}

//------------------------------------------------------------------------------
/**
*/
bool
DeleteFile(const URI& uri)
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
DeleteDirectory(const URI& uri)
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
FileExists(const URI& uri)
{
    // transparent archive support
    if (IsArchiveFileSystemEnabled())
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
DirectoryExists(const URI& uri)
{
    // transparent archive support
    if (IsArchiveFileSystemEnabled())
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
SetReadOnly(const URI& uri, bool readOnly)
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
IsReadOnly(const URI& uri)
{
    n_assert(uri.Scheme() == "file");

    // transparent archive file system support
    if (IsArchiveFileSystemEnabled())
    {
        URI arcUri = state.archiveFileSystem->ConvertFileToArchiveURIIfExists(uri);
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
SetFileWriteTime(const URI& uri, FileTime fileTime)
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
GetFileWriteTime(const URI& uri)
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
AddPathPrefixToArray(const String& prefix, const Array<String>& filenames)
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
Array<String>
ListFiles(const URI& uri, const String& pattern, bool asFullPath)
{
    n_assert(pattern.IsValid());
    Array<String> result;

    //FIXME this should be handled more generically
    if (uri.Scheme() != "file")
    {
        Util::String fileList;
        URI listFile = uri;
        listFile.AppendLocalPath("/_files.lst");
        if (ReadFile(listFile, fileList))
        {
            Util::Array<Util::String> files = fileList.Tokenize("\n");
            for (auto const& fileName : files)
            {
                if (String::MatchPattern(fileName, pattern))
                {
                    result.Append(fileName);
                }
            }
            return result;
        }
    }

    // transparent archive file system support
    if (IsArchiveFileSystemEnabled())
    {
        Ptr<Archive> archive = ArchiveFileSystem::Instance()->FindArchiveWithDir(uri);
        if (archive.isvalid())
        {
            String pathInArchive = archive->ConvertToPathInArchive(uri.LocalPath());
            result = archive->ListFiles(pathInArchive, pattern);
        }
    }
// add local files when not public
#if !PUBLIC_BUILD
    Array<String> localresult;
    localresult = FSWrapper::ListFiles(uri.GetHostAndLocalPath(), pattern);
    for (const String& str : localresult)
    {
        if (result.FindIndex(str) == InvalidIndex)
        {
            result.Append(str);
        }
    }
#endif
    if (asFullPath)
    {
        result = AddPathPrefixToArray(uri.LocalPath(), result);
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
Array<String>
ListDirectories(const URI& uri, const String& pattern, bool asFullPath, bool prioritizeArchive)
{
    n_assert(pattern.IsValid());
    Array<String> result;

    // transparent archive file system support
    if (IsArchiveFileSystemEnabled() && prioritizeArchive)
    {
        Ptr<Archive> archive = ArchiveFileSystem::Instance()->FindArchiveWithDir(uri);
        if (archive.isvalid())
        {
            String pathInArchive = archive->ConvertToPathInArchive(uri.LocalPath());
            result = archive->ListDirectories(pathInArchive, pattern);
            if (asFullPath)
            {
                result = AddPathPrefixToArray(uri.LocalPath(), result);
            }
            return result;
        }
    }

    // fallthrough: not contained in archive, handle conventionally
    result = FSWrapper::ListDirectories(uri.GetHostAndLocalPath(), pattern);
    if (asFullPath)
    {
        result = AddPathPrefixToArray(uri.LocalPath(), result);
    }
    return result;
}



//------------------------------------------------------------------------------
/**
*/
bool
ReadFile(const URI& path, Util::String& contents)
{
    // create file stream
    Ptr<Stream> stream = CreateStream(path);

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
IsLocked(const URI& uri)
{
    n_assert(uri.Scheme() == "file");
    n_assert(FileExists(uri));
    const String path = uri.GetHostAndLocalPath();
    n_assert(path.IsValid());
    return FSWrapper::IsLocked(path);
}

//------------------------------------------------------------------------------
/**
*/
IO::URI
CreateTemporaryFilename(const URI& uri)
{
    n_assert(uri.Scheme() == "file");
    const String path = uri.GetHostAndLocalPath();
    n_assert(path.IsValid());
    const String tmpName = FSWrapper::CreateTemporaryFilename(path);
    if (!tmpName.IsValid())
    {
        return URI();
    }
    return URI(tmpName);
}

//------------------------------------------------------------------------------
/**
*/
Util::String
NativePath(const Util::String& path)
{
#if __cplusplus > 201703L
    std::filesystem::path u8path = std::filesystem::absolute(std::filesystem::path((char8_t*)path.AsCharPtr()));
#else
    std::filesystem::path u8path = std::filesystem::absolute(std::filesystem::u8path(path.AsCharPtr()));
#endif
    return u8path.string().c_str();
}

//------------------------------------------------------------------------------
/**
*/
bool
GetIOInfo(const URI& uri, IOStat& outInfo, bool prioritizeArchive)
{
    // transparent archive support
    if (IsArchiveFileSystemEnabled() && prioritizeArchive)
    {
        Ptr<Archive> archive = ArchiveFileSystem::Instance()->FindArchiveWithFile(uri);
        if (archive.isvalid())
        {
            String pathInArchive = archive->ConvertToPathInArchive(uri.LocalPath());
            return archive->GetIOInfo(pathInArchive, outInfo);
        }
    }

    // doesn't exist as archive, check conventional filesystem
    const String path = uri.GetHostAndLocalPath();
    n_assert(path.IsValid());
    return FSWrapper::GetIOInfo(path, outInfo);
}

//------------------------------------------------------------------------------
/**
*/
void
IterateFolders(const URI& dir, const Util::String& pattern, FileMapFunc iter)
{
    Util::Array<Util::String> dirs;
    dirs.Append(dir.LocalPath());

    while (dirs.Size() > 0)
    {
        Util::String currentDir = dirs.PopFront();
        Util::Array<Util::String> files = ListFiles(currentDir, pattern, false);
        dirs.AppendArray(ListDirectories(currentDir, "*", true));
        for (auto file : files)
        {
            iter(file, currentDir);
        }
    }
}

} // namespace IO
