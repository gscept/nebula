#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::BlobAttrId
  
    Typed attribute id for blob type.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#include "attrid.h"

//------------------------------------------------------------------------------
namespace Attr
{
class BlobAttrId : public AttrId
{
public:
    /// construct from generic attribute id
    BlobAttrId(const AttrId& rhs);
    /// construct from attribute definition
    BlobAttrId(const AttributeDefinition<BlobTypeClass,const Util::Blob&>& rhs);
    /// construct from name
    BlobAttrId(const Util::String& rhs);
    /// construct from fourcc code
    BlobAttrId(const Util::FourCC& rhs);
    /// equality operator
    bool operator==(const BlobAttrId& rhs) const;
    /// inequality operator
    bool operator!=(const BlobAttrId& rhs) const;
};

//------------------------------------------------------------------------------
/**
*/
inline
BlobAttrId::BlobAttrId(const AttrId& rhs) :
    AttrId(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
BlobAttrId::BlobAttrId(const AttributeDefinition<BlobTypeClass,const Util::Blob&>& rhs) :
    AttrId(&rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
BlobAttrId::BlobAttrId(const Util::String& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByName(rhs);
    n_assert(this->GetValueType() == BlobType);
}

//------------------------------------------------------------------------------
/**
*/
inline
BlobAttrId::BlobAttrId(const Util::FourCC& rhs)
{
    this->defPtr = AttributeDefinitionBase::FindByFourCC(rhs);
    n_assert(this->GetValueType() == BlobType);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
BlobAttrId::operator==(const BlobAttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr == rhs.defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
BlobAttrId::operator!=(const BlobAttrId& rhs) const
{
    n_assert((0 != this->defPtr) && (0 != rhs.defPtr));
    return (this->defPtr != rhs.defPtr);
}

} // namespace Attr
//------------------------------------------------------------------------------
