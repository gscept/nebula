#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::Float4AttrId
    
    Typed attribute id for float4 type.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file	
*/
#include "attrid.h"

//------------------------------------------------------------------------------
namespace Attr
{
class Float4AttrId : public AttrId
{
public:
    /// construct from generic attribute id
    Float4AttrId(const AttrId& rhs);
    /// construct from attribute definition
    Float4AttrId(const AttributeDefinition<Float4TypeClass, const Math::float4&>& rhs);
    /// construct from name
    Float4AttrId(const Util::String& rhs);
    /// construct from fourcc code
    Float4AttrId(const Util::FourCC& rhs);
    /// equality operator
    bool operator==(const Float4AttrId& rhs) const;
    /// inequality operator
    bool operator!=(const Float4AttrId& rhs) const;
};

//------------------------------------------------------------------------------
/**
*/
inline
Float4AttrId::Float4AttrId(const AttrId& rhs) :
    AttrId(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Float4AttrId::Float4AttrId(const AttributeDefinition<Float4TypeClass, const Math::float4&>& rhs) :
    AttrId(&rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Float4AttrId::Float4AttrId(const Util::String& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByName(rhs);
    n_assert(this->GetValueType() == Float4Type);
}

//------------------------------------------------------------------------------
/**
*/
inline
Float4AttrId::Float4AttrId(const Util::FourCC& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByFourCC(rhs);
    n_assert(this->GetValueType() == Float4Type);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
Float4AttrId::operator==(const Float4AttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr == rhs.defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
Float4AttrId::operator!=(const Float4AttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr != rhs.defPtr);
}

} // namespace Attr
//------------------------------------------------------------------------------
