//------------------------------------------------------------------------------
//  @file filebrowser.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/widgets/filebrowser.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/drawsurface.h"
#include "neweditor/base/iconregistry.h"
#include "io/assignregistry.h"
#include "io/ioserver.h"
#include "io/uri.h"
#include "timing/calendartime.h"
#include "util/stack.h"
#include "imgui.h"

namespace Editor
{

__ImplementClass(Editor::FileBrowser, 'FLBR', Core::RefCounted);

// The fixed columns. The name column takes the rest, which is where the long entries live
static const float SizeWidth = 90.0f;
static const float TypeWidth = 90.0f;
static const float ModifiedWidth = 130.0f;

// The column and direction the comparator sorts with. SortWithFunc takes a plain function, which
// cannot read
// the browser, so the sort hands the two over here before it runs
static int FileSortColumn = 0;            // a FileBrowser::SortColumn
static bool FileSortDescending = false;

//------------------------------------------------------------------------------
/**
*/
FileBrowser::FileBrowser() :
    core(nullptr),
    filter("*"),
    addressDirty(true),
    sortColumn(SortName),
    sortDescending(false),
    entriesDirty(true),
    renameFocusRequested(false)
{
    this->dir = "work:";
    this->addressBuffer[0] = 0;
    this->renameBuffer[0] = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowser::Setup(Shell* core)
{
    this->core = core;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
FileBrowser::GetDir() const
{
    return this->dir;
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowser::SetFilter(const Util::String& pattern)
{
    const Util::String effective = pattern.IsEmpty() ? "*" : pattern;
    if (effective != this->filter)
    {
        this->filter = effective;
        this->entriesDirty = true;
    }
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
FileBrowser::GetSelectedName() const
{
    return this->selectedName;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<FileBrowser::Entry>&
FileBrowser::GetEntries() const
{
    return this->entries;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
FileBrowser::ConsumeFileOpen()
{
    Util::String name = this->pendingFileOpen;
    this->pendingFileOpen.Clear();
    return name;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
FileBrowser::ConsumeContextEntry()
{
    Util::String name = this->contextName;
    this->contextName.Clear();
    return name;
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowser::SetSelectedName(const Util::String& name)
{
    this->selectedName = name;
}

//------------------------------------------------------------------------------
/**
    The field is opened from a menu item and drawn on the row the entry is on. The buffer is only
    filled when the field is opened, so opening it again on the entry it is already open on does not
    throw away what has been typed.
*/
void
FileBrowser::OpenRenameField(const Util::String& name)
{
    if (this->renamingName != name)
    {
        name.CopyToBuffer(this->renameBuffer, MaxNameLength);
        this->renamingName = name;
    }
    this->renameFocusRequested = true;
}

//------------------------------------------------------------------------------
/**
    One entry goes through the folder's own form, so a delete inside 'work:' does not reach the disk
    through a resolved path. A file is removed straight away; a folder has to be empty first, which
    is what DeleteTree makes it.
*/
bool
FileBrowser::DeleteEntry(const Util::String& name)
{
    const IO::URI path(this->ChildPath(name));
    if (!path.IsValid())
        return false;

    if (IO::DirectoryExists(path))
    {
        if (!this->DeleteTree(path))
            return false;
    }
    else if (!IO::DeleteFile(path))
    {
        return false;
    }

    this->entriesDirty = true;
    return true;
}

//------------------------------------------------------------------------------
/**
    Empty a folder and remove it. The walk is iterative, because a folder tree this deep is not
    worth the call stack. Files have no order to them, so they go as they are found; the folders are
    collected in the order the walk reaches them, which is a parent before its children, so removing
    them in reverse leaves every folder empty by the time its own turn comes.
*/
bool
FileBrowser::DeleteTree(const IO::URI& dir)
{
    Util::Stack<IO::URI> walk;
    Util::Array<IO::URI> folders;
    walk.Push(dir);

    while (!walk.IsEmpty())
    {
        const IO::URI current = walk.Pop();
        folders.Append(current);

        const Util::Array<Util::String> files = IO::ListFiles(current, "*");
        for (IndexT i = 0; i < files.Size(); i++)
        {
            IO::URI file = current;
            file.AppendLocalPath(files[i]);
            if (!IO::DeleteFile(file))
                return false;
        }

        const Util::Array<Util::String> dirs = IO::ListDirectories(current, "*");
        for (IndexT i = 0; i < dirs.Size(); i++)
        {
            IO::URI child = current;
            child.AppendLocalPath(dirs[i]);
            walk.Push(child);
        }
    }

    for (IndexT i = folders.Size() - 1; i >= 0; i--)
    {
        if (!IO::DeleteDirectory(folders[i]))
            return false;
    }

    return true;
}

//------------------------------------------------------------------------------
/**
    The rename is on the disk, through the folder's own form. The listing is left dirty so the new
    name comes back with the next draw, and the selection goes with it, because the row an entry has
    is found by name.
*/
bool
FileBrowser::RenameEntry(const Util::String& name, const Util::String& newName)
{
    const IO::URI from(this->ChildPath(name));
    const IO::URI to(this->ChildPath(newName));
    if (!from.IsValid() || !to.IsValid())
        return false;

    if (!IO::RenameFile(from, to))
        return false;

    this->selectedName = newName;
    this->entriesDirty = true;
    return true;
}

//------------------------------------------------------------------------------
/**
    What the field holds is queued rather than applied during the draw, because the rows are being
    walked when the commit comes in. A name that is empty, the same as the one the entry already
    has, or has a slash in it, is not a rename: the field just closes.
*/
void
FileBrowser::CommitRename(const Util::String& name)
{
    const Util::String newName(this->renameBuffer);
    this->renamingName.Clear();

    if (newName.IsEmpty() || newName == name || newName.FindCharIndex('/') != InvalidIndex)
        return;

    this->pendingRenameName = name;
    this->pendingRenameNew = newName;
    if (this->core != nullptr)
        this->core->QueueAction(Util::Delegate<void()>::FromMethod<FileBrowser, &FileBrowser::ApplyRename>(this));
}

//------------------------------------------------------------------------------
/**
    The rename is refused when the name is already there, which is the same rule the IO layer
    applies, so this is only for the message. The names are the ones the listing has, so the
    comparison is the exact one the engine uses rather than a case-insensitive FileExists.
*/
void
FileBrowser::ApplyRename()
{
    const Util::String name = this->pendingRenameName;
    const Util::String newName = this->pendingRenameNew;
    this->pendingRenameName.Clear();
    this->pendingRenameNew.Clear();
    if (name.IsEmpty() || this->core == nullptr)
        return;

    for (IndexT i = 0; i < this->entries.Size(); i++)
    {
        if (this->entries[i].name == newName)
        {
            this->core->SetStatus("An entry named " + newName + " already exists", 3.0f);
            return;
        }
    }

    if (this->RenameEntry(name, newName))
        this->core->SetStatus("Renamed " + name + " to " + newName, 3.0f);
    else
        this->core->SetStatus("Could not rename " + name, 3.0f);
}

//------------------------------------------------------------------------------
/**
    Read the folder into the entry array. The names come from the directory and file listings, and
    the stat columns come from one GetIOInfo per entry, which is what the detail columns are.
*/
void
FileBrowser::ListDir()
{
    this->entries.Clear();
    this->selectedName.Clear();

    const IO::URI base(this->dir);
    if (!base.IsValid() || !IO::DirectoryExists(base))
    {
        if (this->core != nullptr)
            this->core->SetStatus("Not a folder: " + this->dir, 3.0f);
        return;
    }

    const Util::Array<Util::String> dirs = IO::ListDirectories(base, "*");
    const Util::Array<Util::String> files = IO::ListFiles(base, this->filter);

    for (IndexT pass = 0; pass < 2; pass++)
    {
        const bool isDir = pass == 0;
        const Util::Array<Util::String>& names = isDir ? dirs : files;

        for (IndexT i = 0; i < names.Size(); i++)
        {
            const Util::String& name = names[i];

            // The listings leave the hidden entries in, which is not what the browser wants to show
            if (name.BeginsWithString("."))
                continue;

            Entry entry;
            entry.name = name;
            entry.isDir = isDir;

            // The child is read through the folder's own form, so a listing of 'work:' stays inside
            // the
            // assign rather than reaching the disk through a resolved path
            const IO::URI child(this->ChildPath(name));

            IO::IOStat info;
            if (IO::GetIOInfo(child, info, false))
            {
                entry.size = info.size;
                entry.modified = info.modifiedTime;
            }
            else
            {
                entry.size = 0;
            }

            entry.displaySize = isDir ? "" : SizeText(entry.size);
            entry.displayType = TypeText(name, isDir);
            entry.displayModified = ModifiedText(entry.modified);

            this->entries.Append(entry);
        }
    }

    this->SortEntries();
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowser::SortEntries()
{
    SetSortContext(this->sortColumn, this->sortDescending);
    this->entries.SortWithFunc(&FileBrowser::EntryLess);
}

//------------------------------------------------------------------------------
/**
    Folders before files whatever the direction, the way the file browsers sort. The direction flips
    only the order inside the two groups. Every column falls back to the name, so two entries that
    compare equal in the sort column keep a stable order between rebuilds.
*/
bool
FileBrowser::EntryLess(const Entry& lhs, const Entry& rhs)
{
    if (lhs.isDir != rhs.isDir)
        return lhs.isDir;

    int cmp = 0;
    switch (FileSortColumn)
    {
        case SortSize:
            if (lhs.size != rhs.size)
                cmp = lhs.size < rhs.size ? -1 : 1;
            break;
        case SortType:
            if (lhs.displayType != rhs.displayType)
                cmp = lhs.displayType < rhs.displayType ? -1 : 1;
            break;
        case SortModified:
            if (lhs.modified != rhs.modified)
                cmp = lhs.modified < rhs.modified ? -1 : 1;
            break;
        default:
            break;
    }

    if (cmp == 0)
    {
        // The name comparison is case insensitive, so 'Readme' and 'readme' sort next to each other
        Util::String a = lhs.name;
        Util::String b = rhs.name;
        a.ToLower();
        b.ToLower();
        if (a != b)
            cmp = a < b ? -1 : 1;
    }

    return FileSortDescending ? cmp > 0 : cmp < 0;
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowser::SetSortContext(IndexT sortColumn, bool descending)
{
    FileSortColumn = (int)sortColumn;
    FileSortDescending = descending;
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowser::MarkDirty()
{
    this->entriesDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowser::RefreshNow()
{
    this->ListDir();
    this->entriesDirty = false;
}

//------------------------------------------------------------------------------
/**
    A typed path may be in assign form or a plain path, and either may carry a trailing slash. The
    folder is stored in assign form, so a plain path that lives under an assign shows as the assign
    afterwards.
*/
void
FileBrowser::Navigate(const Util::String& path)
{
    const Util::String normalized = NormalizeDir(path);
    if (normalized.IsEmpty())
    {
        if (this->core != nullptr)
            this->core->SetStatus("No folder given", 3.0f);
        this->addressDirty = true;
        return;
    }

    const IO::URI uri(normalized);
    if (!uri.IsValid() || !IO::DirectoryExists(uri))
    {
        if (this->core != nullptr)
            this->core->SetStatus("Not a folder: " + normalized, 3.0f);
        this->addressDirty = true;
        return;
    }

    this->dir = AssignForm(normalized);
    this->addressDirty = true;
    this->entriesDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowser::CommitAddress()
{
    this->Navigate(Util::String(this->addressBuffer));
}

//------------------------------------------------------------------------------
/**
    'work:scenes/foo' gives 'work:scenes' and 'work:scenes' gives 'work:'. A folder that is exactly
    an assign, 'work:', has its parent in the assign its value sits in, and a plain path loses its
    last segment.
*/
Util::String
FileBrowser::ParentDir(const Util::String& dir)
{
    const IndexT colon = dir.FindCharIndex(':');

    // No assign in front, so this is a plain path: drop the segment after the last slash
    if (colon == InvalidIndex)
    {
        const IndexT slash = dir.FindCharIndexReverse('/');
        if (slash == InvalidIndex || slash == 0)
            return dir;
        return NormalizeDir(dir.ExtractRange(0, slash));
    }

    // The assign carries a path, so a folder below the assign root loses its last segment. One
    // segment below
    // the root, the parent is the root itself
    if (colon + 1 < (IndexT)dir.Length())
    {
        const IndexT slash = dir.FindCharIndexReverse('/');
        if (slash == InvalidIndex || slash < colon)
            return dir.ExtractRange(0, colon + 1);
        return NormalizeDir(dir.ExtractRange(0, slash));
    }

    const Util::String parent = ParentAssignDir(dir.ExtractRange(0, colon));
    return parent.IsEmpty() ? dir : parent;
}

//------------------------------------------------------------------------------
/**
    The assign a value sits in: 'work' -> 'root:work' -> 'root:'. A value that is a plain filesystem
    path, the way 'root' is, has no assign above it, and neither has a name that is not an assign.
*/
Util::String
FileBrowser::ParentAssignDir(const Util::String& assignName)
{
    if (!IO::AssignRegistry::HasInstance() || !IO::AssignRegistry::Instance()->IsValid())
        return "";

    const Util::String value = NormalizeDir(IO::AssignRegistry::Instance()->GetAssign(assignName));
    if (value.IsEmpty())
        return "";

    // The value sits in another assign when what stands before its colon is itself an assign
    const IndexT colon = value.FindCharIndex(':');
    if (colon == InvalidIndex || !IO::AssignRegistry::Instance()->HasAssign(value.ExtractRange(0, colon)))
        return "";

    // Drop the last segment of the part that follows the parent assign. 'root:work' has 'work' as
    // its only
    // segment, so dropping it leaves just 'root:'
    const Util::String rest = value.ExtractRange(colon + 1, value.Length() - (colon + 1));
    const IndexT slash = rest.FindCharIndexReverse('/');

    Util::String parent = value.ExtractRange(0, colon + 1);
    if (slash != InvalidIndex && slash > 0)
        parent.Append(rest.ExtractRange(0, slash));
    return parent;
}

//------------------------------------------------------------------------------
/**
    What is typed stays typed when it has an assign in front. A plain path takes the assign with the
    longest root that covers it, which is what shows 'work:scenes' rather than a resolved path on
    disk, and leaves a path that no assign covers alone.
*/
Util::String
FileBrowser::AssignForm(const Util::String& path)
{
    const IndexT colon = path.FindCharIndex(':');
    if (colon != InvalidIndex && !path.ExtractRange(0, colon).IsEmpty())
        return path;

    if (!IO::AssignRegistry::HasInstance() || !IO::AssignRegistry::Instance()->IsValid())
        return path;

    const Util::String resolved = IO::URI(path).LocalPath();
    if (resolved.IsEmpty())
        return path;

    Util::String bestName;
    Util::String bestRoot;

    const Util::Array<IO::Assign> assigns = IO::AssignRegistry::Instance()->GetAllAssigns();
    for (IndexT i = 0; i < assigns.Size(); i++)
    {
        const Util::String root = IO::URI(assigns[i].GetPath()).LocalPath();
        if (root.IsEmpty() || root.Length() >= resolved.Length())
            continue;

        if (resolved.ExtractRange(0, root.Length()) != root)
            continue;
        if (resolved[root.Length()] != '/')
            continue;

        if ((IndexT)root.Length() > (IndexT)bestRoot.Length())
        {
            bestName = assigns[i].GetName();
            bestRoot = root;
        }
    }

    if (bestName.IsEmpty())
        return path;

    Util::String display = bestName;
    display.AppendChar(':');
    display.Append(resolved.ExtractRange(bestRoot.Length() + 1, resolved.Length() - bestRoot.Length() - 1));
    return display;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
FileBrowser::ChildPath(const Util::String& name) const
{
    Util::String child = this->dir;
    if (!child.IsEmpty() && child[child.Length() - 1] != ':' && child[child.Length() - 1] != '/')
        child.AppendChar('/');
    child.Append(name);
    return child;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
FileBrowser::NormalizeDir(const Util::String& dir)
{
    Util::String result = dir;
    result.TrimRight("/");
    return result;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
FileBrowser::SizeText(uint64_t size)
{
    Util::String text;
    if (size < 1024)
        text.Format("%llu bytes", size);
    else if (size < 1024 * 1024)
        text.Format("%.1f KB", double(size) / 1024.0);
    else if (size < 1024 * 1024 * 1024)
        text.Format("%.1f MB", double(size) / (1024.0 * 1024.0));
    else
        text.Format("%.1f GB", double(size) / (1024.0 * 1024.0 * 1024.0));
    return text;
}

//------------------------------------------------------------------------------
/**
    The extension stands in for the type until the browser knows what each type is.
*/
Util::String
FileBrowser::TypeText(const Util::String& name, bool isDir)
{
    if (isDir)
        return "Folder";

    const IndexT dot = name.FindCharIndexReverse('.');
    if (dot != InvalidIndex && dot + 1 < (IndexT)name.Length())
    {
        Util::String ext = name.ExtractRange(dot + 1, name.Length() - (dot + 1));
        ext.ToUpper();
        return ext;
    }

    return "File";
}

//------------------------------------------------------------------------------
/**
*/
Util::String
FileBrowser::ModifiedText(const IO::FileTime& time)
{
    const Timing::CalendarTime local = Timing::CalendarTime::FileTimeToLocalTime(time);
    return Timing::CalendarTime::Format("{YEAR}-{MONTH}-{DAY} {HOUR}:{MINUTE}", local);
}

//------------------------------------------------------------------------------
/**
    The up button steps one folder, and Enter on the field commits what was typed. The field only
    takes the folder over when the folder changed, so what has been typed survives.
*/
void
FileBrowser::DrawAddressBar()
{
    if (IconButton(IconGlyph("arrow-up"), "go to the folder above"))
        this->Navigate(this->ParentDir(this->dir));
    ImGui::SameLine();

    if (this->addressDirty)
    {
        this->dir.CopyToBuffer(this->addressBuffer, MaxAddressLength);
        this->addressDirty = false;
    }

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::InputText("##address", this->addressBuffer, MaxAddressLength, ImGuiInputTextFlags_EnterReturnsTrue))
        this->CommitAddress();
    ImGui::PopItemWidth();
}

//------------------------------------------------------------------------------
/**
    The headers, the '..' row, and the entries. The folder is read here when it is marked dirty
    rather than when the mark is set, so a folder change that never gets drawn, a closed panel,
    costs no listing.
*/
void
FileBrowser::DrawEntryTable()
{
    if (this->entriesDirty)
    {
        this->ListDir();
        this->entriesDirty = false;
    }

    if (ImGui::BeginTable("entries", 4,
        ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingFixedFit))
    {
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("size", ImGuiTableColumnFlags_WidthFixed, SizeWidth);
        ImGui::TableSetupColumn("type", ImGuiTableColumnFlags_WidthFixed, TypeWidth);
        ImGui::TableSetupColumn("modified", ImGuiTableColumnFlags_WidthFixed, ModifiedWidth);

        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        this->DrawHeader(0, "Name", SortName);
        this->DrawHeader(1, "Size", SortSize);
        this->DrawHeader(2, "Type", SortType);
        this->DrawHeader(3, "Modified", SortModified);

        // Row 0 is always '..'
        ImGuiListClipper clipper;
        clipper.Begin(this->entries.Size() + 1);
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                this->DrawRow((IndexT)i);
        }

        ImGui::EndTable();
    }
}

//------------------------------------------------------------------------------
/**
    A header click sorts by its column: the first click ascending, the next one reversing, and
    another column starting ascending. The arrow is drawn next to the column it is sorting.
*/
void
FileBrowser::DrawHeader(IndexT columnIndex, const char* label, IndexT sortColumn)
{
    ImGui::TableSetColumnIndex(columnIndex);

    Util::String text = label;
    if ((IndexT)this->sortColumn == sortColumn)
        text.Append(this->sortDescending ? "  v" : "  ^");

    ImGui::TableHeader(text.AsCharPtr());
    if (ImGui::IsItemClicked())
    {
        if ((IndexT)this->sortColumn == sortColumn)
            this->sortDescending = !this->sortDescending;
        else
        {
            this->sortColumn = (SortColumn)sortColumn;
            this->sortDescending = false;
        }
        this->SortEntries();
    }
}

//------------------------------------------------------------------------------
/**
    A row is an icon and a selectable that spans the table, so a click anywhere on it lands on the
    entry. Double click enters a folder and double click on '..' goes up. A double click on a file
    is left to the drawer, because what a file is for is not the browser's business. A right click
    on an entry is reported to the drawer's menu, and an entry the rename field is open on draws the
    field instead of the selectable.
*/
void
FileBrowser::DrawRow(IndexT rowIndex)
{
    const bool parentRow = rowIndex == 0;
    const Entry* entry = parentRow ? nullptr : &this->entries[rowIndex - 1];

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    const bool isDir = parentRow || entry->isDir;

    const char* glyph = IconGlyph(isDir ? "folder" : "file");
    ImGui::TextUnformatted(glyph != nullptr ? glyph : (isDir ? "[]" : "<>"));
    ImGui::SameLine();

    const char* name = parentRow ? ".." : entry->name.AsCharPtr();
    const bool selected = !parentRow && entry->name == this->selectedName;

    // While the rename field is open it is the row, so the click and the drag that would have come
    // from the
    // selectable are not drawn
    const bool renaming = !parentRow && entry->name == this->renamingName;

    if (renaming)
    {
        this->DrawRenameField(entry->name);
    }
    else
    {
        ImGui::Selectable(name, selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            const bool doubleClick = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

            if (parentRow)
            {
                if (doubleClick)
                    this->Navigate(this->ParentDir(this->dir));
            }
            else if (doubleClick)
            {
                // A double click selects too, so the footer says which file was taken even though
                // the
                // action may move focus or add a tab before the footer draws
                this->selectedName = entry->name;
                if (entry->isDir)
                    this->Navigate(this->ChildPath(entry->name));
                else
                    this->pendingFileOpen = entry->name;
            }
            else
                this->selectedName = entry->name;
        }
        else if (!parentRow && ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            // The menu is a window of its own, so the entry it applies to has to be remembered
            // rather than read
            // from where the click landed. The right click selects as well, so the footer says
            // which entry the
            // menu is about while it is open
            this->contextName = entry->name;
            this->selectedName = entry->name;
        }
    }

    if (!parentRow && !entry->isDir && !renaming)
    {
        if (ImGui::BeginDragDropSource())
        {
            const Util::String path = this->ChildPath(entry->name);
            ImGui::SetDragDropPayload(FileDragPayload, path.AsCharPtr(), path.Length() + 1);

            ImGui::TextUnformatted(entry->name.AsCharPtr());

            ImGui::EndDragDropSource();
        }
    }

    ImGui::TableSetColumnIndex(1);
    DrawRightAligned(parentRow ? "" : entry->displaySize);

    ImGui::TableSetColumnIndex(2);
    if (!parentRow)
        ImGui::TextUnformatted(entry->displayType.AsCharPtr());

    ImGui::TableSetColumnIndex(3);
    if (!parentRow)
        ImGui::TextUnformatted(entry->displayModified.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
    The field is read after it is drawn, because the cell has to end with an item rather than with a
    cursor move. Escape is the field's own while it has focus: it puts the buffer back to what the
    field was opened with and deactivates it, which is the frame the field closes on, and the name
    it holds then is the one the entry already has, which is not a change.
*/
void
FileBrowser::DrawRenameField(const Util::String& name)
{
    if (this->renameFocusRequested)
    {
        ImGui::SetKeyboardFocusHere();
        this->renameFocusRequested = false;
    }

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
    const bool committed = ImGui::InputTextWithHint("##rename", "Name", this->renameBuffer, MaxNameLength, ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopItemWidth();

    if (committed || ImGui::IsItemDeactivated())
        this->CommitRename(name);
}

//------------------------------------------------------------------------------
/**
    The sizes read as a column, so they sit against the right edge of it rather than the left.
*/
void
FileBrowser::DrawRightAligned(const Util::String& text)
{
    const float textWidth = ImGui::CalcTextSize(text.AsCharPtr()).x;
    const float x = ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - textWidth - ImGui::GetStyle().CellPadding.x;
    if (x > ImGui::GetCursorPosX())
        ImGui::SetCursorPosX(x);
    ImGui::TextUnformatted(text.AsCharPtr());
}

} // namespace Editor
