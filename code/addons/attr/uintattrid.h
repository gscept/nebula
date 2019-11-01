#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::UIntAttrId
    
    Typed attribute id for unsigned integer type.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "attrid.h"

//------------------------------------------------------------------------------
namespace Attr
{
class UIntAttrId : public AttrId
{
public:
    /// construct from generic attribute id
    UIntAttrId(const AttrId& rhs);
    /// construct from attribute definition
    UIntAttrId(const AttributeDefinition<IntTypeClass,int>& rhs);
    /// construct from name
    UIntAttrId(const Util::String& rhs);
    /// construct from fourcc code
    UIntAttrId(const Util::FourCC& rhs);
    /// equality operator
    bool operator==(const UIntAttrId& rhs) const;
    /// inequality operator
    bool operator!=(const UIntAttrId& rhs) const;
};

//------------------------------------------------------------------------------
/**
*/
inline
UIntAttrId::UIntAttrId(const AttrId& rhs) :
    AttrId(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
UIntAttrId::UIntAttrId(const AttributeDefinition<IntTypeClass,int>& rhs) :
    AttrId(&rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
UIntAttrId::UIntAttrId(const Util::String& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByName(rhs);
    n_assert(this->GetValueType() == UIntType);
}

//------------------------------------------------------------------------------
/**
*/
inline
UIntAttrId::UIntAttrId(const Util::FourCC& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByFourCC(rhs);
    n_assert(this->GetValueType() == UIntType);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
UIntAttrId::operator==(const UIntAttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr == rhs.defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
UIntAttrId::operator!=(const UIntAttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr != rhs.defPtr);
}

} // namespace Attr
//------------------------------------------------------------------------------
