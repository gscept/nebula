//------------------------------------------------------------------------------
//  table.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "db/table.h"
#include "db/database.h"
#include "db/dbfactory.h"
#include "db/dataset.h"

namespace Db
{
__ImplementClass(Db::Table, 'TABL', Core::RefCounted);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
Table::Table() :
    isConnected(false),
    primaryColumnIndex(InvalidIndex)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Table::~Table()
{
    if (this->IsConnected())
    {
        this->Disconnect(false);
    }
}

//------------------------------------------------------------------------------
/**
    Create a dataset associated with this table.
*/
Ptr<Dataset>
Table::CreateDataset()
{
    Ptr<Dataset> newDataset = DbFactory::Instance()->CreateDataset();
    newDataset->SetTable(this);
    return newDataset;
}

//------------------------------------------------------------------------------
/**
    Add a new column to the table. The change will not be synchronised with
    the database until a CommitChanges() is called on the Table object.
*/
void
Table::AddColumn(const Column& col)
{
    this->columns.Append(col);
    this->nameIndexMap.Add(col.GetName(), this->columns.Size() - 1);
    this->fourccIndexMap.Add(col.GetFourCC(), this->columns.Size() - 1);
    this->attrIdIndexMap.Add(col.GetAttrId(), this->columns.Size() - 1);
    if (col.GetType() == Column::Primary)
    {
        this->primaryColumnIndex = this->columns.Size() - 1;
    }
}

//------------------------------------------------------------------------------
/**
    Sets a new name for the table. If the Table is attached to a database the
    change will happen immediately (this must be implemented in a subclass).
*/
void
Table::SetName(const Util::String& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
    Connect the table with a database:
    - if the table doesn't exist yet, it will be created, and all 
      table object columns will be marked as uncommitted
    - if the table exists its layout will be read and added as columns to 
      the table object unless a column exists, in this case the column
      will be marked as uncommitted, existing columns will be marked as
      committed.
    All this must happen in a subclass of course.
*/
void
Table::Connect(const Ptr<Database>& db, ConnectMode connectMode, bool ignoreUnknownColumns)
{
    n_assert(!this->isConnected);
    n_assert(0 != db);
    this->database = db;
    this->isConnected = true;
}

//------------------------------------------------------------------------------
/**
    Disconnect the table with a database. If the dropTable argument is true
    (default is false), the actual database table will be deleted from the
    database as well. If dropTable is false, a CommitChanges() should
    be called before the table object is disconnected (this is left to
    the subclass).
*/
void
Table::Disconnect(bool /*dropTable*/)
{
    n_assert(this->isConnected);
    this->database = nullptr;
    this->isConnected = false;
}

//------------------------------------------------------------------------------
/**
    This method should be used to create multicolumn indices on the
    table. The table must be connected for this method to work!
*/
void
Table::CreateMultiColumnIndex(const Array<Attr::AttrId>& /*columnIds*/)
{
    // implement in subclass!
}

//------------------------------------------------------------------------------
/**
    Commit any changes to the database:
    - rows added to the Table object will be added in the database
    - rows deleted from the Table object will be dropped from the database
    All this must be implemented in a subclass.
*/
void
Table::CommitChanges(bool /*resetModifiedState*/, bool /*useTransaction*/)
{
    n_error("Db::Table::CommitChanges() called!");
}

//------------------------------------------------------------------------------
/**
    Bind a value table to the table object. A value table represents actual
    values in the table, adding, removing or modifying rows on a 
    value table will be reflected automatically in the database. Only
    one value table may be bound at any time!
*/
void
Table::BindValueTable(const Ptr<ValueTable>& valTable)
{
    n_assert(0 != valTable);
    n_assert(!this->valueTable.isvalid());
    this->valueTable = valTable;
}

//------------------------------------------------------------------------------
/**
    Unbind the currently bound value table. This will commit any changes
    to the value table into the database.
*/
void
Table::UnbindValueTable()
{
    n_assert(this->valueTable.isvalid());
    this->valueTable = nullptr;
}

//------------------------------------------------------------------------------
/**
    This method returns true if any uncommited columns exist. These are
    columns that have been added with AddColumn() but have not yet
    been committed to the database.
*/
bool
Table::HasUncommittedColumns() const
{
    // NOTE: columns are always added at the end, so its ok if we
    // just check the last columns
    if (this->columns.Size() > 0)
    {
        return !(this->columns.Back().IsCommitted());
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    Commit uncommitted columns to the database, this does not write any
    data to the database, just change the table layout.
*/
void
Table::CommitUncommittedColumns()
{
    n_error("Db::Table::CommitUncommittedColumns() called!");
}

//------------------------------------------------------------------------------
/**
    Immediately delete rows of the connected dataset from the database.
*/
void
Table::CommitDeletedRows()
{
    n_error("Db::Table::CommitDeletedRows() called!");
}

} // namespace Db
