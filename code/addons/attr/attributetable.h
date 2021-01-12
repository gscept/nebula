#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::AttributeTable

    A table of attributes with a compact memory footprint and
    fast random access. Table columns are defined by attribute ids
    which associate a name, a fourcc code, a datatype and an access mode
    (ReadWrite, ReadOnly) to the table. Attribute values are stored in
    one big chunk of memory without additional overhead. Table cells can
    have the NULL status, which means the cell contains no value.

    The table's value buffer consists of 16-byte aligned rows, each
    row consists of a bitfield with 2 bits per row (one bit is set
    if a column/row value is valid, the other is used as modified-marker).

    Rows are allocated contigously in memory, which means padding might be
    inserted "between columns/values" to fulfill memory alignment requirements

    The AttributeTable object keeps track of all changes (added columns,
    added rows, modified rows, modified values).
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "attribute.h"

//------------------------------------------------------------------------------
namespace Attr
{
class AttributeTable : public Core::RefCounted
{
    __DeclareClass(AttributeTable);
public:
    /// constructor
    AttributeTable();
    /// construct with initializer list
    AttributeTable(std::initializer_list<Attr::AttrId> columns, bool recordColumns = false);
    /// destructor
    virtual ~AttributeTable();

    /// enable/disable modified tracking (default is on)
    void SetModifiedTracking(bool b);
    /// get modified tracking flag
    bool GetModifiedTracking() const;
    /// return true if the object has been modified since the last ResetModifiedState()
    bool IsModified() const;
    /// reset all the modified bits in the table
    void ResetModifiedState();
    /// clear the table object
    void Clear();
    /// print the contents of the table for debugging reasons
    void PrintDebug();
    
    /// optional: call before adding columns, speeds up adding many columns at once
    void BeginAddColumns(bool recordNewColumns=true);
    /// add a column
    void AddColumn(const AttrId& id, bool recordNewColumn=true);
    /// optional: call after adding columns, speeds up adding many columns at once
    void EndAddColumns();
    /// return true if a column exists
    bool HasColumn(const AttrId& id) const;
    /// return index of column by id
    IndexT GetColumnIndex(const AttrId& id) const;
    /// get number of columns
    SizeT GetNumColumns() const;
    /// get column definition at index
    const AttrId& GetColumnId(IndexT colIndex) const;
    /// get a column's name
    const Util::String& GetColumnName(IndexT colIndex) const;
    /// get column FourCC
    const Util::FourCC& GetColumnFourCC(IndexT colIndex) const;
    /// get a column's access mode
    AccessMode GetColumnAccessMode(IndexT colIndex) const;
    /// get a column's value type
    ValueType GetColumnValueType(IndexT colIndex) const;
    /// return indices of columns added since the last ResetModifiedState()
    const Util::Array<IndexT>& GetNewColumnIndices() const;
    /// return indices of all ReadWrite columns
    const Util::Array<IndexT>& GetReadWriteColumnIndices() const;

