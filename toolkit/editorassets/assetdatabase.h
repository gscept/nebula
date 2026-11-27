#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::AssetDatabase
    The index of the assets in 'work:assets', which every asset view reads: the folder tree, the files
    of each folder, and the search index over all of them.

    The index is kept in a FileDB, so the next start does not have to read the disk again. At setup
    a background job scans the whole folder and hands the results to the main thread folder by folder,
    and a file watcher reports what changes on disk after that. A folder a view opens before the scan
    has reached it is listed from the disk, or scanned right away once the job is done.

    Views do not hold on to what they read. The version changes whenever the index does, and a view
    reads its folder again when the version it read it at is not the current one. Two views can show
    two folders this way, which is what lets the browser and the picker be open side by side.

    Changes the editor makes, a new folder, a rename or a new asset, go through here, so the disk, the
    FileDB and the index change together.

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/ptr.h"
#include "core/singleton.h"
#include "io/filewatcher.h"
#include "io/path.h"
#include "threading/safeflag.h"
#include "threading/safequeue.h"
#include "toolkitutil/filedb/filedb.h"
#include "toolkitutil/logger.h"
#include "util/array.h"
#include "util/dictionary.h"
#include "util/string.h"

namespace Editor
{

struct AssetType;
class ScanFolderJob;

class AssetDatabase : public Core::RefCounted
{
    __DeclareClass(AssetDatabase)
    __DeclareInterfaceSingleton(AssetDatabase)
public:
    using FileInfo = ToolkitUtil::FileDB::FileInfo;
    using FolderInfo = ToolkitUtil::FileDB::FolderInfo;

    AssetDatabase();
    virtual ~AssetDatabase();

    /// open the FileDB, start the scan and watch the assets folder
    bool Setup();
    /// stop the scan and the watch and close the FileDB
    void Discard();
    /// take what the scan and the watcher found since the last frame. Called once per frame
    void Update();

    /// the folder 'work:assets' is
    uint64_t GetRootFolder() const;
    /// true while the background scan runs
    bool IsScanning() const;
    /// how far the background scan is, 0..1, negative while it still counts the files
    float GetScanProgress() const;
    /// a number that changes whenever the index does
    uint GetVersion() const;

    /// true when the folder is in the index
    bool HasFolder(uint64_t folderId) const;
    /// a folder in the index, HasFolder has to be true for it
    FolderInfo GetFolder(uint64_t folderId) const;
    /// the folder of a path, 0 when it is not in the index yet
    uint64_t FindFolder(const IO::Path& folder) const;
    /// true when a folder is the other one or below it
    bool IsInFolder(uint64_t folderId, uint64_t ancestorId) const;
    /// the folders in a folder, in name order. A folder nothing scanned yet is scanned first
    void GetChildFolders(uint64_t folderId, Util::Array<uint64_t>& out);
    /// the files in a folder. A folder nothing scanned yet is listed from the disk while the scan runs,
    /// and scanned first after it
    void GetFiles(uint64_t folderId, Util::Array<FileInfo>& out);
    /// every file of the folders scanned so far, for the search
    const Util::Array<FileInfo>& GetSearchIndex() const;

    /// make a folder on disk and in the index
    bool CreateFolder(uint64_t parentId, const Util::String& name, uint64_t& outFolderId, Util::String& outError);
    /// rename a folder on disk. Folder ids come from the parent and the name, so the folder and
    /// everything below it get new ids, and the folder's is returned
    bool RenameFolder(uint64_t folderId, const Util::String& newName, uint64_t& outFolderId, Util::String& outError);
    /// make a new asset of a type, with the work file its type writes. It is batched right away, so
    /// it can be referenced before it was edited
    bool CreateAsset(uint64_t folderId, const AssetType& type, const Util::String& variant, const Util::String& name, uint64_t& outFileId, Util::String& outError);
    /// rename a file on disk, keeping its extension, and batch it under its new name. The export under
    /// the old name stays for what still refers to it
    bool RenameFile(uint64_t fileId, const Util::String& newName, uint64_t& outFileId, Util::String& outError);

    /// the type a file is kept as in the FileDB, from its extension
    static ToolkitUtil::FileType DetermineFileType(const Util::String& extension);
    /// the FileDB type of a URN type, Other when the FileDB has none for it
    static ToolkitUtil::FileType FileTypeOf(const Util::String& urnType);
    /// check a folder or asset name. Uses the Windows rules on every platform so asset folders stay
    /// portable, which also keeps the URN separator out of names. Kind names what is named in the
    /// messages
    static bool ValidateName(const Util::String& name, const char* kind, Util::String& outError);

private:
    friend class ScanFolderJob;

    /// what the scan found in one folder
    struct FolderScanResult
    {
        uint64_t folderId = 0;
        Util::Array<FileInfo> files;
        Util::Array<FolderInfo> children;
    };

    /// sync a folder on disk into the FileDB
    static void ScanFolder(ToolkitUtil::FileDB& fileDB, ToolkitUtil::Logger& logger, const IO::URI& folderPath, bool useArchive, uint64_t parent, bool recursive);
    /// scan one folder on the main thread and take it into the index
    void ScanNow(uint64_t folderId);
    /// take a scanned folder into the index
    void CacheScannedFolder(uint64_t folderId, const Util::Array<FolderInfo>& children, const Util::Array<FileInfo>& files);
    /// replace a folder's files in the search index
    void IndexFolderForSearch(uint64_t folderId, const Util::Array<FileInfo>& files);
    /// load a folder subtree from the FileDB into the index
    void CacheFolderSubtree(uint64_t folderId);
    /// remove a folder subtree from the index
    void UncacheFolderSubtree(uint64_t folderId);
    /// read a folder's files from the FileDB into the search index again
    void ReindexFolder(uint64_t folderId);
    /// apply what the watcher reported
    void ApplyWatchEvents();
    /// the index changed
    void Changed();

    ToolkitUtil::FileDB fileDB;
    ToolkitUtil::Logger logger;
    uint64_t rootFolder;

    Util::Dictionary<uint64_t, FolderInfo> folders;
    Util::Dictionary<IO::Path, uint64_t> folderIds;
    Util::Dictionary<uint64_t, Util::Array<uint64_t>> folderChildren;
    Util::Dictionary<uint64_t, bool> scannedFolders;
    Util::Array<FileInfo> searchIndex;

    Ptr<ScanFolderJob> scanJob;
    Threading::SafeQueue<FolderScanResult> pendingScanResults;
    Threading::SafeQueue<IO::WatchEvent> pendingWatchEvents;
    bool watching;
    uint version;
};

} // namespace Editor
//------------------------------------------------------------------------------
