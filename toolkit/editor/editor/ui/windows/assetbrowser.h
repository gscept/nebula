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
#include "filedb/filedb.h"
#include "threading/safeflag.h"
#include "threading/safequeue.h"
#include "io/filewatcher.h"

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
    void PickFile(const Util::String& path, std::function<void(const Util::String& path)> picker);
    /// Pick folder
    void PickFolder(const Util::String& path, std::function<void(const Util::String& path)> picker);
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
    void ScanFolder(ToolkitUtil::FileDB& fileDB, const IO::IoServer* ioServer, const IO::URI& folderPath, bool useArchive, uint64_t parent, bool recursive);
    /// replace this folder's files in the search index after a disk scan
    void IndexFolderForSearch(uint64_t folderId, const Util::Array<ToolkitUtil::FileDB::FileInfo>& files);
    friend class ScanFolderJob;
    Ptr<ScanFolderJob> currentScanJob;
    ToolkitUtil::FileDB fileDB;
    ToolkitUtil::Logger logger;

    void RefreshFileInfoCaches();

    /// set active folder from selection and trigger watcher and sync updates if necessary
    void SetActiveFolder(uint64_t folderId);

    struct FolderScanResult
    {
        uint64_t folderId = 0;
        Util::Array<ToolkitUtil::FileDB::FileInfo> files;
        Util::Array<ToolkitUtil::FileDB::FolderInfo> children;
    };

    Util::Dictionary<uint64_t, ToolkitUtil::FileDB::FolderInfo> folderInfoCache;
    Util::Array<ToolkitUtil::FileDB::FileInfo> fileInfoCache;
    Util::Dictionary<Util::String, uint64_t> fileInfoDict;
    Util::Dictionary<Util::String, uint64_t> folderInfoDict;
    Util::Array<uint64_t> rootFolderIds;
    Util::Dictionary<uint64_t, Util::Array<uint64_t>> folderChildIds;
    Threading::SafeFlag isDoneRefreshingCaches;
    Threading::SafeFlag isDoneWithDictionaries;
    Threading::SafeQueue<IO::WatchEvent> pendingWatchEvents;
    Threading::SafeQueue<uint64_t> refreshedFolders;
    Threading::SafeQueue<uint64_t> pendingFolderRefreshes;
    Threading::SafeQueue<FolderScanResult> pendingScanResults;
    Util::Dictionary<uint64_t, bool> scannedFolders;
    Util::Array<ToolkitUtil::FileDB::FileInfo> searchIndex;
    Util::Array<ToolkitUtil::FileDB::FileInfo> searchResults;
    char searchFilter[512] = { 0 };
    Util::String lastSearchFilter;
    bool searchIndexDirty = false;

    std::function<void(const Util::String& path)> pickFileFunction, pickFolderFunction;
    Util::String pendingPickPath;
    bool fileTreeReady = false;
    bool backgroundScanFinished = false;
    bool showProgress = false;
    bool ownsScanJob = false;
};

} // namespace Presentation

