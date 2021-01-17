#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::BoolAttrId
    
    Typed attribute id for bool type.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "attrid.h"

//------------------------------------------------------------------------------
namespace Attr
{
class BoolAttrId : public AttrId
{
public:
    /// construct from generic attribute id
    BoolAttrId(const AttrId& rhs);
    /// construct from attribute definition
    BoolAttrId(const AttributeDefinition<BoolTypeClass, bool>& rhs);
    /// construct from name
    BoolAttrId(const Util::String& rhs);
    /// construct from fourcc code
    BoolAttrId(const Util::FourCC& rhs);
    /// equality operator
    bool operator==(const BoolAttrId& rhs) const;
    /// inequality operator
    bool operator!=(const BoolAttrId& rhs) const;
};

//------------------------------------------------------------------------------
/**
*/
inline
BoolAttrId::BoolAttrId(const AttrId& rhs) :
    AttrId(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
BoolAttrId::BoolAttrId(const AttributeDefinition<BoolTypeClass, bool>& rhs) :
    AttrId(&rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
BoolAttrId::BoolAttrId(const Util::String& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByName(rhs);
    n_assert(this->GetValueType() == BoolType);
}

//------------------------------------------------------------------------------
/**
*/
inline
BoolAttrId::BoolAttrId(const Util::FourCC& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByFourCC(rhs);
    n_assert(this->GetValueType() == BoolType);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
BoolAttrId::operator==(const BoolAttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr == rhs.defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
BoolAttrId::operator!=(const BoolAttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr != rhs.defPtr);
}

} // namespace Attr
//------------------------------------------------------------------------------
