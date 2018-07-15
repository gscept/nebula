#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::Transform44AttrId
    
    Typed attribute id for transform44 type.
        
    (C) 2016-2018 Individual contributors, see AUTHORS file
*/
#include "game/attr/attrid.h"

//------------------------------------------------------------------------------
namespace Attr
{
class Transform44AttrId : public AttrId
{
public:
    /// construct from generic attribute id
	Transform44AttrId(const AttrId& rhs);
    /// construct from attribute definition
	Transform44AttrId(const AttributeDefinition<Transform44TypeClass,const Math::transform44&>& rhs);
    /// construct from name
	Transform44AttrId(const Util::String& rhs);
    /// construct from fourcc code
	Transform44AttrId(const Util::FourCC& rhs);
    /// equality operator
    bool operator==(const Transform44AttrId& rhs) const;
    /// inequality operator
    bool operator!=(const Transform44AttrId& rhs) const;
};

//------------------------------------------------------------------------------
/**
*/
inline
Transform44AttrId::Transform44AttrId(const AttrId& rhs) :
    AttrId(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Transform44AttrId::Transform44AttrId(const AttributeDefinition<Transform44TypeClass,const Math::transform44&>& rhs) :
    AttrId(&rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Transform44AttrId::Transform44AttrId(const Util::String& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByName(rhs);
    n_assert(this->GetValueType() == Transform44Type);
}

//------------------------------------------------------------------------------
/**
*/
inline
Transform44AttrId::Transform44AttrId(const Util::FourCC& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByFourCC(rhs);
    n_assert(this->GetValueType() == Transform44Type);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
Transform44AttrId::operator==(const Transform44AttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr == rhs.defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
Transform44AttrId::operator!=(const Transform44AttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr != rhs.defPtr);
}

} // namespace Attr
//------------------------------------------------------------------------------
