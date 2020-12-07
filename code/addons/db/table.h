#pragma once
#ifndef DB_TABLE_H
#define DB_TABLE_H
//------------------------------------------------------------------------------
/**
    @class Db::Table
    
    Describes a table in a database, or more abstract, a set of typed 
    columns grouped under a common name. Note that a table object
    is only a descriptor of the table layout, it never contains
    any actual data. If the table is attached to a database, any
    changes to the Table object will also lead to changes in the
    database (changes are batched until a CommitChanges() is invoked).
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "db/column.h"
#include "util/array.h"
#include "util/string.h"
#include "util/dictionary.h"
#include "util/hashtable.h"
#include "db/valuetable.h"

//------------------------------------------------------------------------------
namespace Db
{
class Database;
class Dataset;

class Table : public Core::RefCounted
{
    __DeclareClass(Table);
public:
    /// connection modes
    enum ConnectMode
    {
        ForceCreate,    ///> create new table, even if table exists
        AssumeExists,   ///> assume the table exists
        Default,        ///> check if table exist, create if it doesn't
    };

    /// constructor
    Table();
    /// destructor
    virtual ~Table();

    /// set name of table
    virtual void SetName(const Util::String& n);
    /// get name of table
    const Util::String& GetName() const;
    /// return true if the table is connected to a database
    bool IsConnected() const;

    /// get pointer to database this table is connected to
    const Ptr<Database>& GetDatabase() const;
    /// create a complex multicolumn index on a connected table
    virtual void CreateMultiColumnIndex(const Util::Array<Attr::AttrId>& columnIds);
    /// create a dataset associated with this table
    virtual Ptr<Dataset> CreateDataset();

    /// commit any changes (add columns, insert new rows, update modified rows)
    virtual void CommitChanges(bool resetModifiedState=true, bool useTransaction=true);
    /// check if there are any uncommitted columns in the table
    bool HasUncommittedColumns() const;
    /// commit uncommitted columns only
    virtual void CommitUncommittedColumns();
    /// commit deleted rows only
    virtual void CommitDeletedRows();

    /// add a column to the table
    void AddColumn(const Column& c);
    /// get number of columns in table
    SizeT GetNumColumns() const;
    /// get column at index
    const Column& GetColumn(IndexT i) const;
    /// return true if a column exists by attribute id
    bool HasColumn(const Attr::AttrId& id) const;
    /// return true if a column exists by name
    bool HasColumn(const Util::String& name) const;
    /// return true if a column exists by fourCC
    bool HasColumn(const Util::FourCC& fcc) const;
    /// get a column by attribute id
    const Column& GetColumn(const Attr::AttrId& id) const;
    /// get a column by name
    const Column& GetColumn(const Util::String& name) const;
    /// get a column by fourCC
    const Column& GetColumn(const Util::FourCC& fcc) const;
    /// get all columns in the table
    const Util::Array<Column>& GetColumns() const;
    /// return true if a primary key column exists
    bool HasPrimaryColumn() const;
    /// get the primary key column, fails hard if none exists
    const Column& GetPrimaryColumn() const;
    
protected:
    friend class Database;
    friend class Dataset;
    friend class Writer;

    /// connect the table object with a database
    virtual void Connect(const Ptr<Database>& db, ConnectMode connectMode=Default, bool ignoreUnknownColumns=false);
    /// disconnect the table from a database, optionally remove table from database as well
    virtual void Disconnect(bool dropTable=false);
    /// bind a value table to the table object, unbinds previous table
    virtual void BindValueTable(const Ptr<ValueTable>& valueTable);
    /// unbind current value table from the table object, commits any changes to the database
    virtual void UnbindValueTable();

    bool isConnected;
    Ptr<Database> database;
    Ptr<ValueTable> valueTable;
    Util::String name;
    Util::Array<Column> columns;
    Util::HashTable<Util::String,IndexT> nameIndexMap;
    Util::Dictionary<Util::FourCC,IndexT> fourccIndexMap;
    Util::Dictionary<Attr::AttrId,IndexT> attrIdIndexMap;
    IndexT primaryColumnIndex;
};

//------------------------------------------------------------------------------
/**
*/
inline 
const Ptr<Database>&
Table::GetDatabase() const
{
    return this->database;
}

//------------------------------------------------------------------------------
/**
*/
inline 
bool
Table::HasPrimaryColumn() const
{
    return (InvalidIndex != this->primaryColumnIndex);
}

//------------------------------------------------------------------------------
/**
*/
inline 
const Column&
Table::GetPrimaryColumn() const
{
    n_assert(InvalidIndex != this->primaryColumnIndex);
    return this->columns[this->primaryColumnIndex];
}

//------------------------------------------------------------------------------
/**
*/
inline 
bool
Table::IsConnected() const
{
    return this->isConnected;
}

//------------------------------------------------------------------------------
/**
*/
inline 
const Util::String&
Table::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline 
SizeT
Table::GetNumColumns() const
{
    return this->columns.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline 
const Column&
Table::GetColumn(IndexT i) const
{
    return this->columns[i];
}

//------------------------------------------------------------------------------
/**
*/
inline 
bool
Table::HasColumn(const Attr::AttrId& id) const
{
    return this->attrIdIndexMap.Contains(id);
}

//------------------------------------------------------------------------------
/**
*/
inline 
bool
Table::HasColumn(const Util::String& name) const
{
    return this->nameIndexMap.Contains(name);
}

//------------------------------------------------------------------------------
/**
*/
inline 
bool
Table::HasColumn(const Util::FourCC& fcc) const
{
    return this->fourccIndexMap.Contains(fcc);
}

//------------------------------------------------------------------------------
/**
*/
inline 
const Column&
Table::GetColumn(const Attr::AttrId& id) const
{
    n_assert(this->HasColumn(id));
    return this->columns[this->attrIdIndexMap[id]];
}

//------------------------------------------------------------------------------
/**
*/
inline 
const Column&
Table::GetColumn(const Util::String& name) const
{
    n_assert(this->HasColumn(name));
    return this->columns[this->nameIndexMap[name]];
}

//------------------------------------------------------------------------------
/**
*/
inline 
const Column&
Table::GetColumn(const Util::FourCC& fcc) const
{
    n_assert(this->HasColumn(fcc));
    return this->columns[this->fourccIndexMap[fcc]];
}

//------------------------------------------------------------------------------
/**
*/
inline 
const Util::Array<Column>&
Table::GetColumns() const
{
    return this->columns;
}

} // namespace Db
//------------------------------------------------------------------------------
#endif
