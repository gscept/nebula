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
#include <algorithm>
#include <cstdint>

#include "editor/tools/pathconverter.h"

using namespace Editor;

namespace Presentation
{
__ImplementClass(Presentation::AssetBrowser, 'AsBw', Presentation::BaseWindow);

/// quick & dirty scan job as this is too slow in windows and blocks the main thread too long
class ScanFolderJob : public Threading::Thread
{
    __DeclareClass(ScanFolderJob);
public:
    /// 
    AssetBrowser* browser;

    /// 
    void DoWork() override
    {
        Ptr<IO::IoServer> ioServer = IO::IoServer::Create();
        this->browser->ScanFolder("export", "export:", false);
        this->browser->ScanFolder("sysexport", "export:", true);
        this->browser->ScanFolder("work", "proj:work/", false);
        this->browser->ScanFolder("syswork", "tool:syswork/", false);
        ioServer = nullptr;
        this->browser->currentScanJob = nullptr;
    }
};
__ImplementClass(Presentation::ScanFolderJob, 'ScFj', Threading::Thread);

//------------------------------------------------------------------------------
/**
*/
AssetBrowser::AssetBrowser()
{
    this->fileDB.SetDatabaseURI(IO::URI("int:/filedb.sqlite"));
    this->fileDB.Open(this->logger);
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
    if (this->currentScanJob.isvalid() && this->currentScanJob->IsRunning())
    {
        ImGui::Text("Scanning folders...");
        return;
    }
    DisplayFileTree();
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::DisplayFileTreeFolderHierarchy(uint64_t folderId, int depth)
{
    ToolkitUtil::FileDB::FolderInfo info;
    if (!this->fileDB.GetFolderInfo(folderId, info))
    {
        return;
    }

    Util::Array<ToolkitUtil::FileDB::FolderInfo> children;
    this->fileDB.GetChildFolders(folderId, children);
    std::sort(children.begin(), children.end(), [](const ToolkitUtil::FileDB::FolderInfo& a, const ToolkitUtil::FileDB::FolderInfo& b)
    {
        return a.name < b.name;
    });

    Util::Array<ToolkitUtil::FileDB::FileInfo> files;
    this->fileDB.GetFilesInFolder(folderId, files);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_SpanFullWidth;
    if (depth == 0)
    {
        flags |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen;
    }
    if (children.IsEmpty() && files.IsEmpty())
    {
        flags |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Leaf; 
    }

    ImGui::PushID(reinterpret_cast<void*>(static_cast<uintptr_t>(folderId)));
    bool bIsOpen = ImGui::TreeNodeEx(info.name.AsCharPtr(), flags);
    ImGui::PopID();
    if(ImGui::IsItemClicked())
    {
        this->activeFolder = folderId;
        this->activeFile = 0;
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
        Util::Array<ToolkitUtil::FileDB::FileInfo> folderFiles;
        this->fileDB.GetFilesInFolder(this->activeFolder, folderFiles);

        Util::Array<ToolkitUtil::FileDB::FileInfo> visibleFiles;
        visibleFiles.Reserve(folderFiles.Size());
        for (const auto& file : folderFiles)
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
                    AddDragSourceForFileUri(this->fileDB.GetFilePath(file.id));
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
                    sorts_specs->SpecsDirty = false;
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
                    AddDragSourceForFileUri(this->fileDB.GetFilePath(file.id));
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
                    AddDragSourceForFileUri(this->fileDB.GetFilePath(file.id));
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
            IO::URI uri = this->fileDB.GetFilePath(fileToOpen.id);
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
    static char buffer[NEBULA_MAXPATH];
    ImGui::PushItemWidth(ImGui::GetWindowWidth());
    if (ImGui::Button("work"))
    {
        this->activeFileTree = "work";
        if (this->roots.Contains(this->activeFileTree))
        {
            this->activeFolder = this->roots[this->activeFileTree];
            this->activeFile = 0;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("export"))
    {
        this->activeFileTree = "export";
        if (this->roots.Contains(this->activeFileTree))
        {
            this->activeFolder = this->roots[this->activeFileTree];
            this->activeFile = 0;
        }
    }
    ImGui::SameLine();
    ImGui::InputText("##search", buffer, NEBULA_MAXPATH, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
    
     
    ImGui::PopItemWidth();

    ImGui::Separator();
    ImGui::Columns(2);
    ImGui::BeginChild("ScrollingRegionFolders");
    if (this->roots.Contains(this->activeFileTree))
    {
        this->DisplayFileTreeFolderHierarchy(this->roots[this->activeFileTree], 0);
    }
    else
    {
        ImGui::Text("No root data yet");
    }
    ImGui::Text("System"); 
    ImGui::SameLine();
    ImGui::Separator();
    const Util::String& sysFolder = "sys" + this->activeFileTree;
    if (this->roots.Contains(sysFolder))
    {
        this->DisplayFileTreeFolderHierarchy(this->roots[sysFolder], 0);
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
AssetBrowser::ScanFolder(const Util::String & treeName, const Util::String& folderPathString, bool useArchive)
{
    IO::URI folderPath(folderPathString);
    uint64_t rootId = this->fileDB.CreateRootFolder(folderPathString, IO::FSWrapper::GetFileWriteTime(folderPath.LocalPath()), useArchive);
    if (this->roots.Contains(treeName))
    {
        this->roots[treeName] = rootId;
    }
    else
    {
        uint64_t& root = this->roots.Emplace(treeName);
        root = rootId;
    }

    if (this->activeFolder == 0 && treeName == this->activeFileTree)
    {
        this->activeFolder = rootId;
    }

    IO::IoServer* ioServer = IO::IoServer::Instance();
    // Start recursive scanning from the root
    if (ioServer->DirectoryExists(folderPath))
    {
        ScanFolderRecursive(ioServer, folderPath, useArchive, rootId);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::ScanFolderRecursive(const IO::IoServer* ioServer, const IO::URI& folderPath, bool useArchive, uint64_t parent)
{
    // List all files in the current directory
    Util::Array<Util::String> files = ioServer->ListFiles(folderPath, "*.*", true);
    for (const auto& fileName : files)
    {
        Util::String fileLeaf = fileName.ExtractFileName();
        IO::IOStat ioInfo;
        IO::Stream::Size fileSize = 0;
        IO::FileTime modifiedTime;
        if (ioServer->GetIOInfo(fileName, ioInfo, useArchive))
        {
            fileSize = ioInfo.size;
            modifiedTime = ioInfo.modifiedTime;
        }

        this->fileDB.AddFile(this->logger, fileLeaf, parent, fileSize, DetermineFileType(fileLeaf.GetFileExtension()), modifiedTime);
    }
    
    // List all subdirectories and recursively scan them
    Util::Array<Util::String> directories = ioServer->ListDirectories(folderPath, "*", true, useArchive);
    for (const auto& childDir : directories)
    {
        Util::String childName = childDir.ExtractFileName();
        uint64_t childId = this->fileDB.CreateFolder(this->logger, childName, parent, IO::FSWrapper::GetFileWriteTime(childDir), useArchive);
        
        // Recursively scan the subdirectory
        if (childId != 0)
        {
            ScanFolderRecursive(ioServer, childDir, useArchive, childId);
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
