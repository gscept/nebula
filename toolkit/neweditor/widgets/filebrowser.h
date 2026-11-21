#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::FileBrowser

    The folder logic shared by the file browser window and by the file dialog: the folder being
    shown, the entries read from it, their order, the selection, and the two pieces those are drawn
    with, the address bar and the entry table.

    The folder is kept in assign form, 'work:' and 'work:scenes', so the address field says what the
    project calls the folder rather than a resolved path on disk. Listing resolves the form through
    the assign registry, and a typed path is mapped back to the assign with the longest root that
    covers it. The folder above an assign root is the assign its value sits in, which is what takes
    '..' from 'work:' to 'root:'.

    The folder is read into a flat array of plain entries, with the display strings computed while
    reading, so drawing only walks the array. MarkDirty defers the read until the table is next
    drawn, and RefreshNow reads straight away, which is what a command that wants the counts does
    without anything drawing. The listing is filtered by a file pattern, which is how a dialog asks
    for the files of one format.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "io/filetime.h"
#include "io/uri.h"
#include "util/array.h"
#include "util/string.h"

namespace Editor
{

/// The payload type of a file dragged out of the browser. It carries the path as text, and whatever
/// can
/// turn a file into something takes it: the viewport makes entities out of it, the outline places
/// the
/// entities on the row the drag is released on
static const char* const FileDragPayload = "nebula.file";

class Shell;

class FileBrowser final : public Core::RefCounted
{
    __DeclareClass(FileBrowser)
public:
    FileBrowser();

    /// the core whose status bar takes the message when a folder is refused
    void Setup(Shell* core);

    /// the folder being shown, in assign form
    const Util::String& GetDir() const;
    /// the file listing pattern, '*' when the browser shows every file
    void SetFilter(const Util::String& pattern);

    /// move to a folder. The path may be in assign form or a plain path; a path that is not a
    /// folder is
    /// refused, the old folder stays, and the status bar gets the message
    void Navigate(const Util::String& path);
    /// the folder above the given one, in assign form. The same string when there is no folder
    /// above
    static Util::String ParentDir(const Util::String& dir);
    /// the given name below the folder being shown, still in the folder's own form
    Util::String ChildPath(const Util::String& name) const;

    /// read the folder again before the next table draw
    void MarkDirty();
    /// read the folder now, which is what lets a command report the counts without drawing
    void RefreshNow();

    /// the selected entry by name, empty when there is none. Selection is one row and does not
    /// survive a
    /// folder change
    const Util::String& GetSelectedName() const;

    /// one entry in the folder. The display columns are computed when the folder is read rather
    /// than every
    /// frame, because the strings do not change between rebuilds
    struct Entry
    {
        Util::String name;
        bool isDir;
        uint64_t size;
        IO::FileTime modified;
        /// empty for a folder, like the file browsers leave it
        Util::String displaySize;
        Util::String displayType;
        Util::String displayModified;
    };
    /// the entries of the folder, folders first, in the sort order
    const Util::Array<Entry>& GetEntries() const;

    /// the name of the file double clicked since this was last asked. What a double click on a file
    /// does
    /// belongs to the drawer: the dialog takes the file, the browser window opens it as a scene
    Util::String ConsumeFileOpen();

    /// the name of the entry a right click landed on since this was last asked. What the menu does
    /// with it is
    /// the drawer's, because the browser window has a menu for an entry and the dialog does not
    Util::String ConsumeContextEntry();

    /// keep an entry selected. The menu applies to the entry it was opened on, and the footer shows
    /// it
    void SetSelectedName(const Util::String& name);

    /// open the rename field on an entry. Immediate mode has no element to get a commit from, so
    /// the field is
    /// drawn on the row it was opened on, and the drawer with a menu opens it from the menu item
    void OpenRenameField(const Util::String& name);

    /// delete one entry from the folder. A folder has to be emptied first, because
    /// IO::DeleteDirectory only
    /// removes an empty directory. Called from the action flush, which is where the filesystem is
    /// allowed to change
    bool DeleteEntry(const Util::String& name);

    /// rename one entry in the folder, on disk, and leave the listing on the new name
    bool RenameEntry(const Util::String& name, const Util::String& newName);

