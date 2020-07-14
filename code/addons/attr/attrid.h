#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::AttrId
  
    An attribute ID is used to carry attribute types (no values) around.
    Attribute IDs are compile-time safe, since each attribute ID represents
    its own C++ type.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "attributedefinition.h"

//------------------------------------------------------------------------------
namespace Attr
{
class AttrId
{
public:
    /// default constructor
    AttrId();
    /// copy constructor
    AttrId(const AttrId& rhs);
    /// construct from attribute definition class
    AttrId(const AttributeDefinitionBase& def);
    /// construct from pointer to definition class
    AttrId(const AttributeDefinitionBase* ptr);
    /// construct from attribute name
    AttrId(const Util::String& name);
    /// construct from attribute fourcc code
    AttrId(const Util::FourCC& fcc);

    /// return true if the provided attribute id name is valid
    static bool IsValidName(const Util::String& n);
    /// return true if the provided attribute id fourcc is valid
    static bool IsValidFourCC(const Util::FourCC& fcc);

    /// equality operator
    bool operator==(const AttrId& rhs) const;
    /// inequality operator
    bool operator!=(const AttrId& rhs) const;
    /// greater operator
    bool operator>(const AttrId& rhs) const;
    /// greater-or-equal operator
    bool operator>=(const AttrId& rhs) const;
    /// lesser operator
    bool operator<(const AttrId& rhs) const;
    /// lesser-or-equal operator
    bool operator<=(const AttrId& rhs) const;

    /// get attribute name
    const Util::String& GetName() const;
    /// get attribute type name
    const Util::String& GetTypeName() const;
    /// get fourcc code
    const Util::FourCC& GetFourCC() const;
    /// get access type
    AccessMode GetAccessMode() const;
    /// get value type
    ValueType GetValueType() const;
    /// return true if this attribute was dynamically registered
    bool IsDynamic() const;
    /// return true if the attribute id object is valid
    bool IsValid() const;
    /// return the byte size of the type
    uint GetSizeOfType() const;

    /// get bool default value
    bool GetBoolDefValue() const;
    /// get int default value
    int GetIntDefValue() const;
    /// get uint default value
    uint GetUIntDefValue() const;
    /// get floar default value
    float GetFloatDefValue() const;
    /// get string default value
    const Util::String& GetStringDefValue() const;
    /// get float4 default value
    Math::vec4 GetVec4DefValue() const;
    /// get mat4 default value
    const Math::mat4& GetMat4DefValue() const;
    /// get blob default value
    const Util::Blob& GetBlobDefValue() const;
    /// get guid default value
    const Util::Guid& GetGuidDefValue() const;
    /// get default value variant
    const Util::Variant& GetDefaultValue() const;

