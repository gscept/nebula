#ifndef DB_READER_H
#define DB_READER_H
//------------------------------------------------------------------------------
/**
    @class Db::Reader

    Wrapper to bulk-read the contents of a database table.

    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "attr/attribute.h"
#include "db/dataset.h"
#include "db/valuetable.h"
#include "util/guid.h"
#include "util/blob.h"
#include "db/database.h"

//------------------------------------------------------------------------------
namespace Db
{
class Reader : public Core::RefCounted
{
    __DeclareClass(Reader);
public:
    /// constructor
    Reader();
    /// destructor
    virtual ~Reader();
    
    /// set database object
    void SetDatabase(const Ptr<Database>& db);
    /// set the database table name
    void SetTableName(const Util::String& n);
    /// add an optional filter/WHERE attribute 
    void AddFilterAttr(const Attr::Attribute& attr);

    /// open the reader, this will perform a query on the database
    bool Open();
    /// return true if reader is open
    bool IsOpen() const;
    /// close the reader
    void Close();

    /// get number of rows in the result
    int GetNumRows() const;
    /// set read cursor to specified row
    void SetToRow(int rowIndex);
    /// get current row index
    int GetCurrentRowIndex() const;

    /// return true if attribute exists at current row
    bool HasAttr(Attr::AttrId attrId) const;
    /// return bool attribute value
    bool GetBool(Attr::BoolAttrId attrId) const;
    /// return int attribute value
    int GetInt(Attr::IntAttrId attrId) const;
    /// return float attribute value
    float GetFloat(Attr::FloatAttrId attrId) const;
    /// return string attribute value
    const Util::String& GetString(Attr::StringAttrId attrId) const;    
    /// return vec4 attribute value
    const Math::vec4 GetVec4(Attr::Vec4AttrId attrId) const;
    /// return mat4 attribute value
    const Math::mat4 GetMat4(Attr::Mat4AttrId attrId) const;
    /// return guid attribute value
    const Util::Guid& GetGuid(Attr::GuidAttrId attrId) const;
    /// return blob attribute value
    const Util::Blob& GetBlob(Attr::BlobAttrId attrId) const;
    /// get direct pointer to value table
    const Ptr<Db::ValueTable>& GetValueTable() const;

private:    
    bool isOpen;
    Ptr<Database> database;
    Util::String tableName;
    Util::Array<Attr::Attribute> filterAttrs;
    Ptr<Dataset> dataset;
    Ptr<ValueTable> valueTable;
    IndexT curRowIndex;
};

//------------------------------------------------------------------------------
/**
*/
inline void
Reader::SetDatabase(const Ptr<Database>& db)
{
    this->database = db;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Reader::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Db::ValueTable>&
Reader::GetValueTable() const
{
    return this->valueTable;
}

//------------------------------------------------------------------------------
/**
    Set the name of database table the reader will work on.
*/
inline void
Reader::SetTableName(const Util::String& t)
{
    this->tableName = t;
}

//------------------------------------------------------------------------------
/**
    Add a filter attribute to the reader. The reader will create a
    database query where all filter attribute are ANDed.
*/
inline void
Reader::AddFilterAttr(const Attr::Attribute& attr)
{
    this->filterAttrs.Append(attr);
}

//------------------------------------------------------------------------------
/**
    Returns the number of rows in the result. Only valid while open.
*/
inline int
Reader::GetNumRows() const
{
    n_assert(this->isOpen);
    return this->dataset->Values()->GetNumRows();
}

//------------------------------------------------------------------------------
/**
    Sets the reader cursor to the specified row. Only valid while open.
*/
inline void
Reader::SetToRow(int rowIndex)
{
    n_assert(this->isOpen);
    n_assert((rowIndex >= 0) && (rowIndex < this->GetNumRows()));
    this->curRowIndex = rowIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline int
Reader::GetCurrentRowIndex() const
{
    return this->curRowIndex;
}

}; // namespace Db
//------------------------------------------------------------------------------
#endif

