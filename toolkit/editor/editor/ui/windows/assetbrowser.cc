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

using namespace Editor;

namespace Presentation
{
__ImplementClass(Presentation::AssetBrowser, 'AsBw', Presentation::BaseWindow);

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

/// quick & dirty scan job as this is too slow in windows and blocks the main thread too long
class ScanFolderJob : public Threading::Thread
{
    __DeclareClass(ScanFolderJob);
public:
    /// 
    AssetBrowser* browser;

    volatile float progress = 0.0f;
    /// 
    void DoWork() override
    {
        Ptr<IO::IoServer> ioServer = IO::IoServer::Create();
        ToolkitUtil::Logger logger;
        ToolkitUtil::FileDB fileDB;
        fileDB.SetDatabaseURI(IO::URI("int:/filedb.sqlite"));
        fileDB.Open(logger, false);
        this->browser->ScanFolder(fileDB, "export", "export:", false);
        this->progress = 0.25f;
        this->browser->ScanFolder(fileDB, "sysexport", "export:", true);
        this->progress = 0.5f;
        this->browser->ScanFolder(fileDB, "work", "proj:work/", false);
        this->progress = 0.75f;
        this->browser->ScanFolder(fileDB, "syswork", "tool:syswork/", false);
        this->progress = 1.0f;
        fileDB.Close();
        ioServer = nullptr;
        this->browser->isDoneRefreshingCaches.Set();
        this->browser->currentScanJob = nullptr;
    }
    float GetProgress() const
    {
        return this->progress;
    }
};
__ImplementClass(Presentation::ScanFolderJob, 'ScFj', Threading::Thread);

//------------------------------------------------------------------------------
/**
*/
AssetBrowser::AssetBrowser()
{
    this->fileDB.SetDatabaseURI(IO::URI("int:/filedb.sqlite"));
    if (IO::IoServer::Instance()->FileExists(IO::URI("int:/filedb.sqlite")))
    {
        this->fileDB.Open(this->logger, true);    
    }    
   
    this->currentScanJob = ScanFolderJob::Create();
    this->currentScanJob->browser = this;
    this->currentScanJob->Start();
}

//------------------------------------------------------------------------------
/**
*/
AssetBrowser::~AssetBrowser()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::Update()
{
    
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::Run(SaveMode save)
{
    if(this->isDoneRefreshingCaches.Test())
    {
        this->isDoneRefreshingCaches.Clear();
        this->fileDB.Close();
        this->fileDB.Open(this->logger, true);
        this->RefreshFolderInfoCaches();
        this->RefreshFileInfoCaches();
    }
    if(this->currentScanJob != nullptr)
    {
        ImGui::ProgressBar(this->currentScanJob->GetProgress(), ImVec2(0.0f, 0.0f), "Scanning...");
    }
    if(this->fileDB.IsOpen())
    {
        DisplayFileTree();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::RefreshFolderInfoCaches()
{
    this->folderInfoCache.Clear();
    if (this->activeFileTree != 0)
    {
        std::function<void(uint64_t)> fillFolders;
        fillFolders = [this, &fillFolders](uint64_t folderId)
        {
            Util::Array<ToolkitUtil::FileDB::FolderInfo> children;
            this->fileDB.GetChildFolders(folderId, children);
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
                fillFolders(child.id);
            }
        };

        this->fileDB.GetFolderInfo(this->activeFileTree, this->folderInfoCache.Emplace(this->activeFileTree));
        fillFolders(this->activeFileTree);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::RefreshFileInfoCaches()
{
    this->fileInfoCache.Clear();
    if (this->activeFileTree != 0 && this->activeFolder != 0)
    {
        Util::Array<ToolkitUtil::FileDB::FileInfo> files;
        this->fileDB.GetFilesInFolder(this->activeFolder, files);
        this->fileInfoCache.Reserve(files.Size());
        for (const auto& file : files)
        {
            this->fileInfoCache.Append(file);
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
 
    Util::Array<ToolkitUtil::FileDB::FolderInfo> children;
    this->fileDB.GetChildFolders(folderId, children);
    std::sort(children.begin(), children.end(), [](const ToolkitUtil::FileDB::FolderInfo& a, const ToolkitUtil::FileDB::FolderInfo& b)
    {
        return a.name < b.name;
    });

    
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
    if(ImGui::IsItemClicked())
    {
        this->activeFolder = folderId;
        this->activeFile = 0;
        this->RefreshFileInfoCaches();
    }

    if (bIsOpen)
    {
        for (const auto& child : children)
        {
            DisplayFileTreeFolderHierarchy(child.id, depth + 1);
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
            case ToolkitUtil::FileType::FBX:
            case ToolkitUtil::FileType::GLTF:
            case ToolkitUtil::FileType::Model:
                return AssetEditor::AssetType::Model;
            case ToolkitUtil::FileType::Mesh:
                return AssetEditor::AssetType::Mesh;
            case ToolkitUtil::FileType::Texture:
                return AssetEditor::AssetType::Texture;
            case ToolkitUtil::FileType::Skeleton:
                return AssetEditor::AssetType::Skeleton;
            case ToolkitUtil::FileType::Animation:
                return AssetEditor::AssetType::Animation;
            case ToolkitUtil::FileType::Surface:
                return AssetEditor::AssetType::Material;
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

    static const auto AddDragSourceForFileUri = [](const IO::URI& file)
    {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            Util::String filePath = file.GetHostAndLocalPath();
            ImGui::SetDragDropPayload("resource", filePath.AsCharPtr(), sizeof(char) * filePath.Length() + 1);
            ImGui::EndDragDropSource();
        }
    };

    static const ImGuiTableSortSpecs* s_sort_specs = nullptr;

    if (this->activeFolder != 0)
    {
        bool hasFileToOpen = false;
        ToolkitUtil::FileDB::FileInfo fileToOpen;       

        Util::Array<ToolkitUtil::FileDB::FileInfo> visibleFiles;
        visibleFiles.Reserve(this->fileInfoCache.Size());
        for (const auto& file : this->fileInfoCache)
        {
            if (!filter.IsEmpty() && this->activeFile != file.id && !Util::String::MatchPattern(file.name, filter))
            {
                continue;
            }
            visibleFiles.Append(file);
        }

        switch(this->fileViewMode)
        {
            case FileViewMode::List:
            {
                for (const auto& file : visibleFiles)
                {
                    bool isSelected = (this->activeFile == file.id);
                    ImGui::PushID(reinterpret_cast<void*>(static_cast<uintptr_t>(file.id)));
                    if(ImGui::Selectable(file.name.AsCharPtr(), &isSelected))
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
            }
            break;
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
                    if (ImGui::Selectable(file.name.AsCharPtr(), &isSelected))
                    {
                        this->activeFile = file.id;

                    }
                    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
                    {
                        hasFileToOpen = true;
                        fileToOpen = file;
                    }
                    AddDragSourceForFileUri(file.filePath);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d KB", (int)(file.size / 1024));
                    ImGui::TableNextColumn();  
                    Timing::CalendarTime cal = Timing::CalendarTime::FileTimeToSystemTime(file.modifiedDate);
                    ImGui::Text("%d-%d-%d %d:%d", cal.GetYear(), cal.GetMonth(), cal.GetDay(), cal.GetHour(), cal.GetMinute());
                }
                ImGui::EndTable();
                ImGui::EndGroup();
            }    
            break;
            case FileViewMode::Icons:
            {        
                ImGuiStyle& style = ImGui::GetStyle();
                int numFiles = visibleFiles.Size();
                float windowVisibleX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                static int itemSize = 50;
                ImGui::SliderInt("Zoom", &itemSize, 25, 200);
                int n = 0;
                for (const auto& file : visibleFiles)
                {
                    Util::String const& name = file.name;
                    ImGui::PushID(reinterpret_cast<void*>(static_cast<uintptr_t>(file.id)));
                    ImGui::BeginGroup();
                    //ImGui::ImageButton(name.AsCharPtr(), &Editor::UI::Icons::game, { (float)itemSize, (float)itemSize });
                    if(ImGui::Button("X", { (float)itemSize, (float)itemSize }))
                    {
                        this->activeFile = file.id;
                    }
                    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
                    {
                        hasFileToOpen = true;
                        fileToOpen = file;
                    }
                    AddDragSourceForFileUri(file.filePath);
                    ImGui::BeginChild("##filename00", { (float)itemSize, ImGui::GetTextLineHeight()}, false, ImGuiWindowFlags_NoScrollbar);
                    ImGui::Text(name.AsCharPtr());
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text(name.AsCharPtr());
                        ImGui::EndTooltip();
                    }
                    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
                    {
                        this->activeFile = file.id;
                        hasFileToOpen = true;
                        fileToOpen = file;
                    }
                    AddDragSourceForFileUri(this->fileDB.GetFilePath(file.id));
                    ImGui::EndChild();                    
                    ImGui::EndGroup();

                    float lastButtonX = ImGui::GetItemRectMax().x;
                    float nextButtonX = lastButtonX + style.ItemSpacing.x + (float)itemSize + 30.0f; // Expected position if next button was on same line
                    if (n + 1 < numFiles && nextButtonX < windowVisibleX)
                        ImGui::SameLine();
                    n++;

                    ImGui::PopID();
                }
            }
            break;
            default: break;
        }
        if (hasFileToOpen)
        {
            IO::URI uri = fileToOpen.filePath;
            Ptr<AssetEditor> assetEditor = WindowServer::Instance()->GetWindow("Asset Editor").downcast<AssetEditor>();
            assetEditor->Open(uri.GetHostAndLocalPath(), FileEntryTypeToAssetType(fileToOpen.type));
        }    
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::DisplayFileTree()
{
     Util::Array<ToolkitUtil::FileDB::FolderInfo> rootFolders; 
    this->fileDB.GetRootFolders(rootFolders);

    static char buffer[NEBULA_MAXPATH];
    ImGui::PushItemWidth(ImGui::GetWindowWidth());
    uint64_t oldActiveFileTree = this->activeFileTree;
    for (auto& root : rootFolders)
    {
        if (this->activeFileTree == 0)
        {
            this->activeFileTree = root.id;
        }
        Util::String displayName = root.name;
        if (root.isArchive)
        {
            displayName.Append(" (zip)");
        }
        ImGui::Button(displayName.AsCharPtr());
        if (ImGui::IsItemClicked())
        {
            this->activeFileTree = root.id;
            this->activeFile = 0;
            this->activeFolder = 0;
        }
        ImGui::SameLine();
    }
    if (oldActiveFileTree != this->activeFileTree)
    {
        this->RefreshFolderInfoCaches();
    }
    ImGui::NewLine();
    ImGui::InputText("##search", buffer, NEBULA_MAXPATH, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
    
     
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
    this->DisplaySelectedFolder(buffer);
    ImGui::EndChild();
    ImGui::NextColumn();
}

//------------------------------------------------------------------------------
/**
*/
void 
AssetBrowser::ScanFolder(ToolkitUtil::FileDB& fileDB, const Util::String & treeName, const Util::String& folderPathString, bool useArchive)
{
    IO::URI folderPath(folderPathString);
    uint64_t rootId = fileDB.CreateRootFolder(folderPathString, IO::FSWrapper::GetFileWriteTime(folderPath.LocalPath()), useArchive);

    IO::IoServer* ioServer = IO::IoServer::Instance();
    // Start recursive scanning from the root
    if (ioServer->DirectoryExists(folderPath))
    {
        ScanFolderRecursive(fileDB, ioServer, folderPath, useArchive, rootId);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::ScanFolderRecursive(ToolkitUtil::FileDB& fileDB, const IO::IoServer* ioServer, const IO::URI& folderPath, bool useArchive, uint64_t parent)
{
    // List all files in the current directory
    Util::Array<Util::String> files = ioServer->ListFiles(folderPath, "*.*", true);
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
        if (childId != 0)
        {
            ScanFolderRecursive(fileDB, ioServer, childDir, useArchive, childId);
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
    if (ext == "n3")
    {
        return ToolkitUtil::FileType::Model;
    }

    if (ext == "nvx2" || ext == "nvx")
    {
        return ToolkitUtil::FileType::Mesh;
    }
    
    // Texture files
    if (ext == "dds" || ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "tga" || ext == "bmp")
    {
        return ToolkitUtil::FileType::Texture;
    }
    
    // Surface/Material files
    if (ext == "sur" || ext == "material")
    {
        return ToolkitUtil::FileType::Surface;
    }
    
    // Audio files
    if (ext == "ogg" || ext == "wav" || ext == "mp3" || ext == "flac")
    {
        return ToolkitUtil::FileType::Audio;
    }
    
    // Skeleton files
    if (ext == "nsk")
    {
        return ToolkitUtil::FileType::Skeleton;
    }
    
    // Animation files
    if (ext == "nax")
    {
        return ToolkitUtil::FileType::Animation;
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