    /// add a row to the table, returns index of new row
    IndexT AddRow();
    /// clear the new row flags, so that new rows are treated like updated rows
    void ClearNewRowFlags();
    /// clear deleted rows flags
    void ClearDeletedRowsFlags();
    /// mark a row as deleted from the table
    void DeleteRow(IndexT rowIndex);
    /// mark all rows as deleted
    void DeleteAllRows();
    /// return true if row has been marked as deleted
    bool IsRowDeleted(IndexT rowIndex) const;
    /// create a new row as copy of another row
    IndexT CopyRow(IndexT rowIndex);
    /// Copy contents of one row into another
    void CopyRow(IndexT srcRowIndex, IndexT dstRowIndex);
    /// create a new row as copy of a row from another value table
    IndexT CopyExtRow(AttributeTable* other, IndexT otherRowIndex, bool createMissingRows = false);
    /// get number of rows in table
    SizeT GetNumRows() const;
    /// return true if table has new rows
    bool HasNewRows() const;
    /// return true if table has modified rows
    bool HasModifiedRows() const;
    /// return true if table has deleted rows
    bool HasDeletedRows() const;
    /// return true if a row has been modified since the last ResetModifiedState()
    bool IsRowModified(IndexT rowIndex) const;
    /// return indices of rows added since the last ResetModfiedState()
    const Util::Array<IndexT>& GetNewRowIndices() const;
    /// return indices of rows deleted since the last ResetModifiedState()
    const Util::Array<IndexT>& GetDeletedRowIndices() const;
    /// return array of modified rows, exclude rows marked as rows
    Util::Array<IndexT> GetModifiedRowsExcludeNewAndDeletedRows() const;
    /// reserve rows to reduce re-allocation overhead
    void ReserveRows(SizeT numRows);    
    /// find all matching row indices by attribute value
    Util::Array<IndexT> FindRowIndicesByAttr(const Attribute& attr, bool firstMatchOnly) const;
    /// find all matching row indices by multiple attribute values
    Util::Array<IndexT> FindRowIndicesByAttrs(const Util::Array<Attribute>& attrs, bool firstMatchOnly) const;
    /// find all matching row indices by attribute value
    IndexT FindRowIndexByAttr(const Attribute& attr) const;
    /// find all matching row indices by multiple attribute values
    IndexT FindRowIndexByAttrs(const Util::Array<Attribute>& attrs) const;
    /// set an optional row user data pointer
    void SetRowUserData(IndexT rowIndex, void* p);
    /// get optional row user data pointer
    void* GetRowUserData(IndexT rowIndex) const;

    /// set a generic attribute (slow!)
    void SetAttr(const Attr::Attribute& attr, IndexT rowIndex); 
    /// get a generic attribute
    Attr::Attribute GetAttr(IndexT rowIndex, IndexT colIndex) const;
    /// set variant value
    void SetVariant(const Attr::AttrId& attrId, IndexT rowIndex, const Util::Variant& val); 
    /// set bool value
    void SetBool(const BoolAttrId& colAttrId, IndexT rowIndex, bool val);
    /// get bool value
    bool GetBool(const BoolAttrId& colAttrId, IndexT rowIndex) const;
    /// set float value
    void SetFloat(const FloatAttrId& colAttrId, IndexT rowIndex, float val);
    /// get float value
    float GetFloat(const FloatAttrId& colAttrId, IndexT rowIndex) const;
    /// set int value
    void SetInt(const IntAttrId& colAttrId, IndexT rowIndex, int val);
    /// get int value
    int GetInt(const IntAttrId& colAttrId, IndexT rowIndex) const;
    /// set string value
    void SetString(const StringAttrId& colAttrId, IndexT rowIndex, const Util::String& val);
    /// get string value
    const Util::String& GetString(const StringAttrId& colAttrId, IndexT rowIndex) const;
    /// set float4 value
    void SetVec4(const Vec4AttrId& colAttrId, IndexT rowIndex, const Math::vec4& val);
    /// get float4 value
    Math::vec4 GetVec4(const Vec4AttrId& colAttrId, IndexT rowIndex) const;
    /// set mat4 value
    void SetMat4(const Mat4AttrId& colAttrId, IndexT rowIndex, const Math::mat4& val);
    /// get mat4 value
    Math::mat4 GetMat4(const Mat4AttrId& colAttrId, IndexT rowIndex) const;
    /// set guid value
    void SetGuid(const GuidAttrId& colAttrId, IndexT rowIndex, const Util::Guid& guid);
    /// get guid value
    const Util::Guid& GetGuid(const GuidAttrId& colAttrId, IndexT rowIndex) const;
    /// set blob value
    void SetBlob(const BlobAttrId& colAttrId, IndexT rowIndex, const Util::Blob& blob);
    /// get blob value
    const Util::Blob& GetBlob(const BlobAttrId& colAttrId, IndexT rowIndex) const;

