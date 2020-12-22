#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::Vec4AttrId
    
    Typed attribute id for vec4 type.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file 
*/
#include "attrid.h"

//------------------------------------------------------------------------------
namespace Attr
{
class Vec4AttrId : public AttrId
{
public:
    /// construct from generic attribute id
    Vec4AttrId(const AttrId& rhs);
    /// construct from attribute definition
    Vec4AttrId(const AttributeDefinition<Float4TypeClass, const Math::vec4&>& rhs);
    /// construct from name
    Vec4AttrId(const Util::String& rhs);
    /// construct from fourcc code
    Vec4AttrId(const Util::FourCC& rhs);
    /// equality operator
    bool operator==(const Vec4AttrId& rhs) const;
    /// inequality operator
    bool operator!=(const Vec4AttrId& rhs) const;
};

//------------------------------------------------------------------------------
/**
*/
inline
Vec4AttrId::Vec4AttrId(const AttrId& rhs) :
    AttrId(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Vec4AttrId::Vec4AttrId(const AttributeDefinition<Float4TypeClass, const Math::vec4&>& rhs) :
    AttrId(&rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Vec4AttrId::Vec4AttrId(const Util::String& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByName(rhs);
    n_assert(this->GetValueType() == Vec4Type);
}

//------------------------------------------------------------------------------
/**
*/
inline
Vec4AttrId::Vec4AttrId(const Util::FourCC& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByFourCC(rhs);
    n_assert(this->GetValueType() == Vec4Type);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
Vec4AttrId::operator==(const Vec4AttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr == rhs.defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
Vec4AttrId::operator!=(const Vec4AttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr != rhs.defPtr);
}

} // namespace Attr
//------------------------------------------------------------------------------
