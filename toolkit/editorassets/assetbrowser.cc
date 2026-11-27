//------------------------------------------------------------------------------
//  assetbrowser.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editorassets/assetbrowser.h"
#include "editorassets/assettype.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/drawsurface.h"
#include "neweditor/base/iconregistry.h"
#include "io/ioserver.h"
#include "imgui_internal.h"
#include "timing/calendartime.h"

namespace Editor
{

__ImplementClass(Editor::AssetBrowser, 'ASBR', Core::RefCounted);

//------------------------------------------------------------------------------
/**
    A glyph from the editor's icon table, empty when the table has no such name
*/
static const char*
Glyph(const char* name)
{
    const char* glyph = IconGlyph(name);
    return glyph != nullptr ? glyph : "";
}

//------------------------------------------------------------------------------
/**
    The registered type a FileDB type stands for, nullptr when no subsystem registered it
*/
static const AssetType*
RegisteredType(ToolkitUtil::FileType type)
{
    const char* urn = ToolkitUtil::FileTypeURNMapping[type];
    return urn[0] != '\0' ? AssetTypeRegistry::Find(Util::String(urn)) : nullptr;
}

//------------------------------------------------------------------------------
/**
    Icon shown in front of a file's name in the list and details views. A registered type brings
    its own, the others are the files no subsystem registers
*/
static const char*
FileTypeIcon(ToolkitUtil::FileType type)
{
    const AssetType* registered = RegisteredType(type);
    if (registered != nullptr && registered->icon.IsValid())
    {
        const char* glyph = IconGlyph(registered->icon.AsCharPtr());
        if (glyph != nullptr)
            return glyph;
    }

    switch (type)
    {
        case ToolkitUtil::FileType::Asset:      return Glyph("box");
        case ToolkitUtil::FileType::Texture:    return Glyph("image");
        case ToolkitUtil::FileType::Particle:   return Glyph("sparkles");
        case ToolkitUtil::FileType::Surface:    return Glyph("palette");
        case ToolkitUtil::FileType::Audio:      return Glyph("volume-2");
        case ToolkitUtil::FileType::Text:       return Glyph("file-text");
        case ToolkitUtil::FileType::Frame:      return Glyph("network");
        case ToolkitUtil::FileType::Shader:     return Glyph("file-code");
        case ToolkitUtil::FileType::Physics:    return Glyph("circle-dot");
        case ToolkitUtil::FileType::NavMesh:    return Glyph("map");
        default:                                return Glyph("file");
    }
}

//------------------------------------------------------------------------------
/**
    The name of a file's type, the registered one's when a subsystem registered it
*/
static const char*
FileTypeName(ToolkitUtil::FileType type)
{
    const AssetType* registered = RegisteredType(type);
    if (registered != nullptr)
        return registered->name.AsCharPtr();
    return ToolkitUtil::FileTypeNames[type];
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
        ImGui::SetTooltip("%s", FileTypeName(type));
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
AssetBrowser::AssetBrowser() :
    shell(nullptr)
{
    // empty
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
AssetBrowser::Setup(Shell* shell)
{
    this->shell = shell;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::StartPickFile(const IO::Path& start, const Util::String& type)
{
    this->mode = Mode::PickFile;
    this->pickType = type;
    this->pendingStartPath = start.IsFile() ? IO::Path::Folder(start.GetFolder()) : start;
    this->hasPendingStart = true;
    this->hasPicked = false;
    this->searchFilter[0] = '\0';
    this->focusSearch = true;
    this->CancelFolderEdit();
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::StartPickFolder(const IO::Path& start)
{
    this->mode = Mode::PickFolder;
    this->pickType.Clear();
    this->pendingStartPath = start.IsFile() ? IO::Path::Folder(start.GetFolder()) : start;
    this->hasPendingStart = true;
    this->hasPicked = false;
    this->CancelFolderEdit();
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::StopPicking()
{
    this->mode = Mode::Browse;
    this->pickType.Clear();
    this->hasPicked = false;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetBrowser::ConsumePick(IO::Path& out)
{
    if (!this->hasPicked)
        return false;
    out = this->picked;
    this->hasPicked = false;
    return true;
}

//------------------------------------------------------------------------------
/**
    The selection can be a search result from another folder, so the results are looked in as well
*/
IO::Path
AssetBrowser::GetSelectedAsset() const
{
    if (this->activeFile == 0)
        return IO::Path();
    for (const auto& file : this->files)
    {
        if (file.id == this->activeFile)
            return file.filePath;
    }
    for (const auto& file : this->searchResults)
    {
        if (file.id == this->activeFile)
            return file.filePath;
    }
    return IO::Path();
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetBrowser::GetSelectedFolder(IO::Path& outFolder) const
{
    AssetDatabase* db = AssetDatabase::HasInstance() ? AssetDatabase::Instance() : nullptr;
    if (db == nullptr || !db->HasFolder(this->activeFolder))
        return false;
    outFolder = db->GetFolder(this->activeFolder).folderPath;
    return true;
}

//------------------------------------------------------------------------------
/**
    Names are edited where they are shown, so a file is only renamed in its own folder, not in the
    search results.
*/
bool
AssetBrowser::BeginRename()
{
    if (this->mode != Mode::Browse || this->folderEdit.mode != FolderEditMode::None)
        return false;

    if (this->lastArea == Area::Folders)
    {
        if (this->activeFolder == 0)
            return false;
        this->BeginRenameFolder(this->activeFolder);
    }
    else
    {
        if (this->activeFile == 0 || this->searchFilter[0] != '\0')
            return false;
        this->BeginRenameFile(this->activeFile);
    }
    return this->folderEdit.mode != FolderEditMode::None;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::RefreshFiles()
{
    AssetDatabase* db = AssetDatabase::Instance();
    if (this->activeFolder != 0)
        db->GetFiles(this->activeFolder, this->files);
    else
        this->files.Clear();
    this->filesVersion = db->GetVersion();
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::SetActiveFolder(uint64_t folderId)
{
    if (this->activeFolder == folderId)
        return;
    this->activeFolder = folderId;
    this->activeFile = 0;
    this->RefreshFiles();
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::Draw()
{
    AssetDatabase* db = AssetDatabase::HasInstance() ? AssetDatabase::Instance() : nullptr;
    if (db == nullptr || db->GetRootFolder() == 0)
    {
        ImGui::TextDisabled("The asset index could not be opened");
        return;
    }

    // a rename in the other view, or a folder gone from the disk, can drop the active folder
    if (this->activeFolder != 0 && !db->HasFolder(this->activeFolder))
    {
        this->activeFolder = 0;
        this->activeFile = 0;
        this->files.Clear();
    }

    if (this->hasPendingStart)
    {
        // the folder may not be in the index until the scan reaches it, the root is shown until then
        const uint64_t startFolder = db->FindFolder(this->pendingStartPath);
        if (startFolder != 0)
        {
            this->SetActiveFolder(startFolder);
            this->hasPendingStart = false;
        }
        else if (!db->IsScanning())
        {
            this->SetActiveFolder(db->GetRootFolder());
            this->hasPendingStart = false;
        }
        else if (this->activeFolder == 0)
        {
            this->SetActiveFolder(db->GetRootFolder());
        }
    }

    if (db->GetVersion() != this->filesVersion)
        this->RefreshFiles();

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
    if (db->IsScanning())
    {
        const float progress = db->GetScanProgress();
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
    if (this->mode == Mode::PickFile && !this->pickType.IsEmpty())
        ImGui::Text("Search (only %s files are shown)", this->pickType.AsCharPtr());
    else
        ImGui::TextUnformatted("Search");
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

    // the buttons that end a pick sit above the views, so they are there whatever is scrolled
    if (this->mode == Mode::PickFolder)
    {
        if (ImGui::Button("Select folder"))
        {
            // pick the folder selected in the tree, the root when none is
            const uint64_t folderId = this->activeFolder != 0 ? this->activeFolder : db->GetRootFolder();
            this->picked = db->GetFolder(folderId).folderPath;
            this->hasPicked = true;
        }
    }
    else if (this->mode == Mode::PickFile)
    {
        const IO::Path selected = this->GetSelectedAsset();
        ImGui::BeginDisabled(!selected.IsFile());
        if (ImGui::Button("Select"))
        {
            this->picked = selected;
            this->hasPicked = true;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextDisabled("or double click an asset");
    }

    ImGui::Separator();
    ImGui::Columns(2);
    ImGui::BeginChild("ScrollingRegionFolders");
    this->folderEdit.drawn = false;
    this->DisplayFileTreeFolderHierarchy(db->GetRootFolder(), 0);
    if (ImGui::IsWindowFocused())
        this->lastArea = Area::Folders;
    ImGui::EndChild();

    ImGui::NextColumn();
    if (IconButton(Glyph("table"), "Details"))
    {
        this->fileViewMode = FileViewMode::Details;
    }
    ImGui::SameLine();
    if (IconButton(Glyph("list"), "List"))
    {
        this->fileViewMode = FileViewMode::List;
    }
    ImGui::SameLine();
    if (IconButton(Glyph("layout-grid"), "Grid"))
    {
        this->fileViewMode = FileViewMode::Icons;
    }
    ImGui::BeginChild("ScrollingRegionFiles");
    this->DisplaySelectedFolder(this->searchFilter);
    if (ImGui::IsWindowFocused())
        this->lastArea = Area::Files;
    ImGui::EndChild();
    // the menu belongs to the file view, the child window is the last item
    this->DisplayAssetActions();

    if (this->folderEdit.commitRequested)
    {
        // committed here and not while drawing, as it changes the folder and file lists the views are iterating
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

    if (this->showFolderMenu)
    {
        ImGui::OpenPopup("folder_menu");
        this->showFolderMenu = false;
    }

    if (ImGui::BeginPopup("folder_menu"))
    {
        const bool hasContextFolder = db->HasFolder(this->contextFolder);
        const bool isRootFolder = this->contextFolder == db->GetRootFolder();
        const bool isArchive = hasContextFolder && db->GetFolder(this->contextFolder).isArchive;
        const bool canEdit = hasContextFolder && !isArchive && this->mode != Mode::PickFile;
        if (ImGui::MenuItem("New Folder", nullptr, false, canEdit))
        {
            this->BeginNewFolder(this->contextFolder);
        }
        if (ImGui::MenuItem("Rename", "F2", false, canEdit && !isRootFolder))
        {
            this->BeginRenameFolder(this->contextFolder);
        }
        ImGui::EndPopup();
    }
    ImGui::Columns(1);
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::DisplayFileTreeFolderHierarchy(uint64_t folderId, int depth)
{
    AssetDatabase* db = AssetDatabase::Instance();
    if (!db->HasFolder(folderId))
        return;
    const AssetDatabase::FolderInfo info = db->GetFolder(folderId);

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
    // the root is the assets folder, the FileDB names it after nothing in particular
    const char* label = depth == 0 ? "assets" : info.name.AsCharPtr();
    bool bIsOpen = ImGui::TreeNodeEx("##folder", flags, "%s %s", Glyph("folder"), renaming ? "" : label);
    ImGui::PopID();
    if (ImGui::IsItemClicked())
    {
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && this->mode == Mode::PickFolder)
        {
            this->picked = info.folderPath;
            this->hasPicked = true;
        }
        else
        {
            this->SetActiveFolder(folderId);
        }
    }
    else if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        this->showFolderMenu = true;
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
        // a copy, as reading an unscanned child scans it and changes the index
        Util::Array<uint64_t> children;
        db->GetChildFolders(folderId, children);
        for (uint64_t child : children)
        {
            this->DisplayFileTreeFolderHierarchy(child, depth + 1);
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
*/
bool
AssetBrowser::IsTypeFilterActive() const
{
    // a picker asking for one type shows only that one, the filter can't widen or narrow it
    const bool pickerRestricts = this->mode == Mode::PickFile && !this->pickType.IsEmpty();
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
    const bool pickerRestricts = this->mode == Mode::PickFile && !this->pickType.IsEmpty();
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
            label = Util::Format("%s %s", FileTypeIcon(single), FileTypeName(single));
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
        const Util::String clearLabel = Util::Format("%s##cleartypefilter", Glyph("x"));
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
            const Util::String itemLabel = Util::Format("%s  %s", FileTypeIcon(type), FileTypeName(type));
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
AssetBrowser::AddDragSource(const IO::Path& file)
{
    // only an asset has a URN to drop somewhere
    if (!file.IsFile() || file.GetType().IsEmpty())
        return;

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        const Util::String urn = file.AsString();
        ImGui::SetDragDropPayload(AssetDragPayload, urn.AsCharPtr(), urn.Length() + 1);
        ImGui::TextUnformatted(urn.AsCharPtr());
        ImGui::EndDragDropSource();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::ActivateFile(const AssetDatabase::FileInfo& file)
{
    switch (this->mode)
    {
        case Mode::PickFile:
            this->picked = file.filePath;
            this->hasPicked = true;
            break;
        case Mode::PickFolder:
            // only folders are picked, the files are shown to tell the folders apart
            break;
        case Mode::Browse:
        {
            const AssetType* type = AssetTypeRegistry::Find(file.filePath);
            if (type == nullptr || !type->open.IsValid())
            {
                if (this->shell != nullptr)
                    this->shell->SetStatus(Util::Format("No editor opens %s files yet", FileTypeName(file.type)), 3.0f);
                break;
            }
            // the editor opens a tab, which waits until the panels have drawn
            this->pendingOpen = file.filePath;
            if (this->shell != nullptr)
                this->shell->QueueAction(Util::Delegate<void()>::FromMethod<AssetBrowser, &AssetBrowser::OpenPending>(this));
            break;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::OpenPending()
{
    const AssetType* type = AssetTypeRegistry::Find(this->pendingOpen);
    if (type != nullptr && type->open.IsValid())
        type->open(this->pendingOpen);
    this->pendingOpen = IO::Path();
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::DisplaySelectedFolder(const Util::String& filter)
{
    static const ImGuiTableSortSpecs* s_sort_specs = nullptr;
    AssetDatabase* db = AssetDatabase::Instance();

    const bool searching = !filter.IsEmpty();
    if (searching && (this->searchVersion != db->GetVersion() || this->lastSearchFilter != filter))
    {
        const Util::Array<AssetDatabase::FileInfo>& searchIndex = db->GetSearchIndex();
        this->searchResults.Clear();
        if (filter.FindCharIndex('*') != InvalidIndex || filter.FindCharIndex('?') != InvalidIndex)
        {
            // a wildcard pattern matches as it's written
            for (const auto& file : searchIndex)
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
            for (IndexT i = 0; i < searchIndex.Size(); i++)
            {
                const auto& file = searchIndex[i];
                int score = 0;
                if (FuzzyMatch(filter, file.name, file.filePath.GetFolderAndFile(), score))
                {
                    ranked.Append({ score, i });
                }
            }
            std::sort(ranked.begin(), ranked.end(), [&searchIndex](const Util::KeyValuePair<int, IndexT>& a, const Util::KeyValuePair<int, IndexT>& b)
            {
                if (a.Key() != b.Key())
                    return a.Key() > b.Key();
                return searchIndex[a.Value()].name < searchIndex[b.Value()].name;
            });
            this->searchResults.Reserve(ranked.Size());
            for (const auto& entry : ranked)
            {
                this->searchResults.Append(searchIndex[entry.Value()]);
            }
        }
        this->lastSearchFilter = filter;
        this->searchVersion = db->GetVersion();
    }

    if (!searching && this->activeFolder == 0)
    {
        return;
    }

    // names are edited in the folder they belong to, search results don't show the editor
    const bool editingInFolder = !searching && this->folderEdit.folderId == this->activeFolder;
    const bool newAssetShown = editingInFolder && this->folderEdit.mode == FolderEditMode::NewAsset;
    const uint64_t renamedFile = editingInFolder && this->folderEdit.mode == FolderEditMode::RenameFile ? this->folderEdit.fileId : 0;

    const AssetDatabase::FileInfo* fileToOpen = nullptr;

    Util::Array<AssetDatabase::FileInfo> visibleFiles = searching ? this->searchResults : this->files;
    if (this->mode == Mode::PickFile && !this->pickType.IsEmpty())
    {
        // picking an asset of a type, the others can't be picked. One exported as the type is
        // listed too, a model where a skeleton is picked
        IndexT write = 0;
        for (IndexT read = 0; read < visibleFiles.Size(); read++)
        {
            if (AssetTypeRegistry::Provides(visibleFiles[read].filePath.GetType(), this->pickType))
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

    switch (this->fileViewMode)
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
                    fileToOpen = &file;
                }
                this->AddDragSource(file.filePath);
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
            if (ImGui::BeginTable("##filedetails", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg))
            {
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
                    std::sort(visibleFiles.begin(), visibleFiles.end(), [descending](const AssetDatabase::FileInfo& a, const AssetDatabase::FileInfo& b)
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
                    // the files are copied from the index every frame, so they are sorted every frame as well
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
                            fileToOpen = &file;
                        }
                        this->AddDragSource(file.filePath);
                        ImGui::PopID();
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
                    ImGui::TextUnformatted(Base::CalendarTimeBase::Format("{YEAR}/{MONTH}/{DAY} {HOUR}:{MINUTE}", cal).AsCharPtr());
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
            }
            ImGui::EndGroup();
            break;
        }
        case FileViewMode::Icons:
        {
            ImGuiStyle& style = ImGui::GetStyle();
            int numFiles = visibleFiles.Size() + (newAssetShown ? 1 : 0);
            float windowVisibleX = ImGui::GetWindowPos().x + ImGui::GetContentRegionAvail().x;
            ImGui::SliderInt("Zoom", &this->itemSize, 25, 200);
            int n = 0;
            const float itemSize = (float)this->itemSize;
            const auto sameLineIfFits = [&]()
            {
                float lastButtonX = ImGui::GetItemRectMax().x;
                float nextButtonX = lastButtonX + style.ItemSpacing.x + itemSize + 30.0f; // Expected position if next button was on same line
                if (n + 1 < numFiles && nextButtonX < windowVisibleX)
                    ImGui::SameLine();
                n++;
            };
            for (const auto& file : visibleFiles)
            {
                if (file.id == renamedFile)
                {
                    this->DisplayFileNameEditorEntry(file.type, itemSize);
                    sameLineIfFits();
                    continue;
                }
                Util::String const& name = file.name;
                ImGui::BeginGroup();
                ImGui::PushID(reinterpret_cast<void*>(static_cast<uintptr_t>(file.id)));

                ImVec2 pos = ImGui::GetCursorScreenPos();

                if (ImGui::InvisibleButton(name.AsCharPtr(), { itemSize, itemSize }))
                {
                    this->activeFile = file.id;
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                {
                    this->activeFile = file.id;
                }

                const bool hovered = ImGui::IsItemHovered();
                if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    this->activeFile = file.id;
                    fileToOpen = &file;
                }

                this->AddDragSource(file.filePath);

                DrawFileCard(ImGui::GetWindowDrawList(), pos, itemSize, FileTypeName(file.type), name.AsCharPtr(), hovered || this->activeFile == file.id);
                ImGui::PopID();

                if (hovered)
                {
                    ImGui::SetTooltip("%s", name.AsCharPtr());
                }
                ImGui::EndGroup();
                sameLineIfFits();
            }
            if (newAssetShown)
            {
                this->DisplayFileNameEditorEntry(this->folderEdit.assetType, itemSize);
            }
            break;
        }
        default:
            break;
    }

    if (fileToOpen != nullptr)
    {
        this->ActivateFile(*fileToOpen);
    }
}

//------------------------------------------------------------------------------
/**
    The new assets are the registered types that can be made from nothing, a type with kinds gets a
    menu of them. The rest acts on the selected file.
*/
void
AssetBrowser::DisplayAssetActions()
{
    if (this->activeFolder == 0 || this->mode != Mode::Browse)
        return;

    if (!ImGui::BeginPopupContextItem("AssetActions"))
        return;

    AssetDatabase* db = AssetDatabase::Instance();
    const bool canCreate = db->HasFolder(this->activeFolder) && !db->GetFolder(this->activeFolder).isArchive;

    // the asset is created once its name is committed with Enter
    bool anyCreatable = false;
    for (const AssetType& type : AssetTypeRegistry::GetTypes())
    {
        if (!type.create.IsValid())
            continue;
        anyCreatable = true;

        const Util::String label = Util::Format("%s  New %s", Glyph(type.icon.AsCharPtr()), type.name.AsCharPtr());
        Util::Array<Util::String> variants;
        if (type.variants.IsValid())
            type.variants(variants);

        if (variants.IsEmpty())
        {
            if (ImGui::MenuItem(label.AsCharPtr(), nullptr, false, canCreate))
                this->BeginNewAsset(this->activeFolder, type, Util::String());
        }
        else if (ImGui::BeginMenu(label.AsCharPtr(), canCreate))
        {
            for (const Util::String& variant : variants)
            {
                if (ImGui::MenuItem(variant.AsCharPtr()))
                    this->BeginNewAsset(this->activeFolder, type, variant);
            }
            ImGui::EndMenu();
        }
    }
    if (!anyCreatable)
        ImGui::MenuItem("No asset type can be made here yet", nullptr, false, false);

    // the rest acts on the selected file, which right clicking it selected
    const AssetDatabase::FileInfo* contextFile = nullptr;
    for (const auto& file : this->files)
    {
        if (file.id == this->activeFile)
        {
            contextFile = &file;
            break;
        }
    }
    const AssetType* contextType = contextFile != nullptr ? AssetTypeRegistry::Find(contextFile->filePath) : nullptr;
    const bool isAsset = contextFile != nullptr && contextFile->filePath.IsFile() && !contextFile->filePath.GetType().IsEmpty();

    ImGui::Separator();
    if (ImGui::MenuItem("Rename", "F2", false, canCreate && contextFile != nullptr && this->searchFilter[0] == '\0'))
    {
        this->BeginRenameFile(this->activeFile);
    }
    if (ImGui::MenuItem("Copy URN", "Ctrl+C", false, isAsset))
    {
        ImGui::SetClipboardText(contextFile->filePath.AsString().AsCharPtr());
    }
    // assets imported from source files can be imported again from them
    const bool canReimport = canCreate && contextType != nullptr && contextType->reimport.IsValid();
    if (ImGui::MenuItem("Reimport", nullptr, false, canReimport))
    {
        this->ReimportAsset(contextFile->filePath);
    }
    ImGui::EndPopup();
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
        DrawFileCard(ImGui::GetWindowDrawList(), ImGui::GetCursorScreenPos(), itemSize, FileTypeName(type), nullptr, true);
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
    The default name is the type's, numbered when it's already taken
*/
void
AssetBrowser::BeginNewAsset(uint64_t folderId, const AssetType& type, const Util::String& variant)
{
    AssetDatabase* db = AssetDatabase::Instance();
    if (!db->HasFolder(folderId))
    {
        return;
    }
    const AssetDatabase::FolderInfo info = db->GetFolder(folderId);
    if (info.isArchive)
    {
        return;
    }

    Util::String defaultName = Util::Format("new_%s", type.name.AsCharPtr());
    defaultName.ToLower();
    defaultName.SubstituteChar(' ', '_');
    Util::String name = defaultName;
    IndexT uniqueCounter = 2;
    while (IO::FileExists(IO::Path::File(info.folderPath.GetFolder(), name, type.type).WorkURI("assets")))
    {
        name.Format("%s_%d", defaultName.AsCharPtr(), uniqueCounter++);
    }

    this->folderEdit = FolderEdit();
    this->folderEdit.mode = FolderEditMode::NewAsset;
    this->folderEdit.folderId = folderId;
    this->folderEdit.assetType = AssetDatabase::FileTypeOf(type.type);
    this->folderEdit.createType = type.type;
    this->folderEdit.createVariant = variant;
    this->folderEdit.requestFocus = true;
    this->folderEdit.scrollIntoView = true;
    this->activeFile = 0;
    name.CopyToBuffer(this->folderEdit.name, sizeof(this->folderEdit.name));
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::BeginRenameFile(uint64_t fileId)
{
    AssetDatabase* db = AssetDatabase::Instance();
    if (!db->HasFolder(this->activeFolder) || db->GetFolder(this->activeFolder).isArchive)
    {
        return;
    }
    for (const auto& file : this->files)
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
    The type imports it again with the settings it was imported with, and may let them be changed
    first
*/
void
AssetBrowser::ReimportAsset(const IO::Path& asset)
{
    const AssetType* type = AssetTypeRegistry::Find(asset);
    if (type == nullptr || !type->reimport.IsValid())
        return;

    Util::String error;
    if (!type->reimport(asset, error) && this->shell != nullptr)
    {
        this->shell->Log(LogMessageType::N_WARNING, Util::Format("Could not reimport '%s': %s", asset.GetFolderAndFile().AsCharPtr(), error.AsCharPtr()));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::BeginNewFolder(uint64_t parentId)
{
    AssetDatabase* db = AssetDatabase::Instance();
    if (!db->HasFolder(parentId))
    {
        return;
    }
    const IO::Path parentPath = db->GetFolder(parentId).folderPath;

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
    AssetDatabase* db = AssetDatabase::Instance();
    if (folderId == 0 || folderId == db->GetRootFolder() || !db->HasFolder(folderId))
    {
        return;
    }
    const AssetDatabase::FolderInfo info = db->GetFolder(folderId);
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
    AssetDatabase* db = AssetDatabase::Instance();
    Util::String name = this->folderEdit.name;
    name.Trim(" \t");

    Util::String error;
    const bool newAsset = this->folderEdit.mode == FolderEditMode::NewAsset;
    const bool renameFile = this->folderEdit.mode == FolderEditMode::RenameFile;
    bool success = AssetDatabase::ValidateName(name, newAsset ? "asset" : (renameFile ? "file" : "folder"), error);
    const bool hasType = newAsset || ToolkitUtil::FileTypeURNMapping[this->folderEdit.assetType][0] != '\0';
    if (success && (newAsset || renameFile) && hasType && name.FindCharIndex('.') != InvalidIndex)
    {
        // paths strip everything after a period as the file extension, which an asset gets from its type
        error = "An asset name can't contain a period";
        success = false;
    }
    if (success)
    {
        if (this->folderEdit.mode == FolderEditMode::NewFolder)
        {
            uint64_t folderId = 0;
            success = db->CreateFolder(this->folderEdit.folderId, name, folderId, error);
            if (success)
                this->SetActiveFolder(folderId);
        }
        else if (this->folderEdit.mode == FolderEditMode::Rename)
        {
            // the ids below the renamed folder change, so a selection in there moves to the new one
            const bool selectionInside = db->IsInFolder(this->activeFolder, this->folderEdit.folderId);
            uint64_t folderId = 0;
            success = db->RenameFolder(this->folderEdit.folderId, name, folderId, error);
            if (success && selectionInside)
            {
                this->activeFolder = 0;
                this->SetActiveFolder(folderId);
            }
        }
        else if (newAsset)
        {
            const AssetType* type = AssetTypeRegistry::Find(this->folderEdit.createType);
            uint64_t fileId = 0;
            if (type == nullptr)
            {
                error.Format("'%s' assets are no longer registered", this->folderEdit.createType.AsCharPtr());
                success = false;
            }
            else
            {
                success = db->CreateAsset(this->folderEdit.folderId, *type, this->folderEdit.createVariant, name, fileId, error);
            }
            if (success)
            {
                this->RefreshFiles();
                this->activeFile = fileId;
            }
        }
        else if (renameFile)
        {
            uint64_t fileId = 0;
            success = db->RenameFile(this->folderEdit.fileId, name, fileId, error);
            if (success)
            {
                this->RefreshFiles();
                this->activeFile = fileId;
            }
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

} // namespace Editor
