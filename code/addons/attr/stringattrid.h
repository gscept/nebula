#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::StringAttrId
  
    Typed attribute id for string type.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#include "stringattrid.h"

//------------------------------------------------------------------------------
namespace Attr
{
class StringAttrId : public AttrId
{
public:
    /// construct from generic attribute id
    StringAttrId(const AttrId& rhs);
    /// construct from attribute definition
    StringAttrId(const AttributeDefinition<StringTypeClass, const Util::String&>& rhs);
    /// construct from name
    StringAttrId(const Util::String& rhs);
    /// construct from fourcc code
    StringAttrId(const Util::FourCC& rhs);
    /// equality operator
    bool operator==(const StringAttrId& rhs) const;
    /// inequality operator
    bool operator!=(const StringAttrId& rhs) const;
};

//------------------------------------------------------------------------------
/**
*/
inline
StringAttrId::StringAttrId(const AttrId& rhs) :
    AttrId(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
StringAttrId::StringAttrId(const AttributeDefinition<StringTypeClass, const Util::String&>& rhs) :
    AttrId(&rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
StringAttrId::StringAttrId(const Util::String& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByName(rhs);
    n_assert(this->GetValueType() == StringType);
}

//------------------------------------------------------------------------------
/**
*/
inline
StringAttrId::StringAttrId(const Util::FourCC& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByFourCC(rhs);
    n_assert(this->GetValueType() == StringType);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
StringAttrId::operator==(const StringAttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr == rhs.defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
StringAttrId::operator!=(const StringAttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr != rhs.defPtr);
}

} // namespace Attr
//------------------------------------------------------------------------------
