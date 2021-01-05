//------------------------------------------------------------------------------
//  sqlite3dataset.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "db/sqlite3/sqlite3dataset.h"
#include "db/command.h"
#include "db/dbfactory.h"
#include "db/database.h"
#include "db/table.h"

namespace Db
{
__ImplementClass(Db::Sqlite3Dataset, 'S3DS', Db::Dataset);

using namespace Util;

// static strings to prevent excessive string object construction
const Util::String Sqlite3Dataset::SelectFrag("SELECT ");
const Util::String Sqlite3Dataset::FromFrag(" FROM ");
const Util::String Sqlite3Dataset::CommaFrag(",");
const Util::String Sqlite3Dataset::WhereFrag(" WHERE ");
const Util::String Sqlite3Dataset::StarFrag("*");
const Util::String Sqlite3Dataset::TickFrag("\"");

//------------------------------------------------------------------------------
/**
*/
Sqlite3Dataset::Sqlite3Dataset()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Sqlite3Dataset::~Sqlite3Dataset()
{
    if (this->IsConnected())
    {
        this->Disconnect();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
Sqlite3Dataset::Connect()
{
    if (Dataset::Connect())
    {
        // add any new columns from our value table to the table
        const Util::Array<IndexT>& newColIndices = this->values->GetNewColumnIndices();
        IndexT i;
        for (i = 0; i < newColIndices.Size(); i++)
        {
            const Attr::AttrId& newColAttrId = this->values->GetColumnId(newColIndices[i]);
            if (!this->table->HasColumn(newColAttrId))
            {
                this->table->AddColumn(Db::Column(newColAttrId));
            }
        }
        n_assert(!this->queryCommand.isvalid());
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Dataset::Disconnect()
{
    // release command objects
    this->queryCommand = nullptr;
    Dataset::Disconnect();
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Dataset::PerformQuery(bool appendResult)
{
    n_assert(!this->IsConnected());
    n_assert(this->values.isvalid());
    n_assert(this->table.isvalid());

    // connect to the database table
    this->Connect();

    // if not appending, clear all rows from the value table,
    // otherwise the new query will just be added to the value table
    if (!appendResult)
    {
        this->values->Clear();
    }

    // if our table layout is invalid, we need at least update the database's
    // table layout, so that the following query will compile!
    if (this->table->HasUncommittedColumns())
    {
        this->table->CommitUncommittedColumns();
    }

    // check if we can re-use the existing command object
    if (!this->queryCommand.isvalid())
    {
        this->queryCommand = DbFactory::Instance()->CreateCommand();
    }
    if (!this->queryCommand->IsValid() || this->filter->IsDirty())
    {
        // re-compile the command
        String sql;
        sql.Reserve(4096);
        sql = SelectFrag;
        sql.Append(this->GetSqlSelectColumns());
        sql.Append(FromFrag);
        sql.Append(this->table->GetName());

        // add optional WHERE statement
        if (this->filter.isvalid() && (!this->filter->IsEmpty()))
        {
            sql.Append(WhereFrag);
            sql.Append(this->filter->AsSqlWhere());
        }
        this->filter->ClearDirtyFlag();

        // recompile query command...
        bool compiled = this->queryCommand->Compile(this->table->GetDatabase(), sql, this->values);
        if (!compiled)
        {
            n_error("Sqlite3Dataset::PerformQuery: error compiling SQL statement:\n%s\nWith error:\n%s\n", 
                sql.AsCharPtr(), this->queryCommand->GetError().AsCharPtr());
            return;
        }
    }

    // bind where values to command
    if (this->filter.isvalid() && (!this->filter->IsEmpty()))
    {
        this->filter->BindValuesToCommand(this->queryCommand, 0);
    }

    // ok, the query is compiled and valid, now execute it
    bool executed = this->queryCommand->Execute();
    if (!executed)
    {
        n_error("Sqlite3Dataset::PerformQuery: error executing SQL statement:\n%s\nWith error:\n%s\n", 
            this->queryCommand->GetSqlCommand().AsCharPtr(), this->queryCommand->GetError().AsCharPtr());
        return;
    }
    
    // disconnect from our table
    this->Disconnect();
}

//------------------------------------------------------------------------------
/**
    Commits any changes in the value table into the database.
*/
void
Sqlite3Dataset::CommitChanges(bool newRowsAsUpdate)
{
    n_assert(!this->IsConnected());
    this->Connect();
    this->table->CommitChanges(newRowsAsUpdate);
    this->Disconnect();
}

//------------------------------------------------------------------------------
/**
    Immediately deletes rows from the database.
*/
void
Sqlite3Dataset::CommitDeletedRows()
{
    n_assert(!this->IsConnected());
    this->Connect();
    this->table->CommitDeletedRows();
    this->Disconnect();
}

//------------------------------------------------------------------------------
/**
    Helper method which returns a SQL fragment string containing the columns
    to select. In the simplest case this method just returns a "*". Otherwise
    the format "table.column" will be returned.
*/
String
Sqlite3Dataset::GetSqlSelectColumns() const
{
    n_assert(this->table.isvalid());
    String str;
    str.Reserve(4096);

    // if no columns are defined, then return everything in the table
    if (this->values->GetNumColumns() == 0) 
    {
        str = StarFrag;
    }
    else
    {
        IndexT colIndex;
        SizeT numColumns = this->values->GetNumColumns();
        for (colIndex = 0; colIndex < numColumns; colIndex++)
        {
            str.Append(TickFrag);
            str.Append(this->values->GetColumnName(colIndex));
            str.Append(TickFrag);
            if (colIndex < (numColumns - 1))
            {
                str.Append(CommaFrag);
            }
        }
    }
    return str;
}

} // namespace Db
