#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::AssetBrowser

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/ui/window.h"
#include "io/filetime.h"
#include "util/array.h"
#include "util/dictionary.h"
#include "util/string.h"
#include "io/uri.h"
#include "io/path.h"
#include "filedb/filedb.h"
#include "threading/safeflag.h"
#include "threading/safequeue.h"
#include "io/filewatcher.h"

namespace ToolkitUtil
{
class ModelAssetT;
class TextureResourceT;
}
namespace Presentation
{

class ScanFolderJob;
class AssetEditor;
class AssetBrowser : public BaseWindow
{
public:
    /// Constructor
    AssetBrowser();

    /// Destructor
    ~AssetBrowser();


    void Update();
    void Run(SaveMode save) override;

    /// Pick file (using assigns, like work:system/white)
    void PickFile(const IO::Path& path, std::function<void(const IO::Path& path)> picker);
    /// Pick folder
    void PickFolder(const IO::Path& path, std::function<void(const IO::Path& path)> picker);

private:
    
    void DisplayFileTree();

private:
    static const uint MaterialHash = "sur"_hash;
    static const uint ModelHash = "n3"_hash;
    static const uint MeshHash = "nvx"_hash;
    static const uint SkeletonHash = "nsk"_hash;
    static const uint TextureHash = "dds"_hash;

    enum class FileViewMode
    {
        List,
        Details,
        Icons
    };
        
    uint64_t activeFolder = 0;
    uint64_t activeFile = 0;
    uint64_t activeFileTree = 0;

    FileViewMode fileViewMode = FileViewMode::Details;
    ///
    void DisplayFileTreeFolderHierarchy(uint64_t folderId, int depth);
    ///
    void DisplaySelectedFolder(const Util::String& filter);
    /// Determine file type from file extension
    static ToolkitUtil::FileType DetermineFileType(const Util::String& extension);
    /// Recursively scan a directory and sync entries to FileDB
    void ScanFolder(ToolkitUtil::FileDB& fileDB, const IO::URI& folderPath, bool useArchive, uint64_t parent, bool recursive);
    /// replace this folder's files in the search index after a disk scan
    void IndexFolderForSearch(uint64_t folderId, const Util::Array<ToolkitUtil::FileDB::FileInfo>& files);
    friend class ScanFolderJob;
    Ptr<ScanFolderJob> currentScanJob;
    ToolkitUtil::FileDB fileDB;
    ToolkitUtil::Logger logger;

    void RefreshFileInfoCaches();

    /// set active folder from selection and trigger watcher and sync updates if necessary
    void SetActiveFolder(uint64_t folderId);

    /// inline name editing in the folder tree, nothing touches the disk until the name is committed with Enter
    enum class FolderEditMode
    {
        None,
        NewFolder,      ///< folderId is the parent the new folder will be created in
        Rename          ///< folderId is the folder being renamed
    };
    struct FolderEdit
    {
        FolderEditMode mode = FolderEditMode::None;
        uint64_t folderId = 0;
        char name[256] = { 0 };
        Util::String error;
        bool requestFocus = false;
        int focusRequestFrame = 0;
        bool openParent = false;
        bool wasActive = false;
        bool drawn = false;
        bool commitRequested = false;
    };
    FolderEdit folderEdit;
    /// folder the tree context menu was opened on
    uint64_t contextFolder = 0;

    /// start editing the name of a new, not yet created, subfolder of parentId
    void BeginNewFolder(uint64_t parentId);
    /// start editing the name of an existing folder
    void BeginRenameFolder(uint64_t folderId);
    /// draw the inline name editor at the current cursor position, handles commit and cancel
    void DisplayFolderNameEditor();
    /// try to apply the edited name, returns false and sets an error if the name can't be used
    bool CommitFolderEdit();
    /// drop the current edit without touching anything
    void CancelFolderEdit();
    /// create a folder on disk and in the FileDB
    bool CreateSubFolder(uint64_t parentId, const Util::String& name, Util::String& outError);
    /// rename a folder on disk and resync the FileDB, folder ids are derived from parent and name so the subtree gets new ids
    bool RenameFolder(uint64_t folderId, const Util::String& newName, Util::String& outError);
    /// load a folder subtree from the FileDB into the folder caches and search index
    void CacheFolderSubtree(uint64_t folderId);
    /// remove a folder subtree from the folder caches and search index
    void UncacheFolderSubtree(uint64_t folderId);

    struct FolderScanResult
    {
        uint64_t folderId = 0;
        Util::Array<ToolkitUtil::FileDB::FileInfo> files;
        Util::Array<ToolkitUtil::FileDB::FolderInfo> children;
    };

    static Util::Dictionary<uint64_t, ToolkitUtil::FileDB::FolderInfo> folderInfoCache;
    static Util::Array<ToolkitUtil::FileDB::FileInfo> fileInfoCache;
    static Util::Dictionary<IO::Path, uint64_t> fileInfoDict;
    static Util::Dictionary<IO::Path, uint64_t> folderInfoDict;
    static Util::Dictionary<uint64_t, Util::Array<uint64_t>> folderChildIds;
    Threading::SafeFlag isDoneRefreshingCaches;
    Threading::SafeFlag isDoneWithDictionaries;
    Threading::SafeQueue<IO::WatchEvent> pendingWatchEvents;
    Threading::SafeQueue<uint64_t> refreshedFolders;
    Threading::SafeQueue<uint64_t> pendingFolderRefreshes;
    Threading::SafeQueue<FolderScanResult> pendingScanResults;
    static Util::Dictionary<uint64_t, bool> scannedFolders;
    Util::Array<ToolkitUtil::FileDB::FileInfo> searchIndex;
    Util::Array<ToolkitUtil::FileDB::FileInfo> searchResults;
    char searchFilter[512] = { 0 };
    Util::String lastSearchFilter;
    bool searchIndexDirty = false;

    std::function<void(const IO::Path& path)> pickFileFunction, pickFolderFunction;
    IO::Path pendingPickPath;
    bool fileTreeReady = false;
    bool backgroundScanFinished = false;
    bool showProgress = false;
    bool ownsScanJob = false;
};

/// Helper function to create mesh resource button
void ImGuiMeshResourceButton(const ToolkitUtil::ModelAssetT* asset);
/// Helper function to create mesh resource button
void ImGuiTextureResourceButton(const ToolkitUtil::TextureResourceT* asset);

} // namespace Presentation

