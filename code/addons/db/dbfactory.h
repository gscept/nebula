#pragma once
#ifndef DB_DbFactory_H
#define DB_DbFactory_H
//------------------------------------------------------------------------------
/**
    @class Db::DbFactory
    
    DbFactory object for the Db subsystem.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"

//------------------------------------------------------------------------------
namespace Db
{
class Database;
class Command;
class Table;
class Dataset;
class FilterSet;
class Relation;
class ValueTable;
class Reader;
class Writer;

class DbFactory : public Core::RefCounted
{
    __DeclareClass(DbFactory);
    __DeclareSingleton(DbFactory);
public:
    /// constructor
    DbFactory();
    /// destructor
    virtual ~DbFactory();
    /// create a table object
    virtual Ptr<Table> CreateTable() const;
    /// create a database object
    virtual Ptr<Database> CreateDatabase() const;
    /// create a command object
    virtual Ptr<Command> CreateCommand() const;
    /// create a relation object
    virtual Ptr<Relation> CreateRelation() const;
    /// create a database reader
    virtual Ptr<Reader> CreateReader() const;
    /// create a database writer
    virtual Ptr<Writer> CreateWriter() const;

protected:
    friend class Database;
    friend class Table;
    friend class Dataset;
    friend class Writer;
    friend class Reader;

    /// create a dataset object
    virtual Ptr<Dataset> CreateDataset() const;
    /// create a value table
    virtual Ptr<ValueTable> CreateValueTable() const;
    /// create a filter object
    virtual Ptr<FilterSet> CreateFilterSet() const;
};

} // namespace Db
//------------------------------------------------------------------------------
#endif
