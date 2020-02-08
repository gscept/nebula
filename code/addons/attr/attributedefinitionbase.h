#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::AttributeDefinitionBase

    Implements a universal attribute definition, consisting of an attribute
    name, attribute fourcc code, value type and access type.

    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/fourcc.h"
#include "util/string.h"
#include "util/hashtable.h"
#include "util/dictionary.h"
#include "valuetype.h"
#include "accessmode.h"
#include "attrexithandler.h"
#include "game/entity.h"
#include "util/string.h"
#include "util/blob.h"
#include "math/quat.h"
#include "math/transform44.h"

//------------------------------------------------------------------------------
namespace Attr
{
class AttributeDefinitionBase
{
public:
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, bool isDynamic);
    /// Constructor with default byte value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const byte& defVal, bool isDynamic);
    /// Constructor with default short value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const short& defVal, bool isDynamic);
    /// Constructor with default ushort value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const ushort& defVal, bool isDynamic);
    /// Constructor with default int value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const int& defVal, bool isDynamic);
    /// Constructor with default uint value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const uint& defVal, bool isDynamic);
    /// Constructor with default int64 value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const int64_t& defVal, bool isDynamic);
    /// Constructor with default uint64 value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const uint64_t& defVal, bool isDynamic);
    /// Constructor with default float value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const float& defVal, bool isDynamic);
    /// Constructor with default double value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const double& defVal, bool isDynamic);
    /// Constructor with default bool value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const bool& defVal, bool isDynamic);
    /// Constructor with default float2 value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Math::vec2& defVal, bool isDynamic);
    /// Constructor with default float4 value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Math::vec4& defVal, bool isDynamic);
    /// Constructor with default quaternion value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Math::quat& defVal, bool isDynamic);
    /// Constructor with default string value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::String& defVal, bool isDynamic);
    /// Constructor with default mat4 value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Math::mat4& defVal, bool isDynamic);
    /// Constructor with default transform44 value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Math::transform44& defVal, bool isDynamic);
    /// Constructor with default blob value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Blob& defVal, bool isDynamic);
    /// Constructor with default guid value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Guid& defVal, bool isDynamic);
    /// Constructor with default void pointer value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, void* defVal, bool isDynamic);
    /// Constructor with default int array value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<int>& defVal, bool isDynamic);
    /// Constructor with default float array value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<float>& defVal, bool isDynamic);
    /// Constructor with default bool array value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<bool>& defVal, bool isDynamic);
    /// Constructor with default float2 array value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Math::vec2>& defVal, bool isDynamic);
    /// Constructor with default float4 array value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Math::vec4>& defVal, bool isDynamic);
    /// Constructor with default string array value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Util::String>& defVal, bool isDynamic);
    /// Constructor with default mat4 array value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Math::mat4>& defVal, bool isDynamic);
    /// Constructor with default blob array value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Util::Blob>& defVal, bool isDynamic);
    /// Constructor with default guid array value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Util::Guid>& defVal, bool isDynamic);
    /// Constructor with default entity value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Game::Entity& defVal, bool isDynamic);
    /// Constructor with default variant value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Variant& defVal, ValueType type, bool isDynamic);

    /// destructor
    ~AttributeDefinitionBase();
    /// static destruction method - call to cleanup the registry hashtable
    static void Destroy();

    /// return true if this is a dynamic attribute
    bool IsDynamic() const;
    /// get attribute name
    const Util::String& GetName() const;
    /// get fourcc code
    const Util::FourCC& GetFourCC() const;
    /// get type name
    const Util::String& GetTypeName() const;
    /// get default value
    const Util::Variant& GetDefaultValue() const;
    /// get size of type in bytes
    const uint GetSizeOfType() const;
    /// get access type
    AccessMode GetAccessMode() const;
    /// get value type
    ValueType GetValueType() const;
    /// find by name
    static const AttributeDefinitionBase* FindByName(const Util::String& n);
    /// find by FourCC
    static const AttributeDefinitionBase* FindByFourCC(const Util::FourCC& fcc);
    /// register a dynamic attribute
    static void RegisterDynamicAttribute(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, ValueType valueType, AccessMode accessMode);
    /// clear all dynamic attributes
    static void ClearDynamicAttributes();

