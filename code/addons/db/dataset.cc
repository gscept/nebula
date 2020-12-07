//------------------------------------------------------------------------------
//  dataset.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "db/dataset.h"
#include "db/database.h"
#include "db/dbfactory.h"

namespace Db
{
__ImplementClass(Db::Dataset, 'DTST', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
Dataset::Dataset() :
    isConnected(false)    
{
    this->filter = DbFactory::Instance()->CreateFilterSet();
    this->values = DbFactory::Instance()->CreateValueTable();
}

//------------------------------------------------------------------------------
/**
*/
Dataset::~Dataset()
{
    n_assert(!this->IsConnected());
}

//------------------------------------------------------------------------------
/**
*/
void
Dataset::AddColumn(const Attr::AttrId& attrId)
{
    if (!this->values->HasColumn(attrId))
    {
        this->values->AddColumn(attrId);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Dataset::AddAllTableColumns()
{
    n_assert(this->table.isvalid());
    IndexT colIndex;
    for (colIndex = 0; colIndex < this->table->GetNumColumns(); colIndex++)
    {
        const Attr::AttrId& attrId = this->table->GetColumn(colIndex).GetAttrId();
        if (!this->values->HasColumn(attrId))
        {
            // don't mark the columns as "new columns" 
            this->values->AddColumn(attrId, false);
        }
    }
}

//------------------------------------------------------------------------------
/**
    Connect the dataset to the table it is associated with. A dataset will
    only be connected to its table during database operations.
*/
bool
Dataset::Connect()
{
    n_assert(!this->IsConnected());
    n_assert(this->table.isvalid());

    // bind our value table to the database table
    this->table->BindValueTable(this->values);
    this->isConnected = true;
    return true;
}

//------------------------------------------------------------------------------
/**
    Disconnect the dataset from the table it is associated with. This will
    commit any changes to the value table into the database.
*/
void
Dataset::Disconnect()
{
    n_assert(this->IsConnected());
    this->table->UnbindValueTable();
    this->isConnected = false;
}

//------------------------------------------------------------------------------
/**
    This will create a new value table object by performing a query from
    the database. The query will be built from the TableColumnPairs, 
    Relations and the optional Filter set on this dataset object. If possible,
    the previous query will be re-used to save compile time. The query 
    will completely overwrite the current value table.

    NOTE: the actual functionality of this method should be implemented in
    a subclass.
*/
void
Dataset::PerformQuery(bool appendResult)
{
    n_error("Db::Dataset::PerformQuery() called!");
}

//------------------------------------------------------------------------------
/**
    This will write back any changes in the ValueTable back into the
    database. This includes new columns, new rows and modified existing
    values.

    NOTE: the actual functionality of this method should be implemented in
    a subclass.
*/
void
Dataset::CommitChanges(bool newRowsAsUpdate)
{
    n_error("Db::Dataset::CommitChanges() called!");
}

//------------------------------------------------------------------------------
/**
    This will delete all rows marked as deleted from the database. It is 
    sometimes necessary to perform this operation immediately.
*/
void
Dataset::CommitDeletedRows()
{
    n_error("Db::Dataset::CommitDeletedRows() called!");
}


} // namespace Db