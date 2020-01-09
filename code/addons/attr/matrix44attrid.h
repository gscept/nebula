#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::Matrix44AttrId
    
    Typed attribute id for matrix44 type.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "attrid.h"

//------------------------------------------------------------------------------
namespace Attr
{
class Matrix44AttrId : public AttrId
{
public:
    /// construct from generic attribute id
    Matrix44AttrId(const AttrId& rhs);
    /// construct from attribute definition
    Matrix44AttrId(const AttributeDefinition<Matrix44TypeClass,const Math::matrix44&>& rhs);
    /// construct from name
    Matrix44AttrId(const Util::String& rhs);
    /// construct from fourcc code
    Matrix44AttrId(const Util::FourCC& rhs);
    /// equality operator
    bool operator==(const Matrix44AttrId& rhs) const;
    /// inequality operator
    bool operator!=(const Matrix44AttrId& rhs) const;
};

//------------------------------------------------------------------------------
/**
*/
inline
Matrix44AttrId::Matrix44AttrId(const AttrId& rhs) :
    AttrId(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Matrix44AttrId::Matrix44AttrId(const AttributeDefinition<Matrix44TypeClass,const Math::matrix44&>& rhs) :
    AttrId(&rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Matrix44AttrId::Matrix44AttrId(const Util::String& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByName(rhs);
    n_assert(this->GetValueType() == Matrix44Type);
}

//------------------------------------------------------------------------------
/**
*/
inline
Matrix44AttrId::Matrix44AttrId(const Util::FourCC& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByFourCC(rhs);
    n_assert(this->GetValueType() == Matrix44Type);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
Matrix44AttrId::operator==(const Matrix44AttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr == rhs.defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
Matrix44AttrId::operator!=(const Matrix44AttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr != rhs.defPtr);
}

} // namespace Attr
//------------------------------------------------------------------------------
