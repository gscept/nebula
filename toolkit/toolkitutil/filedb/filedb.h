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
#include "util/guid.h"
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
    Other = 15
};
class FileDB
{
public:

    /// folder information structure
    struct FolderInfo
    {
        Util::Guid id;
        Util::String name;
        Util::Guid parentId;
        IO::FileTime modifiedDate;
        IO::URI uri;
        bool isRoot;
    };

    /// file information structure
    struct FileInfo
    {
        Util::Guid id;
        Util::String name;
        Util::Guid folderId;
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
    bool Open(Logger& logger);
    /// close the database
    void Close();
    /// check if database is open
    bool IsOpen() const;

    // ==================== Folder Operations ====================
    
    /// create a root folder, returns folder ID
    Util::Guid CreateRootFolder(const Util::String& name, const IO::FileTime& modifiedDate, bool isArchive);
    
    /// create a subfolder under a parent folder
    Util::Guid CreateFolder(Logger& logger, const Util::String& name, const Util::Guid& parentFolderId, const IO::FileTime& modifiedDate, bool isArchive);
    
    /// get folder info by ID
    bool GetFolderInfo(const Util::Guid& folderId, FolderInfo& outInfo);
    
    /// get all immediate children folders of a parent
    bool GetChildFolders(const Util::Guid& parentFolderId, Util::Array<FolderInfo>& outFolders);
    
    /// get the path from root to a given folder (each element is folder name)
    bool GetFolderPath(const Util::Guid& folderId, Util::Array<Util::String>& outPath);
    
    /// delete a folder (fails if not empty)
    bool DeleteFolder(Logger& logger, const Util::Guid& folderId);

    // ==================== File Operations ====================
    
    /// add a file to a folder
    Util::Guid AddFile(Logger& logger, const Util::String& name, const Util::Guid& folderId,
                       SizeT size, FileType type, const IO::FileTime& modifiedDate);
    
    /// get file info by ID
    bool GetFileInfo(const Util::Guid& fileId, FileInfo& outInfo);
    
    /// get all files in a folder
    bool GetFilesInFolder(const Util::Guid& folderId, Util::Array<FileInfo>& outFiles);
    
    /// get files in folder by type filter
    bool GetFilesInFolderByType(const Util::Guid& folderId, FileType Type,
                                Util::Array<FileInfo>& outFiles);
    
    /// get files modified after a certain date
    bool GetFilesModifiedAfter(const Util::Guid& folderId, const IO::FileTime& dateTime,
                               Util::Array<FileInfo>& outFiles);
    
    /// get files larger than a certain size
    bool GetFilesLargerThan(const Util::Guid& folderId, SizeT sizeBytes,
                            Util::Array<FileInfo>& outFiles);
    
    /// update file metadata (modified date)
    bool UpdateFileMetadata(Logger& logger, const Util::Guid& fileId,
                           const IO::FileTime& modifiedDate = IO::FileTime());
    
    /// delete a file
    bool DeleteFile(Logger& logger, const Util::Guid& fileId);

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