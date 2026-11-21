//------------------------------------------------------------------------------
//  assetbrowser.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "assetbrowser.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/ui/uimanager.h"
#include "editor/ui/windowserver.h"
#include "io/ioserver.h"
#include "io/fswrapper.h"
#include "imgui_internal.h"
#include "asseteditor/asseteditor.h"
#include "importwindow.h"
#include "timing/calendartime.h"
#include "editor/tools/pathconverter.h"
#include "io/filewatcher.h"
#include "io/assignregistry.h"
#include "asseteditor/particleasseteditor.h"
#include "asseteditor/materialasseteditor.h"
#include "editor/tools/livebatcher.h"
#include "toolkitutil/asset/assetimporter.h"
#include "toolkitutil/filedb/filedb.h"
#include "dynui/nebula_icons.h"

using namespace Editor;

namespace Presentation
{

Util::Dictionary<uint64_t, ToolkitUtil::FileDB::FolderInfo> AssetBrowser::folderInfoCache;
Util::Array<ToolkitUtil::FileDB::FileInfo> AssetBrowser::fileInfoCache;
Util::Dictionary<IO::Path, uint64_t> AssetBrowser::fileInfoDict;
Util::Dictionary<IO::Path, uint64_t> AssetBrowser::folderInfoDict;
Util::Dictionary<uint64_t, Util::Array<uint64_t>> AssetBrowser::folderChildIds;
Util::Dictionary<uint64_t, bool> AssetBrowser::scannedFolders;

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

// Check a folder or asset name typed by the user, kind names which in the messages. Uses the Windows rules on every
// platform so asset folders stay portable, which also keeps the URN separator out of names.
bool
ValidateName(const Util::String& name, const char* kind, Util::String& outError)
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
}

/// filesystem listing and FileDB writes run here so the editor frame is not stalled by _wstat64 / sqlite
class ScanFolderJob : public Threading::Thread
{
    __DeclareClass(ScanFolderJob);
public:
    AssetBrowser* browser;
    AssetBrowser* secondBrowser = nullptr;
    /// 0..1, negative while the work is still being counted
    volatile float progress = -1.0f;

    void DoWork() override
    {
        ToolkitUtil::FileDB fileDB;
        ToolkitUtil::Logger logger;
        fileDB.SetDatabaseURI(IO::URI("int:/filedb.sqlite"));
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
            while (!countStack.IsEmpty())
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
        pathStack.Clear();
        pathStack.Append(IO::Path());
        while (!pathStack.IsEmpty())
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
            this->browser->ScanFolder(fileDB, workUri, false, folderId, false);

            AssetBrowser::FolderScanResult result;
            result.folderId = folderId;
            fileDB.GetFilesInFolder(folderId, result.files);
            fileDB.GetChildFolders(folderId, result.children);
            this->browser->pendingScanResults.Enqueue(result);
            if (this->secondBrowser != nullptr)
            {
                this->secondBrowser->pendingScanResults.Enqueue(result);
            }

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
        this->browser->isDoneRefreshingCaches.Set();
        if (this->secondBrowser != nullptr)
        {
            this->secondBrowser->isDoneRefreshingCaches.Set();
        }

        Util::Array<uint64_t> pendingFolderRefreshes;
        uint64_t lastQueuedFolder = 0;
        do
        {
            this->browser->pendingFolderRefreshes.Wait();
            this->browser->pendingFolderRefreshes.DequeueAll(pendingFolderRefreshes);
            if (pendingFolderRefreshes.IsEmpty())
            {
                continue;
            }
            lastQueuedFolder = pendingFolderRefreshes.Back();
        } while (lastQueuedFolder != -1);
    }
    float GetProgress() const
    {
        return this->progress;
    }
};
__ImplementClass(Presentation::ScanFolderJob, 'ScFj', Threading::Thread);

using NewFunc = void(*)(const Ptr<IO::Stream>& file, const Util::String& path);

//------------------------------------------------------------------------------
/**
    A new material from the asset browser gets the template the importer gives opaque materials
*/
static void
NewMaterial(const Ptr<IO::Stream>& file, const Util::String& path)
{
    MaterialNew(file, path, "GLTF");
}

static const NewFunc NewFuncs[(uint)ToolkitUtil::FileType::Other + 1] =
{
    nullptr,  
    nullptr,  
    nullptr,  
    ParticleNew,
    NewMaterial,
    nullptr,  
    nullptr,  
    nullptr,  
    nullptr,  
    nullptr,  
    nullptr,  
    nullptr
};



//------------------------------------------------------------------------------
/**
*/
AssetBrowser::AssetBrowser()
{
    this->fileDB.SetDatabaseURI(IO::URI("int:/filedb.sqlite"));
    const bool dbOpened = this->fileDB.Open(this->logger, false);
    n_assert(dbOpened);

    uint64_t rootFolderId = fileDB.CreateRootFolder("Root", IO::FSWrapper::GetFileWriteTime(IO::Path().WorkURI("assets").LocalPath()), false);
    ToolkitUtil::FileDB::FolderInfo rootFolder;
    this->fileDB.GetFolderInfo(rootFolderId, rootFolder);
    this->folderInfoCache.Add(rootFolderId, rootFolder);
    this->folderInfoDict.Add(IO::Path(), rootFolderId);

    Util::BitField<8> watchFlags;
    watchFlags.SetBit(IO::WatchFlags::NameChanged);
    watchFlags.SetBit(IO::WatchFlags::SizeChanged);
    watchFlags.SetBit(IO::WatchFlags::Creation);

    // the asset browser set up first owns the folder scan and the watch of the assets folder, the file watcher keeps one watch per
    // folder and asserts on unwatching it twice
    this->ownsScanJob = AssetBrowserWindow == nullptr || AssetBrowserWindow == this;
    if (this->ownsScanJob)
    {
        IO::WatchDelegate callback = [this](IO::WatchEvent const& event)
        {
            this->pendingWatchEvents.Enqueue(event);
        };
        IO::FileWatcher::Instance()->Watch(IO::Path().WorkURI("assets").LocalPath(), true, watchFlags, callback);
    }
    
    this->activeFileTree = rootFolderId;
    this->activeFile = 0;
    this->activeFolder = 0;
    this->RefreshFileInfoCaches();

    this->fileTreeReady = true;
    this->showProgress = true;
    if (!this->ownsScanJob)
    {
        AssetBrowser* owner = (AssetBrowser*)AssetBrowserWindow;
        if (owner->currentScanJob != nullptr)
        {
            owner->currentScanJob->secondBrowser = this;
        }
    }
    else
    {
        this->currentScanJob = ScanFolderJob::Create();
        this->currentScanJob->browser = this;
    }
}

