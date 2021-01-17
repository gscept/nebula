#pragma once
#ifndef DB_SQLITE3COMMAND_H
#define DB_SQLITE3COMMAND_H
//------------------------------------------------------------------------------
/**
    @class Db::Sqlite3Command
    
    SQLite3 implementation of Db::Command.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "db/command.h"
#include "sqlite3.h"

//------------------------------------------------------------------------------
namespace Db
{
class Sqlite3Command : public Command
{
    __DeclareClass(Sqlite3Command);
public:
    /// constructor
    Sqlite3Command();
    /// destructor
    virtual ~Sqlite3Command();

    /// compile an SQL statement with optional placeholders
    virtual bool Compile(const Ptr<Database>& db, const Util::String& sqlCommand, ValueTable* resultTable=0);
    /// execute compiled command
    virtual bool Execute();
    /// clear the current command
    virtual void Clear();
    /// return true if command is compiled and ready for execution
    virtual bool IsValid() const;

    /// convert a parameter name into an integer index
    virtual IndexT IndexOf(const Util::String& name) const;
    /// convert a parameter attribute id into an integer index
    virtual IndexT IndexOf(const Attr::AttrId& attrId) const;

    /// bind an integer by placeholder index
    virtual void BindInt(IndexT index, int val);
    /// bind a float by placeholder index
    virtual void BindFloat(IndexT index, float val);
    /// bind a bool by placeholder index
    virtual void BindBool(IndexT index, bool val);
    /// bind a vec4 by placeholder index
    virtual void BindVec4(IndexT index, const Math::vec4& val);
    /// bind a string by placeholder index
    virtual void BindString(IndexT index, const Util::String& val);
    /// bind a Math::mat4 by placeholder index
    virtual void BindMat4(IndexT index, const Math::mat4& val);
    /// bind a blob by placeholder index
    virtual void BindBlob(IndexT index, const Util::Blob& val);
    /// bind a guid by placeholder index
    virtual void BindGuid(IndexT index, const Util::Guid& val);

    /// bind an integer by placeholder name
    virtual void BindInt(const Util::String& name, int val);
    /// bind a float by placeholder name
    virtual void BindFloat(const Util::String& name, float val);
    /// bind a bool by placeholder name
    virtual void BindBool(const Util::String& name, bool val);
    /// bind a vec4 by placeholder name
    virtual void BindVec4(const Util::String& name, const Math::vec4& val);
    /// bind a string by placeholder name
    virtual void BindString(const Util::String& name, const Util::String& val);
    /// bind a Math::mat4 by placeholder name
    virtual void BindMat4(const Util::String& name, const Math::mat4& val);
    /// bind a blob by placeholder name
    virtual void BindBlob(const Util::String& name, const Util::Blob& val);
    /// bind a guid by placeholder name
    virtual void BindGuid(const Util::String& name, const Util::Guid& val);

    /// bind an integer by placeholder attribute id
    virtual void BindInt(const Attr::AttrId& id, int val);
    /// bind a float by placeholder attribute id
    virtual void BindFloat(const Attr::AttrId& id, float val);
    /// bind a bool by placeholder attribute id
    virtual void BindBool(const Attr::AttrId& id, bool val);
    /// bind a float4 by placeholder attribute id
    virtual void BindVec4(const Attr::AttrId& id, const Math::vec4& val);
    /// bind a string by placeholder attribute id
    virtual void BindString(const Attr::AttrId& id, const Util::String& val);
    /// bind a Math::mat4 by placeholder attribute id
    virtual void BindMat4(const Attr::AttrId& id, const Math::mat4& val);
    /// bind a blob by placeholder attribute id
    virtual void BindBlob(const Attr::AttrId& id, const Util::Blob& val);
    /// bind a guid by placeholder attribute id
    virtual void BindGuid(const Attr::AttrId& id, const Util::Guid& val);

private:
    /// set current error string to SQLite's error message
    void SetSqliteError();
    /// add a new row of results
    void ReadRow();

    sqlite3_stmt* sqliteStatement;
    Util::Array<IndexT> resultIndexMap;
};

} // namespace Db
//------------------------------------------------------------------------------
#endif

