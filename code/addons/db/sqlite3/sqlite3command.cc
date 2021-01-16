//------------------------------------------------------------------------------
//  sqlite3command.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "db/sqlite3/sqlite3command.h"
#include "db/sqlite3/sqlite3database.h"
#include "system/byteorder.h"

namespace Db
{
__ImplementClass(Db::Sqlite3Command, 'S3CD', Core::RefCounted);    // skip virtual base class

using namespace Util;
using namespace Math;
using namespace System;

//------------------------------------------------------------------------------
/**
*/
Sqlite3Command::Sqlite3Command() :
    sqliteStatement(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Sqlite3Command::~Sqlite3Command()
{
    this->Clear();
}

//------------------------------------------------------------------------------
/**
    Clears the currently compiled command.
*/
void
Sqlite3Command::Clear()
{
    if (0 != this->sqliteStatement)
    {
        sqlite3_finalize(this->sqliteStatement);
        this->sqliteStatement = 0;
    }
    this->valueTable = nullptr;
    this->resultIndexMap.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Command::SetSqliteError()
{
    // get database handle from our database
    Ptr<Sqlite3Database> db = this->database.downcast<Sqlite3Database>();
    n_assert(db->IsA(Sqlite3Database::RTTI));
    sqlite3* sqliteHandle = db->GetSqliteHandle();
    this->SetError(sqlite3_errmsg(sqliteHandle));
}

//------------------------------------------------------------------------------
/**
    Compiles a command with optional placeholders. Placeholders are
    identified by a leading $. After the command has been compiled,
    actual values can be bound to the placeholders using the Bind*()
    methods. When everything is in place, use Execute() to execute
    the command. If compilation fails, the method will return false. Get
    an error description with the method GetError().

    Remember that the SQL statement string must be UTF-8 encoded!
*/
bool
Sqlite3Command::Compile(const Ptr<Database>& db, const Util::String& sqlCommand, ValueTable* resultTable)
{
    // first: cleanup
    this->Clear();

    // then call parent class which initializes members
    Command::Compile(db, sqlCommand, resultTable);

    // get database handle from our database
    Ptr<Sqlite3Database> sqliteDatabase = db.downcast<Sqlite3Database>();
    n_assert(db->IsA(Sqlite3Database::RTTI));
    sqlite3* sqliteHandle = sqliteDatabase->GetSqliteHandle();

    // let SQLite compile the SQL command
    const char* cmdTail = 0;
    int err = sqlite3_prepare_v2(sqliteHandle, this->sqlCommand.AsCharPtr(), -1, &this->sqliteStatement, &cmdTail);
    if (err != SQLITE_OK)
    {
        this->SetSqliteError();
        return false;
    }
    n_assert(0 != this->sqliteStatement);

    // check if more then one SQL statement was in the string, we don't support that
    n_assert(0 != cmdTail);
    if (cmdTail[0] != 0)
    {
        n_error("Sqlite3Command::Compile(): Only one SQL statement allowed (cmd: %s)\n", this->sqlCommand.AsCharPtr());
        this->Clear();
        return false;
    }

    // create an index map to map value table indices to sqlite result indices
    if (this->valueTable.isvalid())
    {
        SizeT numResultColumns = sqlite3_column_count(this->sqliteStatement);
        if (numResultColumns > 0)
        {
            // if the value table has no columns we just add them now,
            // it is however better for performance if the value table
            // is pre-configured
            IndexT resultColumnIndex;
            if (0 == this->valueTable->GetNumColumns())
            {
                for (resultColumnIndex = 0; resultColumnIndex < numResultColumns; resultColumnIndex++)
                {
                    Attr::AttrId colAttrId(String(sqlite3_column_name(this->sqliteStatement, resultColumnIndex)));
                    if (!this->valueTable->HasColumn(colAttrId))
                    {
                        this->valueTable->AddColumn(colAttrId, false);
                    }
                }
            }

            // now create an index map which maps sqlite result indices to value table indices
            n_assert(this->resultIndexMap.Size() == 0);
            for (resultColumnIndex = 0; resultColumnIndex < numResultColumns; resultColumnIndex++)
            {
                Attr::AttrId colAttrId(String(sqlite3_column_name(this->sqliteStatement, resultColumnIndex)));
                if (valueTable->HasColumn(colAttrId))
                {
                    IndexT valueTableIndex = this->valueTable->GetColumnIndex(colAttrId);
                    this->resultIndexMap.Append(valueTableIndex);
                }
            }
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Returns true if the command has been compiled and is ready for
    execution, false if the command needs to be recompiled.
*/
bool
Sqlite3Command::IsValid() const
{
    if (0 != this->sqliteStatement)
    {
        // check if the sqlite statement must be recompiled
        if (0 == sqlite3_expired(this->sqliteStatement))
        {
            // all ok
            return true;
        }
    }
    // needs to be compiled
    return false;
}

//------------------------------------------------------------------------------
/**
    Execute a compiled command and gather the result.
*/
bool
Sqlite3Command::Execute()
{
    n_assert(this->IsValid());

    // turn off modification tracking in the value table
    if (this->valueTable.isvalid())
    {
        this->valueTable->SetModifiedTracking(false);
    }

    // execute the command in several steps
    bool done = false;
    while (!done)
    {
        int result = sqlite3_step(this->sqliteStatement);
        if (SQLITE_DONE == result)
        {
            // all ok and done
            done = true;
        }
        else if (SQLITE_BUSY == result)
        {
            // somebody else is currently locking the database, sleep a very little while
            n_sleep(0.0001);
        }
        else if (SQLITE_ROW == result)
        {
            if (this->valueTable.isvalid())
            {
                this->ReadRow();
            }
        }
        else if (SQLITE_ERROR == result)
        {
            this->SetSqliteError();
            return false;
        }
        else if (SQLITE_MISUSE == result)
        {
            n_error("Sqlite3Command::Execute(): sqlite3_step() returned SQLITE_MISUSE!");
            return false;
        }
        else
        {
            n_error("Sqlite3Command::Execute(): unknown error code returned from sqlite3_step() (CAN'T HAPPEN!)");
            return false;
        }
    }

    // reset the command, this clears the bound values
    int err = sqlite3_reset(this->sqliteStatement);
    n_assert(SQLITE_OK == err);

    // turn on modification tracking again
    if (this->valueTable.isvalid())
    {
        this->valueTable->SetModifiedTracking(true);
    }

    return true;
}

//------------------------------------------------------------------------------
/**
    Gather row of result values from SQLite and add them to the result
    ValueTable. Note that modification tracking is turned off in the
    value table, because reading from the database doesn't count as
    modification.
*/
void
Sqlite3Command::ReadRow()
{
    n_assert(0 != this->sqliteStatement);
    n_assert(this->valueTable.isvalid());

    // add a row to the result value table
    IndexT rowIndex = this->valueTable->AddRow();

    ByteOrder byteOrder(System::ByteOrder::LittleEndian, System::ByteOrder::Host);
    // for each column...
    IndexT resultColumnIndex;
    const SizeT numResultColumns = sqlite3_data_count(this->sqliteStatement);
    for (resultColumnIndex = 0; resultColumnIndex < numResultColumns; resultColumnIndex++)
    {
        // get the Sqlite column type, but this is only used for
        // NULL-value and assertion checks, the attribute id defines the 
        // actual data type!
        int sqliteColumnType = sqlite3_column_type(this->sqliteStatement, resultColumnIndex);
        if (SQLITE_NULL != sqliteColumnType)
        {
            // map result index to value table index
            IndexT valueTableColumnIndex = this->resultIndexMap[resultColumnIndex];
            switch (this->valueTable->GetColumnValueType(valueTableColumnIndex))
            {
                case Attr::IntType:
                    {
                        n_assert(SQLITE_INTEGER == sqliteColumnType);
                        int val = sqlite3_column_int(this->sqliteStatement, resultColumnIndex);                        
                        this->valueTable->SetInt(valueTableColumnIndex, rowIndex, val);
                    }
                    break;

                case Attr::FloatType:
                    {
                        n_assert(SQLITE_FLOAT == sqliteColumnType);
                        float val = (float) sqlite3_column_double(this->sqliteStatement, resultColumnIndex);                        
                        this->valueTable->SetFloat(valueTableColumnIndex, rowIndex, val);
                    }
                    break;

                case Attr::BoolType:
                    {
                        n_assert(SQLITE_INTEGER == sqliteColumnType);
                        int val = sqlite3_column_int(this->sqliteStatement, resultColumnIndex);
                        this->valueTable->SetBool(valueTableColumnIndex, rowIndex, (val == 1));
                    }
                    break;

                case Attr::Vec4Type:
                    {
                        n_assert(SQLITE_BLOB == sqliteColumnType);
                        const void* ptr = sqlite3_column_blob(this->sqliteStatement, resultColumnIndex);
                        uint size = sqlite3_column_bytes(this->sqliteStatement, resultColumnIndex);                        

                        vec4 alignedVal;
                        alignedVal.loadu((Math::scalar*)ptr);
                    #if NEBULA3_DATABASE_LEGACY_VECTORS
                        float4 value;
                        // nebula2 legacy support
                        if (size < sizeof(float4))
                        {
                            n_assert(size == 12);
                            // converted vector3, add 0
                            value.set(alignedVal.x(), alignedVal.y(), alignedVal.z(), 0);
                        }
                        else
                        {
                            n_assert(size == sizeof(float4));
                            value.set(alignedVal.x(), alignedVal.y(), alignedVal.z(), alignedVal.w());
                        }
                        
                        byteOrder.ConvertInPlace<Math::float4>(value);
                        this->valueTable->SetFloat4(valueTableColumnIndex, rowIndex, value);
                    #else
                        n_assert(size == sizeof(float4));
                        ByteOrder byteOrder(ByteOrder::LittleEndian, ByteOrder::Host);
                        byteOrder.ConvertInPlace<Math::vec4>(alignedVal);
                        /**ptr = ByteOrder::ConvertFloat4(ByteOrder::LittleEndian, ByteOrder::Host, *ptr);*/
                        this->valueTable->SetVec4(valueTableColumnIndex, rowIndex, alignedVal);
                    #endif
                    }
                    break;

                case Attr::StringType:
                    {
                        n_assert(SQLITE_TEXT == sqliteColumnType);
                        String val = (const char*) sqlite3_column_text(this->sqliteStatement, resultColumnIndex);
                        this->valueTable->SetString(valueTableColumnIndex, rowIndex, val);
                    }
                    break;

                case Attr::Mat4Type:
                    {
                        n_assert(SQLITE_BLOB == sqliteColumnType);
                        const void* ptr = sqlite3_column_blob(this->sqliteStatement, resultColumnIndex);
                        int size = sqlite3_column_bytes(this->sqliteStatement, resultColumnIndex);
                        n_assert(size == sizeof(Math::mat4));
                        
                        Math::mat4 alignedVal;
                        alignedVal.loadu((scalar*)ptr);
                        byteOrder.ConvertInPlace<Math::mat4>(alignedVal);
                        this->valueTable->SetMat4(valueTableColumnIndex, rowIndex, alignedVal);                                            
                    }                   
                    break;

                case Attr::BlobType:
                    {
                        n_assert(SQLITE_BLOB == sqliteColumnType);
                        const void* ptr = sqlite3_column_blob(this->sqliteStatement, resultColumnIndex);
                        int size = sqlite3_column_bytes(this->sqliteStatement, resultColumnIndex);
                        Blob blob(ptr, size);
                        this->valueTable->SetBlob(valueTableColumnIndex, rowIndex, blob);
                    }
                    break;

                case Attr::GuidType:
                    {
                        n_assert(SQLITE_BLOB == sqliteColumnType);
                        const unsigned char* ptr = (const unsigned char*) sqlite3_column_blob(this->sqliteStatement, resultColumnIndex);
                        int size = sqlite3_column_bytes(this->sqliteStatement, resultColumnIndex);
                        Guid guid(ptr, size);
                        this->valueTable->SetGuid(valueTableColumnIndex, rowIndex, guid);
                    }
                    break;

                default:
                    n_error("Sqlite3Command::ReadResultRow(): invalid attribute type!");
                    break;
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Returns the index of a placeholder value by name. The placeholder name
    must be UTF-8 encoded! NOTE: this method is slow because a temporary string
    must be created! It's much faster to work with unnamed wildcards ("?") and
    set them directly by index!
*/
IndexT
Sqlite3Command::IndexOf(const Util::String& name) const
{
    n_assert(0 != this->sqliteStatement);
 
    String wildcard = ":";
    wildcard.Append(name);

    IndexT index = sqlite3_bind_parameter_index(this->sqliteStatement, wildcard.AsCharPtr());
    n_assert2(0 != index, "Invalid wildcard name.");

    // Sqlite's indices are 1-based, convert to 0-based
    return (index - 1); 
}

//------------------------------------------------------------------------------
/**
    Returns the index of a placeholder value by attribute id. 
    NOTE: this method is slow because a temporary string
    must be created! It's much faster to work with unnamed wildcards ("?") and
    set them directly by index!
*/
IndexT
Sqlite3Command::IndexOf(const Attr::AttrId& attrId) const
{
    n_assert(0 != this->sqliteStatement);

    String wildcard = ":";
    wildcard.Append(attrId.GetName());

    IndexT index = sqlite3_bind_parameter_index(this->sqliteStatement, wildcard.AsCharPtr());
    n_assert2(0 != index, "Invalid wildcard name.");

    // Sqlite's indices are 1-based, convert to 0-based
    return (index - 1); 
}

//------------------------------------------------------------------------------
/**
    Bind an integer value by placeholder index. Either get the index from
    one of the IndexOf methods or use a 0-based index.
*/
void
Sqlite3Command::BindInt(IndexT index, int val)
{
    n_assert(0 != this->sqliteStatement);
    int err = sqlite3_bind_int(this->sqliteStatement, index + 1, val);
    n_assert(SQLITE_OK == err);
}

//------------------------------------------------------------------------------
/**
    Bind a float value by placeholder index. Either get the index from
    one of the IndexOf methods or use a 0-based index.
*/
void
Sqlite3Command::BindFloat(IndexT index, float val)
{
    n_assert(0 != this->sqliteStatement);
    int err = sqlite3_bind_double(this->sqliteStatement, index + 1, (double) val);
    n_assert(SQLITE_OK == err);
}

//------------------------------------------------------------------------------
/**
    Bind a bool value by placeholder index. Either get the index from
    one of the IndexOf methods or use a 0-based index.
*/
void
Sqlite3Command::BindBool(IndexT index, bool b)
{
    n_assert(0 != this->sqliteStatement);
    // NOTE: SQLite supports no native bools, instead we use 
    // an int (1=true, 0=false)
    int val = b ? 1 : 0;
    int err = sqlite3_bind_int(this->sqliteStatement, index + 1, val);
    n_assert(SQLITE_OK == err);
}

//------------------------------------------------------------------------------
/**
    Bind a float4 value by placeholder index. Either get the index from
    one of the IndexOf methods or use a 0-based index.
*/
void
Sqlite3Command::BindVec4(IndexT index, const vec4& val)
{
    n_assert(0 != this->sqliteStatement);
    // NOTE: float4's will be saved as blobs in the database, since the 
    // float4 may go away at any time, let SQLite make its own copy of the data
    ByteOrder byteOrder(System::ByteOrder::LittleEndian, System::ByteOrder::Host);
    vec4 convertedVal = val;
    byteOrder.ConvertInPlace<Math::vec4>(convertedVal);
    int err = sqlite3_bind_blob(this->sqliteStatement, index + 1, &convertedVal, sizeof(val), SQLITE_TRANSIENT);
    n_assert(SQLITE_OK == err);
}

//------------------------------------------------------------------------------
/**
    Bind a Math::mat4 value by placeholder index. Either get the index from
    one of the IndexOf methods or use a 0-based index.
*/
void
Sqlite3Command::BindMat4(IndexT index, const Math::mat4& val)
{
    n_assert(0 != this->sqliteStatement);
    // NOTE: Math::matrix44's will be saved as blobs in the database, since the 
    // Math::matrix44 may go away at any time, let SQLite make its own copy of the data
    ByteOrder byteOrder(System::ByteOrder::LittleEndian, System::ByteOrder::Host);
    mat4 convertedVal = val;
    byteOrder.ConvertInPlace<Math::mat4>(convertedVal);
    int err = sqlite3_bind_blob(this->sqliteStatement, index + 1, &val, sizeof(val), SQLITE_TRANSIENT);
    n_assert(SQLITE_OK == err);
}

//------------------------------------------------------------------------------
/**
    Bind a string value by placeholder index. Either get the index from
    one of the IndexOf methods or use a 0-based index.. NOTE: the 
    string should be in UTF-8 format.
*/
void
Sqlite3Command::BindString(IndexT index, const Util::String& val)
{
    n_assert(0 != this->sqliteStatement);
    int err = sqlite3_bind_text(this->sqliteStatement, index + 1, val.AsCharPtr(), -1, SQLITE_TRANSIENT);
    n_assert(SQLITE_OK == err);
}

//------------------------------------------------------------------------------
/**
    Bind a string value by placeholder index. Either get the index from
    one of the IndexOf methods or use a 0-based index. index.
*/
void
Sqlite3Command::BindBlob(IndexT index, const Util::Blob& val)
{
    n_assert(0 != this->sqliteStatement);
    n_assert(val.Size() < INT_MAX);
    if (val.IsValid())
    {
    int err = sqlite3_bind_blob(this->sqliteStatement, index + 1, val.GetPtr(), (SizeT)val.Size(), SQLITE_TRANSIENT);
    n_assert(SQLITE_OK == err);
}
}

//------------------------------------------------------------------------------
/**
    Bind a guid value by placeholder index. Either get the index from
    one of the IndexOf methods or use a 0-based index. index.
*/
void
Sqlite3Command::BindGuid(IndexT index, const Util::Guid& val)
{
    n_assert(0 != this->sqliteStatement);
    // NOTE: GUIDs will be stored as blobs in the database!
    const unsigned char* ptr = 0;
    SizeT size = val.AsBinary(ptr);
    int err = sqlite3_bind_blob(this->sqliteStatement, index + 1, ptr, size, SQLITE_TRANSIENT);
    n_assert(SQLITE_OK == err);
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Command::BindInt(const Util::String& name, int val)
{
    Sqlite3Command::BindInt(Sqlite3Command::IndexOf(name), val);
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Command::BindFloat(const Util::String& name, float val)
{
    Sqlite3Command::BindFloat(Sqlite3Command::IndexOf(name), val);
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Command::BindBool(const Util::String& name, bool val)
{
    Sqlite3Command::BindBool(Sqlite3Command::IndexOf(name), val);
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Command::BindVec4(const Util::String& name, const vec4& val)
{
    Sqlite3Command::BindVec4(Sqlite3Command::IndexOf(name), val);
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Command::BindMat4(const Util::String& name, const Math::mat4& val)
{
    Sqlite3Command::BindMat4(Sqlite3Command::IndexOf(name), val);
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Command::BindString(const Util::String& name, const Util::String& val)
{
    Sqlite3Command::BindString(Sqlite3Command::IndexOf(name), val);
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Command::BindBlob(const Util::String& name, const Util::Blob& val)
{
    Sqlite3Command::BindBlob(Sqlite3Command::IndexOf(name), val);
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Command::BindGuid(const Util::String& name, const Util::Guid& val)
{
    Sqlite3Command::BindGuid(Sqlite3Command::IndexOf(name), val);
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Command::BindInt(const Attr::AttrId& id, int val)
{
    Sqlite3Command::BindInt(Sqlite3Command::IndexOf(id), val);
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Command::BindFloat(const Attr::AttrId& id, float val)
{
    Sqlite3Command::BindFloat(Sqlite3Command::IndexOf(id), val);
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Command::BindBool(const Attr::AttrId& id, bool val)
{
    Sqlite3Command::BindBool(Sqlite3Command::IndexOf(id), val);
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Command::BindVec4(const Attr::AttrId& id, const vec4& val)
{
    Sqlite3Command::BindVec4(Sqlite3Command::IndexOf(id), val);
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Command::BindMat4(const Attr::AttrId& id, const Math::mat4& val)
{
    Sqlite3Command::BindMat4(Sqlite3Command::IndexOf(id), val);
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Command::BindString(const Attr::AttrId& id, const Util::String& val)
{
    Sqlite3Command::BindString(Sqlite3Command::IndexOf(id), val);
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Command::BindBlob(const Attr::AttrId& id, const Util::Blob& val)
{
    Sqlite3Command::BindBlob(Sqlite3Command::IndexOf(id), val);
}

//------------------------------------------------------------------------------
/**
*/
void
Sqlite3Command::BindGuid(const Attr::AttrId& id, const Util::Guid& val)
{
    Sqlite3Command::BindGuid(Sqlite3Command::IndexOf(id), val);
}

} // namespace Db
