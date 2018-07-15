#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::IntAttrId
    
    Typed attribute id for integer type.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "game/attr/attrid.h"

//------------------------------------------------------------------------------
namespace Attr
{
class IntAttrId : public AttrId
{
public:
    /// construct from generic attribute id
    IntAttrId(const AttrId& rhs);
    /// construct from attribute definition
    IntAttrId(const AttributeDefinition<IntTypeClass,int>& rhs);
    /// construct from name
    IntAttrId(const Util::String& rhs);
    /// construct from fourcc code
    IntAttrId(const Util::FourCC& rhs);
    /// equality operator
    bool operator==(const IntAttrId& rhs) const;
    /// inequality operator
    bool operator!=(const IntAttrId& rhs) const;
};

//------------------------------------------------------------------------------
/**
*/
inline
IntAttrId::IntAttrId(const AttrId& rhs) :
    AttrId(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
IntAttrId::IntAttrId(const AttributeDefinition<IntTypeClass,int>& rhs) :
    AttrId(&rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
IntAttrId::IntAttrId(const Util::String& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByName(rhs);
    n_assert(this->GetValueType() == IntType);
}

//------------------------------------------------------------------------------
/**
*/
inline
IntAttrId::IntAttrId(const Util::FourCC& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByFourCC(rhs);
    n_assert(this->GetValueType() == IntType);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
IntAttrId::operator==(const IntAttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr == rhs.defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
IntAttrId::operator!=(const IntAttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr != rhs.defPtr);
}

} // namespace Attr
//------------------------------------------------------------------------------
