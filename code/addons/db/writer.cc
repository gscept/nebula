//------------------------------------------------------------------------------
//  writer.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "db/writer.h"
#include "db/dbserver.h"
#include "db/database.h"
#include "db/dbfactory.h"

namespace Db
{
__ImplementClass(Db::Writer, 'DBWR', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
Writer::Writer() :
    isOpen(false),
    inBeginRow(false),
    flushTable(false),
    rowIndex(InvalidIndex)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Writer::~Writer()
{
    if (this->IsOpen())
    {
        this->Close();
    }
}

//------------------------------------------------------------------------------
/**
    This opens the writer.
*/
bool
Writer::Open()
{
    n_assert(!this->isOpen);
    n_assert(!this->inBeginRow);

    // create a new value table
    this->valueTable = DbFactory::Instance()->CreateValueTable();
    IndexT i;
    for (i = 0; i < this->columns.Size(); i++)
    {
        this->valueTable->AddColumn(this->columns[i].GetAttrId(), false);
    }
    this->isOpen = true;
    return true;
}

//------------------------------------------------------------------------------
/**
    This opens the writer from an existing value table.
    Note: you still need to add primary and/or indexed columns,
    all others will be taken from the value table.
*/
bool
Writer::OpenFromValueTable(const Ptr<ValueTable>& srcValues)
{
    n_assert(!this->isOpen);
    n_assert(!this->inBeginRow);
    
    this->valueTable = DbFactory::Instance()->CreateValueTable();

    // copy src value table into writer column table
    IndexT i;
    for (i = 0; i < srcValues->GetNumColumns(); i++)
    {
        if (!this->columnMap.Contains(srcValues->GetColumnId(i)))
        {
            this->AddColumn(Column(srcValues->GetColumnId(i)));
        }
    }

    // update the value table columns
    for (i = 0; i < this->columns.Size(); i++)
    {
        this->valueTable->AddColumn(this->columns[i].GetAttrId(), false);
    }

    // finally copy source value rows to destination table
    IndexT srcRowIndex = 0;
    for (srcRowIndex = 0; srcRowIndex < srcValues->GetNumRows(); srcRowIndex++)
    {
        this->valueTable->CopyExtRow(srcValues, srcRowIndex);
    }

    this->isOpen = true;
    return true;
}

//------------------------------------------------------------------------------
/**
    Close the writer. The database update stuff will happen here.
*/
void
Writer::Close()
{
    n_assert(this->isOpen);
    n_assert(!this->inBeginRow);
    n_assert(this->database.isvalid());

    // delete the table if requested
    if (this->flushTable)
    {
        if (this->database->HasTable(this->tableName))
        {
            this->database->DeleteTable(this->tableName);
        }
    }

    // check if a new table must be created in the database
    bool tableWasCreated = false;
    Ptr<Table> table;
    if (this->flushTable || (!this->database->HasTable(this->tableName)))
    {
        table = DbFactory::Instance()->CreateTable();
        table->SetName(tableName);
        tableWasCreated = true;
    }
    else
    {
        table = this->database->GetTableByName(this->tableName);
    }

    // update the table columns
    IndexT colIndex;
    for (colIndex = 0; colIndex < this->columns.Size(); colIndex++)
    {
        if (!table->HasColumn(this->columns[colIndex].GetAttrId()))
        {
            table->AddColumn(this->columns[colIndex]);
        }
    }

    // add table to database if it was created
    if (tableWasCreated)
    {
        this->database->AddTable(table);
    }

    // if the number of columns in our value table is smaller then
    // the number of table columns, then this updates just a 
    // portion of the table, in this case, new rows must be
    // treated like modified rows, so instead of an INSERT SQL operation,
    // an UPDATE operation must be used
    if (this->valueTable->GetNumColumns() < table->GetNumColumns())
    {
        this->valueTable->ClearNewRowFlags();
    }

    // bind value table to table and commit changes to the database
    table->BindValueTable(this->valueTable);
    table->CommitChanges();
    table->UnbindValueTable();

    // finally release the value table and return
	this->valueTable = nullptr;
    this->database = nullptr;
    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
    Begin writing a new row to the database.
*/
void
Writer::BeginRow()
{
    n_assert(this->isOpen);
    n_assert(!this->inBeginRow);
    n_assert(this->valueTable.isvalid());
    this->inBeginRow = true;
    this->rowIndex = this->valueTable->AddRow();
}

//------------------------------------------------------------------------------
/**
    Finish writing the current row. This will update the columns
    array with any new attribute ids in the current row.
*/
void
Writer::EndRow()
{
    n_assert(this->isOpen);
    n_assert(this->inBeginRow);
    this->inBeginRow = false;
    this->rowIndex = InvalidIndex;
}

}; // namespace Db
