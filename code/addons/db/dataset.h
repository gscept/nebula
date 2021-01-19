#pragma once
#ifndef DB_DATASET_H
#define DB_DATASET_H
//------------------------------------------------------------------------------
/**
    @class Db::Dataset
    
    A dataset is an efficient in-memory-cache for rlational database data. It 
    is optimized for read/modify/write operations. Usually you tell the dataset object
    what you want from the database, modify the queried data, and write
    the queried data back. Datasets can also be used to write new data into
    the database.

    Configuring the dataset:

    * Add table objects which contain the columns you're interested in.
    * Add an optional Filter object which basically describes a WHERE statement.

    Once the dataset has been configured you can fill the dataset with data by
    either doing a PerformQuery() (which creates a new result ValueTable), or by filling
    out the ValueTable by hand.
    
    To write modified data in the ValueTable back to the database, invoke the
    CommitChanges() method. This will try to figure out the most efficient way to write
    the modified data back into the database.

    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "db/filterset.h"
#include "db/valuetable.h"
#include "db/relation.h"

//------------------------------------------------------------------------------
namespace Db
{
class Database;
class Table;

class Dataset : public Core::RefCounted
{
    __DeclareClass(Dataset);
public:
    /// constructor
    Dataset();
    /// destructor
    virtual ~Dataset();

    /// add the columns you're interested in
    void AddColumn(const Attr::AttrId& attrId);
    /// add all the table columns to the dataset
    void AddAllTableColumns();
    /// get pointer to database table this set is associated with
    const Ptr<Table>& GetTable() const;
    /// access to query filter
    const Ptr<FilterSet>& Filter();
    /// access to value table
    const Ptr<ValueTable>& Values();

    /// fill value table from database
    virtual void PerformQuery(bool appendResult=false);
    /// commit modified values to the database
    virtual void CommitChanges(bool newRowsAsUpdate=false);
    /// commit deleted rows only to the database
    virtual void CommitDeletedRows();

protected:
    friend class Table;

    /// set pointer to database table
    void SetTable(const Ptr<Table>& t);
    /// connect to database table
    virtual bool Connect();
    /// disconnect from database table
    virtual void Disconnect();
    /// return true if the dataset is connected
    bool IsConnected() const;

    bool isConnected;
    Ptr<Table> table;
    Ptr<FilterSet> filter;
    Ptr<ValueTable> values;
};

//------------------------------------------------------------------------------
/**
*/
inline void
Dataset::SetTable(const Ptr<Table>& t)
{
    n_assert(0 != t);
    this->table = t;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Table>&
Dataset::GetTable() const
{
    return this->table;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Dataset::IsConnected() const
{
    return this->isConnected;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<FilterSet>&
Dataset::Filter()
{
    return this->filter;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<ValueTable>&
Dataset::Values()
{
    return this->values;
}

} // namespace Db
//------------------------------------------------------------------------------
#endif
