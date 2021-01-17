#pragma once
#ifndef DB_COLUMN_H
#define DB_COLUMN_H
//------------------------------------------------------------------------------
/**
    @class Db::Column
    
    Describes a column in a database table. Mainly a wrapper around
    attribute id, but contains additional data, like if the column
    should be indexed, or if it contains a primary key.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "attr/attrid.h"

//------------------------------------------------------------------------------
namespace Db
{
class Column
{
public:
    /// column types
    enum Type
    {
        Default,        // a default column
        Primary,        // the primary column
        Indexed,        // an indexed column
    };
    /// default constructor
    Column();
    /// construct from attribute id
    Column(const Attr::AttrId& attrId, Type=Default);

    /// set attribute id (overrides name, fourcc, type and access mode)
    void SetAttrId(const Attr::AttrId& attrId);
    /// get attribute id
    const Attr::AttrId& GetAttrId() const;
    /// get column name 
    const Util::String& GetName() const;
    /// get column fourcc 
    const Util::FourCC& GetFourCC() const;
    /// get column value type
    Attr::ValueType GetValueType() const;
    /// get column access mode
    Attr::AccessMode GetAccessMode() const;
    /// set column type
    void SetType(Type t);
    /// get column type
    Type GetType() const;
    /// set to true when the column is in-sync with the database
    void SetCommitted(bool b);
    /// return true if the column has been sync'd with the database
    bool IsCommitted() const;

private:
    Attr::AttrId attrId;
    Type type;
    bool committed;
};

//------------------------------------------------------------------------------
/**
*/
inline void
Column::SetAttrId(const Attr::AttrId& id)
{
    this->attrId = id;
}

//------------------------------------------------------------------------------
/**
*/
inline const Attr::AttrId&
Column::GetAttrId() const
{
    return this->attrId;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
Column::GetName() const
{
    return this->attrId.GetName();
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::FourCC&
Column::GetFourCC() const
{
    return this->attrId.GetFourCC();
}

//------------------------------------------------------------------------------
/**
*/
inline Attr::ValueType
Column::GetValueType() const
{
    return this->attrId.GetValueType();
}

//------------------------------------------------------------------------------
/**
*/
inline Attr::AccessMode
Column::GetAccessMode() const
{
    return this->attrId.GetAccessMode();
}

//------------------------------------------------------------------------------
/**
*/
inline void
Column::SetType(Type t)
{
    this->type = t;
}

//------------------------------------------------------------------------------
/**
*/
inline Column::Type
Column::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Column::SetCommitted(bool b)
{
    this->committed = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Column::IsCommitted() const
{
    return this->committed;
}

} // namespace Db
//------------------------------------------------------------------------------
#endif
    