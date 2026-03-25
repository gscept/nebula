#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FileDB
    
    A hierarchical file metadata database using the db addon as backing storage.
    Supports per-folder access control and rich metadata queries (name, size, type,
    creation/modification/access dates). Provides efficient traversal up/down the
    folder hierarchy and permission-aware file/folder listing.
    
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "db/dbfactory.h"
#include "db/database.h"
#include "db/table.h"
#include "db/dataset.h"
#include "io/uri.h"
#include "io/filetime.h"
#include "util/string.h"
#include "util/array.h"
#include "toolkit-common/logger.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
enum class FileType : int
{
    Unknown = 0,
    FBX = 1,
    GLTF = 2,
    Model = 3,
    Mesh = 4,
    Texture = 5,
    Surface = 6,
    Audio = 7,
    Text = 8,
    Skeleton = 9,
    Animation = 10,
    Frame = 11,
    Shader = 12,
    Physics = 13,
    NavMesh = 14,
    Particle = 15,
    Other = 16
};
class FileDB
{
public:

    /// folder information structure
    struct FolderInfo
    {
        uint64_t id;
        Util::String name;
        uint64_t parentId;
        IO::FileTime modifiedDate;
        IO::URI uri;
        bool isRoot;
        bool isArchive;
        bool hasChildren;
    };

    /// file information structure
    struct FileInfo
    {
        uint64_t id;
        Util::String name;
        Util::String filePath;
        uint64_t folderId;
        SizeT size;
        FileType type;
        IO::FileTime modifiedDate;
    };

    /// constructor
    FileDB();
    /// destructor
    virtual ~FileDB();

    /// set the database URI (file path)
    void SetDatabaseURI(const IO::URI& uri);
    /// get the database URI
    const IO::URI& GetDatabaseURI() const;
    
    /// open the database (creates if doesn't exist)
    bool Open(Logger& logger, bool memoryOnly);
    /// close the database
    void Close();
    /// check if database is open
    bool IsOpen() const;

    // ==================== Folder Operations ====================
    
    /// create a root folder, returns folder ID
    uint64_t CreateRootFolder(const Util::String& name, const IO::FileTime& modifiedDate, bool isArchive);
    
    /// create a subfolder under a parent folder
    uint64_t CreateFolder(Logger& logger, const Util::String& name, uint64_t parentFolderId, const IO::FileTime& modifiedDate, bool isArchive);
    
    /// get folder info by ID
    bool GetFolderInfo(uint64_t folderId, FolderInfo& outInfo);
    
    /// get all immediate children folders of a parent
    bool GetChildFolders(uint64_t parentFolderId, Util::Array<FolderInfo>& outFolders);

    /// get all root folders
    bool GetRootFolders(Util::Array<FolderInfo>& outFolders);
    
    /// get the path from root to a given folder
    Util::String GetFolderPath(uint64_t folderId);
    
    /// delete a folder (fails if not empty)
    bool DeleteFolder(Logger& logger, uint64_t folderId);

    // ==================== File Operations ====================
    
    /// add a file to a folder
    uint64_t AddFile(Logger& logger, const Util::String& name, uint64_t folderId,
                       SizeT size, FileType type, const IO::FileTime& modifiedDate);
    
    /// get the path from root to a given file
    Util::String GetFilePath(uint64_t fileId);

    /// get file info by ID
    bool GetFileInfo(uint64_t fileId, FileInfo& outInfo);
    
    /// get all files in a folder
    bool GetFilesInFolder(uint64_t folderId, Util::Array<FileInfo>& outFiles);
    
    /// get files in folder by type filter
    bool GetFilesInFolderByType(uint64_t folderId, FileType Type,
                                Util::Array<FileInfo>& outFiles);
    
    /// get files modified after a certain date
    bool GetFilesModifiedAfter(uint64_t folderId, const IO::FileTime& dateTime,
                               Util::Array<FileInfo>& outFiles);
    
    /// get files larger than a certain size
    bool GetFilesLargerThan(uint64_t folderId, SizeT sizeBytes,
                            Util::Array<FileInfo>& outFiles);
    
    /// update file metadata (modified date)
    bool UpdateFileMetadata(Logger& logger, uint64_t fileId,
                           const IO::FileTime& modifiedDate = IO::FileTime());
    
    /// delete a file
    bool DeleteFile(Logger& logger, uint64_t fileId);

    // ==================== Utility ====================
    
    /// flush all pending changes to disk
    void CommitChanges(Logger& logger);
    
    /// dump database schema and contents for debugging
    void DumpSchema(Logger& logger) const;

private:
    /// helper to ensure tables exist
    void InitializeTables();
    /// helper to create tables if they don't exist
    bool CreateTables();

    Ptr<Db::Database> database;
    IO::URI databaseURI;
    bool isOpen;
    Ptr<Db::DbFactory> dbFactory;
};

} // namespace ToolkitUtil
//------------------------------------------------------------------------------