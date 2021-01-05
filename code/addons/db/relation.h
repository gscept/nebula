#pragma once
#ifndef DB_RELATION_H
#define DB_RELATION_H
//------------------------------------------------------------------------------
/**
    @class Db::Relation
  
    A Relation object describes a relation between 2 tables which are
    linked through a common attribute.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "db/table.h"

//------------------------------------------------------------------------------
namespace Db
{
class Relation : public Core::RefCounted
{
    __DeclareClass(Relation);
public:
    /// constructor
    Relation();
    /// destructor
    virtual ~Relation();
    /// set the parent table and column id
    void SetParent(const Ptr<Table>& table, const Attr::AttrId& columnId);
    /// get parent table
    const Ptr<Table>& GetParentTable() const;
    /// get parent column id
    const Attr::AttrId& GetParentColumnId() const;
    /// set child table and column id
    void SetChild(const Ptr<Table>& table, const Attr::AttrId& columnId);
    /// get child table
    const Ptr<Table>& GetChildTable() const;
    /// get child column id
    const Attr::AttrId& GetChildColumnId() const;

private:
    Ptr<Table> parentTable;
    Attr::AttrId parentColumnId;
    Ptr<Table> childTable;
    Attr::AttrId childColumnId;
};

//------------------------------------------------------------------------------
/**
*/
inline void
Relation::SetParent(const Ptr<Table>& table, const Attr::AttrId& columnId)
{
    n_assert(0 != table);
    this->parentTable = table;
    this->parentColumnId = columnId;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Table>&
Relation::GetParentTable() const
{
    return this->parentTable;
}

//------------------------------------------------------------------------------
/**
*/
inline const Attr::AttrId&
Relation::GetParentColumnId() const
{
    return this->parentColumnId;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Relation::SetChild(const Ptr<Table>& table, const Attr::AttrId& columnId)
{
    n_assert(0 != table);
    this->childTable = table;
    this->childColumnId = columnId;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Table>&
Relation::GetChildTable() const
{
    return this->childTable;
}

//------------------------------------------------------------------------------
/**
*/
inline const Attr::AttrId&
Relation::GetChildColumnId() const
{
    return this->childColumnId;
}

} // namespace Db
//------------------------------------------------------------------------------
#endif
