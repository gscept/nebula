#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::GuidAttrId
    
    Typed attribute id for guid type.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "attrid.h"

//------------------------------------------------------------------------------
namespace Attr
{
class GuidAttrId : public AttrId
{
public:
    /// construct from generic attribute id
    GuidAttrId(const AttrId& rhs);
    /// construct from attribute definition
    GuidAttrId(const AttributeDefinition<GuidTypeClass,const Util::Guid&>& rhs);
    /// construct from name
    GuidAttrId(const Util::String& rhs);
    /// construct from fourcc code
    GuidAttrId(const Util::FourCC& rhs);
    /// equality operator
    bool operator==(const GuidAttrId& rhs) const;
    /// inequality operator
    bool operator!=(const GuidAttrId& rhs) const;
};

//------------------------------------------------------------------------------
/**
*/
inline
GuidAttrId::GuidAttrId(const AttrId& rhs) :
    AttrId(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
GuidAttrId::GuidAttrId(const AttributeDefinition<GuidTypeClass,const Util::Guid&>& rhs) :
    AttrId(&rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
GuidAttrId::GuidAttrId(const Util::String& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByName(rhs);
    n_assert(this->GetValueType() == GuidType);
}

//------------------------------------------------------------------------------
/**
*/
inline
GuidAttrId::GuidAttrId(const Util::FourCC& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByFourCC(rhs);
    n_assert(this->GetValueType() == GuidType);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
GuidAttrId::operator==(const GuidAttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr == rhs.defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
GuidAttrId::operator!=(const GuidAttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr != rhs.defPtr);
}

} // namespace Attr
//------------------------------------------------------------------------------
