#pragma once
#ifndef DB_SQLITE3TABLE_H
#define DB_SQLITE3TABLE_H
//------------------------------------------------------------------------------
/**
    @class Db::Sqlite3Table
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "db/table.h"

//------------------------------------------------------------------------------
namespace Db
{
class Command;

class Sqlite3Table : public Table
{
    __DeclareClass(Sqlite3Table);
public:
    /// constructor
    Sqlite3Table();
    /// destructor
    virtual ~Sqlite3Table();

    /// connect the table object with a database
    virtual void Connect(const Ptr<Database>& db, ConnectMode connectMode, bool ignoreUnknownColumn);
    /// disconnect the table from a database
    virtual void Disconnect(bool dropTable);
    // create a complex multicolumn index on a connected table
    virtual void CreateMultiColumnIndex(const Util::Array<Attr::AttrId>& columnIds);
    /// bind a value table to the table object, unbinds previous table
    virtual void BindValueTable(const Ptr<ValueTable>& valueTable);
    /// unbind current value table from the table object, commits any changes to the database
    virtual void UnbindValueTable();

    /// set name of table
    virtual void SetName(const Util::String& n);
    /// commit any changes
    virtual void CommitChanges(bool resetModifiedState, bool useTransaction);
    /// commit uncommitted columns only
    virtual void CommitUncommittedColumns();
    /// commit deleted rows only
    virtual void CommitDeletedRows();

private:
    friend class Sqlite3Database;

    /// return true if database table exist
    bool TableExists();
    /// drop database table
    void DropTable();
    /// create database table
    void CreateTable();
    /// read the database table columns
    void ReadTableLayout(bool ignoreUnknownColumns);
    /// build a column definition SQL fragment
    Util::String BuildColumnDef(const Column& column);
    /// (re)compile the INSERT SQL command
    void CompileInsertCommand();
    /// (re)compile the UPDATE SQL command
    void CompileUpdateCommand();
    /// (re)compile the DELETE SQL command
    void CompileDeleteCommand();
    /// execute the pre-compiled INSERT command for each new row
    void ExecuteInsertCommand();
    /// execute the pre-compiled UPDATE command for each modified row
    void ExecuteUpdateCommand();
    /// execute the pre-compiled DELETE command for each deleted row
    void ExecuteDeleteCommand();
    /// bind a value from the value table to a wildcard of a compiled command
    void BindValueToCommand(const Ptr<Command>& cmd, IndexT wildcardIndex, IndexT valueTableColIndex, IndexT valueTableRowIndex);

    Ptr<Command> insertCommand;
    Ptr<Command> updateCommand;
    Ptr<Command> deleteCommand;

    // static string fragments for string construction (prevents excessive string object construction)
    static const Util::String InsertIntoFrag;
    static const Util::String OpenBracketFrag;
    static const Util::String CommaFrag;
    static const Util::String ValuesFrag;
    static const Util::String WildcardFrag;
    static const Util::String CloseBracketFrag;
    static const Util::String UpdateFrag;
    static const Util::String SetFrag;
    static const Util::String AssignWildcardFrag;
    static const Util::String TickFrag;
    static const Util::String IntegerFrag;
    static const Util::String RealFrag;
    static const Util::String TextFrag;
    static const Util::String BlobFrag;
    static const Util::String WhereFrag;
    static const Util::String DeleteFromFrag;
};    

} // namespace Db
//------------------------------------------------------------------------------
#endif

