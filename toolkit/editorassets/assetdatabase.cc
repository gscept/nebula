//------------------------------------------------------------------------------
//  assetdatabase.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editorassets/assetdatabase.h"
#include "editorassets/assettype.h"
#include "editorassets/livebatcher.h"
#include "io/fswrapper.h"
#include "io/ioserver.h"
#include "threading/thread.h"

namespace Editor
{

__ImplementClass(Editor::AssetDatabase, 'ASDB', Core::RefCounted);
__ImplementInterfaceSingleton(Editor::AssetDatabase);

namespace
{
// Recursively remove a folder subtree from FileDB so parent deletion can succeed.
void
DeleteFolderSubtree(ToolkitUtil::FileDB& fileDB, ToolkitUtil::Logger& logger, uint64_t folderId)
{
    Util::Array<ToolkitUtil::FileDB::FileInfo> files;
    fileDB.GetFilesInFolder(folderId, files);
    for (const auto& file : files)
    {
        fileDB.DeleteFile(logger, file.id);
    }

    Util::Array<ToolkitUtil::FileDB::FolderInfo> children;
    fileDB.GetChildFolders(folderId, children);
    for (const auto& child : children)
    {
        DeleteFolderSubtree(fileDB, logger, child.id);
        fileDB.DeleteFolder(logger, child.id);
    }
}

// The FileDB the asset index is kept in
const char* const DatabaseURI = "int:/filedb.sqlite";
}

/// filesystem listing and FileDB writes run here so the editor frame is not stalled by _wstat64 / sqlite
class ScanFolderJob : public Threading::Thread
{
    __DeclareClass(ScanFolderJob);
public:
    AssetDatabase* database = nullptr;
    /// 0..1, negative while the work is still being counted
    volatile float progress = -1.0f;
    /// set when the whole tree has been scanned
    Threading::SafeFlag done;

