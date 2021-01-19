#pragma once
//------------------------------------------------------------------------------
/**
    @class Db::Database
  
    Wraps an entire database into a C++ object.
    
    @copyright
    (C) 2006 Radon Labs
    (C) 2013-2021 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "io/uri.h"
#include "util/dictionary.h"
#include "db/table.h"
#include "db/command.h"

//------------------------------------------------------------------------------
namespace Attr
{
    DeclareString(AttrName, 'ATNA', Attr::ReadWrite);
    DeclareString(AttrType, 'ATTY', Attr::ReadWrite);
    DeclareBool(AttrReadWrite, 'ATRW', Attr::ReadWrite);
    DeclareBool(AttrDynamic, 'ATDY', Attr::ReadWrite);
}
//------------------------------------------------------------------------------
namespace Db
{
class Database : public Core::RefCounted
{
    __DeclareClass(Database);
public:
    /// access modes
    enum AccessMode
    {
        ReadOnly,
        ReadWriteExisting,  // fail if database doesn't exist
        ReadWriteCreate,    // create new database if doesn't exist
    };

    /// constructor
    Database();
    /// destructor
    virtual ~Database();

    /// set the data source URI (usually filename of database file)
    void SetURI(const IO::URI& uri);
    /// get the database URI
    const IO::URI& GetURI() const;
    /// set the access mode (ReadOnly or ReadWrite)
    void SetAccessMode(AccessMode m);
    /// get access mode
    AccessMode GetAccessMode() const;
    /// enable/disable exclusive mode
    void SetExclusiveMode(bool b);
    /// get exclusive mode
    bool GetExclusiveMode() const;
    /// ignore unknown columns (not existing as attribute), yes/no
    void SetIgnoreUnknownColumns(bool b);
    /// get ignore unknown columns flag
    bool GetIgnoreUnknownColumns() const;
    
    /// open the database
    virtual bool Open();
    /// close the database
    virtual void Close();
    /// return true if database is open
    bool IsOpen() const;
    /// get last error string
    const Util::String& GetError() const;

    /// attach another database to the current database
    virtual bool AttachDatabase(const IO::URI& uri, const Util::String& dbName);
    /// detach an attached database
    virtual void DetachDatabase(const Util::String& dbName);
    /// begin a transaction on the database
    virtual void BeginTransaction();
    /// end a transaction on the database
    virtual void EndTransaction();
    /// create a new table in the database
    virtual void AddTable(const Ptr<Table>& table);
    /// delete a table from the database
    virtual void DeleteTable(const Util::String& tableName);
    /// return true if a table exists by name
    bool HasTable(const Util::String& tableName) const;
    /// get number of tables in the database
    SizeT GetNumTables() const;
    /// get table by index
    const Ptr<Table>& GetTableByIndex(IndexT i) const;
    /// get table by name
    const Ptr<Table>& GetTableByName(const Util::String& tableName) const;
    
    /// set in-memory database only
    void SetInMemoryDatabase(bool b);
    /// copy in memory database to file
    virtual void CopyInMemoryDatabaseToFile(const IO::URI& fileUri);
    
protected:
    /// set error string
    void SetError(const Util::String& error);
    /// find a table index by name
    IndexT FindTableIndex(const Util::String& tableName) const;

    bool isOpen;
    bool ignoreUnknownColumns;
    IO::URI uri;         
    Util::String error;
    AccessMode accessMode;
    bool memoryDatabase;
    bool exclusiveMode;
    Util::Array<Ptr<Table> > tables;     // NOTE: not in a Dictionary, because name may change externally!
};

//------------------------------------------------------------------------------
/**
*/
inline void
Database::SetIgnoreUnknownColumns(bool b)
{
    this->ignoreUnknownColumns = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Database::GetIgnoreUnknownColumns() const
{
    return this->ignoreUnknownColumns;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Database::SetError(const Util::String& err)
{
    this->error = err;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
Database::GetError() const
{
    return this->error;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Database::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Database::SetURI(const IO::URI& u)
{
    this->uri = u;
}

//------------------------------------------------------------------------------
/**
*/
inline const IO::URI&
Database::GetURI() const
{
    return this->uri;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Database::SetAccessMode(AccessMode m)
{
    this->accessMode = m;
}

//------------------------------------------------------------------------------
/**
*/
inline Database::AccessMode
Database::GetAccessMode() const
{
    return this->accessMode;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Database::SetInMemoryDatabase(bool b)
{
    this->memoryDatabase = b;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Database::SetExclusiveMode(bool b)
{
    this->exclusiveMode = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Database::GetExclusiveMode() const
{
    return this->exclusiveMode;
}
} // namespace Db
//------------------------------------------------------------------------------