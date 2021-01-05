#pragma once
#ifndef DB_VALUETABLE_H
#define DB_VALUETABLE_H
//------------------------------------------------------------------------------
/**
    @class Db::ValueTable
    
    A table of database values. This is the basic data container of the
    database subsystem. ValueTables are used to store the result of a
    query or to define the data which should be written back into the database.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "attr/attributetable.h"

//------------------------------------------------------------------------------
namespace Db
{
class ValueTable : public Attr::AttributeTable
{
    __DeclareClass(ValueTable);
public:
    /// constructor
    ValueTable();
    /// destructor
    virtual ~ValueTable();
}; 

} // namespace Db
//------------------------------------------------------------------------------
#endif