    void DoWork() override
    {
        ToolkitUtil::FileDB fileDB;
        ToolkitUtil::Logger logger;
        fileDB.SetDatabaseURI(IO::URI(DatabaseURI));
        const bool dbOpened = fileDB.Open(logger, false);
        n_assert(dbOpened);

        uint64_t rootFolderId = fileDB.CreateRootFolder("Root", IO::FSWrapper::GetFileWriteTime(IO::Path().WorkURI("assets").LocalPath()), false);
        ToolkitUtil::FileDB::FolderInfo rootFolderInfo;
        fileDB.GetFolderInfo(rootFolderId, rootFolderInfo);

        Util::Dictionary<IO::Path, uint64_t> pathToId;
        pathToId.Add(rootFolderInfo.folderPath, rootFolderId);

        // count the work up front, listing is cheap compared to the stat and FileDB write each file gets in the scan.
        // one unit per folder and per file
        SizeT totalWork = 0;
        {
            Util::Array<IO::URI> countStack;
            countStack.Append(IO::Path().WorkURI("assets"));
            while (!countStack.IsEmpty() && !this->ThreadStopRequested())
            {
                const IO::URI uri = countStack.Back();
                countStack.EraseBack();
                if (!IO::DirectoryExists(uri))
                {
                    continue;
                }
                totalWork += 1 + IO::ListFiles(uri, "*", false).Size();
                for (const Util::String& dir : IO::ListDirectories(uri, "*", true, false))
                {
                    countStack.Append(IO::URI(dir));
                }
            }
        }
        SizeT doneWork = 0;
        this->progress = 0.0f;

        Util::Array<IO::Path> pathStack;
        pathStack.Append(IO::Path());
        // the editor can close while the tree is scanned, so each folder checks whether to stop
        while (!pathStack.IsEmpty() && !this->ThreadStopRequested())
        {
            const IO::Path path = pathStack.Back();
            pathStack.EraseBack();

            const IO::URI workUri = path.WorkURI("assets");

            if (!IO::DirectoryExists(workUri))
            {
                continue;
            }

            IndexT idIndex = pathToId.FindIndex(path);
            n_assert(idIndex != InvalidIndex);
            const uint64_t folderId = pathToId.ValueAtIndex(idIndex);
            AssetDatabase::ScanFolder(fileDB, logger, workUri, false, folderId, false);

            AssetDatabase::FolderScanResult result;
            result.folderId = folderId;
            fileDB.GetFilesInFolder(folderId, result.files);
            fileDB.GetChildFolders(folderId, result.children);
            this->database->pendingScanResults.Enqueue(result);

            // files can come and go between counting and scanning, so keep it in range
            doneWork += 1 + result.files.Size();
            if (totalWork > 0)
            {
                this->progress = Math::min(float(doneWork) / float(totalWork), 1.0f);
            }

            // pushed in reverse so the children are scanned in listing order
            for (SizeT i = result.children.Size(); i > 0; i--)
            {
                const ToolkitUtil::FileDB::FolderInfo& child = result.children[i - 1];
                if (!pathToId.Contains(child.folderPath))
                {
                    pathToId.Add(child.folderPath, child.id);
                    pathStack.Append(child.folderPath);
                }
            }
        }
        this->progress = 1.0f;
        fileDB.Close();
        this->done.Set();
    }
};
__ImplementClass(Editor::ScanFolderJob, 'ScFj', Threading::Thread);

//------------------------------------------------------------------------------
/**
*/
AssetDatabase::AssetDatabase() :
    rootFolder(0),
    watching(false),
    version(0)
{
    __ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
AssetDatabase::~AssetDatabase()
{
    __DestructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetDatabase::Setup()
{
    this->fileDB.SetDatabaseURI(IO::URI(DatabaseURI));
    if (!this->fileDB.Open(this->logger, false))
    {
        n_warning("Editor assets: could not open the asset index at '%s'\n", DatabaseURI);
        return false;
    }

    this->rootFolder = this->fileDB.CreateRootFolder("Root", IO::FSWrapper::GetFileWriteTime(IO::Path().WorkURI("assets").LocalPath()), false);
    ToolkitUtil::FileDB::FolderInfo rootInfo;
    this->fileDB.GetFolderInfo(this->rootFolder, rootInfo);
    this->folders.Add(this->rootFolder, rootInfo);
    this->folderIds.Add(IO::Path(), this->rootFolder);

    Util::BitField<8> watchFlags;
    watchFlags.SetBit(IO::WatchFlags::NameChanged);
    watchFlags.SetBit(IO::WatchFlags::SizeChanged);
    watchFlags.SetBit(IO::WatchFlags::Creation);
    IO::WatchDelegate callback = [this](IO::WatchEvent const& event)
    {
        this->pendingWatchEvents.Enqueue(event);
    };
    IO::FileWatcher::Instance()->Watch(IO::Path().WorkURI("assets").LocalPath(), true, watchFlags, callback);
    this->watching = true;

    this->scanJob = ScanFolderJob::Create();
    this->scanJob->database = this;
    this->scanJob->Start();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDatabase::Discard()
{
    // IsWatched can't be asked here, the watch is added on the file watcher thread and may not be there yet
    if (this->watching)
    {
        IO::FileWatcher::Instance()->Unwatch(IO::Path().WorkURI("assets").LocalPath());
        this->watching = false;
    }

    if (this->scanJob.isvalid())
    {
        this->scanJob->Stop();
        this->scanJob = nullptr;
    }

    if (this->fileDB.IsOpen())
        this->fileDB.Close();

    this->folders.Clear();
    this->folderIds.Clear();
    this->folderChildren.Clear();
    this->scannedFolders.Clear();
    this->searchIndex.Clear();
    this->rootFolder = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDatabase::Update()
{
    if (!this->fileDB.IsOpen())
        return;

    Util::Array<FolderScanResult> scanResults;
    scanResults.Reserve(this->pendingScanResults.Size());
    this->pendingScanResults.DequeueAll(scanResults);
    for (const FolderScanResult& result : scanResults)
    {
        if (!this->folders.Contains(result.folderId))
        {
            // a parent's result always arrives first and caches the folder, so this one was renamed while the scan was running
            continue;
        }
        this->CacheScannedFolder(result.folderId, result.children, result.files);
    }

    // the results of the last folders are queued before the job says it is done
    if (this->scanJob.isvalid() && this->scanJob->done.Test())
    {
        this->scanJob->Stop();
        this->scanJob = nullptr;
        this->Changed();
    }

    this->ApplyWatchEvents();
}

//------------------------------------------------------------------------------
/**
*/
uint64_t
AssetDatabase::GetRootFolder() const
{
    return this->rootFolder;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetDatabase::IsScanning() const
{
    return this->scanJob.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
float
AssetDatabase::GetScanProgress() const
{
    return this->scanJob.isvalid() ? this->scanJob->progress : 1.0f;
}

//------------------------------------------------------------------------------
/**
*/
uint
AssetDatabase::GetVersion() const
{
    return this->version;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDatabase::Changed()
{
    this->version++;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetDatabase::HasFolder(uint64_t folderId) const
{
    return folderId != 0 && this->folders.Contains(folderId);
}

//------------------------------------------------------------------------------
/**
*/
AssetDatabase::FolderInfo
AssetDatabase::GetFolder(uint64_t folderId) const
{
    n_assert(this->HasFolder(folderId));
    return this->folders[folderId];
}

//------------------------------------------------------------------------------
/**
*/
uint64_t
AssetDatabase::FindFolder(const IO::Path& folder) const
{
    IndexT i = this->folderIds.FindIndex(folder);
    return i != InvalidIndex ? this->folderIds.ValueAtIndex(i) : 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetDatabase::IsInFolder(uint64_t folderId, uint64_t ancestorId) const
{
    uint64_t walkId = folderId;
    while (walkId != 0 && this->folders.Contains(walkId))
    {
        if (walkId == ancestorId)
            return true;
        const FolderInfo& walkInfo = this->folders[walkId];
        if (walkInfo.isRoot)
            break;
        walkId = walkInfo.parentId;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    While the background scan runs it reaches every folder, so a folder it has not reached yet is
    left to it rather than scanned twice.
*/
void
AssetDatabase::GetChildFolders(uint64_t folderId, Util::Array<uint64_t>& out)
{
    out.Clear();
    if (!this->HasFolder(folderId))
        return;

    if (!this->scannedFolders.Contains(folderId) && !this->IsScanning())
        this->ScanNow(folderId);

    IndexT i = this->folderChildren.FindIndex(folderId);
    if (i == InvalidIndex)
        return;

    out = this->folderChildren.ValueAtIndex(i);
    std::sort(out.begin(), out.end(), [this](uint64_t a, uint64_t b)
    {
        return this->folders[a].name < this->folders[b].name;
    });
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDatabase::GetFiles(uint64_t folderId, Util::Array<FileInfo>& out)
{
    out.Clear();
    if (!this->HasFolder(folderId))
        return;

    if (!this->scannedFolders.Contains(folderId))
    {
        if (this->IsScanning())
        {
            // the scan gets to it, until then the files come straight from the disk. The ids are made
            // up, the folder is read again from the FileDB once the scan has it
            const FolderInfo info = this->folders[folderId];
            Util::Array<Util::String> files = IO::ListFiles(info.folderPath.WorkURI("assets"), "*", false);
            out.Reserve(files.Size());
            for (IndexT i = 0; i < files.Size(); i++)
            {
                const Util::String& fileName = files[i];
                FileInfo file;
                file.id = files.Size() - i;
                file.name = fileName;
                file.type = DetermineFileType(fileName.GetFileExtension());
                file.folderId = folderId;
                Util::String stem = fileName;
                stem.StripFileExtension();
                file.filePath = IO::Path::File(info.folderPath.GetFolder(), stem, ToolkitUtil::FileTypeURNMapping[file.type]);
                IO::IOStat stat;
                IO::FSWrapper::GetIOInfo(file.filePath.WorkURI("assets"), stat);
                file.size = stat.size;
                file.modifiedDate = stat.modifiedTime;
                out.Append(file);
            }
            return;
        }
        this->ScanNow(folderId);
    }

    this->fileDB.GetFilesInFolder(folderId, out);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<AssetDatabase::FileInfo>&
AssetDatabase::GetSearchIndex() const
{
    return this->searchIndex;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDatabase::ScanNow(uint64_t folderId)
{
    const FolderInfo info = this->folders[folderId];
    ScanFolder(this->fileDB, this->logger, info.folderPath.WorkURI("assets"), info.isArchive, folderId, false);

    Util::Array<FolderInfo> children;
    this->fileDB.GetChildFolders(folderId, children);
    Util::Array<FileInfo> files;
    this->fileDB.GetFilesInFolder(folderId, files);
    this->CacheScannedFolder(folderId, children, files);
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDatabase::CacheScannedFolder(uint64_t folderId, const Util::Array<FolderInfo>& children, const Util::Array<FileInfo>& files)
{
    Util::Array<uint64_t> childIds;
    childIds.Reserve(children.Size());
    for (const auto& child : children)
    {
        if (this->folders.Contains(child.id))
        {
            this->folders[child.id] = child;
        }
        else
        {
            this->folders.Add(child.id, child);
        }
        if (!this->folderIds.Contains(child.folderPath))
        {
            this->folderIds.Add(child.folderPath, child.id);
        }
        childIds.Append(child.id);
    }
    this->folderChildren.Emplace(folderId) = childIds;

    this->IndexFolderForSearch(folderId, files);
    this->scannedFolders.Emplace(folderId) = true;
    this->Changed();
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDatabase::IndexFolderForSearch(uint64_t folderId, const Util::Array<FileInfo>& files)
{
    if (this->scannedFolders.Contains(folderId))
    {
        IndexT write = 0;
        for (IndexT read = 0; read < this->searchIndex.Size(); read++)
        {
            if (this->searchIndex[read].folderId != folderId)
            {
                if (write != read)
                {
                    this->searchIndex[write] = this->searchIndex[read];
                }
                write++;
            }
        }
        this->searchIndex.Resize(write);
    }
    this->searchIndex.Reserve(this->searchIndex.Size() + files.Size());
    for (const auto& file : files)
    {
        this->searchIndex.Append(file);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDatabase::ReindexFolder(uint64_t folderId)
{
    Util::Array<FileInfo> files;
    this->fileDB.GetFilesInFolder(folderId, files);
    this->IndexFolderForSearch(folderId, files);
    this->Changed();
}

//------------------------------------------------------------------------------
/**
    The watcher reports a file by the folder it is in, relative to the assets folder. A folder the
    index has not scanned yet is left alone, the scan reads it as it is then.
*/
void
AssetDatabase::ApplyWatchEvents()
{
    Util::Array<IO::WatchEvent> events;
    events.Reserve(this->pendingWatchEvents.Size());
    this->pendingWatchEvents.DequeueAll(events);
    if (events.IsEmpty())
        return;

    Util::Array<uint64_t> changedFolders;
    for (const IO::WatchEvent& event : events)
    {
        const Util::String& fileName = event.file;
        if (fileName.IsEmpty())
            continue;

        Util::String relative = event.relativePath;
        relative.SubstituteChar('\\', '/');
        relative.Trim("/");
        const IO::Path folderPath = relative.IsEmpty() ? IO::Path() : IO::Path::Folder(relative);
        const uint64_t folderId = this->FindFolder(folderPath);
        if (folderId == 0 || !this->scannedFolders.Contains(folderId))
            continue;

        // files are addressed by their name on disk, as not every file type has a work mapping to build it from
        Util::String folder = folderPath.WorkURI("assets").LocalPath();
        folder.TrimRight("/\\");
        const IO::URI fileUri = Util::Format("%s/%s", folder.AsCharPtr(), fileName.AsCharPtr());
        const bool fileExistsOnDisk = IO::FileExists(fileUri);

        uint64_t knownId = 0;
        Util::Array<FileInfo> files;
        this->fileDB.GetFilesInFolder(folderId, files);
        for (const FileInfo& file : files)
        {
            if (file.name == fileName)
            {
                knownId = file.id;
                break;
            }
        }

        bool changed = false;
        switch (event.type)
        {
            case IO::WatchEventType::Created:
            case IO::WatchEventType::NameChange:
            {
                if (fileExistsOnDisk)
                {
                    IO::IOStat ioInfo;
                    IO::Stream::Size fileSize = 0;
                    IO::FileTime modifiedTime;
                    if (IO::GetIOInfo(fileUri, ioInfo, false))
                    {
                        fileSize = ioInfo.size;
                        modifiedTime = ioInfo.modifiedTime;
                    }
                    this->fileDB.AddFile(this->logger, fileName, folderId, fileSize, DetermineFileType(fileName.GetFileExtension()), modifiedTime);
                    changed = true;
                }
                else if (knownId != 0)
                {
                    // renamed away, the new name comes as an event of its own
                    this->fileDB.DeleteFile(this->logger, knownId);
                    changed = true;
                }
                break;
            }
            case IO::WatchEventType::Deleted:
            {
                if (knownId != 0)
                {
                    this->fileDB.DeleteFile(this->logger, knownId);
                    changed = true;
                }
                break;
            }
            case IO::WatchEventType::Modified:
            {
                IO::IOStat ioInfo;
                if (knownId != 0 && IO::GetIOInfo(fileUri, ioInfo, false))
                {
                    this->fileDB.UpdateFileMetadata(this->logger, knownId, ioInfo.modifiedTime);
                    changed = true;
                }
                break;
            }
            default:
                break;
        }

        if (changed && changedFolders.FindIndex(folderId) == InvalidIndex)
            changedFolders.Append(folderId);
    }

    for (uint64_t folderId : changedFolders)
        this->ReindexFolder(folderId);
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetDatabase::CreateFolder(uint64_t parentId, const Util::String& name, uint64_t& outFolderId, Util::String& outError)
{
    ToolkitUtil::FileDB::FolderInfo parentInfo;
    if (!this->fileDB.GetFolderInfo(parentId, parentInfo))
    {
        outError = "The parent folder doesn't exist anymore";
        return false;
    }
    if (parentInfo.isArchive)
    {
        outError = "Folders can't be created inside archives";
        return false;
    }

    const IO::URI uri = (parentInfo.folderPath / name).WorkURI("assets");
    if (IO::DirectoryExists(uri) || IO::FileExists(uri))
    {
        outError.Format("'%s' already exists", name.AsCharPtr());
        return false;
    }
    if (!IO::CreateDirectory(uri))
    {
        outError.Format("Could not create the folder '%s'", name.AsCharPtr());
        return false;
    }

    outFolderId = this->fileDB.CreateFolder(this->logger, name, parentId, IO::FSWrapper::GetFileWriteTime(uri.LocalPath()), false);
    this->CacheFolderSubtree(outFolderId);
    this->Changed();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetDatabase::RenameFolder(uint64_t folderId, const Util::String& newName, uint64_t& outFolderId, Util::String& outError)
{
    ToolkitUtil::FileDB::FolderInfo info, parentInfo;
    if (!this->fileDB.GetFolderInfo(folderId, info) || info.isRoot || !this->fileDB.GetFolderInfo(info.parentId, parentInfo))
    {
        outError = "This folder can't be renamed";
        return false;
    }
    if (info.isArchive)
    {
        outError = "Folders inside archives can't be renamed";
        return false;
    }
    if (newName == info.name)
    {
        outFolderId = folderId;
        return true;
    }

    const IO::URI fromUri = info.folderPath.WorkURI("assets");
    const IO::URI toUri = (parentInfo.folderPath / newName).WorkURI("assets");
    // on case insensitive file systems a case-only rename finds the folder itself as the target
    const bool caseOnly = Util::String::StrICmp(newName.AsCharPtr(), info.name.AsCharPtr()) == 0;
    if (!caseOnly && (IO::DirectoryExists(toUri) || IO::FileExists(toUri)))
    {
        outError.Format("'%s' already exists", newName.AsCharPtr());
        return false;
    }
    if (!IO::RenameFile(fromUri, toUri))
    {
        outError.Format("Could not rename '%s', a file inside it might be in use", info.name.AsCharPtr());
        return false;
    }

    // folder ids are derived from parent id and name, so the subtree is registered again under the new name
    this->UncacheFolderSubtree(folderId);
    IndexT siblingsIndex = this->folderChildren.FindIndex(info.parentId);
    if (siblingsIndex != InvalidIndex)
    {
        Util::Array<uint64_t>& siblings = this->folderChildren.ValueAtIndex(siblingsIndex);
        IndexT i = siblings.FindIndex(folderId);
        if (i != InvalidIndex)
        {
            siblings.EraseIndex(i);
        }
    }

    this->fileDB.BeginTransaction();
    DeleteFolderSubtree(this->fileDB, this->logger, folderId);
    this->fileDB.DeleteFolder(this->logger, folderId);
    outFolderId = this->fileDB.CreateFolder(
        this->logger, newName, info.parentId, IO::FSWrapper::GetFileWriteTime(toUri.LocalPath()), false
    );
    ScanFolder(this->fileDB, this->logger, toUri, false, outFolderId, true);
    this->fileDB.EndTransaction();
    this->CacheFolderSubtree(outFolderId);
    this->Changed();
    return true;
}

//------------------------------------------------------------------------------
/**
    The work file is the one the asset's URN, folder:name:type, resolves to. The type writes it, the
    database only checks that it can go there and indexes it.
*/
bool
AssetDatabase::CreateAsset(uint64_t folderId, const AssetType& type, const Util::String& variant, const Util::String& name, uint64_t& outFileId, Util::String& outError)
{
    ToolkitUtil::FileDB::FolderInfo folderInfo;
    if (!this->fileDB.GetFolderInfo(folderId, folderInfo))
    {
        outError = "The folder doesn't exist anymore";
        return false;
    }
    if (folderInfo.isArchive)
    {
        outError = "Assets can't be created inside archives";
        return false;
    }
    if (!type.create.IsValid())
    {
        outError.Format("A %s can't be created from nothing", type.name.AsCharPtr());
        return false;
    }

    const IO::Path path = IO::Path::File(folderInfo.folderPath.GetFolder(), name, type.type);
    const IO::URI uri = path.WorkURI("assets");
    if (IO::FileExists(uri))
    {
        outError.Format("'%s' already exists", path.GetWorkFile().AsCharPtr());
        return false;
    }
    if (!type.create(path, uri, variant, outError))
    {
        if (outError.IsEmpty())
            outError.Format("Could not create '%s'", path.GetWorkFile().AsCharPtr());
        return false;
    }

    IO::IOStat stat;
    IO::Stream::Size size = 0;
    if (IO::GetIOInfo(uri, stat, false))
        size = stat.size;

    outFileId = this->fileDB.AddFile(this->logger, path.GetWorkFile(), folderId, size, FileTypeOf(type.type), IO::FileTime());
    LiveBatcher::BatchFile(path);

    this->ReindexFolder(folderId);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetDatabase::RenameFile(uint64_t fileId, const Util::String& newName, uint64_t& outFileId, Util::String& outError)
{
    ToolkitUtil::FileDB::FileInfo info;
    ToolkitUtil::FileDB::FolderInfo folderInfo;
    if (!this->fileDB.GetFileInfo(fileId, info) || !this->fileDB.GetFolderInfo(info.folderId, folderInfo))
    {
        outError = "The file doesn't exist anymore";
        return false;
    }
    if (folderInfo.isArchive)
    {
        outError = "Files inside archives can't be renamed";
        return false;
    }

    const Util::String extension = info.name.GetFileExtension();
    const Util::String newFileName = extension.IsEmpty() ? newName : Util::Format("%s.%s", newName.AsCharPtr(), extension.AsCharPtr());
    if (newFileName == info.name)
    {
        outFileId = fileId;
        return true;
    }

    // files are addressed by their name on disk, as not every file type has a work mapping to build it from
    Util::String folder = folderInfo.folderPath.WorkURI("assets").LocalPath();
    folder.TrimRight("/\\");
    const IO::URI fromUri = Util::Format("%s/%s", folder.AsCharPtr(), info.name.AsCharPtr());
    const IO::URI toUri = Util::Format("%s/%s", folder.AsCharPtr(), newFileName.AsCharPtr());
    // on case insensitive file systems a case-only rename finds the file itself as the target
    const bool caseOnly = Util::String::StrICmp(newFileName.AsCharPtr(), info.name.AsCharPtr()) == 0;
    if (!caseOnly && (IO::FileExists(toUri) || IO::DirectoryExists(toUri)))
    {
        outError.Format("'%s' already exists", newFileName.AsCharPtr());
        return false;
    }
    if (!IO::RenameFile(fromUri, toUri))
    {
        outError.Format("Could not rename '%s', it might be in use", info.name.AsCharPtr());
        return false;
    }

    // file ids are derived from the name, so the file is registered again under the new one
    this->fileDB.DeleteFile(this->logger, fileId);
    outFileId = this->fileDB.AddFile(this->logger, newFileName, info.folderId, info.size, info.type, info.modifiedDate);

    const char* ns = ToolkitUtil::FileTypeURNMapping[info.type];
    if (IO::Path::WorkExtensions.Contains(ns))
    {
        LiveBatcher::BatchFile(IO::Path::File(folderInfo.folderPath.GetFolder(), newName, ns));
    }

    this->ReindexFolder(info.folderId);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDatabase::CacheFolderSubtree(uint64_t folderId)
{
    ToolkitUtil::FileDB::FolderInfo info;
    if (!this->fileDB.GetFolderInfo(folderId, info))
    {
        return;
    }
    this->folders.Emplace(folderId) = info;
    this->folderIds.Emplace(info.folderPath) = folderId;
    if (!info.isRoot)
    {
        Util::Array<uint64_t>& siblings = this->folderChildren.Emplace(info.parentId);
        if (siblings.FindIndex(folderId) == InvalidIndex)
        {
            siblings.Append(folderId);
        }
    }

    Util::Array<ToolkitUtil::FileDB::FolderInfo> children;
    this->fileDB.GetChildFolders(folderId, children);
    Util::Array<uint64_t>& childIds = this->folderChildren.Emplace(folderId);
    childIds.Clear();
    for (const auto& child : children)
    {
        childIds.Append(child.id);
    }

    Util::Array<ToolkitUtil::FileDB::FileInfo> files;
    this->fileDB.GetFilesInFolder(folderId, files);
    this->IndexFolderForSearch(folderId, files);
    this->scannedFolders.Emplace(folderId) = true;

    for (const auto& child : children)
    {
        this->CacheFolderSubtree(child.id);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDatabase::UncacheFolderSubtree(uint64_t folderId)
{
    IndexT childIndex = this->folderChildren.FindIndex(folderId);
    if (childIndex != InvalidIndex)
    {
        const Util::Array<uint64_t> childIds = this->folderChildren.ValueAtIndex(childIndex);
        this->folderChildren.EraseAtIndex(childIndex);
        for (uint64_t childId : childIds)
        {
            this->UncacheFolderSubtree(childId);
        }
    }

    IndexT infoIndex = this->folders.FindIndex(folderId);
    if (infoIndex != InvalidIndex)
    {
        const IO::Path path = this->folders.ValueAtIndex(infoIndex).folderPath;
        this->folders.EraseAtIndex(infoIndex);
        IndexT pathIndex = this->folderIds.FindIndex(path);
        if (pathIndex != InvalidIndex && this->folderIds.ValueAtIndex(pathIndex) == folderId)
        {
            this->folderIds.EraseAtIndex(pathIndex);
        }
    }

    IndexT scannedIndex = this->scannedFolders.FindIndex(folderId);
    if (scannedIndex != InvalidIndex)
    {
        this->scannedFolders.EraseAtIndex(scannedIndex);
    }

    IndexT write = 0;
    for (IndexT read = 0; read < this->searchIndex.Size(); read++)
    {
        if (this->searchIndex[read].folderId != folderId)
        {
            if (write != read)
            {
                this->searchIndex[write] = this->searchIndex[read];
            }
            write++;
        }
    }
    this->searchIndex.Resize(write);
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDatabase::ScanFolder(ToolkitUtil::FileDB& fileDB, ToolkitUtil::Logger& logger, const IO::URI& folderPath, bool useArchive, uint64_t parent, bool recursive)
{
    if (!recursive)
    {
        fileDB.BeginTransaction();
    }

    // List all files in the current directory
    Util::Array<Util::String> files = IO::ListFiles(folderPath, "*", true);
    Util::Array<Util::String> filesystemFileNames;
    filesystemFileNames.Reserve(files.Size());
    for (const auto& fileName : files)
    {
        Util::String fileLeaf = fileName.ExtractFileName();
        filesystemFileNames.Append(fileLeaf);
        IO::IOStat ioInfo;
        IO::Stream::Size fileSize = 0;
        IO::FileTime modifiedTime;
        if (IO::GetIOInfo(fileName, ioInfo, useArchive))
        {
            fileSize = ioInfo.size;
            modifiedTime = ioInfo.modifiedTime;
        }

        fileDB.AddFile(logger, fileLeaf, parent, fileSize, DetermineFileType(fileLeaf.GetFileExtension()), modifiedTime);
    }

    // Remove files from DB that no longer exist in the filesystem for this folder.
    Util::Array<ToolkitUtil::FileDB::FileInfo> dbFiles;
    fileDB.GetFilesInFolder(parent, dbFiles);
    for (const auto& dbFile : dbFiles)
    {
        bool existsOnDisk = false;
        for (const auto& fsName : filesystemFileNames)
        {
            if (fsName == dbFile.name)
            {
                existsOnDisk = true;
                break;
            }
        }
        if (!existsOnDisk)
        {
            fileDB.DeleteFile(logger, dbFile.id);
        }
    }

    // List all subdirectories and recursively scan them
    Util::Array<Util::String> directories = IO::ListDirectories(folderPath, "*", true, useArchive);
    Util::Array<Util::String> filesystemDirectoryNames;
    filesystemDirectoryNames.Reserve(directories.Size());
    for (const auto& childDir : directories)
    {
        Util::String childName = childDir.ExtractFileName();
        filesystemDirectoryNames.Append(childName);
        uint64_t childId = fileDB.CreateFolder(logger, childName, parent, IO::FSWrapper::GetFileWriteTime(childDir), useArchive);

        // Recursively scan the subdirectory
        if (childId != 0 && recursive)
        {
            ScanFolder(fileDB, logger, childDir, useArchive, childId, true);
        }
    }

    // Remove folders from DB that no longer exist in the filesystem for this parent.
    Util::Array<ToolkitUtil::FileDB::FolderInfo> dbChildren;
    fileDB.GetChildFolders(parent, dbChildren);
    for (const auto& dbChild : dbChildren)
    {
        bool existsOnDisk = false;
        for (const auto& fsDirName : filesystemDirectoryNames)
        {
            if (fsDirName == dbChild.name)
            {
                existsOnDisk = true;
                break;
            }
        }

        if (!existsOnDisk)
        {
            DeleteFolderSubtree(fileDB, logger, dbChild.id);
            fileDB.DeleteFolder(logger, dbChild.id);
        }
    }

    if (!recursive)
    {
        fileDB.EndTransaction();
    }
}

//------------------------------------------------------------------------------
/**
    A registered type knows its work extension, so a type a subsystem adds is recognised without
    anything here. The table after it is for the files no subsystem registers yet.
*/
ToolkitUtil::FileType
AssetDatabase::DetermineFileType(const Util::String& extension)
{
    if (extension.IsEmpty())
    {
        return ToolkitUtil::FileType::Text;
    }

    Util::String ext(extension);
    ext.ToLower();

    for (const AssetType& type : AssetTypeRegistry::GetTypes())
    {
        if (type.workExtension == ext)
        {
            const ToolkitUtil::FileType fileType = FileTypeOf(type.type);
            if (fileType != ToolkitUtil::FileType::Other)
                return fileType;
        }
    }

    // Model files
    if (ext == "nasset")
    {
        return ToolkitUtil::FileType::Asset;
    }

    // Texture files
    if (ext == "natex")
    {
        return ToolkitUtil::FileType::Texture;
    }

    // Surface/Material files
    if (ext == "namat")
    {
        return ToolkitUtil::FileType::Surface;
    }

    // Particle files
    if (ext == "napar")
    {
        return ToolkitUtil::FileType::Particle;
    }

    // Audio files
    if (ext == "naaud")
    {
        return ToolkitUtil::FileType::Audio;
    }

    // Frame files
    if (ext == "json")
    {
        return ToolkitUtil::FileType::Frame;
    }

    // Shader files
    if (ext == "gplb" || ext == "gpul")
    {
        return ToolkitUtil::FileType::Shader;
    }

    // Physics files
    if (ext == "actor" || ext == "physics")
    {
        return ToolkitUtil::FileType::Physics;
    }

    // NavMesh work files, baked to nav:folder/file.navmesh
    if (ext == "nanav")
    {
        return ToolkitUtil::FileType::NavMesh;
    }

    // Default to Other for unknown extensions
    return ToolkitUtil::FileType::Other;
}

//------------------------------------------------------------------------------
/**
    The FileDB keeps a type as one of its own, so a URN type it has no entry for is kept as Other.
*/
ToolkitUtil::FileType
AssetDatabase::FileTypeOf(const Util::String& urnType)
{
    if (urnType.IsEmpty())
        return ToolkitUtil::FileType::Other;

    for (IndexT i = 0; i < ToolkitUtil::FileTypeURNMapping.Size(); i++)
    {
        if (urnType == ToolkitUtil::FileTypeURNMapping.ValueAtIndex(i))
            return ToolkitUtil::FileTypeURNMapping.KeyAtIndex(i);
    }
    return ToolkitUtil::FileType::Other;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetDatabase::ValidateName(const Util::String& name, const char* kind, Util::String& outError)
{
    if (name.IsEmpty())
    {
        outError.Format("The %s name can't be empty", kind);
        return false;
    }
    if (name == "." || name == "..")
    {
        outError.Format("'%s' is not a valid %s name", name.AsCharPtr(), kind);
        return false;
    }
    static const char* InvalidChars = "\\/:*?\"<>|";
    for (IndexT i = 0; i < name.Length(); i++)
    {
        const char c = name[i];
        if ((unsigned char)c < 32 || strchr(InvalidChars, c) != nullptr)
        {
            outError.Format("A %s name can't contain any of these characters: \\ / : * ? \" < > |", kind);
            return false;
        }
    }
    if (name[name.Length() - 1] == '.')
    {
        outError.Format("A %s name can't end with a period", kind);
        return false;
    }
    return true;
}

} // namespace Editor
