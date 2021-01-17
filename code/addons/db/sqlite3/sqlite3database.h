#pragma once
#ifndef DB_SQLITE3DATABASE_H
#define DB_SQLITE3DATABASE_H
//------------------------------------------------------------------------------
/**
    @class Db::Sqlite3Database
    
    SQLite3 implementation of Db::Database.

    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#include "attr/attribute.h"
#include "db/database.h"
#include "sqlite3.h"

//------------------------------------------------------------------------------
namespace Attr
{
    // some SQLite-internal names
    DeclareString(name, 'name', ReadOnly);
    DeclareString(type, 'type', ReadOnly);
    DeclareInt(cid, 'cid_', ReadOnly);
    DeclareInt(notnull, 'ntnl', ReadOnly);
    DeclareBlob(dflt_value, 'dflv', ReadOnly);
    DeclareBool(pk, 'pk__', ReadOnly);
    DeclareInt(seq, 'seq_', ReadOnly);
    DeclareBool(unique, 'uniq', ReadOnly);
    DeclareInt(seqno, 'sqno', ReadOnly);
}

//------------------------------------------------------------------------------
namespace Db
{
class Sqlite3Database : public Database
{
    __DeclareClass(Sqlite3Database);
public:
    /// temporary storage location
    enum TempStore
    {
        File,
        Memory,
    };

    /// constructor
    Sqlite3Database();
    /// destructor
    virtual ~Sqlite3Database();

    /// set the SQLite3 database cache size in number of pages
    void SetCacheNumPages(SizeT numCachePages);
    /// get SQLite3 database cache size
    SizeT GetCacheNumPages() const;
    /// set temporary storage location
    void SetTempStore(TempStore s);
    /// get temporary storage location
    TempStore GetTempStore() const;
    /// synchronous mode on/off
    void SetSynchronousMode(bool b);
    /// get synchronous mode
    bool GetSynchronousMode() const;
    /// set busy timeout in milliseconds (default is 100, 0 disabled busy handling)
    void SetBusyTimeout(int ms);
    /// get busy timeout in milliseconds
    int GetBusyTimeout() const;

    /// open the database
    virtual bool Open();
    /// close the database
    virtual void Close();

    /// attach another database to the current database
    virtual bool AttachDatabase(const IO::URI& uri, const Util::String& dbName);
    /// detach an attached database
    virtual void DetachDatabase(const Util::String& dbName);
    /// begin a transaction on the database
    virtual void BeginTransaction();
    /// end a transaction on the database
    virtual void EndTransaction();

    /// get the SQLite database handle
    sqlite3* GetSqliteHandle() const;
    
    /// copy in memory database to file
    virtual void CopyInMemoryDatabaseToFile(const IO::URI& fileUri);

private:
    /// read table layouts from database
    void ReadTableLayouts();
    /// dynamically register attributes from special _Attributes db table
    void RegisterAttributes(Ptr<Table>& attrTable);

    SizeT cacheNumPages;
    TempStore tempStore;
    bool syncMode;
    int busyTimeout;
    sqlite3* sqliteHandle;
    Ptr<Command> beginTransactionCmd;
    Ptr<Command> endTransactionCmd;
};

//------------------------------------------------------------------------------
/**
*/
inline sqlite3*
Sqlite3Database::GetSqliteHandle() const
{
    n_assert(0 != this->sqliteHandle);
    return this->sqliteHandle;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Sqlite3Database::SetCacheNumPages(SizeT p)
{
    n_assert(!this->IsOpen());
    n_assert(p > 0);
    this->cacheNumPages = p;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
Sqlite3Database::GetCacheNumPages() const
{
    return this->cacheNumPages;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Sqlite3Database::SetTempStore(TempStore s)
{
    this->tempStore = s;
}

//------------------------------------------------------------------------------
/**
*/
inline Sqlite3Database::TempStore
Sqlite3Database::GetTempStore() const
{
    return this->tempStore;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Sqlite3Database::SetSynchronousMode(bool b)
{
    this->syncMode = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Sqlite3Database::GetSynchronousMode() const
{
    return this->syncMode;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Sqlite3Database::SetBusyTimeout(int ms)
{
    this->busyTimeout = ms;
}

//------------------------------------------------------------------------------
/**
*/
inline int
Sqlite3Database::GetBusyTimeout() const
{
    return this->busyTimeout;
}

} // namespace Db
//------------------------------------------------------------------------------
#endif

