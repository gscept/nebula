//------------------------------------------------------------------------------
//  filedb.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "filedb.h"
#include "filedbattrs.h"
#include "db/sqlite3/sqlite3factory.h"
#include "io/ioserver.h"
#include "io/filetime.h"
#include "timing/calendartime.h"
#include "util/variant.h"


namespace ToolkitUtil
{

using namespace Util;
using namespace IO;
using namespace Db;

//------------------------------------------------------------------------------
/**
*/
FileDB::FileDB() :
    isOpen(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FileDB::~FileDB()
{
    if (this->isOpen)
    {
        this->Close();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FileDB::SetDatabaseURI(const IO::URI& uri)
{
    this->databaseURI = uri;
}

//------------------------------------------------------------------------------
/**
*/
const IO::URI&
FileDB::GetDatabaseURI() const
{
    return this->databaseURI;
}

//------------------------------------------------------------------------------
/**
*/
bool
FileDB::Open(Logger& logger)
{
    n_assert(!this->isOpen);
    n_assert(this->databaseURI.IsValid());

    // Create database factory and database
    this->dbFactory = Db::Sqlite3Factory::Create();
    this->database = DbFactory::Instance()->CreateDatabase();
    
    // Determine access mode
    IO::IoServer* ioServer = IO::IoServer::Instance();
    Db::Database::AccessMode accessMode = Db::Database::ReadWriteCreate;
    
    this->database->SetURI(this->databaseURI);
    this->database->SetAccessMode(accessMode);
    this->database->SetIgnoreUnknownColumns(false);
    
    // Try to open database
    if (!this->database->Open())
    {
        logger.Error("Failed to open FileDB database at %s\n", this->databaseURI.AsString().AsCharPtr());
        return false;
    }
    
    // Initialize tables if needed
    this->InitializeTables();
    
    this->isOpen = true;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
FileDB::Close()
{
    if (this->database.isvalid())
    {
        this->database->Close();
        this->database = nullptr;
    }
    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
bool
FileDB::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
void
FileDB::InitializeTables(                                                                                                                                           )
{
    // Check if tables already exist
    if (!this->database->HasTable("Folders") ||
        !this->database->HasTable("Files"))
    {
        this->CreateTables();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
FileDB::CreateTables()
{
    n_assert(this->database.isvalid());

    // ==================== Create Folders Table ====================
    {
        Ptr<Table> folderTable = DbFactory::Instance()->CreateTable();
        folderTable->SetName("Folders");
        folderTable->AddColumn(Column(Attr::FolderId, Column::Primary));
        folderTable->AddColumn(Column(Attr::ParentFolderId, Column::Indexed));
        folderTable->AddColumn(Column(Attr::EntityName));
        folderTable->AddColumn(Column(Attr::ModifiedDate));
        folderTable->AddColumn(Column(Attr::IsRootFolder));
        folderTable->AddColumn(Column(Attr::IsArchive));
        
        this->database->AddTable(folderTable);
    }

    // ==================== Create Files Table ====================
    {
        Ptr<Table> fileTable = DbFactory::Instance()->CreateTable();
        fileTable->SetName("Files");
        fileTable->AddColumn(Column(Attr::FileId, Column::Primary));
        fileTable->AddColumn(Column(Attr::FileFolderId, Column::Indexed));
        fileTable->AddColumn(Column(Attr::EntityName));
        fileTable->AddColumn(Column(Attr::FileType, Column::Indexed));
        fileTable->AddColumn(Column(Attr::FileSize));
        fileTable->AddColumn(Column(Attr::ModifiedDate));
        
        this->database->AddTable(fileTable);
    }

    return true;
}

//------------------------------------------------------------------------------
/**
*/
Guid
FileDB::CreateRootFolder(const Util::String& name, const IO::FileTime& modifiedDate, bool isArchive)
{
    n_assert(this->isOpen);
    
    Guid folderId;
    folderId.Generate();
    
    Ptr<Table> folderTable = this->database->GetTableByName("Folders");
    Ptr<Dataset> dataset = folderTable->CreateDataset();
    
    dataset->AddColumn(Attr::FolderId);
    dataset->AddColumn(Attr::EntityName);
    dataset->AddColumn(Attr::ModifiedDate);
    dataset->AddColumn(Attr::IsRootFolder);
    dataset->AddColumn(Attr::IsArchive);
    
    Ptr<ValueTable> values = dataset->Values();
    values->AddRow();
    IndexT lastRowIdx = values->GetNumRows() - 1;
    
    values->SetGuid(Attr::FolderId, lastRowIdx, folderId);
    values->SetString(Attr::EntityName, lastRowIdx, name);
    values->SetUInt64(Attr::ModifiedDate, lastRowIdx, modifiedDate.AsEpochTime());
    values->SetBool(Attr::IsRootFolder, lastRowIdx, true);
    values->SetBool(Attr::IsArchive, lastRowIdx, isArchive);
    
    dataset->CommitChanges();
    return folderId;
}

//------------------------------------------------------------------------------
/**
*/
Guid
FileDB::CreateFolder(Logger& logger, const Util::String& name, const Guid& parentFolderId, const IO::FileTime& modifiedDate, bool isArchive)
{
    n_assert(this->isOpen);
    
    // Verify parent exists
    FolderInfo parentInfo;
    if (!this->GetFolderInfo(parentFolderId, parentInfo))
    {
        logger.Error("Parent folder with ID %s does not exist\n", parentFolderId.AsString().AsCharPtr());
        return Guid();
    }
    
    Guid folderId;
    folderId.Generate();
        
    Ptr<Table> folderTable = this->database->GetTableByName("Folders");
    Ptr<Dataset> dataset = folderTable->CreateDataset();
    
    dataset->AddColumn(Attr::FolderId);
    dataset->AddColumn(Attr::ParentFolderId);
    dataset->AddColumn(Attr::EntityName);
    dataset->AddColumn(Attr::ModifiedDate);
    dataset->AddColumn(Attr::IsRootFolder);
    dataset->AddColumn(Attr::IsArchive);
    
    Ptr<ValueTable> values = dataset->Values();
    values->AddRow();
    IndexT lastRowIdx = values->GetNumRows() - 1;
    
    values->SetGuid(Attr::FolderId, lastRowIdx, folderId);
    values->SetGuid(Attr::ParentFolderId, lastRowIdx, parentFolderId);
    values->SetString(Attr::EntityName, lastRowIdx, name);
    values->SetUInt64(Attr::ModifiedDate, lastRowIdx, modifiedDate.AsEpochTime());
    values->SetBool(Attr::IsRootFolder, lastRowIdx, false);
    values->SetBool(Attr::IsArchive, lastRowIdx, isArchive);
    
    dataset->CommitChanges();
    return folderId;
}

//------------------------------------------------------------------------------
/**
*/
bool
FileDB::GetFolderInfo(const Guid& folderId, FolderInfo& outInfo)
{
    n_assert(this->isOpen);
    
    Ptr<Table> folderTable = this->database->GetTableByName("Folders");
    Ptr<Dataset> dataset = folderTable->CreateDataset();
    
    dataset->AddColumn(Attr::FolderId);
    dataset->AddColumn(Attr::ParentFolderId);
    dataset->AddColumn(Attr::EntityName);
    dataset->AddColumn(Attr::ModifiedDate);
    
    Ptr<FilterSet> filter = dataset->Filter();
    filter->AddEqualCheck(Attr::Attribute(Attr::FolderId, folderId));
    
    dataset->PerformQuery();
    Ptr<ValueTable> values = dataset->Values();
    
    if (values->GetNumRows() == 0)
    {
        return false;
    }
    
    outInfo.id = folderId;
    outInfo.name = values->GetString(Attr::EntityName, 0);
    outInfo.parentId = values->GetGuid(Attr::ParentFolderId, 0);
    outInfo.modifiedDate = IO::FileTime(values->GetUInt64(Attr::ModifiedDate, 0));
    
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
FileDB::GetChildFolders(const Guid& parentFolderId, Array<FolderInfo>& outFolders)
{
    n_assert(this->isOpen);
    
    Ptr<Table> folderTable = this->database->GetTableByName("Folders");
    Ptr<Dataset> dataset = folderTable->CreateDataset();
    
    dataset->AddColumn(Attr::FolderId);
    dataset->AddColumn(Attr::ParentFolderId);
    dataset->AddColumn(Attr::EntityName);
    dataset->AddColumn(Attr::ModifiedDate);
    
    Ptr<FilterSet> filter = dataset->Filter();
    filter->AddEqualCheck(Attr::Attribute(Attr::ParentFolderId, parentFolderId));
    
    dataset->PerformQuery();
    Ptr<ValueTable> values = dataset->Values();
    
    for (IndexT i = 0; i < values->GetNumRows(); ++i)
    {
        FolderInfo info;
        info.id = values->GetGuid(Attr::FolderId, i);
        info.parentId = parentFolderId;
        info.name = values->GetString(Attr::EntityName, i);
        info.modifiedDate = IO::FileTime(values->GetUInt64(Attr::ModifiedDate, i));
        
        outFolders.Append(info);
    }
    
    return outFolders.Size() > 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
FileDB::GetFolderPath(const Guid& folderId, Array<String>& outPath)
{
    n_assert(this->isOpen);
    
    FolderInfo current;
    if (!this->GetFolderInfo(folderId, current))
    {
        return false;
    }
    
    // Build path from leaf to root
    do
    {
        outPath.Insert(0, current.name);
        
        // Check if this is a root folder
        Ptr<Table> folderTable = this->database->GetTableByName("Folders");
        Ptr<Dataset> dataset = folderTable->CreateDataset();
        dataset->AddColumn(Attr::IsRootFolder);
        Ptr<FilterSet> filter = dataset->Filter();
        filter->AddEqualCheck(Attr::Attribute(Attr::FolderId, current.id));
        dataset->PerformQuery();
        Ptr<ValueTable> values = dataset->Values();
        
        if (values->GetNumRows() > 0 && values->GetBool(Attr::IsRootFolder, 0))
        {
            break;
        }
        
        // Get parent and continue
        if (!current.parentId.IsValid())
        {
            break;
        }
        
        if (!this->GetFolderInfo(current.parentId, current))
        {
            break;
        }
    } while (current.parentId.IsValid());
    
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
FileDB::DeleteFolder(Logger& logger, const Guid& folderId)
{
    n_assert(this->isOpen);
    
    // Check if folder is empty
    Array<FolderInfo> children;
    if (this->GetChildFolders(folderId, children) && children.Size() > 0)
    {
        logger.Error("Cannot delete folder with ID %s - it has child folders\n", folderId.AsString().AsCharPtr());
        return false;
    }
    
    Array<FileInfo> files;
    if (this->GetFilesInFolder(folderId, files) && files.Size() > 0)
    {
        logger.Error("Cannot delete folder with ID %s - it contains files\n", folderId.AsString().AsCharPtr());
        return false;
    }
    
    // Delete from database
    Ptr<Table> folderTable = this->database->GetTableByName("Folders");
    Ptr<Dataset> dataset = folderTable->CreateDataset();
    dataset->AddColumn(Attr::FolderId);
    
    Ptr<FilterSet> filter = dataset->Filter();
    filter->AddEqualCheck(Attr::Attribute(Attr::FolderId, folderId));
    dataset->PerformQuery();
    Ptr<ValueTable> values = dataset->Values();
    
    if (values->GetNumRows() > 0)
    {
        values->DeleteRow(0);
        dataset->CommitChanges();
        return true;
    }
    
    return false;
}

//------------------------------------------------------------------------------
/**
*/
Guid
FileDB::AddFile(Logger& logger, const Util::String& name, const Guid& folderId,
               SizeT size, FileType type, const IO::FileTime& modifiedDate)
{
    n_assert(this->isOpen);
    
    // Verify folder exists
    FolderInfo folderInfo;
    if (!this->GetFolderInfo(folderId, folderInfo))
    {
        logger.Error("Folder with ID %s does not exist\n", folderId.AsString().AsCharPtr());
        return Guid();
    }
    
    Guid fileId;
    fileId.Generate();
        
    Ptr<Table> fileTable = this->database->GetTableByName("Files");
    Ptr<Dataset> dataset = fileTable->CreateDataset();
    
    dataset->AddColumn(Attr::FileId);
    dataset->AddColumn(Attr::FileFolderId);
    dataset->AddColumn(Attr::EntityName);
    dataset->AddColumn(Attr::FileType);
    dataset->AddColumn(Attr::FileSize);
    dataset->AddColumn(Attr::ModifiedDate);
    
    Ptr<ValueTable> values = dataset->Values();
    values->AddRow();
    IndexT lastRowIdx = values->GetNumRows() - 1;
    
    values->SetGuid(Attr::FileId, lastRowIdx, fileId);
    values->SetGuid(Attr::FileFolderId, lastRowIdx, folderId);
    
    values->SetString(Attr::EntityName, lastRowIdx, name);
    values->SetInt(Attr::FileType, lastRowIdx, static_cast<int>(type));
    values->SetInt64(Attr::FileSize, lastRowIdx, size);
    values->SetUInt64(Attr::ModifiedDate, lastRowIdx, modifiedDate.AsEpochTime());
    
    dataset->CommitChanges();
    return fileId;
}

//------------------------------------------------------------------------------
/**
*/
bool
FileDB::GetFileInfo(const Guid& fileId, FileInfo& outInfo)
{
    n_assert(this->isOpen);
    
    Ptr<Table> fileTable = this->database->GetTableByName("Files");
    Ptr<Dataset> dataset = fileTable->CreateDataset();
    
    dataset->AddColumn(Attr::FileId);
    dataset->AddColumn(Attr::FileFolderId);
    dataset->AddColumn(Attr::EntityName);
    dataset->AddColumn(Attr::FileType);
    dataset->AddColumn(Attr::FileSize);
    dataset->AddColumn(Attr::ModifiedDate);
    
    Ptr<FilterSet> filter = dataset->Filter();
    filter->AddEqualCheck(Attr::Attribute(Attr::FileId, fileId));
    
    dataset->PerformQuery();
    Ptr<ValueTable> values = dataset->Values();
    
    if (values->GetNumRows() == 0)
    {
        return false;
    }
    
    outInfo.id = fileId;
    outInfo.folderId = values->GetGuid(Attr::FileFolderId, 0);
    outInfo.name = values->GetString(Attr::EntityName, 0);
    outInfo.type = static_cast<FileType>(values->GetInt(Attr::FileType, 0));
    outInfo.size = (SizeT)values->GetInt64(Attr::FileSize, 0);
    outInfo.modifiedDate = IO::FileTime(values->GetUInt64(Attr::ModifiedDate, 0));
    
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
FileDB::GetFilesInFolder(const Guid& folderId, Array<FileInfo>& outFiles)
{
    n_assert(this->isOpen);
    
    Ptr<Table> fileTable = this->database->GetTableByName("Files");
    Ptr<Dataset> dataset = fileTable->CreateDataset();
    
    dataset->AddColumn(Attr::FileId);
    dataset->AddColumn(Attr::FileFolderId);
    dataset->AddColumn(Attr::EntityName);
    dataset->AddColumn(Attr::FileType);
    dataset->AddColumn(Attr::FileSize);
    dataset->AddColumn(Attr::ModifiedDate);
    
    Ptr<FilterSet> filter = dataset->Filter();
    filter->AddEqualCheck(Attr::Attribute(Attr::FileFolderId, folderId));
    
    dataset->PerformQuery();
    Ptr<ValueTable> values = dataset->Values();
    
    for (IndexT i = 0; i < values->GetNumRows(); ++i)
    {
        FileInfo info;
        info.id = values->GetGuid(Attr::FileId, i);
        info.folderId = folderId;
        info.name = values->GetString(Attr::EntityName, i);
        info.type = static_cast<FileType>(values->GetInt(Attr::FileType, i));
        info.size = (SizeT)values->GetInt64(Attr::FileSize, i);
        info.modifiedDate = IO::FileTime(values->GetUInt64(Attr::ModifiedDate, i));
        
        outFiles.Append(info);
    }
    
    return outFiles.Size() > 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
FileDB::GetFilesInFolderByType(const Guid& folderId, FileType type,
                               Array<FileInfo>& outFiles)
{
    n_assert(this->isOpen);
    
    Ptr<Table> fileTable = this->database->GetTableByName("Files");
    Ptr<Dataset> dataset = fileTable->CreateDataset();
    
    dataset->AddColumn(Attr::FileId);
    dataset->AddColumn(Attr::FileFolderId);
    dataset->AddColumn(Attr::EntityName);
    dataset->AddColumn(Attr::FileType);
    dataset->AddColumn(Attr::FileSize);
    dataset->AddColumn(Attr::ModifiedDate);
    
    Ptr<FilterSet> filter = dataset->Filter();
    filter->BeginBlock();
    filter->AddEqualCheck(Attr::Attribute(Attr::FileFolderId, folderId));
    filter->AddAnd();
    filter->AddEqualCheck(Attr::Attribute(Attr::FileType, static_cast<int>(type)));
    filter->EndBlock();
    
    dataset->PerformQuery();
    Ptr<ValueTable> values = dataset->Values();
    
    for (IndexT i = 0; i < values->GetNumRows(); ++i)
    {
        FileInfo info;
        info.id = values->GetGuid(Attr::FileId, i);
        info.folderId = folderId;
        info.name = values->GetString(Attr::EntityName, i);
        info.type = static_cast<FileType>(values->GetInt(Attr::FileType, i));
        info.size = (SizeT)values->GetInt64(Attr::FileSize, i);
        info.modifiedDate = IO::FileTime(values->GetUInt64(Attr::ModifiedDate, i));
        
        outFiles.Append(info);
    }
    
    return outFiles.Size() > 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
FileDB::GetFilesModifiedAfter(const Guid& folderId, const IO::FileTime& dateTime,
                              Array<FileInfo>& outFiles)
{
    n_assert(this->isOpen);
    
    Ptr<Table> fileTable = this->database->GetTableByName("Files");
    Ptr<Dataset> dataset = fileTable->CreateDataset();
    
    dataset->AddColumn(Attr::FileId);
    dataset->AddColumn(Attr::FileFolderId);
    dataset->AddColumn(Attr::EntityName);
    dataset->AddColumn(Attr::FileType);
    dataset->AddColumn(Attr::FileSize);
    dataset->AddColumn(Attr::ModifiedDate);
    
    Ptr<FilterSet> filter = dataset->Filter();
    filter->BeginBlock();
    filter->AddEqualCheck(Attr::Attribute(Attr::FileFolderId, folderId));
    filter->AddAnd();
    filter->AddGreaterThenCheck(Attr::Attribute(Attr::ModifiedDate, dateTime.AsEpochTime()));
    filter->EndBlock();
    
    dataset->PerformQuery();
    Ptr<ValueTable> values = dataset->Values();
    
    for (IndexT i = 0; i < values->GetNumRows(); ++i)
    {
        FileInfo info;
        info.id = values->GetGuid(Attr::FileId, i);
        info.folderId = folderId;
        info.name = values->GetString(Attr::EntityName, i);
        info.type = static_cast<FileType>(values->GetInt(Attr::FileType, i));
        info.size = (SizeT)values->GetInt64(Attr::FileSize, i);
        info.modifiedDate = IO::FileTime(values->GetUInt64(Attr::ModifiedDate, i));
        
        outFiles.Append(info);
    }
    
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
FileDB::GetFilesLargerThan(const Guid& folderId, SizeT sizeBytes,
                           Array<FileInfo>& outFiles)
{
    n_assert(this->isOpen);
    
    Ptr<Table> fileTable = this->database->GetTableByName("Files");
    Ptr<Dataset> dataset = fileTable->CreateDataset();
    
    dataset->AddColumn(Attr::FileId);
    dataset->AddColumn(Attr::FileFolderId);
    dataset->AddColumn(Attr::EntityName);
    dataset->AddColumn(Attr::FileType);
    dataset->AddColumn(Attr::FileSize);
    dataset->AddColumn(Attr::ModifiedDate);
    
    Ptr<FilterSet> filter = dataset->Filter();
    filter->BeginBlock();
    filter->AddEqualCheck(Attr::Attribute(Attr::FileFolderId, folderId));
    filter->AddAnd();
    filter->AddGreaterThenCheck(Attr::Attribute(Attr::FileSize, (int)sizeBytes));
    filter->EndBlock();
    
    dataset->PerformQuery();
    Ptr<ValueTable> values = dataset->Values();
    
    for (IndexT i = 0; i < values->GetNumRows(); ++i)
    {
        FileInfo info;
        info.id = values->GetGuid(Attr::FileId, i);
        info.folderId = folderId;
        info.name = values->GetString(Attr::EntityName, i);
        info.type = static_cast<FileType>(values->GetInt(Attr::FileType, i));
        info.size = (SizeT)values->GetInt64(Attr::FileSize, i);
        info.modifiedDate = IO::FileTime(values->GetUInt64(Attr::ModifiedDate, i));
        
        outFiles.Append(info);
    }
    
    return outFiles.Size() > 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
FileDB::UpdateFileMetadata(Logger& logger, const Guid& fileId,
                          const IO::FileTime& modifiedDate)
{
    n_assert(this->isOpen);
    
    FileInfo fileInfo;
    if (!this->GetFileInfo(fileId, fileInfo))
    {
        logger.Error("File with ID %s does not exist\n", fileId.AsString().AsCharPtr());
        return false;
    }
    
    Ptr<Table> fileTable = this->database->GetTableByName("Files");
    Ptr<Dataset> dataset = fileTable->CreateDataset();
    
    // only add column if we actually received a non-default time
    if (!(modifiedDate == IO::FileTime()))
    {
        dataset->AddColumn(Attr::ModifiedDate);
    }
    
    if (dataset->GetTable()->GetNumColumns() == 0)
    {
        return false; // Nothing to update
    }
    
    Ptr<FilterSet> filter = dataset->Filter();
    filter->AddEqualCheck(Attr::Attribute(Attr::FileId, fileId));
    
    dataset->PerformQuery();
    Ptr<ValueTable> values = dataset->Values();
    
    if (values->GetNumRows() > 0)
    {
        if (!(modifiedDate == IO::FileTime()))
        {
            values->SetUInt64(Attr::ModifiedDate, 0, modifiedDate.AsEpochTime());
        }
        
        dataset->CommitChanges();
        return true;
    }
    
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
FileDB::DeleteFile(Logger& logger, const Guid& fileId)
{
    n_assert(this->isOpen);
    
    Ptr<Table> fileTable = this->database->GetTableByName("Files");
    Ptr<Dataset> dataset = fileTable->CreateDataset();
    dataset->AddColumn(Attr::FileId);
    
    Ptr<FilterSet> filter = dataset->Filter();
    filter->AddEqualCheck(Attr::Attribute(Attr::FileId, fileId));
    
    dataset->PerformQuery();
    Ptr<ValueTable> values = dataset->Values();
    
    if (values->GetNumRows() > 0)
    {
        values->DeleteRow(0);
        dataset->CommitChanges();
        return true;
    }
    
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
FileDB::CommitChanges(Logger& logger)
{
    if (this->database.isvalid())
    {
        logger.Print("Committing all pending database changes\n");
        // Changes are committed per-dataset, but we can ensure the database is synchronized
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FileDB::DumpSchema(Logger& logger) const
{
    if (!this->database.isvalid())
    {
        logger.Print("Database not open\n");
        return;
    }
    
    logger.Print("=== FileDB Schema ===\n");
    for (IndexT tableIdx = 0; tableIdx < this->database->GetNumTables(); ++tableIdx)
    {
        const Ptr<Table>& table = this->database->GetTableByIndex(tableIdx);
        logger.Print("\nTable: %s\n", table->GetName().AsCharPtr());
        
        for (IndexT colIdx = 0; colIdx < table->GetNumColumns(); ++colIdx)
        {
            const Column& col = table->GetColumn(colIdx);
            logger.Print("  Column: %s (FourCC: %s)\n", 
                        col.GetName().AsCharPtr(), 
                        col.GetFourCC().AsString().AsCharPtr());
        }
    }
}

} // namespace ToolkitUtil