    /// return all attribute id's (slow!)
    static Util::FixedArray<AttrId> GetAllAttrIds();

protected:
    const AttributeDefinitionBase* defPtr;
};

//------------------------------------------------------------------------------
/**
*/
inline
AttrId::AttrId() :
    defPtr(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
AttrId::AttrId(const AttrId& rhs) :
    defPtr(rhs.defPtr)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
AttrId::AttrId(const AttributeDefinitionBase& def) :
    defPtr(&def)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
AttrId::AttrId(const AttributeDefinitionBase* ptr) :
    defPtr(ptr)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
AttrId::AttrId(const Util::String& name)
{
    this->defPtr = AttributeDefinitionBase::FindByName(name);
    if (0 == this->defPtr)
    {
        n_error("AttrId::AttrId(): invalid attribute name '%s'\n", name.AsCharPtr());
    }
}
//------------------------------------------------------------------------------
/**
*/
inline
AttrId::AttrId(const Util::FourCC& fcc)
{
    this->defPtr = AttributeDefinitionBase::FindByFourCC(fcc);
    n_assert(0 != this->defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AttrId::IsValidName(const Util::String& n)
{
    return 0 != AttributeDefinitionBase::FindByName(n);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AttrId::IsValidFourCC(const Util::FourCC& fcc)
{
    return 0 != AttributeDefinitionBase::FindByFourCC(fcc);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
AttrId::IsValid() const
{
    return (0 != this->defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
uint
AttrId::GetSizeOfType() const
{
    n_assert(this->IsValid());
    return this->defPtr->GetSizeOfType();
    
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
AttrId::operator==(const AttrId& rhs) const
{
    n_assert(this->IsValid() && rhs.IsValid());
    return (this->defPtr == rhs.defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
AttrId::operator!=(const AttrId& rhs) const
{
    n_assert(this->IsValid() && rhs.IsValid());
    return (this->defPtr != rhs.defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
AttrId::operator>(const AttrId& rhs) const
{
    n_assert(this->IsValid() && rhs.IsValid());
    return (this->defPtr > rhs.defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
AttrId::operator>=(const AttrId& rhs) const
{
    n_assert(this->IsValid() && rhs.IsValid());
    return (this->defPtr >= rhs.defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
AttrId::operator<(const AttrId& rhs) const
{
    n_assert(this->IsValid() && rhs.IsValid());
    return (this->defPtr < rhs.defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
AttrId::operator<=(const AttrId& rhs) const
{
    n_assert(this->IsValid() && rhs.IsValid());
    return (this->defPtr <= rhs.defPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
AttrId::GetName() const
{
    n_assert(this->IsValid());
    return this->defPtr->GetName();
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
AttrId::GetTypeName() const
{
    n_assert(this->IsValid());
    return this->defPtr->GetTypeName();
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::FourCC&
AttrId::GetFourCC() const
{
    n_assert(this->IsValid());
    return this->defPtr->GetFourCC();
}

//------------------------------------------------------------------------------
/**
*/
inline
AccessMode
AttrId::GetAccessMode() const
{
    n_assert(this->IsValid());
    return this->defPtr->GetAccessMode();
}

//------------------------------------------------------------------------------
/**
*/
inline
ValueType
AttrId::GetValueType() const
{
    n_assert(this->IsValid());
    return this->defPtr->GetValueType();
}
//------------------------------------------------------------------------------
/**
*/
inline
bool
AttrId::IsDynamic() const
{
    n_assert(0 != this->defPtr);
    return this->defPtr->IsDynamic();
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
AttrId::GetBoolDefValue() const
{
    n_assert(0 != this->defPtr);
    return this->defPtr->GetDefaultValue().GetBool();
}

//------------------------------------------------------------------------------
/**
*/
inline
int
AttrId::GetIntDefValue() const
{
    n_assert(0 != this->defPtr);
    return this->defPtr->GetDefaultValue().GetInt();
}

//------------------------------------------------------------------------------
/**
*/
inline
uint
AttrId::GetUIntDefValue() const
{
    n_assert(0 != this->defPtr);
    return this->defPtr->GetDefaultValue().GetUInt();
}

//------------------------------------------------------------------------------
/**
*/
inline
float
AttrId::GetFloatDefValue() const
{
    n_assert(0 != this->defPtr);
    return this->defPtr->GetDefaultValue().GetFloat();
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
AttrId::GetStringDefValue() const
{
    n_assert(0 != this->defPtr);
    return this->defPtr->GetDefaultValue().GetString();
}

//------------------------------------------------------------------------------
/**
*/
inline
Math::vec4
AttrId::GetVec4DefValue() const
{
    n_assert(0 != this->defPtr);
    return this->defPtr->GetDefaultValue().GetVec4();
}

//------------------------------------------------------------------------------
/**
*/
inline
const Math::mat4&
AttrId::GetMat4DefValue() const
{
    n_assert(0 != this->defPtr);
    return this->defPtr->GetDefaultValue().GetMat4();
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::Blob&
AttrId::GetBlobDefValue() const
{
    n_assert(0 != this->defPtr);
    return this->defPtr->GetDefaultValue().GetBlob();
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::Guid&
AttrId::GetGuidDefValue() const
{
    n_assert(0 != this->defPtr);
    return this->defPtr->GetDefaultValue().GetGuid();
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::Variant&
AttrId::GetDefaultValue() const
{
    n_assert(0 != this->defPtr);
    return this->defPtr->GetDefaultValue();
}

} // namespace Attr
//------------------------------------------------------------------------------
