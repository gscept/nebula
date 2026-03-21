#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::AssetBrowser

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"
#include "io/filetime.h"
#include "util/array.h"
#include "util/dictionary.h"
#include "util/string.h"
#include "io/uri.h"
#include "filedb/filedb.h"
#include "threading/safeflag.h"

namespace Presentation
{

class ScanFolderJob;
class AssetEditor;
class AssetBrowser : public BaseWindow
{
    __DeclareClass(AssetBrowser)
public:
    AssetBrowser();
    ~AssetBrowser();

    void Update();
    void Run(SaveMode save) override;
private:
    
    void ScanFolder(ToolkitUtil::FileDB& fileDB, const Util::String& treeName, const Util::String& folderPath, bool useArchive);
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
    void ScanFolderRecursive(ToolkitUtil::FileDB& fileDB, const IO::IoServer* ioServer, const IO::URI& folderPath, bool useArchive, uint64_t parent);
    friend class ScanFolderJob;
    Ptr<ScanFolderJob> currentScanJob;
    ToolkitUtil::FileDB fileDB;
    ToolkitUtil::Logger logger;

    /// refresh caches for folder and file info to avoid redundant DB queries during tree display
    void RefreshFolderInfoCaches();
    void RefreshFileInfoCaches();

    Util::Dictionary<uint64_t, ToolkitUtil::FileDB::FolderInfo> folderInfoCache;
    Util::Array<ToolkitUtil::FileDB::FileInfo> fileInfoCache;
    Threading::SafeFlag isDoneRefreshingCaches;
};
__RegisterClass(AssetBrowser)

} // namespace Presentation