//------------------------------------------------------------------------------
/**
*/
AssetBrowser::~AssetBrowser()
{
    // IsWatched can't be asked here, the watch is added on the file watcher thread and may not be there yet
    if (this->ownsScanJob)
        IO::FileWatcher::Instance()->Unwatch(IO::Path().WorkURI("assets").LocalPath());

    if (this->ownsScanJob && this->currentScanJob != nullptr)
    {
        this->pendingFolderRefreshes.Enqueue(-1);
        this->currentScanJob->Stop();
        this->currentScanJob = nullptr;
    }
    else if (AssetBrowserWindow != nullptr && AssetBrowserWindow != this)
    {
        AssetBrowser* owner = (AssetBrowser*)AssetBrowserWindow;
        if (owner->currentScanJob != nullptr)
        {
            owner->currentScanJob->secondBrowser = nullptr;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::Update()
{
    if (this->ownsScanJob && this->currentScanJob != nullptr && !this->currentScanJob->IsRunning())
    {
        if (AssetBrowserPickerWindow != nullptr && AssetBrowserPickerWindow != this)
        {
            this->currentScanJob->secondBrowser = (AssetBrowser*)AssetBrowserPickerWindow;
        }
        this->currentScanJob->Start();
    }

    if (this->isDoneRefreshingCaches.Test())
    {
        this->isDoneRefreshingCaches.Clear();
        this->backgroundScanFinished = true;
    }

    Util::Array<FolderScanResult> scanResults;
    scanResults.Reserve(this->pendingScanResults.Size());
    this->pendingScanResults.DequeueAll(scanResults);
    for (IndexT resultIndex = 0; resultIndex < scanResults.Size(); resultIndex++)
    {
        const FolderScanResult& result = scanResults[resultIndex];
        if (!this->folderInfoCache.Contains(result.folderId))
        {
            // a parent's result always arrives first and caches the folder, so this one was renamed while the scan was running
            continue;
        }
        Util::Array<uint64_t> childIds;
        childIds.Reserve(result.children.Size());
        for (const auto& child : result.children)
        {
            if (this->folderInfoCache.Contains(child.id))
            {
                this->folderInfoCache[child.id] = child;
            }
            else
            {
                this->folderInfoCache.Add(child.id, child);
            }
            if (!this->folderInfoDict.Contains(child.folderPath))
            {
                this->folderInfoDict.Add(child.folderPath, child.id);
            }
            childIds.Append(child.id);
        }
        if (this->folderChildIds.Contains(result.folderId))
        {
            this->folderChildIds[result.folderId] = childIds;
        }
        else
        {
            this->folderChildIds.Add(result.folderId, childIds);
        }

        this->IndexFolderForSearch(result.folderId, result.files);
        if (!this->scannedFolders.Contains(result.folderId))
        {
            this->scannedFolders.Add(result.folderId, true);
        }
        if (result.folderId == this->activeFolder)
        {
            this->fileInfoCache = result.files;
            this->fileInfoDict.Clear();
            for (const auto& file : this->fileInfoCache)
            {
                this->fileInfoDict.Add(file.filePath, file.id);
            }
        }
    }

    if (this->backgroundScanFinished && this->pendingScanResults.IsEmpty())
    {
        this->showProgress = false;
    }
    {
        if (this->activeFolder != 0)
        {
            Util::Array<IO::WatchEvent> events;
            this->pendingWatchEvents.DequeueAll(events);
            if (!events.IsEmpty() && this->fileDB.IsOpen())
            {
                bool cacheNeedsRefresh = false;
                for (const IO::WatchEvent& event : events)
                {
                    Util::String fileName = event.file;
                    if (fileName.IsEmpty())
                        continue;

                    IO::Path path = IO::Path::File(event.relativePath, fileName, ToolkitUtil::FileTypeURNMapping[DetermineFileType(fileName.GetFileExtension())]);
                    IO::URI fileUri = path.WorkURI("assets");
                    bool fileExistsOnDisk = IO::FileExists(fileUri);

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
                                this->fileDB.AddFile(this->logger, fileName, this->activeFolder, fileSize, DetermineFileType(fileName.GetFileExtension()), modifiedTime);
                            }
                            else
                            {
                                if (this->fileInfoDict.Contains(path))
                                {
                                    uint64_t fileId = this->fileInfoDict[path];
                                    this->fileDB.DeleteFile(this->logger, fileId);
                                }
                            }
                            cacheNeedsRefresh = true;
                        }
                        break;
                        case IO::WatchEventType::Deleted:
                        {
                            if (this->fileInfoDict.Contains(path))
                            {
                                uint64_t fileId = this->fileInfoDict[path];
                                this->fileDB.DeleteFile(this->logger, fileId);
                            }
                            cacheNeedsRefresh = true;
                        }
                        break;
                        case IO::WatchEventType::Modified:
                        {
                            if (this->fileInfoDict.Contains(path))
                            {
                                uint64_t fileId = this->fileInfoDict[path];
                                IO::IOStat ioInfo;
                                if (IO::GetIOInfo(fileUri, ioInfo, false))
                                {
                                    this->fileDB.UpdateFileMetadata(this->logger, fileId, ioInfo.modifiedTime);
                                }
                                break;
                            }
                            cacheNeedsRefresh = true;
                        }
                        break;
                    }
                }
                if (cacheNeedsRefresh)
                {
                    this->RefreshFileInfoCaches();
                    this->IndexFolderForSearch(this->activeFolder, this->fileInfoCache);
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::Run(SaveMode save)
{
    if (this->fileTreeReady && !this->pendingPickPath.IsEmpty())
    {
        IndexT folderIndex = this->folderInfoDict.FindIndex(this->pendingPickPath);
        if (folderIndex == InvalidIndex)
        {
            folderIndex = this->folderInfoDict.FindIndex(IO::Path());
        }
        if (folderIndex != InvalidIndex)
        {
            this->SetActiveFolder(this->folderInfoDict.ValueAtIndex(folderIndex));
            this->pendingPickPath.Clear();
        }
    }

    if (this->fileTreeReady && this->fileDB.IsOpen())
    {
        DisplayFileTree();
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
AssetBrowser::PickFile(const IO::Path& path, std::function<void(const IO::Path& path)> picker, const Util::String& type)
{
    this->open = true;
    this->popupThisFrame = true;
    this->pickFileFunction = picker;
    this->pendingPickPath = path;
    this->pickType = type;
    this->searchFilter[0] = '\0';
    this->focusSearch = true;
}

//------------------------------------------------------------------------------
/**
*/
void 
AssetBrowser::PickFolder(const IO::Path& path, std::function<void(const IO::Path& path)> picker)
{
    this->open = true;
    this->popupThisFrame = true;
    this->pickFolderFunction = picker;
    this->pendingPickPath = path;
    this->pickType.Clear();
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetBrowser::GetSelectedFolder(IO::Path& outFolder) const
{
    if (this->activeFolder == 0)
        return false;
    IndexT i = folderInfoCache.FindIndex(this->activeFolder);
    if (i == InvalidIndex)
        return false;
    outFolder = folderInfoCache.ValueAtIndex(i).folderPath;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::RefreshFileInfoCaches()
{
    this->fileInfoCache.Clear();
    this->fileInfoDict.Clear();

    if (this->activeFileTree != 0 && this->activeFolder != 0)
    {
        Util::Array<ToolkitUtil::FileDB::FileInfo> files;
        this->fileDB.GetFilesInFolder(this->activeFolder, files);
        this->fileInfoCache.Reserve(files.Size());
        for (const auto& file : files)
        {
            this->fileInfoCache.Append(file);
            this->fileInfoDict.Add(file.filePath, file.id);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::IndexFolderForSearch(uint64_t folderId, const Util::Array<ToolkitUtil::FileDB::FileInfo>& files)
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
    this->searchIndexDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::SetActiveFolder(uint64_t folderId)
{
    if (this->activeFolder != folderId)
    {
        if (this->activeFolder != 0)
        {
            ToolkitUtil::FileDB::FolderInfo oldInfo = this->folderInfoCache[this->activeFolder];
            //if (!oldInfo.isArchive)
            //{
            //    IO::Path oldFolder = oldInfo.folderPath;
            //    if (IO::FileWatcher::Instance()->IsWatched(oldFolder.GetFolderAndFile()))
            //    {
            //        IO::FileWatcher::Instance()->Unwatch(oldFolder.GetFolderAndFile());
            //    }
            //}
        }
        ToolkitUtil::FileDB::FolderInfo info = this->folderInfoCache[folderId];
        IO::URI folderPath = info.folderPath.WorkURI("assets");
        if (!this->scannedFolders.Contains(folderId))
        {
            if (this->currentScanJob != nullptr)
            {
                Util::Array<Util::String> files = IO::ListFiles(folderPath, "*", false);
                this->fileInfoCache.Clear();
                this->fileInfoDict.Clear();
                this->fileInfoCache.Reserve(files.Size());
                for (const auto& fileName : files)
                {
                    ToolkitUtil::FileDB::FileInfo file;
                    file.id = files.End() - &fileName; // Use memory address to calculate file id
                    file.name = fileName;
                    file.name.StripFileExtension();
                    file.type = DetermineFileType(fileName.GetFileExtension());
                    file.folderId = folderId;
                    file.filePath = IO::Path::File(info.folderPath.GetFolder(), file.name, ToolkitUtil::FileTypeURNMapping[file.type]);
                    IO::IOStat stat;
                    IO::FSWrapper::GetIOInfo(file.filePath.WorkURI("assets"), stat);
                    file.size = stat.size;
                    file.modifiedDate = stat.modifiedTime;
                    this->fileInfoCache.Append(file);
                }
            }
            else
            {
                this->ScanFolder(this->fileDB, folderPath, info.isArchive, folderId, false);
                Util::Array<ToolkitUtil::FileDB::FolderInfo> children;
                this->fileDB.GetChildFolders(folderId, children);
                Util::Array<uint64_t> childIds;
                childIds.Reserve(children.Size());
                for (const auto& child : children)
                {
                    if (this->folderInfoCache.Contains(child.id))
                    {
                        this->folderInfoCache[child.id] = child;
                    }
                    else
                    {
                        this->folderInfoCache.Add(child.id, child);
                    }
                    if (!this->folderInfoDict.Contains(child.folderPath))
                    {
                        this->folderInfoDict.Add(child.folderPath, child.id);
                    }
                    childIds.Append(child.id);
                }
                if (this->folderChildIds.Contains(folderId))
                {
                    this->folderChildIds[folderId] = childIds;
                }
                else
                {
                    this->folderChildIds.Add(folderId, childIds);
                }
                Util::Array<ToolkitUtil::FileDB::FileInfo> files;
                this->fileDB.GetFilesInFolder(folderId, files);
                this->IndexFolderForSearch(folderId, files);
                this->scannedFolders.Add(folderId, true);
            }
        }
        //if (!info.isArchive)
        //{
        //    Util::BitField<8> watchFlags;
        //    watchFlags.SetBit(IO::WatchFlags::NameChanged);
        //    watchFlags.SetBit(IO::WatchFlags::SizeChanged);
        //    watchFlags.SetBit(IO::WatchFlags::Creation);
        //    IO::WatchDelegate callback = [this](IO::WatchEvent const& event)
        //    {
        //        this->pendingWatchEvents.Enqueue(event);
        //    };
        //
        //    IO::FileWatcher::Instance()->Watch(folderPath.LocalPath(), true, watchFlags, callback);
        //}
        this->activeFolder = folderId;
        this->activeFile = 0;
        if (this->scannedFolders.Contains(folderId))
        {
            this->RefreshFileInfoCaches();
        }
    }
}

static const char* ContextMenuName = "file_context_menu";
static bool ShowContextMenu = false;

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::DisplayFileTreeFolderHierarchy(uint64_t folderId, int depth)
{
    ToolkitUtil::FileDB::FolderInfo info = this->folderInfoCache[folderId];

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_SpanFullWidth;
    if (depth == 0)
    {
        flags |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen;
    }
    const bool renaming = this->folderEdit.mode == FolderEditMode::Rename && this->folderEdit.folderId == folderId;
    const bool addingChild = this->folderEdit.mode == FolderEditMode::NewFolder && this->folderEdit.folderId == folderId;
    // the name editor has no highlight behind it, like the one of a new folder
    if (info.id == this->activeFolder && !renaming)
    {
        flags |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;
    }
    if (renaming)
    {
        // the name editor is drawn on top of the node
        flags |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_AllowOverlap;
    }
    if (addingChild && this->folderEdit.openParent)
    {
        ImGui::SetNextItemOpen(true);
        this->folderEdit.openParent = false;
    }

    const float nodeX = ImGui::GetCursorPosX();
    ImGui::PushID(reinterpret_cast<void*>(static_cast<uintptr_t>(folderId)));
    bool bIsOpen = ImGui::TreeNodeEx("##folder", flags, "%s", renaming ? "" : info.name.AsCharPtr());
    ImGui::PopID();
    if (ImGui::IsItemClicked())
    {
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && this->pickFolderFunction)
        {
            this->pickFolderFunction(info.folderPath);
            this->pickFolderFunction = nullptr;
            this->open = false;
            this->popupThisFrame = false;
        }

        else
        {
            this->SetActiveFolder(folderId);
        }
    }
    else if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        ShowContextMenu = true;
        this->contextFolder = folderId;
    }

    if (renaming)
    {
        // the text starts where the folder name does, the field's frame padding is left of it
        ImGui::SameLine(nodeX + ImGui::GetTreeNodeToLabelSpacing() - ImGui::GetStyle().FramePadding.x);
        this->DisplayFolderNameEditor();
    }

    if (bIsOpen)
    {
        if (!this->scannedFolders.Contains(folderId) && this->currentScanJob == nullptr)
        {
            this->ScanFolder(this->fileDB, info.folderPath.WorkURI("assets"), info.isArchive, folderId, false);
            Util::Array<ToolkitUtil::FileDB::FolderInfo> children;
            this->fileDB.GetChildFolders(folderId, children);
            Util::Array<uint64_t> childIds;
            childIds.Reserve(children.Size());
            for (const auto& child : children)
            {
                if (this->folderInfoCache.Contains(child.id))
                {
                    this->folderInfoCache[child.id] = child;
                }
                else
                {
                    this->folderInfoCache.Add(child.id, child);
                }
                if (!this->folderInfoDict.Contains(child.folderPath))
                {
                    this->folderInfoDict.Add(child.folderPath, child.id);
                }
                childIds.Append(child.id);
            }
            if (this->folderChildIds.Contains(folderId))
            {
                this->folderChildIds[folderId] = childIds;
            }
            else
            {
                this->folderChildIds.Add(folderId, childIds);
            }
            Util::Array<ToolkitUtil::FileDB::FileInfo> files;
            this->fileDB.GetFilesInFolder(folderId, files);
            this->IndexFolderForSearch(folderId, files);
            this->scannedFolders.Add(folderId, true);
        }

        if (this->folderChildIds.Contains(folderId))
        {
            const Util::Array<uint64_t>& childIds = this->folderChildIds[folderId];
            Util::Array<ToolkitUtil::FileDB::FolderInfo> children;
            children.Reserve(childIds.Size());
            for (IndexT i = 0; i < childIds.Size(); i++)
            {
                children.Append(this->folderInfoCache[childIds[i]]);
            }
            std::sort(children.begin(), children.end(), [](const ToolkitUtil::FileDB::FolderInfo& a, const ToolkitUtil::FileDB::FolderInfo& b)
            {
                return a.name < b.name;
            });
            for (const auto& child : children)
            {
                this->DisplayFileTreeFolderHierarchy(child.id, depth + 1);
            }
        }
        if (addingChild)
        {
            // the name of the folder that gets created once it's confirmed, the text starts where the names of its
            // siblings start, past the space for the arrow, the field's frame padding is left of it
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetTreeNodeToLabelSpacing() - ImGui::GetStyle().FramePadding.x);
            this->DisplayFolderNameEditor();
        }
        ImGui::TreePop();
    }
}

//------------------------------------------------------------------------------
/**
    Draws a card of the grid view. The name is left out when the name editor is drawn in its place
*/
static void
DrawFileCard(ImDrawList* draw, ImVec2 pos, float itemSize, const char* typeName, const char* name, bool highlighted)
{
    const float s = itemSize / 150.0f;

    const float padding = 8.0f * s;
    const float headerHeight = 24.0f * s;
    const float nameHeight = 22.0f * s;
    const float rounding = 6.0f * s;
    const float headerFontSize = 13.0f * s;
    const float nameFontSize = 13.0f * s;

    ImVec2 max = ImVec2(pos.x + itemSize, pos.y + itemSize);

    // Card background
    draw->AddRectFilled(
        pos,
        max,
        IM_COL32(40, 40, 40, 255),
        6.0f
    );

    // Header
    draw->AddRectFilled(
        pos,
        ImVec2(max.x, pos.y + headerHeight),
        IM_COL32(55, 55, 55, 255),
        6.0f,
        ImDrawFlags_RoundCornersTop
    );

    // Header text
    draw->AddText(
        ImGui::GetFont(),
        headerFontSize,
        ImVec2(
            pos.x + padding,
            pos.y + (headerHeight - headerFontSize) * 0.5f
        ),
        IM_COL32(210, 210, 210, 255),
        typeName
    );

    // Thumbnail area
    ImVec2 imageMin(
        pos.x + padding,
        pos.y + headerHeight + padding
    );

    ImVec2 imageMax(
        max.x - padding,
        max.y - nameHeight - padding
    );

    // Draw a square where the image will be
    draw->AddRectFilled(
        imageMin,
        imageMax,
        IM_COL32(55, 55, 55, 255),
        6.0f,
        ImDrawFlags_RoundCornersAll
    );

    /*
    // Icon / thumbnail
    draw->AddImage(
        iconTexture,
        ImVec2(min.x + 10, min.y + 34),
        ImVec2(max.x - 10, max.y - 32)
    );
    */

    // Name
    if (name != nullptr)
    {
        draw->AddText(
            ImGui::GetFont(),
            nameFontSize,
            ImVec2(
                pos.x + padding,
                max.y - nameHeight
            ),
            IM_COL32(230, 230, 230, 255),
            name
        );
    }

    if (highlighted)
    {
        draw->AddRect(
            pos,
            max,
            IM_COL32(100, 160, 255, 255),
            rounding,
            2.0f * s,
            ImDrawFlags_None
        );
    }
}

//------------------------------------------------------------------------------
/**
    Draws the background of a selected entry for the list view row about to be drawn
*/
static void
HighlightListRow()
{
    const ImVec2 rowMin = ImGui::GetCursorScreenPos();
    const ImVec2 rowMax(rowMin.x + ImGui::GetContentRegionAvail().x, rowMin.y + ImGui::GetFrameHeight());
    ImGui::GetWindowDrawList()->AddRectFilled(rowMin, rowMax, ImGui::GetColorU32(ImGuiCol_Header));
}

//------------------------------------------------------------------------------
/**
    Icon shown in front of a file's name in the list and details views
*/
static const char*
FileTypeIcon(ToolkitUtil::FileType type)
{
    switch (type)
    {
        case ToolkitUtil::FileType::Asset:      return ICON_ttf_CUBE;
        case ToolkitUtil::FileType::Texture:    return ICON_ttf_PICTURE;
        case ToolkitUtil::FileType::Particle:   return ICON_ttf_FIRE;
        case ToolkitUtil::FileType::Surface:    return ICON_ttf_PAINT_BRUSH;
        case ToolkitUtil::FileType::Audio:      return ICON_ttf_VOLUME_UP;
        case ToolkitUtil::FileType::Text:       return ICON_ttf_FILE_TEXT_O;
        case ToolkitUtil::FileType::Frame:      return ICON_ttf_SITEMAP;
        case ToolkitUtil::FileType::Shader:     return ICON_ttf_CODE;
        case ToolkitUtil::FileType::Physics:    return ICON_ttf_FUTBOL_O;
        case ToolkitUtil::FileType::NavMesh:    return ICON_ttf_MAP_O;
        default:                                return ICON_ttf_FILE_O;
    }
}

//------------------------------------------------------------------------------
/**
    Draws the type icon centered in a slot of fixed width, so the names after it line up whatever the icon's width.
    Leaves the cursor after the slot on the same line. The type name shows as tooltip over the icon if the row is hovered.
*/
static void
DisplayFileTypeIcon(ToolkitUtil::FileType type, bool rowHovered)
{
    const char* icon = FileTypeIcon(type);
    const float slotWidth = ImGui::GetFontSize() * 1.5f;
    const ImVec2 slotPos = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(slotPos.x + (slotWidth - ImGui::CalcTextSize(icon).x) * 0.5f, slotPos.y));
    ImGui::TextUnformatted(icon);
    if (rowHovered && ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()))
    {
        ImGui::SetTooltip("%s", ToolkitUtil::FileTypeNames[type]);
    }
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetCursorScreenPos(ImVec2(slotPos.x + slotWidth, ImGui::GetCursorScreenPos().y));
}

//------------------------------------------------------------------------------
/**
    Scores how well one word of a search matches a text, its characters have to appear in it in order,
    ignoring case. Runs of characters and ones starting a word (after a separator or a lowercase letter
    before an uppercase one) count more, characters skipped count against it. Returns false if it doesn't match
*/
static bool
FuzzyMatchWord(const char* word, SizeT wordLength, const Util::String& text, int& outScore)
{
    outScore = 0;
    IndexT w = 0;
    IndexT lastMatch = InvalidIndex;
    for (IndexT t = 0; t < text.Length() && w < wordLength; t++)
    {
        const char c = text[t];
        if (Util::String::ToLower(c) != Util::String::ToLower(word[w]))
            continue;

        int score = 1;
        const char previous = t > 0 ? text[t - 1] : '\0';
        const bool wordStart = t == 0 || previous == '/' || previous == ':' || previous == '_' || previous == '-' || previous == ' ' || previous == '.'
            || (previous >= 'a' && previous <= 'z' && c >= 'A' && c <= 'Z');
        if (wordStart)
            score += 8;
        if (lastMatch != InvalidIndex && lastMatch == t - 1)
            score += 5;
        else if (lastMatch != InvalidIndex)
            score -= Math::min(t - lastMatch - 1, 5);
        outScore += score;
        lastMatch = t;
        w++;
    }
    return w == wordLength;
}

//------------------------------------------------------------------------------
/**
    Fuzzy matches a search against a file, each word of the search (split by spaces) has to match its name
    or its folder and name. The name counts more, a match in it is what's usually looked for
*/
static bool
FuzzyMatch(const Util::String& search, const Util::String& name, const Util::String& folderAndName, int& outScore)
{
    outScore = 0;
    const Util::Array<Util::String> words = search.Tokenize(" ");
    if (words.IsEmpty())
        return false;
    for (const Util::String& word : words)
    {
        int nameScore = 0, pathScore = 0;
        const bool inName = FuzzyMatchWord(word.AsCharPtr(), word.Length(), name, nameScore);
        const bool inPath = FuzzyMatchWord(word.AsCharPtr(), word.Length(), folderAndName, pathScore);
        if (!inName && !inPath)
            return false;
        outScore += inName ? Math::max(nameScore + 10, inPath ? pathScore : 0) : pathScore;
    }
    // shorter names with the same matches are closer to what was typed
    outScore -= name.Length() / 8;
    return true;
}

//------------------------------------------------------------------------------
/**
    The types offered by the type filter, in the order they're listed. Assets people look for most come first,
    Unknown is left out as no file in the FileDB has it
*/
static const ToolkitUtil::FileType FilterableFileTypes[] =
{
    ToolkitUtil::FileType::Asset,
    ToolkitUtil::FileType::Texture,
    ToolkitUtil::FileType::Surface,
    ToolkitUtil::FileType::Particle,
    ToolkitUtil::FileType::Audio,
    ToolkitUtil::FileType::Physics,
    ToolkitUtil::FileType::NavMesh,
    ToolkitUtil::FileType::Shader,
    ToolkitUtil::FileType::Frame,
    ToolkitUtil::FileType::Text,
    ToolkitUtil::FileType::Other,
};

//------------------------------------------------------------------------------
/**
*/
static uint32_t
FileTypeFilterBit(ToolkitUtil::FileType type)
{
    return 1u << (uint32_t)type;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetBrowser::IsTypeFilterActive() const
{
    // a picker asking for one type shows only that one, the filter can't widen or narrow it
    const bool pickerRestricts = this->pickFileFunction && !this->pickType.IsEmpty();
    return this->typeFilter != 0 && !pickerRestricts;
}

//------------------------------------------------------------------------------
/**
    A button showing the types shown (their icons, or "All types"), opening a popup with a checkbox per type.
    The popup stays open while types are toggled, so several can be picked in one go. Right of it a button
    clears the filter in one click, it's only there while a filter is set
*/
void
AssetBrowser::DisplayTypeFilter(float width)
{
    const bool pickerRestricts = this->pickFileFunction && !this->pickType.IsEmpty();
    const bool active = this->IsTypeFilterActive();
    const float clearWidth = ImGui::GetFrameHeight();
    const float buttonWidth = active ? Math::max(width - clearWidth - ImGui::GetStyle().ItemSpacing.x, clearWidth) : width;

    Util::String label;
    if (pickerRestricts)
    {
        label = "Picker type";
    }
    else if (!active)
    {
        label = "All types";
    }
    else
    {
        // one type reads best with its name, several only fit as icons
        uint32_t count = 0;
        ToolkitUtil::FileType single = ToolkitUtil::FileType::Unknown;
        for (ToolkitUtil::FileType type : FilterableFileTypes)
        {
            if (this->typeFilter & FileTypeFilterBit(type))
            {
                if (count > 0)
                {
                    label.Append(" ");
                }
                label.Append(FileTypeIcon(type));
                single = type;
                count++;
            }
        }
        if (count == 1)
        {
            label = Util::Format("%s %s", FileTypeIcon(single), ToolkitUtil::FileTypeNames[single]);
        }
    }
    label.Append("##typefilter");

    ImGui::BeginDisabled(pickerRestricts);
    if (active)
    {
        // tinted so it's clear at a glance that files are hidden
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }
    if (ImGui::Button(label.AsCharPtr(), ImVec2(buttonWidth, 0.0f)))
    {
        ImGui::OpenPopup("TypeFilterPopup");
    }
    if (active)
    {
        ImGui::PopStyleColor();
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        if (pickerRestricts)
        {
            ImGui::SetTooltip("Only %s files can be picked here", this->pickType.AsCharPtr());
        }
        else if (active)
        {
            ImGui::SetTooltip("Only some asset types are shown, click to change them");
        }
        else
        {
            ImGui::SetTooltip("Show only some asset types");
        }
    }

    if (active)
    {
        ImGui::SameLine();
        const Util::String clearLabel = Util::Format("%s##cleartypefilter", ICON_ttf_REMOVE);
        if (ImGui::Button(clearLabel.AsCharPtr(), ImVec2(clearWidth, 0.0f)))
        {
            this->typeFilter = 0;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Show all types");
        }
    }

    if (ImGui::BeginPopup("TypeFilterPopup"))
    {
        if (ImGui::Selectable("All types", this->typeFilter == 0, ImGuiSelectableFlags_NoAutoClosePopups))
        {
            this->typeFilter = 0;
        }
        ImGui::Separator();
        for (ToolkitUtil::FileType type : FilterableFileTypes)
        {
            bool shown = (this->typeFilter & FileTypeFilterBit(type)) != 0;
            const Util::String itemLabel = Util::Format("%s  %s", FileTypeIcon(type), ToolkitUtil::FileTypeNames[type]);
            if (ImGui::Checkbox(itemLabel.AsCharPtr(), &shown))
            {
                if (ImGui::GetIO().KeyCtrl)
                {
                    // ctrl-click shows just this type, quicker than unchecking the others
                    this->typeFilter = FileTypeFilterBit(type);
                }
                else if (shown)
                {
                    this->typeFilter |= FileTypeFilterBit(type);
                }
                else
                {
                    this->typeFilter &= ~FileTypeFilterBit(type);
                }
            }
        }
        ImGui::Separator();
        ImGui::TextDisabled("Ctrl-click to show only one type");
        ImGui::EndPopup();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::DisplaySelectedFolder(const Util::String& filter)
{
    static const auto FileEntryTypeToAssetType = [](ToolkitUtil::FileType type) -> AssetEditor::AssetType
    {
        switch (type)
        {
            case ToolkitUtil::FileType::Asset:
                return AssetEditor::AssetType::Model;
            case ToolkitUtil::FileType::Texture:
                return AssetEditor::AssetType::Texture;
            case ToolkitUtil::FileType::Surface:
                return AssetEditor::AssetType::Material;
            case ToolkitUtil::FileType::Particle:
                return AssetEditor::AssetType::Particle;
            case ToolkitUtil::FileType::Audio:
            case ToolkitUtil::FileType::Text:
            case ToolkitUtil::FileType::Frame:
            case ToolkitUtil::FileType::Shader:
            case ToolkitUtil::FileType::Physics:
            case ToolkitUtil::FileType::NavMesh:
            default:
                return AssetEditor::AssetType::None;
        }
    };

    static const auto AddDragSourceForFileUri = [this](const IO::Path& file)
    {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            ToolkitUtil::FileDB::FolderInfo folder = this->folderInfoCache[this->activeFileTree];
            const Util::String filePath = file.AsString();
            ImGui::SetDragDropPayload("resource", filePath.AsCharPtr(), sizeof(char) * filePath.Length() + 1);
            ImGui::Text(filePath.AsCharPtr());
            ImGui::EndDragDropSource();
        }
    };

    static const ImGuiTableSortSpecs* s_sort_specs = nullptr;

    const bool searching = !filter.IsEmpty();
    if (searching && (this->searchIndexDirty || this->lastSearchFilter != filter))
    {
        this->searchResults.Clear();
        if (filter.FindCharIndex('*') != InvalidIndex || filter.FindCharIndex('?') != InvalidIndex)
        {
            // a wildcard pattern matches as it's written
            for (const auto& file : this->searchIndex)
            {
                if (Util::String::MatchPattern(file.name, filter) || Util::String::MatchPattern(file.filePath.GetFolderAndFile(), filter))
                {
                    this->searchResults.Append(file);
                }
            }
        }
        else
        {
            // otherwise fuzzy, best matches first
            Util::Array<Util::KeyValuePair<int, IndexT>> ranked;
            for (IndexT i = 0; i < this->searchIndex.Size(); i++)
            {
                const auto& file = this->searchIndex[i];
                int score = 0;
                if (FuzzyMatch(filter, file.name, file.filePath.GetFolderAndFile(), score))
                {
                    ranked.Append({ score, i });
                }
            }
            std::sort(ranked.begin(), ranked.end(), [this](const Util::KeyValuePair<int, IndexT>& a, const Util::KeyValuePair<int, IndexT>& b)
            {
                if (a.Key() != b.Key())
                    return a.Key() > b.Key();
                return this->searchIndex[a.Value()].name < this->searchIndex[b.Value()].name;
            });
            this->searchResults.Reserve(ranked.Size());
            for (const auto& entry : ranked)
            {
                this->searchResults.Append(this->searchIndex[entry.Value()]);
            }
        }
        this->lastSearchFilter = filter;
        this->searchIndexDirty = false;
    }

    if (!searching && this->activeFolder == 0)
    {
        return;
    }

    // names are edited in the folder they belong to, search results don't show the editor
    const bool editingInFolder = !searching && this->folderEdit.folderId == this->activeFolder;
    const bool newAssetShown = editingInFolder && this->folderEdit.mode == FolderEditMode::NewAsset;
    const uint64_t renamedFile = editingInFolder && this->folderEdit.mode == FolderEditMode::RenameFile ? this->folderEdit.fileId : 0;

    if (this->activeFolder != 0 || searching)
    {
        Util::Array<uint64_t> refreshedFolderIds;
        this->refreshedFolders.DequeueAll(refreshedFolderIds);
        if (this->activeFolder != 0 && refreshedFolderIds.FindIndex(this->activeFolder) != InvalidIndex)
        {
            this->RefreshFileInfoCaches();
        }
        bool hasFileToOpen = false;
        ToolkitUtil::FileDB::FileInfo fileToOpen;       

        Util::Array<ToolkitUtil::FileDB::FileInfo> visibleFiles;
        if (searching)
        {
            visibleFiles = this->searchResults;
        }
        else
        {
            visibleFiles.Reserve(this->fileInfoCache.Size());
            for (const auto& file : this->fileInfoCache)
            {
                visibleFiles.Append(file);
            }
        }
        if (this->pickFileFunction && !this->pickType.IsEmpty())
        {
            // picking an asset of a type, the others can't be picked
            IndexT write = 0;
            for (IndexT read = 0; read < visibleFiles.Size(); read++)
            {
                if (visibleFiles[read].filePath.GetType() == this->pickType)
                {
                    if (write != read)
                        visibleFiles[write] = visibleFiles[read];
                    write++;
                }
            }
            visibleFiles.Resize(write);
        }
        SizeT hiddenByTypeFilter = 0;
        if (this->IsTypeFilterActive())
        {
            // applied to the folder listing and the search results alike, so it narrows a search too
            IndexT write = 0;
            for (IndexT read = 0; read < visibleFiles.Size(); read++)
            {
                if (this->typeFilter & FileTypeFilterBit(visibleFiles[read].type))
                {
                    if (write != read)
                    {
                        visibleFiles[write] = visibleFiles[read];
                    }
                    write++;
                }
            }
            hiddenByTypeFilter = visibleFiles.Size() - write;
            visibleFiles.Resize(write);
        }
        if (visibleFiles.IsEmpty() && hiddenByTypeFilter > 0)
        {
            // otherwise an empty view looks like an empty folder
            ImGui::TextDisabled("%d file(s) hidden by the type filter", hiddenByTypeFilter);
        }


        switch(this->fileViewMode)
        {
            case FileViewMode::List:
            {
                for (const auto& file : visibleFiles)
                {
                    if (file.id == renamedFile)
                    {
                        HighlightListRow();
                        this->DisplayFileNameEditorEntry(file.type, 0.0f);
                        continue;
                    }
                    bool isSelected = (this->activeFile == file.id);
                    ImGui::PushID(reinterpret_cast<void*>(static_cast<uintptr_t>(file.id)));
                    // no label, icon and name are drawn after it as the selectable takes no width
                    if (ImGui::Selectable("##file", &isSelected))
                    {
                        this->activeFile = file.id;
                    }
                    const bool rowHovered = ImGui::IsItemHovered();
                    // the context menu acts on the selected file, so right clicking selects it
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                    {
                        this->activeFile = file.id;
                    }
                    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        hasFileToOpen = true;
                        fileToOpen = file;
                    }
                    AddDragSourceForFileUri(file.filePath);
                    ImGui::SameLine(0.0f, 0.0f);
                    DisplayFileTypeIcon(file.type, rowHovered);
                    ImGui::TextUnformatted(file.name.AsCharPtr());
                    ImGui::PopID();
                }
                if (newAssetShown)
                {
                    HighlightListRow();
                    this->DisplayFileNameEditorEntry(this->folderEdit.assetType, 0.0f);
                }
                break;
            }
            case FileViewMode::Details:
            {
                ImGui::BeginGroup();
                ImGui::BeginTable("##filedetails", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings| ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_DefaultSort);
                ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Modified", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();

                // search results stay ranked by how well they match
                if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs())
                if (sorts_specs->SpecsDirty && visibleFiles.Size() > 1 && !searching)
                {
                    s_sort_specs = sorts_specs;
                    const bool descending = s_sort_specs->Specs[0].SortDirection == ImGuiSortDirection_Descending;
                    std::sort(visibleFiles.begin(), visibleFiles.end(), [descending](const ToolkitUtil::FileDB::FileInfo& a, const ToolkitUtil::FileDB::FileInfo& b)
                    {
                        if (s_sort_specs->SpecsCount != 1)
                        {
                            return descending ? (b.name < a.name) : (a.name < b.name);
                        }
                        switch (s_sort_specs->Specs[0].ColumnIndex)
                        {
                            case 0:
                                return descending ? (b.name < a.name) : (a.name < b.name);
                            case 1:
                                return descending ? (b.size < a.size) : (a.size < b.size);
                            case 2:
                                return descending ? (b.modifiedDate < a.modifiedDate) : (a.modifiedDate < b.modifiedDate);
                            default:
                                return false;
                        }
                    });
                    // fixme as we currently read the files from the database everytime we sort them everytime. needs a cache of sorts maybe. seems fast enough for now though.
                    //sorts_specs->SpecsDirty = false;
                }
                
                for (const auto& file : visibleFiles)
                {
                    bool isSelected = (this->activeFile == file.id);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    if (file.id == renamedFile)
                    {
                        // drawn like a selected row
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImGuiCol_Header));
                        this->DisplayFileNameEditorEntry(file.type, 0.0f);
                    }
                    else
                    {
                        ImGui::PushID(file.name.HashCode() + (uint)file.type);
                        // no label, icon and name are drawn after it as the selectable takes no width
                        if (ImGui::Selectable("##file", &isSelected))
                        {
                            this->activeFile = file.id;
                        }
                        const bool rowHovered = ImGui::IsItemHovered();
                        // the context menu acts on the selected file, so right clicking selects it
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                        {
                            this->activeFile = file.id;
                        }
                        if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                        {
                            hasFileToOpen = true;
                            fileToOpen = file;
                        }
                        ImGui::PopID();
                        AddDragSourceForFileUri(file.filePath);
                        ImGui::SameLine(0.0f, 0.0f);
                        DisplayFileTypeIcon(file.type, rowHovered);
                        ImGui::TextUnformatted(file.name.AsCharPtr());
                    }
                    ImGui::TableNextColumn();
                    if (file.size > 1_GB)
                        ImGui::Text("%d GB", (int)(file.size / 1_GB));
                    else if (file.size > 1_MB)
                        ImGui::Text("%d MB", (int)(file.size / 1_MB));
                    else if (file.size > 1_KB)
                        ImGui::Text("%d KB", (int)(file.size / 1_KB));
                    else
                        ImGui::Text("%d B", (int)(file.size));

                    ImGui::TableNextColumn();  
                    Timing::CalendarTime cal = Timing::CalendarTime::FileTimeToSystemTime(file.modifiedDate);
                    ImGui::Text(Base::CalendarTimeBase::Format("{YEAR}/{MONTH}/{DAY} {HOUR}:{MINUTE}", cal).AsCharPtr());
                }
                if (newAssetShown)
                {
                    // drawn like a selected row
                    ImGui::TableNextRow();
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImGuiCol_Header));
                    ImGui::TableNextColumn();
                    this->DisplayFileNameEditorEntry(this->folderEdit.assetType, 0.0f);
                }
                ImGui::EndTable();
                ImGui::EndGroup();
                break;
            }    
            case FileViewMode::Icons:
            {        
                ImGuiStyle& style = ImGui::GetStyle();
                int numFiles = visibleFiles.Size() + (newAssetShown ? 1 : 0);
                float windowVisibleX = ImGui::GetWindowPos().x + ImGui::GetContentRegionAvail().x;
                static int itemSize = 150;
                ImGui::SliderInt("Zoom", &itemSize, 25, 200);
                int n = 0;
                const auto sameLineIfFits = [&]()
                {
                    float lastButtonX = ImGui::GetItemRectMax().x;
                    float nextButtonX = lastButtonX + style.ItemSpacing.x + (float)itemSize + 30.0f; // Expected position if next button was on same line
                    if (n + 1 < numFiles && nextButtonX < windowVisibleX)
                        ImGui::SameLine();
                    n++;
                };
                for (const auto& file : visibleFiles)
                {
                    if (file.id == renamedFile)
                    {
                        this->DisplayFileNameEditorEntry(file.type, (float)itemSize);
                        sameLineIfFits();
                        continue;
                    }
                    Util::String const& name = file.name;
                    ImGui::BeginGroup();
                    ImGui::PushID(file.id);

                    ImVec2 pos = ImGui::GetCursorScreenPos();

                    //ImGui::ImageButton(name.AsCharPtr(), &Editor::UI::Icons::game, { (float)itemSize, (float)itemSize });
                    if(ImGui::InvisibleButton(name.AsCharPtr(), { (float)itemSize, (float)itemSize }))
                    {
                        this->activeFile = file.id;
                    }
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                    {
                        this->activeFile = file.id;
                    }

                    bool hovered = ImGui::IsItemHovered();
                    bool clicked = ImGui::IsItemClicked();
                    bool doubleClicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
                    if (clicked && doubleClicked)
                    {
                        hasFileToOpen = true;
                        fileToOpen = file;
                    }

                    AddDragSourceForFileUri(file.filePath);

                    // Now draw the widget yourself.
                    DrawFileCard(ImGui::GetWindowDrawList(), pos, (float)itemSize, ToolkitUtil::FileTypeNames[file.type], name.AsCharPtr(), hovered);
                    ImGui::PopID();


                    if (hovered)
                    {
                        if (ImGui::BeginTooltip())
                        {
                            ImGui::Text(name.AsCharPtr());
                            ImGui::EndTooltip();
                        }
                    }
                    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        this->activeFile = file.id;
                        hasFileToOpen = true;
                        fileToOpen = file;
                    }
                    AddDragSourceForFileUri(file.filePath);
                    ImGui::EndGroup();
                    sameLineIfFits();
                }
                if (newAssetShown)
                {
                    this->DisplayFileNameEditorEntry(this->folderEdit.assetType, (float)itemSize);
                }
                break;
            }
            default: 
                break;
        }

        if (hasFileToOpen)
        {
            IO::Path path = fileToOpen.filePath;
            if (this->pickFileFunction)
            {
                this->pickFileFunction(path);
                this->pickFileFunction = nullptr;
                this->open = false;
                this->popupThisFrame = false;
            }
            else
            {
                AssetEditor* assetEditor = (AssetEditor*)Presentation::AssetEditorWindow;
                IO::Path rootFolderPath = this->folderInfoCache[this->activeFileTree].folderPath;
                assetEditor->Open(path, rootFolderPath.GetFolder(), FileEntryTypeToAssetType(fileToOpen.type));
            }
        }    
    }
}

//------------------------------------------------------------------------------
/**
    Like in the OS file browsers, a new file is shown at the end of the folder and a renamed one in its place,
    with the name editor where its name would be. The grid view passes its card size, the other views 0.
*/
void
AssetBrowser::DisplayFileNameEditorEntry(ToolkitUtil::FileType type, float itemSize)
{
    ImGui::PushID("##editedfile");
    if (itemSize > 0.0f)
    {
        // the name editor takes the place of the card's name
        const float nameHeight = 22.0f * itemSize / 150.0f;
        ImGui::BeginGroup();
        DrawFileCard(ImGui::GetWindowDrawList(), ImGui::GetCursorScreenPos(), itemSize, ToolkitUtil::FileTypeNames[type], nullptr, true);
        ImGui::Dummy(ImVec2(itemSize, itemSize - nameHeight));
        this->DisplayFolderNameEditor(itemSize);
        ImGui::EndGroup();
    }
    else
    {
        ImGui::AlignTextToFramePadding();
        DisplayFileTypeIcon(type, false);
        this->DisplayFolderNameEditor();
    }
    if (this->folderEdit.scrollIntoView)
    {
        if (!ImGui::IsItemVisible())
        {
            ImGui::SetScrollHereY(1.0f);
        }
        this->folderEdit.scrollIntoView = false;
    }
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::BeginNewAsset(uint64_t folderId, ToolkitUtil::FileType type, const Util::String& defaultName)
{
    if (!this->folderInfoCache.Contains(folderId))
    {
        return;
    }
    const ToolkitUtil::FileDB::FolderInfo& info = this->folderInfoCache[folderId];
    if (info.isArchive)
    {
        return;
    }

    // default name, numbered when it's already taken
    const char* ns = ToolkitUtil::FileTypeURNMapping[type];
    Util::String name = defaultName;
    IndexT uniqueCounter = 2;
    while (IO::FileExists(IO::Path::File(info.folderPath.GetFolder(), name, ns).WorkURI("assets")))
    {
        name.Format("%s_%d", defaultName.AsCharPtr(), uniqueCounter++);
    }

    this->folderEdit = FolderEdit();
    this->folderEdit.mode = FolderEditMode::NewAsset;
    this->folderEdit.folderId = folderId;
    this->folderEdit.assetType = type;
    this->folderEdit.requestFocus = true;
    this->folderEdit.scrollIntoView = true;
    this->activeFile = 0;
    name.CopyToBuffer(this->folderEdit.name, sizeof(this->folderEdit.name));
}

//------------------------------------------------------------------------------
/**
    The work file is the one the asset's URN, folder:name:type, resolves to. The asset is batched right away,
    so it can be referenced before it has been edited, and is selected once it's created.
*/
bool
AssetBrowser::CreateAsset(uint64_t folderId, ToolkitUtil::FileType type, const Util::String& name, Util::String& outError)
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

    const IO::Path path = IO::Path::File(folderInfo.folderPath.GetFolder(), name, ToolkitUtil::FileTypeURNMapping[type]);
    const IO::URI uri = path.WorkURI("assets");
    if (IO::FileExists(uri))
    {
        outError.Format("'%s' already exists", path.GetWorkFile().AsCharPtr());
        return false;
    }
    Ptr<IO::Stream> stream = IO::CreateStream(uri);
    stream->SetAccessMode(IO::Stream::AccessMode::WriteAccess);
    if (!stream->Open())
    {
        outError.Format("Could not create '%s'", path.GetWorkFile().AsCharPtr());
        return false;
    }
    if (NewFuncs[(uint)type] != nullptr)
    {
        NewFuncs[(uint)type](stream, uri.LocalPath());
    }
    const IO::Stream::Size size = stream->GetSize();
    stream->Close();

    const uint64_t fileId = this->fileDB.AddFile(this->logger, path.GetWorkFile(), folderId, size, type, IO::FileTime());
    Editor::LiveBatcher::BatchFile(path);

    this->RefreshFileInfoCaches();
    this->IndexFolderForSearch(this->activeFolder, this->fileInfoCache);
    this->activeFile = fileId;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::BeginRenameFile(uint64_t fileId)
{
    if (!this->folderInfoCache.Contains(this->activeFolder) || this->folderInfoCache[this->activeFolder].isArchive)
    {
        return;
    }
    for (const auto& file : this->fileInfoCache)
    {
        if (file.id == fileId)
        {
            // like the OS file browsers, the name is edited without its extension
            Util::String name = file.name;
            name.StripFileExtension();

            this->folderEdit = FolderEdit();
            this->folderEdit.mode = FolderEditMode::RenameFile;
            this->folderEdit.folderId = this->activeFolder;
            this->folderEdit.fileId = fileId;
            this->folderEdit.assetType = file.type;
            this->folderEdit.requestFocus = true;
            this->folderEdit.scrollIntoView = true;
            name.CopyToBuffer(this->folderEdit.name, sizeof(this->folderEdit.name));
            return;
        }
    }
}

//------------------------------------------------------------------------------
/**
    Opens the import window for a model or texture asset with its source files and the settings it was imported
    with. Importing a model generates the scene again, keeping the materials assigned to it.
*/
void
AssetBrowser::ReimportAsset(const IO::Path& asset)
{
    // an open tab holds the whole asset as it was loaded, and saving it would write the old mesh back
    if (AssetEditor::GetOpenItem(asset) != nullptr)
    {
        n_log_warn(Asset Browser, "Close '%s' in the asset editor before reimporting it\n", asset.GetFolderAndFile().AsCharPtr());
        return;
    }

    // the import window imports and batches it, its settings can be changed before
    Util::String error;
    if (!AssetImporterWindow::Reimport(asset, error))
    {
        n_log_warn(Asset Browser, "Could not reimport '%s': %s\n", asset.GetFolderAndFile().AsCharPtr(), error.AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
    Only the work file is renamed. Its export is batched under the new name, and the one under the old name is
    kept for the assets and levels that still refer to it.
*/
bool
AssetBrowser::RenameFile(uint64_t fileId, const Util::String& newName, Util::String& outError)
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
    const uint64_t newFileId = this->fileDB.AddFile(this->logger, newFileName, info.folderId, info.size, info.type, info.modifiedDate);

    const char* ns = ToolkitUtil::FileTypeURNMapping[info.type];
    if (IO::Path::WorkExtensions.Contains(ns))
    {
        Editor::LiveBatcher::BatchFile(IO::Path::File(folderInfo.folderPath.GetFolder(), newName, ns));
    }

    this->RefreshFileInfoCaches();
    this->IndexFolderForSearch(this->activeFolder, this->fileInfoCache);
    this->activeFile = newFileId;
    return true;
}


//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::DisplayFileTree()
{
    // the folder caches are shared between browsers, a rename in the other one can drop the active folder
    if (this->activeFolder != 0 && !this->folderInfoCache.Contains(this->activeFolder))
    {
        this->activeFolder = 0;
        this->activeFile = 0;
        this->RefreshFileInfoCaches();
    }

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
    if (this->showProgress)
    {
        ScanFolderJob* scanJob = this->currentScanJob;
        if (scanJob == nullptr && AssetBrowserWindow != nullptr)
        {
            scanJob = ((AssetBrowser*)AssetBrowserWindow)->currentScanJob;
        }
        if (scanJob != nullptr)
        {
            const float progress = scanJob->GetProgress();
            if (progress < 0.0f)
            {
                // a negative fraction animates an indeterminate bar
                ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(160.0f, 0.0f), "Counting files...");
            }
            else
            {
                ImGui::ProgressBar(progress, ImVec2(160.0f, 0.0f), Util::Format("Indexing... %d%%", (int)(progress * 100.0f)).AsCharPtr());
            }
        }
    }
    if (this->pickFileFunction && !this->pickType.IsEmpty())
        ImGui::Text("Search (only %s files are shown)", this->pickType.AsCharPtr());
    else
        ImGui::Text("Search");
    if (this->focusSearch)
    {
        ImGui::SetKeyboardFocusHere();
        this->focusSearch = false;
    }
    // the type filter sits right of the search, both narrow the same file view
    const float typeFilterWidth = Math::min(ImGui::GetFontSize() * 10.0f, ImGui::GetContentRegionAvail().x * 0.4f);
    ImGui::SetNextItemWidth(Math::max(ImGui::GetContentRegionAvail().x - typeFilterWidth - ImGui::GetStyle().ItemSpacing.x, 1.0f));
    ImGui::InputTextWithHint("##search", "Fuzzy search, or a pattern with * and ?", this->searchFilter, sizeof(this->searchFilter));
    ImGui::SameLine();
    this->DisplayTypeFilter(typeFilterWidth);

     
    ImGui::PopItemWidth();

    ImGui::Separator();
    ImGui::Columns(2);
    ImGui::BeginChild("ScrollingRegionFolders");
    this->folderEdit.drawn = false;
    if (this->activeFileTree != 0)
    {
        this->DisplayFileTreeFolderHierarchy(this->activeFileTree, 0);
    }
    else
    {
        ImGui::Text("No root data yet");
    }
    const bool folderTreeFocused = ImGui::IsWindowFocused();
    ImGui::EndChild();

    ImGui::NextColumn();
    if (ImGui::Button("Details"))
    {
        this->fileViewMode = FileViewMode::Details;
    }
    ImGui::SameLine();
    if (ImGui::Button("List"))
    {
        this->fileViewMode = FileViewMode::List;
    }
    ImGui::SameLine();
    if (ImGui::Button("Grid"))
    {
        this->fileViewMode = FileViewMode::Icons;
    }
    ImGui::BeginChild("ScrollingRegionFiles");
    this->DisplaySelectedFolder(this->searchFilter);
    const bool fileViewFocused = ImGui::IsWindowFocused();
    ImGui::EndChild();

    if (this->folderEdit.commitRequested)
    {
        // committed here and not while drawing, as it changes the folder and file caches the views are iterating
        this->folderEdit.commitRequested = false;
        if (!this->CommitFolderEdit())
        {
            // keep editing so the name can be fixed
            this->folderEdit.requestFocus = true;
            this->folderEdit.wasActive = false;
        }
    }
    else if (this->folderEdit.mode != FolderEditMode::None && !this->folderEdit.drawn)
    {
        // the edited entry went out of view (parent collapsed, folder vanished, other folder shown), drop the edit
        this->CancelFolderEdit();
    }
    if (folderTreeFocused && this->folderEdit.mode == FolderEditMode::None && ImGui::IsKeyPressed(ImGuiKey_F2, false))
    {
        this->BeginRenameFolder(this->activeFolder);
    }
    else if (fileViewFocused && this->folderEdit.mode == FolderEditMode::None && this->activeFile != 0 && this->searchFilter[0] == '\0' && ImGui::IsKeyPressed(ImGuiKey_F2, false))
    {
        this->BeginRenameFile(this->activeFile);
    }

    if (ShowContextMenu)
    {
        ImGui::OpenPopup(ContextMenuName);
        ShowContextMenu = false;
    }

    if (ImGui::BeginPopup(ContextMenuName))
    {
        const bool hasContextFolder = this->folderInfoCache.Contains(this->contextFolder);
        const bool isRootFolder = this->contextFolder == this->activeFileTree;
        bool isArchive = false;
        if (hasContextFolder)
        {
            isArchive = this->folderInfoCache[this->contextFolder].isArchive;
        }
        if (ImGui::MenuItem("New Folder", nullptr, false, hasContextFolder && !isArchive))
        {
            this->BeginNewFolder(this->contextFolder);
        }
        if (ImGui::MenuItem("Rename", "F2", false, hasContextFolder && !isRootFolder && !isArchive))
        {
            this->BeginRenameFolder(this->contextFolder);
        }
        ImGui::EndPopup();
    }

    if (this->pickFolderFunction && this->activeFileTree != 0)
    {
        if (ImGui::Button("Select folder"))
        {
            // pick the folder selected in the tree, the file tree is always the root folder
            uint64_t folderId = this->activeFolder != 0 ? this->activeFolder : this->activeFileTree;
            auto info = this->folderInfoCache[folderId];
            this->pickFolderFunction(info.folderPath);
            this->pickFolderFunction = nullptr;
            this->open = false;
            this->popupThisFrame = false;
        }
    }

    if (this->activeFolder != 0)
    {
        if (ImGui::BeginPopupContextItem("AssetActions"))
        {
            // the asset is created once its name is committed with Enter
            const bool canCreate = this->folderInfoCache.Contains(this->activeFolder) && !this->folderInfoCache[this->activeFolder].isArchive;
            if (ImGui::MenuItem("Create Particle System", nullptr, false, canCreate))
            {
                this->BeginNewAsset(this->activeFolder, ToolkitUtil::FileType::Particle, "new_particle_system");
            }
            if (ImGui::MenuItem("Create Material", nullptr, false, canCreate))
            {
                this->BeginNewAsset(this->activeFolder, ToolkitUtil::FileType::Surface, "new_material");
            }

            // model assets can be imported again from the file they were imported from
            const ToolkitUtil::FileDB::FileInfo* contextFile = nullptr;
            for (const auto& file : this->fileInfoCache)
            {
                if (file.id == this->activeFile)
                {
                    contextFile = &file;
                    break;
                }
            }
            const bool canReimport = canCreate && contextFile != nullptr && (contextFile->type == ToolkitUtil::FileType::Asset || contextFile->type == ToolkitUtil::FileType::Texture);
            ImGui::Separator();
            if (ImGui::MenuItem("Reimport", nullptr, false, canReimport))
            {
                this->ReimportAsset(contextFile->filePath);
            }
            ImGui::EndPopup();
        }
    }
    ImGui::NextColumn();
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::BeginNewFolder(uint64_t parentId)
{
    if (!this->folderInfoCache.Contains(parentId))
    {
        return;
    }
    const IO::Path parentPath = this->folderInfoCache[parentId].folderPath;

    // default name like the OS file browsers, numbered when it's already taken
    Util::String name = "New Folder";
    IndexT uniqueCounter = 2;
    while (IO::DirectoryExists((parentPath / name).WorkURI("assets")) || IO::FileExists((parentPath / name).WorkURI("assets")))
    {
        name.Format("New Folder (%d)", uniqueCounter++);
    }

    this->folderEdit = FolderEdit();
    this->folderEdit.mode = FolderEditMode::NewFolder;
    this->folderEdit.folderId = parentId;
    this->folderEdit.openParent = true;
    this->folderEdit.requestFocus = true;
    name.CopyToBuffer(this->folderEdit.name, sizeof(this->folderEdit.name));
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::BeginRenameFolder(uint64_t folderId)
{
    if (folderId == 0 || folderId == this->activeFileTree || !this->folderInfoCache.Contains(folderId))
    {
        return;
    }
    const ToolkitUtil::FileDB::FolderInfo& info = this->folderInfoCache[folderId];
    if (info.isRoot || info.isArchive)
    {
        return;
    }

    this->folderEdit = FolderEdit();
    this->folderEdit.mode = FolderEditMode::Rename;
    this->folderEdit.folderId = folderId;
    this->folderEdit.requestFocus = true;
    info.name.CopyToBuffer(this->folderEdit.name, sizeof(this->folderEdit.name));
}

//------------------------------------------------------------------------------
/**
    Draws the name field. Enter commits (deferred until the tree is drawn, as committing changes the folder caches),
    Escape or clicking somewhere else cancels.
*/
void
AssetBrowser::DisplayFolderNameEditor(float width)
{
    this->folderEdit.drawn = true;
    if (this->folderEdit.requestFocus)
    {
        ImGui::SetKeyboardFocusHere();
        this->folderEdit.requestFocus = false;
        this->folderEdit.focusRequestFrame = ImGui::GetFrameCount();
    }
    ImGui::SetNextItemWidth(width > 0.0f ? width : ImMax(ImGui::GetContentRegionAvail().x, 100.0f));
    const bool submitted = ImGui::InputText(
        "##foldername",
        this->folderEdit.name,
        sizeof(this->folderEdit.name),
        ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue
    );
    if (ImGui::IsItemActive())
    {
        this->folderEdit.wasActive = true;
    }
    else if (!this->folderEdit.wasActive && ImGui::GetFrameCount() - this->folderEdit.focusRequestFrame > 2)
    {
        // focus went elsewhere before the field got active (e.g. while the context menu closed), ask again
        this->folderEdit.requestFocus = true;
    }
    if (ImGui::IsItemEdited())
    {
        this->folderEdit.error.Clear();
    }

    if (submitted)
    {
        this->folderEdit.commitRequested = true;
    }
    else if (this->folderEdit.wasActive && ImGui::IsItemDeactivated())
    {
        this->CancelFolderEdit();
        return;
    }

    if (!this->folderEdit.error.IsEmpty())
    {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 2.0f));
        if (ImGui::BeginTooltip())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.4f, 1.0f), "%s", this->folderEdit.error.AsCharPtr());
            ImGui::EndTooltip();
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetBrowser::CommitFolderEdit()
{
    Util::String name = this->folderEdit.name;
    name.Trim(" \t");

    Util::String error;
    const bool newAsset = this->folderEdit.mode == FolderEditMode::NewAsset;
    const bool renameFile = this->folderEdit.mode == FolderEditMode::RenameFile;
    bool success = ValidateName(name, newAsset ? "asset" : (renameFile ? "file" : "folder"), error);
    if (success && (newAsset || renameFile) && ToolkitUtil::FileTypeURNMapping[this->folderEdit.assetType][0] != '\0' && name.FindCharIndex('.') != InvalidIndex)
    {
        // paths strip everything after a period as the file extension, which an asset gets from its type
        error = "An asset name can't contain a period";
        success = false;
    }
    if (success)
    {
        if (this->folderEdit.mode == FolderEditMode::NewFolder)
        {
            success = this->CreateSubFolder(this->folderEdit.folderId, name, error);
        }
        else if (this->folderEdit.mode == FolderEditMode::Rename)
        {
            success = this->RenameFolder(this->folderEdit.folderId, name, error);
        }
        else if (newAsset)
        {
            success = this->CreateAsset(this->folderEdit.folderId, this->folderEdit.assetType, name, error);
        }
        else if (renameFile)
        {
            success = this->RenameFile(this->folderEdit.fileId, name, error);
        }
    }

    if (!success)
    {
        this->folderEdit.error = error;
        return false;
    }
    this->folderEdit = FolderEdit();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::CancelFolderEdit()
{
    this->folderEdit = FolderEdit();
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetBrowser::CreateSubFolder(uint64_t parentId, const Util::String& name, Util::String& outError)
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

    const uint64_t folderId =
        this->fileDB.CreateFolder(this->logger, name, parentId, IO::FSWrapper::GetFileWriteTime(uri.LocalPath()), false);
    this->CacheFolderSubtree(folderId);
    this->SetActiveFolder(folderId);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetBrowser::RenameFolder(uint64_t folderId, const Util::String& newName, Util::String& outError)
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

    // the folder ids below the renamed folder are about to change, remember if the selection is in there
    bool selectionInside = false;
    uint64_t walkId = this->activeFolder;
    while (walkId != 0 && this->folderInfoCache.Contains(walkId))
    {
        if (walkId == folderId)
        {
            selectionInside = true;
            break;
        }
        const ToolkitUtil::FileDB::FolderInfo& walkInfo = this->folderInfoCache[walkId];
        if (walkInfo.isRoot)
        {
            break;
        }
        walkId = walkInfo.parentId;
    }

    // folder ids are derived from parent id and name, so the subtree is registered again under the new name
    this->UncacheFolderSubtree(folderId);
    IndexT siblingsIndex = this->folderChildIds.FindIndex(info.parentId);
    if (siblingsIndex != InvalidIndex)
    {
        Util::Array<uint64_t>& siblings = this->folderChildIds.ValueAtIndex(siblingsIndex);
        IndexT i = siblings.FindIndex(folderId);
        if (i != InvalidIndex)
        {
            siblings.EraseIndex(i);
        }
    }

    this->fileDB.BeginTransaction();
    DeleteFolderSubtree(this->fileDB, this->logger, folderId);
    this->fileDB.DeleteFolder(this->logger, folderId);
    const uint64_t newFolderId = this->fileDB.CreateFolder(
        this->logger, newName, info.parentId, IO::FSWrapper::GetFileWriteTime(toUri.LocalPath()), false
    );
    this->ScanFolder(this->fileDB, toUri, false, newFolderId, true);
    this->fileDB.EndTransaction();
    this->CacheFolderSubtree(newFolderId);

    if (selectionInside)
    {
        // the old active folder id is gone, don't let SetActiveFolder look it up
        this->activeFolder = 0;
        this->SetActiveFolder(newFolderId);
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::CacheFolderSubtree(uint64_t folderId)
{
    ToolkitUtil::FileDB::FolderInfo info;
    if (!this->fileDB.GetFolderInfo(folderId, info))
    {
        return;
    }
    this->folderInfoCache.Emplace(folderId) = info;
    this->folderInfoDict.Emplace(info.folderPath) = folderId;
    if (!info.isRoot)
    {
        Util::Array<uint64_t>& siblings = this->folderChildIds.Emplace(info.parentId);
        if (siblings.FindIndex(folderId) == InvalidIndex)
        {
            siblings.Append(folderId);
        }
    }

    Util::Array<ToolkitUtil::FileDB::FolderInfo> children;
    this->fileDB.GetChildFolders(folderId, children);
    Util::Array<uint64_t>& childIds = this->folderChildIds.Emplace(folderId);
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
AssetBrowser::UncacheFolderSubtree(uint64_t folderId)
{
    IndexT childIndex = this->folderChildIds.FindIndex(folderId);
    if (childIndex != InvalidIndex)
    {
        const Util::Array<uint64_t> childIds = this->folderChildIds.ValueAtIndex(childIndex);
        this->folderChildIds.EraseAtIndex(childIndex);
        for (uint64_t childId : childIds)
        {
            this->UncacheFolderSubtree(childId);
        }
    }

    IndexT infoIndex = this->folderInfoCache.FindIndex(folderId);
    if (infoIndex != InvalidIndex)
    {
        const IO::Path path = this->folderInfoCache.ValueAtIndex(infoIndex).folderPath;
        this->folderInfoCache.EraseAtIndex(infoIndex);
        IndexT pathIndex = this->folderInfoDict.FindIndex(path);
        if (pathIndex != InvalidIndex && this->folderInfoDict.ValueAtIndex(pathIndex) == folderId)
        {
            this->folderInfoDict.EraseAtIndex(pathIndex);
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
    if (write != this->searchIndex.Size())
    {
        this->searchIndex.Resize(write);
        this->searchIndexDirty = true;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::ScanFolder(ToolkitUtil::FileDB& fileDB, const IO::URI& folderPath, bool useArchive, uint64_t parent, bool recursive)
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

        fileDB.AddFile(this->logger, fileLeaf, parent, fileSize, DetermineFileType(fileLeaf.GetFileExtension()), modifiedTime);
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
            fileDB.DeleteFile(this->logger, dbFile.id);
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
        uint64_t childId = fileDB.CreateFolder(this->logger, childName, parent, IO::FSWrapper::GetFileWriteTime(childDir), useArchive);
        
        // Recursively scan the subdirectory
        if (childId != 0 && recursive)
        {
            ScanFolder(fileDB, childDir, useArchive, childId, true);
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
            DeleteFolderSubtree(fileDB, this->logger, dbChild.id);
            fileDB.DeleteFolder(this->logger, dbChild.id);
        }
    }

    if (!recursive)
    {
        fileDB.EndTransaction();
    }
}

//------------------------------------------------------------------------------
/**
*/
ToolkitUtil::FileType
AssetBrowser::DetermineFileType(const Util::String& extension)
{
    if (extension.IsEmpty())
    {
        return ToolkitUtil::FileType::Text;
    }
    
    Util::String ext(extension);
    ext.ToLower();
    
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

}
