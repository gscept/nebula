#pragma once
#ifndef DB_SQLITE3DATASET_H
#define DB_SQLITE3DATASET_H
//------------------------------------------------------------------------------
/**
    @class Db::Sqlite3Dataset
    
    SQLite implemention of Dataset.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "db/dataset.h"

//------------------------------------------------------------------------------
namespace Db
{
class Command;

class Sqlite3Dataset : public Dataset
{
    __DeclareClass(Sqlite3Dataset);
public:
    /// constructor
    Sqlite3Dataset();
    /// destructor
    virtual ~Sqlite3Dataset();
    /// fill value table from database
    virtual void PerformQuery(bool appendResult=false);
    /// commit modified values to the database
    virtual void CommitChanges(bool newRowsAsUpdate=false);
    /// commit deleted rows only to the database
    virtual void CommitDeletedRows();
    
private:
    /// connect to database
    virtual bool Connect();
    /// disconnect from database
    virtual void Disconnect();
    /// return SQL string fragment with selected columns
    Util::String GetSqlSelectColumns() const;

    Ptr<Command> queryCommand;

    static const Util::String SelectFrag;
    static const Util::String FromFrag;
    static const Util::String CommaFrag;
    static const Util::String WhereFrag;
    static const Util::String StarFrag;
    static const Util::String TickFrag;
};

} // namespace Db
//------------------------------------------------------------------------------
#endif
    