//------------------------------------------------------------------------------
//  command.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "db/command.h"
#include "db/database.h"

namespace Db
{

//------------------------------------------------------------------------------
/**
*/
Command::Command()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Command::~Command()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This compiles an SQL statement against the provided database. The SQL
    statement may contain placeholders which should be filled with 
    values using the various BindXXX() methods. After values have been
    bound, the statement can be executed using the Execute() method.
*/
bool
Command::Compile(const Ptr<Database>& db, const Util::String& cmd, ValueTable* resultTable)
{
    n_assert(0 != db);
    this->SetSqlCommand(cmd);
    this->database = db;
    this->valueTable = resultTable;
    return true;
}

//------------------------------------------------------------------------------
/**
    This is a simple helper method which compiles and executes a simple
    SQL command in one step. This is only recommended for simple
    commands which are not executed repaetedly and don't take any
    parameters.
*/
bool
Command::CompileAndExecute(const Ptr<Database>& db, const Util::String& cmd, ValueTable* resultTable)
{
    if (this->Compile(db, cmd, resultTable))
    {
        return this->Execute();
    }
    return false;
}

}