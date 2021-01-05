#pragma once
#ifndef DB_COMMAND_H
#define DB_COMMAND_H
//------------------------------------------------------------------------------
/**
    @class Db::Command
    
    Wraps a general SQL command. Commands may contain placeholders
    and can be precompiled for faster execution.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "attr/attrid.h"
#include "util/string.h"
#include "db/valuetable.h"

//------------------------------------------------------------------------------
namespace Db
{
class Database;

class Command : public Core::RefCounted
{
public:
    /// constructor
    Command();
    /// destructor
    virtual ~Command();
    
    /// compile an SQL statement with optional placeholders
    virtual bool Compile(const Ptr<Database>& db, const Util::String& sqlCommand, ValueTable* resultTable=0);
    /// execute compiled command
    virtual bool Execute() = 0;
    /// shortcut for simple SQL commands
    bool CompileAndExecute(const Ptr<Database>& db, const Util::String& sqlCommand, ValueTable* resultTable=0);
    /// clear the current command
    virtual void Clear() = 0;
    /// return true if command is compiled and ready for execution
    virtual bool IsValid() const = 0;
    /// get last error
    const Util::String& GetError() const;

    /// get back pointer to database
    const Ptr<Database>& GetDatabase() const;
    /// get the last compiled SQL command
    const Util::String& GetSqlCommand() const;
    /// get  the last set value table (can be 0!)
    const Ptr<ValueTable>& GetValueTable() const;

    /// convert a parameter name into an integer index
    virtual IndexT IndexOf(const Util::String& name) const =  0;
    /// convert a parameter attribute id into an integer index
    virtual IndexT IndexOf(const Attr::AttrId& attrId) const = 0;

    /// bind an integer by placeholder to 0-based index
    virtual void BindInt(IndexT index, int val) = 0;
    /// bind a float by placeholder index to 0-based index
    virtual void BindFloat(IndexT index, float val) = 0;
    /// bind a bool by placeholder index 0-based index
    virtual void BindBool(IndexT index, bool val) = 0;
    /// bind a vec4 by placeholder index 0-based index
    virtual void BindVec4(IndexT index, const Math::vec4& val) = 0;
    /// bind a string by placeholder index 0-based index
    virtual void BindString(IndexT index, const Util::String& val) = 0;
    /// bind a mat4 by placeholder index 0-based index
    virtual void BindMat4(IndexT index, const Math::mat4& val) = 0;
    /// bind a blob by placeholder index 0-based index
    virtual void BindBlob(IndexT index, const Util::Blob& val) = 0;
    /// bind a guid by placeholder index 0-based index
    virtual void BindGuid(IndexT index, const Util::Guid& val) = 0;

    /// bind an integer by placeholder name
    virtual void BindInt(const Util::String& name, int val) = 0;
    /// bind a float by placeholder name
    virtual void BindFloat(const Util::String& name, float val) = 0;
    /// bind a bool by placeholder name
    virtual void BindBool(const Util::String& name, bool val) = 0;
    /// bind a vec4 by placeholder name
    virtual void BindVec4(const Util::String& name, const Math::vec4& val) = 0;
    /// bind a string by placeholder name
    virtual void BindString(const Util::String& name, const Util::String& val) = 0;
    /// bind a mat4 by placeholder name
    virtual void BindMat4(const Util::String& name, const Math::mat4& val) = 0;
    /// bind a blob by placeholder name
    virtual void BindBlob(const Util::String& name, const Util::Blob& val) = 0;
    /// bind a guid by placeholder name
    virtual void BindGuid(const Util::String& name, const Util::Guid& val) = 0;

    /// bind an integer by placeholder attribute id
    virtual void BindInt(const Attr::AttrId& id, int val) = 0;
    /// bind a float by placeholder attribute id
    virtual void BindFloat(const Attr::AttrId& id, float val) = 0;
    /// bind a bool by placeholder attribute id
    virtual void BindBool(const Attr::AttrId& id, bool val) = 0;
    /// bind a vec4 by placeholder attribute id
    virtual void BindVec4(const Attr::AttrId& id, const Math::vec4& val) = 0;
    /// bind a string by placeholder attribute id
    virtual void BindString(const Attr::AttrId& id, const Util::String& val) = 0;
    /// bind a mat4 by placeholder attribute id
    virtual void BindMat4(const Attr::AttrId& id, const Math::mat4& val) = 0;
    /// bind a blob by placeholder attribute id
    virtual void BindBlob(const Attr::AttrId& id, const Util::Blob& val) = 0;
    /// bind a guid by placeholder attribute id
    virtual void BindGuid(const Attr::AttrId& id, const Util::Guid& val) = 0;

protected:
    /// set error string
    void SetError(const Util::String& err);
    /// set the SQL statement to exeute
    void SetSqlCommand(const Util::String& cmd);

    Util::String sqlCommand;
    Util::String error;
    Ptr<Database> database;
    Ptr<ValueTable> valueTable;
};

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<ValueTable>&
Command::GetValueTable() const
{
    return this->valueTable;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Database>&
Command::GetDatabase() const
{
    return this->database;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Command::SetSqlCommand(const Util::String& cmd)
{
    n_assert(cmd.IsValid());
    this->sqlCommand = cmd;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
Command::GetSqlCommand() const
{
    return this->sqlCommand;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Command::SetError(const Util::String& err)
{
    this->error = err;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
Command::GetError() const
{
    return this->error;
}

} // namespace Db
//------------------------------------------------------------------------------
#endif
