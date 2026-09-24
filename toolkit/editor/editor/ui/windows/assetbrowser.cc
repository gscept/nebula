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
#include "timing/calendartime.h"
#include "editor/tools/pathconverter.h"
#include "io/filewatcher.h"
#include "io/assignregistry.h"
#include "asseteditor/particleasseteditor.h"
#include "toolkitutil/filedb/filedb.h"

using namespace Editor;

namespace Presentation
{

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
}

/// filesystem listing and FileDB writes run here so the editor frame is not stalled by _wstat64 / sqlite
class ScanFolderJob : public Threading::Thread
{
    __DeclareClass(ScanFolderJob);
public:
    AssetBrowser* browser;
    AssetBrowser* secondBrowser = nullptr;
    volatile float progress = 0.0f;

    void DoWork() override
    {
        Ptr<IO::IoServer> ioServer = IO::IoServer::Create();
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

        Util::Array<IO::Path> pathStack;
        pathStack.Clear();
        pathStack.Append(IO::Path());
        while (!pathStack.IsEmpty())
        {
            const IO::Path path = pathStack.Back();
            pathStack.EraseBack();

            const IO::URI workUri = path.WorkURI("assets");

            if (!ioServer->DirectoryExists(workUri))
            {
                continue;
            }

            IndexT idIndex = pathToId.FindIndex(path);
            n_assert(idIndex != InvalidIndex);
            const uint64_t folderId = pathToId.ValueAtIndex(idIndex);
            this->browser->ScanFolder(fileDB, ioServer, workUri, false, folderId, false);

            AssetBrowser::FolderScanResult result;
            result.folderId = folderId;
            fileDB.GetFilesInFolder(folderId, result.files);
            fileDB.GetChildFolders(folderId, result.children);
            this->browser->pendingScanResults.Enqueue(result);
            if (this->secondBrowser != nullptr)
            {
                this->secondBrowser->pendingScanResults.Enqueue(result);
            }

            for (SizeT i = result.children.Size(); i > 0; i--)
            {
                const ToolkitUtil::FileDB::FolderInfo& child = result.children[i - 1];
                if (!pathToId.Contains(child.folderPath))
                {
                    pathToId.Add(child.folderPath, child.id);
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
static const NewFunc NewFuncs[(uint)ToolkitUtil::FileType::Other + 1] =
{
    nullptr,  
    nullptr,  
    nullptr,  
    ParticleNew,
    nullptr,  
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
    
    this->activeFileTree = rootFolderId;
    this->activeFile = 0;
    this->activeFolder = 0;
    this->RefreshFileInfoCaches();

    this->fileTreeReady = true;
    this->showProgress = true;
    if (AssetBrowserWindow != nullptr && AssetBrowserWindow != this)
    {
        AssetBrowser* owner = (AssetBrowser*)AssetBrowserWindow;
        if (owner->currentScanJob != nullptr)
        {
            owner->currentScanJob->secondBrowser = this;
        }
    }
    else
    {
        this->ownsScanJob = true;
        this->currentScanJob = ScanFolderJob::Create();
        this->currentScanJob->browser = this;
    }
}

//------------------------------------------------------------------------------
/**
*/
AssetBrowser::~AssetBrowser()
{
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
        Util::Array<IO::WatchEvent> events;
        this->pendingWatchEvents.DequeueAll(events);
        if (!events.IsEmpty() && this->fileDB.IsOpen() && this->activeFolder != 0)
        {
            IO::IoServer* ioServer = IO::IoServer::Instance();
            bool cacheNeedsRefresh = false;
            for (const IO::WatchEvent& event : events)
            {
                Util::String fileName = event.file;
                if (fileName.IsEmpty())
                    continue;

                IO::Path relFilePath = IO::Path::File(event.relativePath, fileName, ToolkitUtil::FileTypeURNMapping[DetermineFileType(fileName.GetFileExtension())]);
                IO::Path folderPath = this->folderInfoCache[this->activeFolder].folderPath;
                IO::Path filePath = folderPath / relFilePath;
                IO::URI fileUri = filePath.WorkURI("assets");
                bool fileExistsOnDisk = ioServer->FileExists(fileUri);

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
                            if (ioServer->GetIOInfo(fileUri, ioInfo, false))
                            {
                                fileSize = ioInfo.size;
                                modifiedTime = ioInfo.modifiedTime;
                            }
                            this->fileDB.AddFile(this->logger, fileName, this->activeFolder, fileSize, DetermineFileType(fileName.GetFileExtension()), modifiedTime);
                        }
                        else
                        {
                            if (this->fileInfoDict.Contains(filePath))
                            {
                                uint64_t fileId = this->fileInfoDict[filePath];
                                this->fileDB.DeleteFile(this->logger, fileId);
                            }
                        }
                        cacheNeedsRefresh = true;
                    }
                    break;
                    case IO::WatchEventType::Deleted:
                    {
                        if (this->fileInfoDict.Contains(filePath))
                        {
                            uint64_t fileId = this->fileInfoDict[filePath];
                            this->fileDB.DeleteFile(this->logger, fileId);
                        }
                        cacheNeedsRefresh = true;
                    }
                    break;
                    case IO::WatchEventType::Modified:
                    {
                        if (this->fileInfoDict.Contains(filePath))
                        {
                            uint64_t fileId = this->fileInfoDict[filePath];
                            IO::IOStat ioInfo;
                            if (ioServer->GetIOInfo(fileUri, ioInfo, false))
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
AssetBrowser::PickFile(const IO::Path& path, std::function<void(const IO::Path& path)> picker)
{
    this->open = true;
    this->popupThisFrame = true;
    this->pickFileFunction = picker;
    this->pendingPickPath = path;
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
            if (!oldInfo.isArchive)
            {
                IO::Path oldFolder = oldInfo.folderPath;
                if (IO::FileWatcher::Instance()->IsWatched(oldFolder.GetFolderAndFile()))
                {
                    IO::FileWatcher::Instance()->Unwatch(oldFolder.GetFolderAndFile());
                }
            }
        }
        ToolkitUtil::FileDB::FolderInfo info = this->folderInfoCache[folderId];
        IO::URI folderPath = info.folderPath.WorkURI("assets");
        if (!this->scannedFolders.Contains(folderId))
        {
            IO::IoServer* ioServer = IO::IoServer::Instance();
            if (this->currentScanJob != nullptr)
            {
                Util::Array<Util::String> files = ioServer->ListFiles(folderPath, "*", false);
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
                this->ScanFolder(this->fileDB, ioServer, folderPath, info.isArchive, folderId, false);
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
        if (!info.isArchive)
        {
            Util::BitField<8> watchFlags;
            watchFlags.SetBit(IO::WatchFlags::NameChanged);
            watchFlags.SetBit(IO::WatchFlags::SizeChanged);
            watchFlags.SetBit(IO::WatchFlags::Creation);
            IO::WatchDelegate callback = [this](IO::WatchEvent const& event)
            {
                this->pendingWatchEvents.Enqueue(event);
            };

            IO::FileWatcher::Instance()->Watch(folderPath.LocalPath(), true, watchFlags, callback);
        }
        this->activeFolder = folderId;
        this->activeFile = 0;
        if (this->scannedFolders.Contains(folderId))
        {
            this->RefreshFileInfoCaches();
        }
    }
}
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
    if (info.id == this->activeFolder)
    {
        flags |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;
    }

    ImGui::PushID(reinterpret_cast<void*>(static_cast<uintptr_t>(folderId)));
    bool bIsOpen = ImGui::TreeNodeEx(info.name.AsCharPtr(), flags);
    ImGui::PopID();
    if (ImGui::IsItemClicked())
    {
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && this->pickFolderFunction)
        {
            this->pickFolderFunction(info.folderPath);
            this->pickFolderFunction = nullptr;
            this->open = false;
        }
        else
        {
            this->SetActiveFolder(folderId);
        }
    }

    if (bIsOpen)
    {
        if (!this->scannedFolders.Contains(folderId) && this->currentScanJob == nullptr)
        {
            IO::IoServer* ioServer = IO::IoServer::Instance();
            this->ScanFolder(this->fileDB, ioServer,info.folderPath.WorkURI("assets"), info.isArchive, folderId, false);
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
        ImGui::TreePop();
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
            static Util::String filePath = file.GetFolderAndFile();
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
        Util::String pattern = filter;
        if (pattern.FindCharIndex('*') == InvalidIndex && pattern.FindCharIndex('?') == InvalidIndex)
        {
            pattern = Util::String::Sprintf("*%s*", filter.AsCharPtr());
        }
        for (const auto& file : this->searchIndex)
        {
            if (Util::String::MatchPattern(file.name, pattern) || Util::String::MatchPattern(file.filePath.GetFolderAndFile(), pattern))
            {
                this->searchResults.Append(file);
            }
        }
        this->lastSearchFilter = filter;
        this->searchIndexDirty = false;
    }

    if (!searching && this->activeFolder == 0)
    {
        return;
    }

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

        switch(this->fileViewMode)
        {
            case FileViewMode::List:
            {
                for (const auto& file : visibleFiles)
                {
                    bool isSelected = (this->activeFile == file.id);
                    ImGui::PushID(reinterpret_cast<void*>(static_cast<uintptr_t>(file.id)));
                    if (ImGui::Selectable(Util::Format("[%s] %s", ToolkitUtil::FileTypeNames[file.type], file.name.AsCharPtr()).AsCharPtr(), &isSelected))
                    {
                        this->activeFile = file.id;
                    }
                    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
                    {
                        hasFileToOpen = true;
                        fileToOpen = file;
                    }
                    AddDragSourceForFileUri(file.filePath);
                    ImGui::PopID();
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

                if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs())
                if (sorts_specs->SpecsDirty && visibleFiles.Size() > 1)
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
                    ImGui::PushID(file.name.HashCode() + (uint)file.type);
                    if (ImGui::Selectable(Util::Format("[%s] %s", ToolkitUtil::FileTypeNames[file.type], file.name.AsCharPtr()).AsCharPtr(), &isSelected))
                    {
                        this->activeFile = file.id;
                    }
                    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
                    {
                        hasFileToOpen = true;
                        fileToOpen = file;
                    }
                    ImGui::PopID();
                    AddDragSourceForFileUri(file.filePath);
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
                ImGui::EndTable();
                ImGui::EndGroup();
                break;
            }    
            case FileViewMode::Icons:
            {        
                ImGuiStyle& style = ImGui::GetStyle();
                int numFiles = visibleFiles.Size();
                float windowVisibleX = ImGui::GetWindowPos().x + ImGui::GetContentRegionAvail().x;
                static int itemSize = 150;
                ImGui::SliderInt("Zoom", &itemSize, 25, 200);
                int n = 0;
                for (const auto& file : visibleFiles)
                {
                    Util::String const& name = file.name;
                    ImGui::BeginGroup();
                    ImGui::PushID(file.id);

                    ImVec2 pos = ImGui::GetCursorScreenPos();

                    //ImGui::ImageButton(name.AsCharPtr(), &Editor::UI::Icons::game, { (float)itemSize, (float)itemSize });
                    if(ImGui::InvisibleButton(name.AsCharPtr(), { (float)itemSize, (float)itemSize }))
                    {
                        this->activeFile = file.id;
                    }

                    bool hovered = ImGui::IsItemHovered();
                    bool clicked = ImGui::IsItemClicked();
                    bool doubleClicked = ImGui::IsMouseDoubleClicked(0);
                    if (clicked && doubleClicked)
                    {
                        hasFileToOpen = true;
                        fileToOpen = file;
                    }
                    AddDragSourceForFileUri(file.filePath);

                    // Now draw the widget yourself.
                    ImDrawList* draw = ImGui::GetWindowDrawList();

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

                    // Clip everything inside the card.
                    //draw->PushClipRect(pos, max, true);

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
                        ToolkitUtil::FileTypeNames[file.type]
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
                    draw->AddText(
                        ImGui::GetFont(),
                        nameFontSize,
                        ImVec2(
                            pos.x + padding,
                            max.y - nameHeight
                        ),
                        IM_COL32(230, 230, 230, 255),
                        name.AsCharPtr()
                    );

                    //draw->PopClipRect();

                    if (hovered)
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
                    ImGui::PopID();


                    if (hovered)
                    {
                        if (ImGui::BeginTooltip())
                        {
                            ImGui::Text(name.AsCharPtr());
                            ImGui::EndTooltip();
                        }
                    }
                    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
                    {
                        this->activeFile = file.id;
                        hasFileToOpen = true;
                        fileToOpen = file;
                    }
                    AddDragSourceForFileUri(file.filePath);
                    ImGui::EndGroup();

                    float lastButtonX = ImGui::GetItemRectMax().x;
                    float nextButtonX = lastButtonX + style.ItemSpacing.x + (float)itemSize + 30.0f; // Expected position if next button was on same line
                    if (n + 1 < numFiles && nextButtonX < windowVisibleX)
                        ImGui::SameLine();
                    n++;

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
*/
void
NewAsset(Util::String requestedFileName, ToolkitUtil::FileDB& db, ToolkitUtil::FileType type, uint64_t folderEntry, ToolkitUtil::Logger& logger)
{
    IO::Path folderPath = db.GetFolderPath(folderEntry);
    Util::Array<ToolkitUtil::FileDB::FileInfo> files(32, 8);
    db.GetFilesInFolder(folderEntry, files);
    Util::String newFilePath = Util::Format("%s/%s", folderPath.GetFolderAndFile().AsCharPtr(), requestedFileName.AsCharPtr());
retry:
    SizeT counter = 0;
    for (const auto& file : files)
    {
        if (file.name == requestedFileName)
        {
            Util::String ext = requestedFileName.GetFileExtension();
            requestedFileName.StripFileExtension();
            requestedFileName += Util::String::Sprintf(" (%d).%s", counter++, ext.AsCharPtr());
            newFilePath = Util::Format("%s/%s", folderPath.GetFolderAndFile().AsCharPtr(), requestedFileName.AsCharPtr());
            goto retry;
        }
    }
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(newFilePath);
    stream->SetAccessMode(IO::Stream::AccessMode::WriteAccess);
    stream->Open();

    if (NewFuncs[(uint)type] != nullptr)
        NewFuncs[(uint)type](stream, newFilePath);

    db.AddFile(
        logger,
        stream->GetURI().LocalPath().ExtractFileName(),
        folderEntry,
        stream->GetSize(),
        ToolkitUtil::FileType::Particle,
        IO::FileTime()
    );
    stream->Close();
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::DisplayFileTree()
{
    ImGui::PushItemWidth(ImGui::GetWindowWidth());
    if (this->showProgress)
    {
        ScanFolderJob* scanJob = this->currentScanJob;
        if (scanJob == nullptr && AssetBrowserWindow != nullptr)
        {
            scanJob = ((AssetBrowser*)AssetBrowserWindow)->currentScanJob;
        }
        if (scanJob != nullptr)
        {
            ImGui::ProgressBar(scanJob->GetProgress(), ImVec2(160.0f, 0.0f), "Indexing...");
        }
    }
    ImGui::NewLine();
    ImGui::InputText("##search", this->searchFilter, sizeof(this->searchFilter));
    
     
    ImGui::PopItemWidth();

    ImGui::Separator();
    ImGui::Columns(2);
    ImGui::BeginChild("ScrollingRegionFolders");
    if (this->activeFileTree != 0)
    {
        this->DisplayFileTreeFolderHierarchy(this->activeFileTree, 0);
    }
    else
    {
        ImGui::Text("No root data yet");
    }
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
    ImGui::EndChild();

    if (this->pickFolderFunction && this->activeFileTree != 0)
    {
        if (ImGui::Button("Select folder"))
        {
            auto info = this->folderInfoCache[this->activeFileTree];
            this->pickFolderFunction(info.folderPath);
            this->pickFolderFunction = nullptr;
            this->open = false;
        }
    }

    if (this->activeFolder != 0)
    {
        if (ImGui::BeginPopupContextItem("AssetActions"))
        {
            if (ImGui::MenuItem("Create Particle System###Item"))
            {
                NewAsset("new_particle_system.par", this->fileDB, ToolkitUtil::FileType::Particle, this->activeFolder, this->logger);
                this->RefreshFileInfoCaches();
            }
            if (ImGui::MenuItem("Create Material###Item"))
            {
                NewAsset("new_surface.mat", this->fileDB, ToolkitUtil::FileType::Surface, this->activeFolder, this->logger);
                this->RefreshFileInfoCaches();
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
AssetBrowser::ScanFolder(ToolkitUtil::FileDB& fileDB, const IO::IoServer* ioServer, const IO::URI& folderPath, bool useArchive, uint64_t parent, bool recursive)
{
    if (!recursive)
    {
        fileDB.BeginTransaction();
    }

    // List all files in the current directory
    Util::Array<Util::String> files = ioServer->ListFiles(folderPath, "*", true);
    Util::Array<Util::String> filesystemFileNames;
    filesystemFileNames.Reserve(files.Size());
    for (const auto& fileName : files)
    {
        Util::String fileLeaf = fileName.ExtractFileName();
        filesystemFileNames.Append(fileLeaf);
        IO::IOStat ioInfo;
        IO::Stream::Size fileSize = 0;
        IO::FileTime modifiedTime;
        if (ioServer->GetIOInfo(fileName, ioInfo, useArchive))
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
    Util::Array<Util::String> directories = ioServer->ListDirectories(folderPath, "*", true, useArchive);
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
            ScanFolder(fileDB, ioServer, childDir, useArchive, childId, true);
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
    
    // NavMesh files
    if (ext == "nav")
    {
        return ToolkitUtil::FileType::NavMesh;
    }

    
    // Default to Other for unknown extensions
    return ToolkitUtil::FileType::Other;
}

}