    /// setup registry if it doesn't exist
    static void InitializeRegistry();

protected:
    /// register an attribute definition
    void Register();

private:
    /// default constructor is private
    AttributeDefinitionBase();
    /// check if name registry exists and create on demand
    static void CheckCreateNameRegistry();
    /// check if fourcc registry exists and create on demand
    static void CheckCreateFourCCRegistry();
    /// check if dynamic attribute array exists and create on demand
    static void CheckCreateDynamicAttributesArray();

    bool isDynamic;
    Util::String name;
    Util::String typeName;
    Util::FourCC fourCC;
    AccessMode accessMode;
    Util::Variant defaultValue;
    const Attr::ValueType valueType;

    friend class AttrId;
    static AttrExitHandler attrExitHandler;
    static Util::HashTable<Util::String, const AttributeDefinitionBase*>* NameRegistry;
    static Util::Dictionary<Util::FourCC, const AttributeDefinitionBase*>* FourCCRegistry;
    static Util::Array<const AttributeDefinitionBase*>* DynamicAttributes;

    /**
        @todo   Add constrains/attribute rules, ex. range of values, min, max, stepsize, rounding etc.
                can be a class that can be overloaded
    */
    

};

//------------------------------------------------------------------------------
/**
*/
inline
const AttributeDefinitionBase*
AttributeDefinitionBase::FindByName(const Util::String& n)
{
    n_assert(0 != NameRegistry);
    if (!NameRegistry->Contains(n))
    {
        return nullptr;
    }
    else
    {
        return (*NameRegistry)[n];
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
const AttributeDefinitionBase*
AttributeDefinitionBase::FindByFourCC(const Util::FourCC& fcc)
{
    n_assert(0 != FourCCRegistry);
    if (!FourCCRegistry->Contains(fcc))
    {
        return nullptr;
    }
    else
    {
        return (*FourCCRegistry)[fcc];
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
AttributeDefinitionBase::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::FourCC&
AttributeDefinitionBase::GetFourCC() const
{
    return this->fourCC;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
AttributeDefinitionBase::GetTypeName() const
{
    return this->typeName;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::Variant&
AttributeDefinitionBase::GetDefaultValue() const
{
    return this->defaultValue;
}

//------------------------------------------------------------------------------
/**
*/
inline
const uint
AttributeDefinitionBase::GetSizeOfType() const
{
    switch (this->valueType)
    {
    case VoidType:          return 0;
    case ByteType:          return sizeof(uint8);
    case ShortType:         return sizeof(uint16);
    case UShortType:        return sizeof(uint16);
    case IntType:           return sizeof(uint32);
    case UIntType:          return sizeof(uint32);
    case Int64Type:         return sizeof(uint64);
    case UInt64Type:        return sizeof(uint64);
    case FloatType:         return sizeof(float);
    case DoubleType:        return sizeof(double);
    case BoolType:          return sizeof(bool);
    case Float2Type:		return sizeof(float) * 2;
    case Float4Type:        return sizeof(float) * 4;
    case QuaternionType:    return sizeof(float) * 4;
    case StringType:        return sizeof(Util::String);
    case Matrix44Type:      return sizeof(Math::matrix44);
    case Transform44Type:   return sizeof(Math::transform44);
    case BlobType:          return sizeof(Util::Blob);
    case GuidType:          return sizeof(Util::Guid);
    case VoidPtrType:       return sizeof(void*);
    case IntArrayType:      return sizeof(Util::Array<int>);
    case FloatArrayType:    return sizeof(Util::Array<float>);
    case BoolArrayType:     return sizeof(Util::Array<bool>);
    case Float2ArrayType:	return sizeof(Util::Array<Math::float2>);
    case Float4ArrayType:   return sizeof(Util::Array<Math::float4>);
    case Matrix44ArrayType: return sizeof(Util::Array<Math::matrix44>);
    case StringArrayType:   return sizeof(Util::Array<Util::String>);
    case GuidArrayType:     return sizeof(Util::Array<Util::Guid>);
    case BlobArrayType:     return sizeof(Util::Array<Util::Blob>);
    default:
        n_error("AttributeDefinitionBase::GetSizeOfType(): invalid type enum '%d'!", this->valueType);
        return 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
ValueType
AttributeDefinitionBase::GetValueType() const
{
    return (ValueType)this->defaultValue.GetType();
}

//------------------------------------------------------------------------------
/**
*/
inline
AccessMode
AttributeDefinitionBase::GetAccessMode() const
{
    return this->accessMode;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
AttributeDefinitionBase::IsDynamic() const
{
    return this->isDynamic;
}

} // namespace Attr
//------------------------------------------------------------------------------
