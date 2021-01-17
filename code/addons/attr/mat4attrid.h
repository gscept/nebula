#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::Mat4AttrId
    
    Typed attribute id for mat4 type.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "attrid.h"

//------------------------------------------------------------------------------
namespace Attr
{
class Mat4AttrId : public AttrId
{
public:
    /// construct from generic attribute id
    Mat4AttrId(const AttrId& rhs);
    /// construct from attribute definition
    Mat4AttrId(const AttributeDefinition<Matrix44TypeClass,const Math::mat4&>& rhs);
    /// construct from name
    Mat4AttrId(const Util::String& rhs);
    /// construct from fourcc code
    Mat4AttrId(const Util::FourCC& rhs);
    /// equality operator
    bool operator==(const Mat4AttrId& rhs) const;
    /// inequality operator
    bool operator!=(const Mat4AttrId& rhs) const;
};

//------------------------------------------------------------------------------
/**
*/
inline
Mat4AttrId::Mat4AttrId(const AttrId& rhs) :
    AttrId(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Mat4AttrId::Mat4AttrId(const AttributeDefinition<Matrix44TypeClass,const Math::mat4&>& rhs) :
    AttrId(&rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Mat4AttrId::Mat4AttrId(const Util::String& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByName(rhs);
    n_assert(this->GetValueType() == Mat4Type);
}

//------------------------------------------------------------------------------
/**
*/
inline
Mat4AttrId::Mat4AttrId(const Util::FourCC& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByFourCC(rhs);
    n_assert(this->GetValueType() == Mat4Type);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
Mat4AttrId::operator==(const Mat4AttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr == rhs.defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
Mat4AttrId::operator!=(const Mat4AttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr != rhs.defPtr);
}

} // namespace Attr
//------------------------------------------------------------------------------
