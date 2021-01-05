#pragma once
#ifndef DB_SQLITE3FACTORY_H
#define DB_SQLITE3FACTORY_H
//------------------------------------------------------------------------------
/**
    @class Db::Sqlite3Factory
    
    Creates Db objects derived for the Sqlite3 Db wrapper.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "db/dbfactory.h"

//------------------------------------------------------------------------------
namespace Db
{
class Sqlite3Factory : public DbFactory
{
    __DeclareClass(Sqlite3Factory);
    __DeclareSingleton(Sqlite3Factory);
public:
    /// constructor
    Sqlite3Factory();
    /// destructor
    virtual ~Sqlite3Factory();
    /// create a database object
    virtual Ptr<Database> CreateDatabase() const;
    /// create a command object
    virtual Ptr<Command> CreateCommand() const;
    /// create a table object
    virtual Ptr<Table> CreateTable() const;

protected:
    friend class Sqlite3Database;
    friend class Sqlite3Table;

    /// create a dataset object
    virtual Ptr<Dataset> CreateDataset() const;
    /// create a filter object
    virtual Ptr<FilterSet> CreateFilterSet() const;
};

} // namespace Db
//------------------------------------------------------------------------------
#endif
