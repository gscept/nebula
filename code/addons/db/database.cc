//------------------------------------------------------------------------------
//  database.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "db/database.h"


namespace Attr
{
    DefineString(AttrName, 'ATNA', Attr::ReadWrite);
    DefineString(AttrType, 'ATTY', Attr::ReadWrite);
    DefineBool(AttrReadWrite, 'ATRW', Attr::ReadWrite);
    DefineBool(AttrDynamic, 'ATDY', Attr::ReadWrite);
}

namespace Db
{
__ImplementClass(Db::Database, 'DBAS', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
Database::Database() :        
    isOpen(false),
    ignoreUnknownColumns(false),
    accessMode(ReadWriteExisting),
    memoryDatabase(false),
    exclusiveMode(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Database::~Database()
{
    n_assert(!this->IsOpen());
}

//------------------------------------------------------------------------------
/**
    Open the database. A subclass should connect to the database here
    and create a DataTable object for each table in the database.
*/
bool
Database::Open()
{
    n_assert(!this->IsOpen());
    this->isOpen = true;
    return true;
}

//------------------------------------------------------------------------------
/**
    Close the database. A subclass should flush all pending changes
    and disconnect from the database.
*/
void
Database::Close()
{
    n_assert(this->IsOpen());

    // flush and disconnect tables
    int tableIndex;
    int numTables = this->tables.Size();
    for (tableIndex = 0; tableIndex < numTables; tableIndex++)
    {
        this->tables[tableIndex]->Disconnect(false);
    }
    this->tables.Clear();
    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
    Begin a transaction on the database. Override this method in a subclass.
*/
void
Database::BeginTransaction()
{
    n_error("Database::BeginTransaction() called!");
}

//------------------------------------------------------------------------------
/**
    Finish a transaction on the database. Override this method in a subclass.
*/
void
Database::EndTransaction()
{
    n_error("Database::EndTransaction() called!");
}   

//------------------------------------------------------------------------------
/**
    Add a table to the database. No table of the same name may exist
    yet in the database, otherwise an error will be thrown. If the table
    has values, the new database table will be initialized with those
    values. The table object will be connected to the database, which
    means any later changes to the table may be written to the database
    by calling the CommitChanges() method on the table.
 */
void
Database::AddTable(const Ptr<Table>& table)
{
    n_assert(this->IsOpen());
    n_assert(0 != table);
    n_assert(!this->HasTable(table->GetName()));
    this->tables.Append(table);
    table->Connect(this, Table::ForceCreate);
}

//------------------------------------------------------------------------------
/**
    Remove a data table from the database. A subclass must drop the table
    from the database when this method is called.
*/
void
Database::DeleteTable(const Util::String& tableName)
{
    IndexT tableIndex = this->FindTableIndex(tableName);
    n_assert(InvalidIndex != tableIndex);
    this->tables[tableIndex]->Disconnect(true);
    this->tables.EraseIndex(tableIndex);
}

//------------------------------------------------------------------------------
/**
    NOTE: we cannot use a more efficient container for the tables
    because the table name can change anytime outside our control.
*/
IndexT
Database::FindTableIndex(const Util::String& tableName) const
{
    n_assert(tableName.IsValid());
    n_assert(this->IsOpen());
    IndexT i;
    for (i = 0; i < this->tables.Size(); i++)
    {
        if (this->tables[i]->GetName() == tableName)
        {
            return i;
        }
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    NOTE: we cannot use a more efficient container for the tables
    because the table name can change anytime outside our control.
*/
bool
Database::HasTable(const Util::String& tableName) const
{
    return (InvalidIndex != this->FindTableIndex(tableName));
}

//------------------------------------------------------------------------------
/**
    NOTE: we cannot use a more efficient container for the tables
    because the table name can change anytime outside our control.
*/
SizeT
Database::GetNumTables() const
{
    n_assert(this->IsOpen());
    return this->tables.Size();
}

//------------------------------------------------------------------------------
/**
    NOTE: we cannot use a more efficient container for the tables
    because the table name can change anytime outside our control.
*/
const Ptr<Table>&
Database::GetTableByIndex(IndexT i) const
{
    n_assert(this->IsOpen());
    return this->tables[i];
}

//------------------------------------------------------------------------------
/**
    NOTE: we cannot use a more efficient container for the tables
    because the table name can change anytime outside our control.
*/
const Ptr<Table>&
Database::GetTableByName(const Util::String& tableName) const
{
	IndexT idx = this->FindTableIndex(tableName);
	if(idx == InvalidIndex)
	{				
		n_error("Invalid table name in database: %s\ndid you forget a template file?\n",tableName.AsCharPtr());
	}
	
	return this->tables[idx];
}

//------------------------------------------------------------------------------
/**
    Attach another database to this database. Override this method in 
    a subclass!
*/
bool
Database::AttachDatabase(const IO::URI& /*uri*/, const Util::String& /*dbName*/)
{
    n_error("Database::AttachDatabase called!");
    return false;
}

//------------------------------------------------------------------------------
/**
    Detach an attached database from this database. Override this method in 
    a subclass!
*/
void
Database::DetachDatabase(const Util::String& /*dbName*/)
{
    n_error("Database::DetachDatabase() called!");
}

//------------------------------------------------------------------------------
/**
    Copy database to file. Override this method in 
    a subclass!
*/
void
Database::CopyInMemoryDatabaseToFile(const IO::URI& fileUri)
{
    n_error("Database::DetachDatabase() called!");        
}

} // namespace Db