    /// the address bar: the up button and the field, Enter commits what was typed
    void DrawAddressBar();
    /// the table: the headers, the '..' row, and the entries with the clipper
    void DrawEntryTable();

private:
    // The sort columns, in table order. The sort itself runs through a plain function, because that
    // is what
    // Array::SortWithFunc takes, so the column and the direction it sorts by are read from the
    // file-static
    // pair the sort sets before it calls
    enum SortColumn
    {
        SortName,
        SortSize,
        SortType,
        SortModified,
    };

    /// read the folder into the entry array and sort it
    void ListDir();
    /// order the entries by the sort column. Folders always stay before files
    void SortEntries();
    /// true when lhs sorts before rhs in the column and direction the sort is running with
    static bool EntryLess(const Entry& lhs, const Entry& rhs);
    static void SetSortContext(IndexT sortColumn, bool descending);

    /// draw one header cell. A click sorts by the column, reversing on a second click
    void DrawHeader(IndexT columnIndex, const char* label, IndexT sortColumn);
    /// draw one row. Row 0 is always '..'
    void DrawRow(IndexT rowIndex);
    /// text drawn against the right edge of the column it is in
    static void DrawRightAligned(const Util::String& text);

    /// the rename field drawn on the row the entry is on, in place of the selectable
    void DrawRenameField(const Util::String& name);
    /// queue what the rename field committed and close the field
    void CommitRename(const Util::String& name);
    void ApplyRename();
    /// empty a folder before it is removed. The walk is iterative, and the folders are removed in
    /// the reverse
    /// order they were walked in, which is deepest first, so a folder is empty by the time its own
    /// turn comes
    bool DeleteTree(const IO::URI& dir);

    /// the address field is what a typed path goes through
    void CommitAddress();
    /// a path in assign form: what is typed stays typed when it has an assign, and a resolved path
    /// takes
    /// the assign with the longest root that covers it
    static Util::String AssignForm(const Util::String& path);
    /// what '..' on an assign root gives: the value the assign points to has an assign of its own
    /// in it, so
    /// 'work' -> 'root:work' -> 'root:', and a value that is just a filesystem path has no parent
    /// assign
    static Util::String ParentAssignDir(const Util::String& assignName);

    /// drop the trailing slashes, which is what the field and the command can both be handed
    static Util::String NormalizeDir(const Util::String& dir);

    /// the size column: '4.2 KB', and empty for a folder
    static Util::String SizeText(uint64_t size);
    /// the type column: 'Folder', or the extension in capitals, or 'File' when the name has no
    /// extension
    static Util::String TypeText(const Util::String& name, bool isDir);
    /// the modified column, local time
    static Util::String ModifiedText(const IO::FileTime& time);

    Shell* core;

    /// the folder being shown, in assign form
    Util::String dir;
    /// the file listing pattern the listings go through
    Util::String filter;

    /// the address field. Immediate mode needs a buffer that survives between frames. It is
    /// refilled from the
    /// folder only when the folder changed, so that what has been typed is not thrown away
    static const IndexT MaxAddressLength = 512;
    char addressBuffer[MaxAddressLength];
    bool addressDirty;

    /// the entries of the folder, folders first, in the sort order
    Util::Array<Entry> entries;
    /// the selected entry by name
    Util::String selectedName;

    SortColumn sortColumn;
    bool sortDescending;

    /// the folder has to be read again before the next draw
    bool entriesDirty;
    /// the file that was double clicked, waiting for the drawer to take it
    Util::String pendingFileOpen;
    /// the entry a right click landed on, waiting for the drawer's menu to take it
    Util::String contextName;

    /// the rename field. Immediate mode needs a buffer that survives between frames
    static const IndexT MaxNameLength = 128;
    char renameBuffer[MaxNameLength];
    /// the entry the field is open on. The row is found by name rather than by index, because a
    /// rebuild moves
    /// the rows around while the field is open
    Util::String renamingName;
    bool renameFocusRequested;

    /// the rename the field committed, waiting for the action flush
    Util::String pendingRenameName;
    Util::String pendingRenameNew;
};

} // namespace Editor
