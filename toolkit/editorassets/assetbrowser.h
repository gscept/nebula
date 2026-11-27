#pragma once
//------------------------------------------------------------------------------
/**
    @class  Editor::AssetBrowser

    A view of the asset index: the folder tree on the left, the files of the selected folder on the
    right, a fuzzy search over every asset and a type filter. Folders and files are renamed and new
    folders and assets made in place, with the name edited where it is shown.

    The asset browser panel and the asset picker each have one of these, over the same index. The
    view only holds what it shows: the folder, the selection, the search and the filter. What it
    knows about a type comes from the type registry, so it lists, makes and opens whatever the
    subsystems register.

    A file is dragged out as its URN, folder:file:type, under AssetDragPayload.

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editorassets/assetdatabase.h"
#include "neweditor/api/editorfeature.h" // AssetDragPayload, the payload a file is dragged out as
#include "io/path.h"
#include "util/array.h"
#include "util/string.h"
#include <functional>

namespace Editor
{

class Shell;
struct AssetType;

class AssetBrowser : public Core::RefCounted
{
    __DeclareClass(AssetBrowser)
public:
    /// what a double click does
    enum class Mode
    {
        Browse,         ///< opens the asset in its editor
        PickFile,       ///< picks the asset
        PickFolder      ///< picks the folder, files are only shown
    };

    AssetBrowser();
    virtual ~AssetBrowser();

    /// the shell whose status bar and queue the view uses
    void Setup(Shell* shell);
    /// draw the view into the current window
    void Draw();

    /// show the files to pick one from, starting in the folder of the given asset. With a type only
    /// assets of it are shown
    void StartPickFile(const IO::Path& start, const Util::String& type);
    /// show the folders to pick one from
    void StartPickFolder(const IO::Path& start);
    /// browse again, after a pick
    void StopPicking();
    /// the asset or folder picked since this was last asked
    bool ConsumePick(IO::Path& out);

    /// the selected asset, an empty path when nothing is selected
    IO::Path GetSelectedAsset() const;
    /// the selected folder, false when none is selected
    bool GetSelectedFolder(IO::Path& outFolder) const;

    /// open the name field on the selected folder or file, whichever was worked in last. Returns
    /// false when there is nothing to rename
    bool BeginRename();

private:
    enum class FileViewMode
    {
        List,
        Details,
        Icons
    };

    /// which half of the view was worked in last, which is what F2 renames in
    enum class Area
    {
        Folders,
        Files
    };

    /// inline name editing in the folder tree and the file view, nothing touches the disk until the name is committed with Enter
    enum class FolderEditMode
    {
        None,
        NewFolder,      ///< folderId is the parent the new folder will be created in
        Rename,         ///< folderId is the folder being renamed
        NewAsset,       ///< folderId is the folder the new asset of createType will be created in
        RenameFile      ///< fileId is the file being renamed in folderId, the name is edited without the extension
    };
    struct FolderEdit
    {
        FolderEditMode mode = FolderEditMode::None;
        uint64_t folderId = 0;
        uint64_t fileId = 0;
        ToolkitUtil::FileType assetType = ToolkitUtil::FileType::Unknown;
        /// the registered type and kind a new asset is made as
        Util::String createType;
        Util::String createVariant;
        char name[256] = { 0 };
        Util::String error;
        bool requestFocus = false;
        int focusRequestFrame = 0;
        bool openParent = false;
        bool scrollIntoView = false;
        bool wasActive = false;
        bool drawn = false;
        bool commitRequested = false;
    };

    /// read the files of the active folder again
    void RefreshFiles();
    /// set the active folder and read its files
    void SetActiveFolder(uint64_t folderId);
    ///
    void DisplayFileTreeFolderHierarchy(uint64_t folderId, int depth);
    ///
    void DisplaySelectedFolder(const Util::String& filter);
    /// draw the type filter button and its popup on the current line, filling the given width
    void DisplayTypeFilter(float width);
    /// true if the type filter is set and isn't overridden by the picker's own type
    bool IsTypeFilterActive() const;
    /// the right click menu of the file view
    void DisplayAssetActions();
    /// make a file a drag source of its URN
    void AddDragSource(const IO::Path& file);
    /// a double click on a file
    void ActivateFile(const AssetDatabase::FileInfo& file);
    /// open the queued asset in its editor, after the frame has drawn
    void OpenPending();

    /// start editing the name of a new, not yet created, subfolder of parentId
    void BeginNewFolder(uint64_t parentId);
    /// start editing the name of an existing folder
    void BeginRenameFolder(uint64_t folderId);
    /// start editing the name of a new, not yet created, asset in the folder, shown at the end of the file view
    void BeginNewAsset(uint64_t folderId, const AssetType& type, const Util::String& variant);
    /// start editing the name of a file in the folder shown
    void BeginRenameFile(uint64_t fileId);
    /// draw the inline name editor at the current cursor position, handles commit and cancel. A width of 0 fills the available space
    void DisplayFolderNameEditor(float width = 0.0f);
    /// draw a file view entry with the name editor in place of its name, for a new asset or a file being renamed
    void DisplayFileNameEditorEntry(ToolkitUtil::FileType type, float itemSize);
    /// try to apply the edited name, returns false and sets an error if the name can't be used
    bool CommitFolderEdit();
    /// drop the current edit without touching anything
    void CancelFolderEdit();
    /// import a model or texture asset again from its source files, with the settings it was imported with
    void ReimportAsset(const IO::Path& asset);

    Shell* shell;

    uint64_t activeFolder = 0;
    uint64_t activeFile = 0;
    /// the files of the active folder, and the index version they were read at
    Util::Array<AssetDatabase::FileInfo> files;
    uint filesVersion = ~0u;

    FileViewMode fileViewMode = FileViewMode::Details;
    /// the card size of the grid view
    int itemSize = 150;
    Area lastArea = Area::Files;

    FolderEdit folderEdit;
    /// folder the tree context menu was opened on
    uint64_t contextFolder = 0;
    bool showFolderMenu = false;

    Util::Array<AssetDatabase::FileInfo> searchResults;
    char searchFilter[512] = { 0 };
    Util::String lastSearchFilter;
    uint searchVersion = ~0u;

    /// one bit per ToolkitUtil::FileType of the types shown in the file view, all are shown when it's 0.
    /// Kept per view, so the picker's filter doesn't change what the browser shows
    uint32_t typeFilter = 0;

    Mode mode = Mode::Browse;
    /// the type of the files shown while picking one, all are shown when it's empty
    Util::String pickType;
    /// the folder a pick starts in, set once the index has it
    IO::Path pendingStartPath;
    bool hasPendingStart = false;
    IO::Path picked;
    bool hasPicked = false;
    /// the search field is focused when the picker opens, so typing filters right away
    bool focusSearch = false;

    /// the asset a double click opens once the frame has drawn
    IO::Path pendingOpen;
};

} // namespace Editor