    /// set variant value
    void SetVariant(IndexT colIndex, IndexT rowIndex, const Util::Variant& val); 
    /// set bool value by column index
    void SetBool(IndexT colIndex, IndexT rowIndex, bool val);
    /// get bool value by column index
    bool GetBool(IndexT colIndex, IndexT rowIndex) const;
    /// set float value by column index
    void SetFloat(IndexT colIndex, IndexT rowIndex, float val);
    /// get float value by column index
    float GetFloat(IndexT colIndex, IndexT rowIndex) const;
    /// set int value by column index
    void SetInt(IndexT colIndex, IndexT rowIndex, int val);
    /// get int value by column index
    int GetInt(IndexT colIndex, IndexT rowIndex) const;
    /// set uint value by column index
    void SetUInt(IndexT colIndex, IndexT rowIndex, uint val);
    /// get uint value by column index
    uint GetUInt(IndexT colIndex, IndexT rowIndex) const;
    /// set string value by column index
    void SetString(IndexT colIndex, IndexT rowIndex, const Util::String& val);
    /// get string value by column index
    const Util::String& GetString(IndexT colIndex, IndexT rowIndex) const;
    /// set float4 value by column index
    void SetVec4(IndexT colIndex, IndexT rowIndex, const Math::vec4& val);
    /// get float4 value by column index
    Math::vec4 GetVec4(IndexT colIndex, IndexT rowIndex) const;
    /// set mat4 value by column index
    void SetMat4(IndexT colIndex, IndexT rowIndex, const Math::mat4& val);
    /// get mat4 value by column index
    Math::mat4 GetMat4(IndexT colIndex, IndexT rowIndex) const;
    /// set guid value by column index
    void SetGuid(IndexT colIndex, IndexT rowIndex, const Util::Guid& guid);
    /// get guid value by column index
    const Util::Guid& GetGuid(IndexT colIndex, IndexT rowIndex) const;
    /// set blob value by column index
    void SetBlob(IndexT colIndex, IndexT rowIndex, const Util::Blob& blob);
    /// get blob value by column index
    const Util::Blob& GetBlob(IndexT colIndex, IndexT rowIndex) const;

    /// load xml table
    bool LoadXmlTable(const Util::String& fileName);

private:
    /// (re-)allocate the current buffer
    void Realloc(SizeT newPitch, SizeT newAllocRows);
    /// update the column byte offset and return new pitch
    SizeT UpdateColumnOffsets();
    /// returns the byte size of the given value type
    SizeT GetValueTypeSize(ValueType type) const;
    /// returns pointer to a value's memory location
    void* GetValuePtr(IndexT colIndex, IndexT rowIndex) const;
    /// delete all externally allocated data of this object
    void Delete();
    /// delete row data, frees all memory used in one row
    void DeleteRowData(IndexT rowIndex);
    /// delete a string from the table
    void DeleteString(IndexT colIndex, IndexT rowIndex);
    /// copy a string into the table
    void CopyString(IndexT colIndex, IndexT rowIndex, const Util::String& val);
    /// delete a blob from the table
    void DeleteBlob(IndexT colIndex, IndexT rowIndex);
    /// copy a blob into the table
    void CopyBlob(IndexT colIndex, IndexT rowIndex, const Util::Blob& val);
    /// delete a guid from the table
    void DeleteGuid(IndexT colIndex, IndexT rowIndex);
    /// copy a guid into the table
    void CopyGuid(IndexT colIndex, IndexT rowIndex, const Util::Guid& val);
    /// internal row-indices-by-attr find method
    Util::Array<IndexT> InternalFindRowIndicesByAttr(const Attr::Attribute& attr, bool firstMatchOnly) const;
    /// internal row-indices-by-multiple-attrs find method
    Util::Array<IndexT> InternalFindRowIndicesByAttrs(const Util::Array<Attr::Attribute>& attr, bool firstMatchOnly) const;
    /// internal helper method for adding a column
    void InternalAddColumnHelper(const AttrId& id, bool recordAsNewColumn);

    /// set entire column to its default values
    void SetColumnToDefaultValues(IndexT colIndex);
    /// set a row to its default values
    void SetRowToDefaultValues(IndexT rowIndex);
    
