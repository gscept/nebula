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

//------------------------------------------------------------------------------
/**
*/
AssetBrowser::AssetBrowser()
{
    AssetBrowser::FileTree& exportTree =  this->fileTrees.Emplace("export");
    exportTree.path = "export:";
    AssetBrowser::FileTree& workTree =  this->fileTrees.Emplace("work");
    workTree.path = "proj:work/";
    this->ScanFolder(exportTree);
    this->ScanFolder(workTree);
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
    DisplayFileTree();
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::DisplayFileTreeFolderHierarchy(const FileTreeNode& node, int depth)
{
    Util::String rootFolder = IO::URI("proj:").GetHostAndLocalPath();
    Util::String relativePath = node.path.LocalPath().ExtractToEnd(rootFolder.Length());
    
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_SpanFullWidth;
    if (depth == 0)
    {
        flags |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen;
    }
    if (node.children.IsEmpty() && node.files.IsEmpty())
    {
        flags |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Leaf; 
    }
    bool bIsOpen = ImGui::TreeNodeEx(node.name.AsCharPtr(), flags);
    if(ImGui::IsItemClicked())
    {
        this->activeFolder = node.hash;
    }
    if (bIsOpen)
    {
        for (const auto& childHash : node.children)
        {
            const FileTreeNode& childNode = this->nodes[childHash];
            DisplayFileTreeFolderHierarchy(childNode, depth + 1);
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
    static const auto FileEntryTypeToAssetType = [](FileEntry::Type type) -> AssetEditor::AssetType
    {
        switch (type)
        {
            case FileEntry::Type::FBX:
            case FileEntry::Type::GLTF:
            case FileEntry::Type::Model:
                return AssetEditor::AssetType::Model;
            case FileEntry::Type::Mesh:
                return AssetEditor::AssetType::Mesh;
            case FileEntry::Type::Texture:
                return AssetEditor::AssetType::Texture;
            case FileEntry::Type::Skeleton:
                return AssetEditor::AssetType::Skeleton;
            case FileEntry::Type::Animation:
                return AssetEditor::AssetType::Animation;
            case FileEntry::Type::Surface:                
            case FileEntry::Type::Audio:                
            case FileEntry::Type::Text:
            case FileEntry::Type::Frame:
            case FileEntry::Type::Shader:
            case FileEntry::Type::Physics:
            case FileEntry::Type::NavMesh:            
            default:
                return AssetEditor::AssetType::None;
        }
    };
    
    static const auto AddDragSourceForFileEntry = [](const FileEntry& file)
    {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            Util::String filePath = file.path.GetHostAndLocalPath();
            ImGui::SetDragDropPayload("resource", filePath.AsCharPtr(), sizeof(char) * filePath.Length() + 1);
            ImGui::EndDragDropSource();
        }
    };
    static const ImGuiTableSortSpecs* s_sort_specs = nullptr;
    static const auto SortEntryFunction = [this](const uint& a, const uint& b) -> bool
    {
        n_assert(s_sort_specs != nullptr);
        // weirdness sort passes us 0 sometimes.
        if (!this->files.Contains(a)|| !this->files.Contains(b))
        {
            return false;
        }
        const FileEntry& entryA = this->files[a];
        const FileEntry& entryB = this->files[b];
        const bool descending = s_sort_specs->Specs[0].SortDirection == ImGuiSortDirection_Descending ? true : false;

        if (s_sort_specs->SpecsCount != 1)
        {
            return descending ^ (entryA.name < entryB.name);
        }
        switch (s_sort_specs->Specs[0].ColumnIndex)
        {
            case 0:
                return descending ^ (entryA.name < entryB.name);
            case 1:
                return descending ^ (entryA.size < entryB.size);
            case 2:
                return descending ^ (entryA.modifiedTime < entryB.modifiedTime);
            default:
                return false;
        }
        return false;
    };
    
    if (this->activeFolder != -1)
    {
        bool doubleClicked = false;
        FileTreeNode& node = this->nodes[this->activeFolder];
        switch(this->fileViewMode)
        {
            case FileViewMode::List:
            {
                for (uint fileHash : node.files)
                {
                    const FileEntry& file = this->files[fileHash];
                    bool isSelected = (this->activeFile == fileHash);
                    if (!filter.IsEmpty() && !isSelected && !Util::String::MatchPattern(file.name, filter))
                    {
                        continue;
                    }
                    ImGui::PushID(fileHash);
                    if(ImGui::Selectable(file.name.AsCharPtr(), &isSelected))
                    {
                        this->activeFile = fileHash;
                    }
                    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
                    {
                        doubleClicked = true;
                    }
                    AddDragSourceForFileEntry(file);
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
                if (sorts_specs->SpecsDirty && node.files.Size() > 1)
                {
                    if (sorts_specs->SpecsCount != 1)
                    {
                        break;
                    }
                    s_sort_specs = sorts_specs;
                    std::sort(node.files.begin(), node.files.end(), SortEntryFunction);
                    sorts_specs->SpecsDirty = false;
                }
                
                for (uint fileHash : node.files)
                {
                    const FileEntry& file = this->files[fileHash];
                    bool isSelected = (this->activeFile == fileHash);
                    if (!filter.IsEmpty() && !isSelected && !Util::String::MatchPattern(file.name, filter))
                    {
                        continue;
                    }
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    if (ImGui::Selectable(file.name.AsCharPtr(), &isSelected))
                    {
                        this->activeFile = fileHash;

                    }
                    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
                    {
                        doubleClicked = true;
                    }
                    AddDragSourceForFileEntry(file);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d KB", (int)(file.size / 1024));
                    ImGui::TableNextColumn();  
                    Timing::CalendarTime cal = Timing::CalendarTime::FileTimeToSystemTime(file.modifiedTime);
                    ImGui::Text("%d-%d-%d %d:%d", cal.GetYear(), cal.GetMonth(), cal.GetDay(), cal.GetHour(), cal.GetMinute());
                }
                ImGui::EndTable();
                ImGui::EndGroup();
            }    
            break;
            case FileViewMode::Icons:
            {        
                ImGuiStyle& style = ImGui::GetStyle();
                int numFiles = node.files.Size();
                float windowVisibleX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                static int itemSize = 50;
                ImGui::SliderInt("Zoom", &itemSize, 25, 200);
                int n = 0;
                for (uint fileHash : node.files)
                {
                    const FileEntry& file = this->files[fileHash];
                    Util::String const& name = file.name;
                    ImGui::PushID(fileHash);
                    ImGui::BeginGroup();
                    //ImGui::ImageButton(name.AsCharPtr(), &Editor::UI::Icons::game, { (float)itemSize, (float)itemSize });
                    if(ImGui::Button("X", { (float)itemSize, (float)itemSize }))
                    {
                        this->activeFile = fileHash;
                    }
                    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
                    {
                        doubleClicked = true;
                    }
                    AddDragSourceForFileEntry(file);
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
                        this->activeFile = fileHash;
                        doubleClicked = true;
                    }
                    AddDragSourceForFileEntry(file);
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
        if (doubleClicked)
        {
            const FileEntry& file = this->files[this->activeFile];
            IO::URI uri = file.path;
            Ptr<AssetEditor> assetEditor = WindowServer::Instance()->GetWindow("Asset Editor").downcast<AssetEditor>();
            assetEditor->Open(uri.GetHostAndLocalPath(), FileEntryTypeToAssetType(file.type));
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
    }
    ImGui::SameLine();
    if (ImGui::Button("export"))
    {
        this->activeFileTree = "export";
    }
    ImGui::SameLine();
    ImGui::InputText("##search", buffer, NEBULA_MAXPATH, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
    
     
    ImGui::PopItemWidth();

    ImGui::Separator();
    ImGui::Columns(2);
    ImGui::BeginChild("ScrollingRegionFolders");
    this->DisplayFileTreeFolderHierarchy(this->nodes[this->fileTrees[this->activeFileTree].rootHash], 0);
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
AssetBrowser::ScanFolder(FileTree& tree)
{
    // Initialize the root node
    uint rootHash = tree.path.LocalPath().HashCode();
    tree.rootHash = rootHash;
    FileTreeNode& root = this->nodes.Emplace(rootHash);
    root.name = tree.path.LocalPath().ExtractFileName();
    root.path = tree.path;
    root.hash = rootHash;
    root.parentHash = -1; // No parent for root    

    IO::IoServer* ioServer = IO::IoServer::Instance();
    // Start recursive scanning from the root
    if (ioServer->DirectoryExists(tree.path))
    {
        ScanFolderRecursive(ioServer, tree.path, rootHash);
    }
    auto& node = this->nodes[rootHash];
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::ScanFolderRecursive(const IO::IoServer* ioServer, const IO::URI& folderPath, uint nodeHash)
{
    // List all files in the current directory
    Util::Array<Util::String> files = ioServer->ListFiles(folderPath, "*.*", true);
    for (const auto& fileName : files)
    {
        uint fileHash = fileName.HashCode();
        FileEntry& entry = this->files.Emplace(fileHash);
        this->nodes[nodeHash].files.Append(fileHash);
        entry.path = fileName;
        entry.name = fileName.ExtractFileName();
        entry.folder = nodeHash;
        
        // Extract file extension
        entry.extension = entry.name.GetFileExtension();
                        
        // Get file size by opening it       
        entry.size = IO::FSWrapper::GetFileSize(fileName);
        
        // Get file modification time
        entry.modifiedTime = IO::FSWrapper::GetFileWriteTime(fileName);
        
        // Determine file type
        entry.type = DetermineFileType(entry.extension);
    }
    
    // List all subdirectories and recursively scan them
    Util::Array<Util::String> directories = ioServer->ListDirectories(folderPath, "*", true, false);
    for (const auto& childDir : directories)
    {
        uint hash = childDir.HashCode();
        this->nodes[nodeHash].children.Append(hash);
        FileTreeNode& childNode = this->nodes.Emplace(hash);
        childNode.name = childDir.ExtractFileName();
        
        childNode.path = childDir;
        childNode.hash = hash;
        
        // Recursively scan the subdirectory
        ScanFolderRecursive(ioServer, childDir, hash);
    }
}

//------------------------------------------------------------------------------
/**
*/
AssetBrowser::FileEntry::Type
AssetBrowser::DetermineFileType(const Util::String& extension)
{
    if (extension.IsEmpty())
    {
        return FileEntry::Type::Text;
    }
    
    Util::String ext(extension);
    ext.ToLower();
    
    // Model files
    if (ext == "n3")
    {
        return FileEntry::Type::Model;
    }

    if (ext == "nvx2" || ext == "nvx")
    {
        return FileEntry::Type::Mesh;
    }
    
    // Texture files
    if (ext == "dds" || ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "tga" || ext == "bmp")
    {
        return FileEntry::Type::Texture;
    }
    
    // Surface/Material files
    if (ext == "sur" || ext == "material")
    {
        return FileEntry::Type::Surface;
    }
    
    // Audio files
    if (ext == "ogg" || ext == "wav" || ext == "mp3" || ext == "flac")
    {
        return FileEntry::Type::Audio;
    }
    
    // Skeleton files
    if (ext == "nsk")
    {
        return FileEntry::Type::Skeleton;
    }
    
    // Animation files
    if (ext == "nax")
    {
        return FileEntry::Type::Animation;
    }
    
    // Frame files
    if (ext == "json")
    {
        return FileEntry::Type::Frame;
    }
    
    // Shader files
    if (ext == "gplb" || ext == "gpul")
    {
        return FileEntry::Type::Shader;
    }
    
    // Physics files
    if (ext == "actor" || ext == "physics")
    {
        return FileEntry::Type::Physics;
    }
    
    // NavMesh files
    if (ext == "nav")
    {
        return FileEntry::Type::NavMesh;
    }
    
    // Default to Other for unknown extensions
    return FileEntry::Type::Other;
}
}
