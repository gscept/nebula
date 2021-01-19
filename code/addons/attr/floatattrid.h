#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::FloatAttrId
    
    Typed attribute id for float type.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "attrid.h"

//------------------------------------------------------------------------------
namespace Attr
{
class FloatAttrId : public AttrId
{
public:
    /// construct from generic attribute id
    FloatAttrId(const AttrId& rhs);
    /// construct from attribute definition
    FloatAttrId(const AttributeDefinition<FloatTypeClass,float>& rhs);
    /// construct from name
    FloatAttrId(const Util::String& rhs);
    /// construct from fourcc code
    FloatAttrId(const Util::FourCC& rhs);
    /// equality operator
    bool operator==(const FloatAttrId& rhs) const;
    /// inequality operator
    bool operator!=(const FloatAttrId& rhs) const;
};

//------------------------------------------------------------------------------
/**
*/
inline
FloatAttrId::FloatAttrId(const AttrId& rhs) :
    AttrId(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
FloatAttrId::FloatAttrId(const AttributeDefinition<FloatTypeClass, float>& rhs) :
    AttrId(&rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
FloatAttrId::FloatAttrId(const Util::String& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByName(rhs);
    n_assert(this->GetValueType() == FloatType);
}

//------------------------------------------------------------------------------
/**
*/
inline
FloatAttrId::FloatAttrId(const Util::FourCC& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByFourCC(rhs);
    n_assert(this->GetValueType() == FloatType);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
FloatAttrId::operator==(const FloatAttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr == rhs.defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
FloatAttrId::operator!=(const FloatAttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr != rhs.defPtr);
}

} // namespace Attr
//------------------------------------------------------------------------------