    /// set entire column float value
    void SetColumnBool(const BoolAttrId& attrId, bool val);
    /// set entire column float value
    void SetColumnFloat(const FloatAttrId& attrId, float val);
    /// set entire column to int value
    void SetColumnInt(const IntAttrId& attrId, int val);
    /// set entire column to string value
    void SetColumnString(const StringAttrId& attrId, const Util::String& val);
    /// set entire column vec4 value
    void SetColumnVec4(const Vec4AttrId& attrId, const Math::vec4& val);
    /// set entire column to mat4 value
    void SetColumnMat4(const Mat4AttrId& attrId, const Math::mat4& val);
    /// set entire column to guid value
    void SetColumnGuid(const GuidAttrId& attrId, const Util::Guid& guid);
    /// set entire column to blob value
    void SetColumnBlob(const BlobAttrId& attrId, const Util::Blob& blob);

    struct ColumnInfo
    {
        AttrId attrId;          // attribute id of the column
        IndexT byteOffset;      // byte offset into a row
    };
    Util::Array<ColumnInfo> columns;
    Util::Dictionary<AttrId,IndexT> indexMap;   // map attribute id to column index
    Util::Array<IndexT> readWriteColumnIndices;      // indices of all columns that are read/writable
    Util::Array<IndexT> newColumnIndices;       // indices of new columns since last ResetModifiedState
    Util::Array<IndexT> newRowIndices;          // indices of new rows since last ResetModifiedState
    Util::Array<IndexT> deletedRowIndices;      // indices of rows that are marked for deletion
    Util::Array<void*>  userData; 
    SizeT rowPitch;                             // pitch of a row in bytes
    SizeT numRows;                              // number of rows
    SizeT allocatedRows;                        // number of allocated rows
    void* valueBuffer;                          // points to value buffer
    uchar* rowModifiedBuffer;                   // 1 for a modified row, zero for not modified
    uchar* rowDeletedBuffer;                    // 1 for a deleted row, zero for not deleted
    uchar* rowNewBuffer;                        // 1 for a new row, zero for not new
    bool trackModifications;                    // modified tracking on/off
    bool isModified;                            // global modified flag
    bool rowsModified;                          // return true if modified rows exist
    bool inBeginAddColumns;                     // current inside BeginAddColumns()/EndAddColumns()
    bool addColumnsRecordAsNewColumns;          // recordAsNewColumns flag in BeginAddColumns
    IndexT firstNewColumnIndex;                 // set in BeginAddColumns
};

//------------------------------------------------------------------------------
/**
*/
inline
void
AttributeTable::SetModifiedTracking(bool b)
{
    this->trackModifications = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
AttributeTable::GetModifiedTracking() const
{
    return this->trackModifications;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
AttributeTable::HasNewRows() const
{
    return this->newRowIndices.Size() > 0;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
AttributeTable::HasDeletedRows() const
{
    return this->deletedRowIndices.Size() > 0;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
AttributeTable::HasModifiedRows() const
{
    return this->rowsModified;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::Array<IndexT>&
AttributeTable::GetReadWriteColumnIndices() const
{
    return this->readWriteColumnIndices;
}

//------------------------------------------------------------------------------
/**
    Quickly check if a row is modified.
*/
inline bool
AttributeTable::IsRowModified(IndexT rowIndex) const
{
    n_assert(rowIndex < this->numRows);
    n_assert(0 != this->rowModifiedBuffer);
    return (0 != this->rowModifiedBuffer[rowIndex]);
}

//------------------------------------------------------------------------------
/**
    Return true if a row has been marked as deleted.
*/
inline bool
AttributeTable::IsRowDeleted(IndexT rowIndex) const
{
    n_assert(rowIndex < this->numRows);
    n_assert(0 != this->rowDeletedBuffer);
    return (this->rowDeletedBuffer[rowIndex] == 1);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AttributeTable::IsModified() const
{
    return this->isModified;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<IndexT>&
AttributeTable::GetNewRowIndices() const
{
    return this->newRowIndices;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<IndexT>&
AttributeTable::GetDeletedRowIndices() const
{
    return this->deletedRowIndices;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<IndexT>&
AttributeTable::GetNewColumnIndices() const
{
    return this->newColumnIndices;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AttributeTable::HasColumn(const AttrId& id) const
{
    return this->indexMap.Contains(id);
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
AttributeTable::GetColumnIndex(const AttrId& id) const
{
    return this->indexMap[id];
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
AttributeTable::GetNumColumns() const
{
    return this->columns.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline const AttrId&
AttributeTable::GetColumnId(IndexT colIndex) const
{
    return this->columns[colIndex].attrId;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
AttributeTable::GetColumnName(IndexT colIndex) const
{
    return this->columns[colIndex].attrId.GetName();
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::FourCC&
AttributeTable::GetColumnFourCC(IndexT colIndex) const
{
    return this->columns[colIndex].attrId.GetFourCC();
}

//------------------------------------------------------------------------------
/**
*/
inline AccessMode
AttributeTable::GetColumnAccessMode(IndexT colIndex) const
{
    return this->columns[colIndex].attrId.GetAccessMode();
}

//------------------------------------------------------------------------------
/**
*/
inline ValueType
AttributeTable::GetColumnValueType(IndexT colIndex) const
{
    return this->columns[colIndex].attrId.GetValueType();
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
AttributeTable::GetNumRows() const
{
    return this->numRows;
}

//------------------------------------------------------------------------------
/**
    This returns a pointer to a value's memory location.
*/
inline void*
AttributeTable::GetValuePtr(IndexT colIndex, IndexT rowIndex) const
{
    n_assert((colIndex < this->columns.Size()) && (rowIndex < this->numRows));
    IndexT bufferOffset = (rowIndex * this->rowPitch) + this->columns[colIndex].byteOffset;
    return (void*)((char*)this->valueBuffer + bufferOffset);
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeTable::SetBool(IndexT colIndex, IndexT rowIndex, bool val)
{
    n_assert(this->GetColumnValueType(colIndex) == BoolType);
    n_assert(!this->IsRowDeleted(rowIndex));
    int* valuePtr = (int*) this->GetValuePtr(colIndex, rowIndex);
    *valuePtr = val ? 1 : 0;
    this->rowModifiedBuffer[rowIndex] = 1;
    this->isModified = true;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AttributeTable::GetBool(IndexT colIndex, IndexT rowIndex) const
{
    n_assert(this->GetColumnValueType(colIndex) == BoolType);
    int* valuePtr = (int*) this->GetValuePtr(colIndex, rowIndex);
    return (*valuePtr != 0);
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeTable::SetFloat(IndexT colIndex, IndexT rowIndex, float val)
{
    n_assert(this->GetColumnValueType(colIndex) == FloatType);
    n_assert(!this->IsRowDeleted(rowIndex));
    float* valuePtr = (float*) this->GetValuePtr(colIndex, rowIndex);
    *valuePtr = val;
    this->rowModifiedBuffer[rowIndex] = 1;
    this->isModified = true;
}

//------------------------------------------------------------------------------
/**
*/
inline float
AttributeTable::GetFloat(IndexT colIndex, IndexT rowIndex) const
{
    n_assert(this->GetColumnValueType(colIndex) == FloatType);
    float* valuePtr = (float*) this->GetValuePtr(colIndex, rowIndex);
    return *valuePtr;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeTable::SetInt(IndexT colIndex, IndexT rowIndex, int val)
{
    n_assert(this->GetColumnValueType(colIndex) == IntType);
    n_assert(!this->IsRowDeleted(rowIndex));
    int* valuePtr = (int*) this->GetValuePtr(colIndex, rowIndex);
    *valuePtr = val;
    this->rowModifiedBuffer[rowIndex] = 1;
    this->isModified = true;
}

//------------------------------------------------------------------------------
/**
*/
inline int
AttributeTable::GetInt(IndexT colIndex, IndexT rowIndex) const
{
    n_assert(this->GetColumnValueType(colIndex) == IntType);
    int* valuePtr = (int*) this->GetValuePtr(colIndex, rowIndex);
    return *valuePtr;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeTable::SetUInt(IndexT colIndex, IndexT rowIndex, uint val)
{
    n_assert(this->GetColumnValueType(colIndex) == UIntType);
    n_assert(!this->IsRowDeleted(rowIndex));
    uint* valuePtr = (uint*)this->GetValuePtr(colIndex, rowIndex);
    *valuePtr = val;
    this->rowModifiedBuffer[rowIndex] = 1;
    this->isModified = true;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
AttributeTable::GetUInt(IndexT colIndex, IndexT rowIndex) const
{
    n_assert(this->GetColumnValueType(colIndex) == UIntType);
    uint* valuePtr = (uint*)this->GetValuePtr(colIndex, rowIndex);
    return *valuePtr;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeTable::SetVec4(IndexT colIndex, IndexT rowIndex, const Math::vec4& val)
{
    n_assert(this->GetColumnValueType(colIndex) == Vec4Type);
    n_assert(!this->IsRowDeleted(rowIndex));
    Math::scalar* valuePtr = (Math::scalar*) this->GetValuePtr(colIndex, rowIndex);
    val.storeu(valuePtr);
    if (this->trackModifications)
    {
        this->rowModifiedBuffer[rowIndex] = 1;
        this->isModified = true;
        this->rowsModified = true;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline Math::vec4
AttributeTable::GetVec4(IndexT colIndex, IndexT rowIndex) const
{
    n_assert(this->GetColumnValueType(colIndex) == Vec4Type);
    Math::scalar* valuePtr = (Math::scalar*) this->GetValuePtr(colIndex, rowIndex);
    Math::vec4 vec;
    vec.loadu(valuePtr);
    return vec;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeTable::SetMat4(IndexT colIndex, IndexT rowIndex, const Math::mat4& val)
{
    n_assert(this->GetColumnValueType(colIndex) == Mat4Type);
    n_assert(!this->IsRowDeleted(rowIndex));
    Math::scalar* valuePtr = (Math::scalar*) this->GetValuePtr(colIndex, rowIndex);
    val.storeu(valuePtr);
    if (this->trackModifications)
    {
        this->rowModifiedBuffer[rowIndex] = 1;
        this->isModified = true;
        this->rowsModified = true;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline Math::mat4
AttributeTable::GetMat4(IndexT colIndex, IndexT rowIndex) const
{
    n_assert(this->GetColumnValueType(colIndex) == Mat4Type);
    Math::scalar* valuePtr = (Math::scalar*) this->GetValuePtr(colIndex, rowIndex);
    Math::mat4 mx;
    mx.loadu(valuePtr);
    return mx;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeTable::SetString(IndexT colIndex, IndexT rowIndex, const Util::String& val)
{
    n_assert(this->GetColumnValueType(colIndex) == StringType);
    n_assert(!this->IsRowDeleted(rowIndex));
    this->CopyString(colIndex, rowIndex, val);
    if (this->trackModifications)
    {
        this->rowModifiedBuffer[rowIndex] = 1;
        this->isModified = true;
        this->rowsModified = true;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
AttributeTable::GetString(IndexT colIndex, IndexT rowIndex) const
{
    n_assert(this->GetColumnValueType(colIndex) == StringType);
    Util::String** valuePtr = (Util::String**) this->GetValuePtr(colIndex, rowIndex);
    return **valuePtr;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeTable::SetGuid(IndexT colIndex, IndexT rowIndex, const Util::Guid& val)
{
    n_assert(this->GetColumnValueType(colIndex) == GuidType);
    n_assert(!this->IsRowDeleted(rowIndex));
    this->CopyGuid(colIndex, rowIndex, val);
    if (this->trackModifications)
    {
        this->rowModifiedBuffer[rowIndex] = 1;
        this->isModified = true;
        this->rowsModified = true;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Guid&
AttributeTable::GetGuid(IndexT colIndex, IndexT rowIndex) const
{
    n_assert(this->GetColumnValueType(colIndex) == GuidType);
    Util::Guid** valuePtr = (Util::Guid**) this->GetValuePtr(colIndex, rowIndex);
    return **valuePtr;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeTable::SetBlob(IndexT colIndex, IndexT rowIndex, const Util::Blob& val)
{
    n_assert(this->GetColumnValueType(colIndex) == BlobType);
    n_assert(!this->IsRowDeleted(rowIndex));
    this->CopyBlob(colIndex, rowIndex, val);
    if (this->trackModifications)
    {
        this->rowModifiedBuffer[rowIndex] = 1;
        this->isModified = true;
        this->rowsModified = true;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Blob&
AttributeTable::GetBlob(IndexT colIndex, IndexT rowIndex) const
{
    n_assert(this->GetColumnValueType(colIndex) == BlobType);
    Util::Blob** valuePtr = (Util::Blob**) this->GetValuePtr(colIndex, rowIndex);
    return **valuePtr;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeTable::SetBool(const BoolAttrId& colAttrId, IndexT rowIndex, bool val)
{
    this->SetBool(this->indexMap[colAttrId], rowIndex, val);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AttributeTable::GetBool(const BoolAttrId& colAttrId, IndexT rowIndex) const
{
    return this->GetBool(this->indexMap[colAttrId], rowIndex);
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeTable::SetFloat(const FloatAttrId& colAttrId, IndexT rowIndex, float val)
{
    this->SetFloat(this->indexMap[colAttrId], rowIndex, val);
}

//------------------------------------------------------------------------------
/**
*/
inline float
AttributeTable::GetFloat(const FloatAttrId& colAttrId, IndexT rowIndex) const
{
    return this->GetFloat(this->indexMap[colAttrId], rowIndex);
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeTable::SetInt(const IntAttrId& colAttrId, IndexT rowIndex, int val)
{
    this->SetInt(this->indexMap[colAttrId], rowIndex, val);
}

//------------------------------------------------------------------------------
/**
*/
inline int
AttributeTable::GetInt(const IntAttrId& colAttrId, IndexT rowIndex) const
{
    return this->GetInt(this->indexMap[colAttrId], rowIndex);
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeTable::SetVec4(const Vec4AttrId& colAttrId, IndexT rowIndex, const Math::vec4& val)
{
    this->SetVec4(this->indexMap[colAttrId], rowIndex, val);
}

//------------------------------------------------------------------------------
/**
*/
inline Math::vec4
AttributeTable::GetVec4(const Vec4AttrId& colAttrId, IndexT rowIndex) const
{
    return this->GetVec4(this->indexMap[colAttrId], rowIndex);
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeTable::SetMat4(const Mat4AttrId& colAttrId, IndexT rowIndex, const Math::mat4& val)
{
    this->SetMat4(this->indexMap[colAttrId], rowIndex, val);
}

//------------------------------------------------------------------------------
/**
*/
inline Math::mat4
AttributeTable::GetMat4(const Mat4AttrId& colAttrId, IndexT rowIndex) const
{
    return this->GetMat4(this->indexMap[colAttrId], rowIndex);
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeTable::SetString(const StringAttrId& colAttrId, IndexT rowIndex, const Util::String& val)
{
    this->SetString(this->indexMap[colAttrId], rowIndex, val);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
AttributeTable::GetString(const StringAttrId& colAttrId, IndexT rowIndex) const
{
    return this->GetString(this->indexMap[colAttrId], rowIndex);
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeTable::SetGuid(const GuidAttrId& colAttrId, IndexT rowIndex, const Util::Guid& val)
{
    this->SetGuid(this->indexMap[colAttrId], rowIndex, val);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Guid&
AttributeTable::GetGuid(const GuidAttrId& colAttrId, IndexT rowIndex) const
{
    return this->GetGuid(this->indexMap[colAttrId], rowIndex);
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeTable::SetBlob(const BlobAttrId& colAttrId, IndexT rowIndex, const Util::Blob& val)
{
    this->SetBlob(this->indexMap[colAttrId], rowIndex, val);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Blob&
AttributeTable::GetBlob(const BlobAttrId& colAttrId, IndexT rowIndex) const
{
    return this->GetBlob(this->indexMap[colAttrId], rowIndex);
}

} // namespace Db
//------------------------------------------------------------------------------
