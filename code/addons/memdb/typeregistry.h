#pragma once
//------------------------------------------------------------------------------
/**
    MemDb::TypeRegistry

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "propertydescription.h"
#include "util/stringatom.h"

namespace MemDb
{

class TypeRegistry
{
public:
    /// register a type (templated)
    template<typename TYPE>
    static PropertyId Register(Util::StringAtom name, TYPE defaultValue, uint32_t flags = 0);

    /// register a POD, mem-copyable type
    static PropertyId Register(Util::StringAtom name, SizeT typeSize, void const* defaultValue, uint32_t flags = 0);

    /// get property id from name
    static PropertyId GetPropertyId(Util::StringAtom name);
    /// get property description by id
    static PropertyDescription* GetDescription(PropertyId descriptor);
    /// get type size by property id
    static SizeT TypeSize(PropertyId descriptor);
    /// get flags by property id
    static uint32_t Flags(PropertyId descriptor);
    /// get property default value pointer
    static void const* const DefaultValue(PropertyId descriptor);
    /// get an array of all property descriptions
    static Util::Array<PropertyDescription*> const& GetAllProperties();

private:
    static TypeRegistry* Instance();
    static void Destroy();

    TypeRegistry();
    ~TypeRegistry();

    static TypeRegistry* Singleton;

    Util::Array<PropertyDescription*> propertyDescriptions;
    Util::Dictionary<Util::StringAtom, PropertyId> registry;
};

//------------------------------------------------------------------------------
/**
    TYPE must be trivially copyable and destructible, and also standard layout.
    Essentially a POD type, but we do allow non-trivially-constructible types since
    properties are created by copying the default value, not with constructors.
    The reason for this is because it allows us to do value initialization in declarations.
*/
template<typename TYPE>
inline PropertyId
TypeRegistry::Register(Util::StringAtom name, TYPE defaultValue, uint32_t flags)
{
    // Special case for string atoms since they actually are trivial to copy and destroy
    //if constexpr (!std::is_same<TYPE, Util::StringAtom>())
    //{
    //    static_assert(std::is_trivially_copyable<TYPE>(), "TYPE must be trivially copyable.");
    //    static_assert(std::is_trivially_destructible<TYPE>(), "TYPE must be trivially destructible.");
    //}
    
    static_assert(std::is_standard_layout<TYPE>(), "TYPE must be standard layout.");
    
    auto* reg = Instance();
    if (!reg->registry.Contains(name))
    {
        // setup a state description with the default values from the type
        PropertyDescription* desc = n_new(PropertyDescription(name, defaultValue, flags));

        PropertyId descriptor = reg->propertyDescriptions.Size();
        reg->propertyDescriptions.Append(desc);
        reg->registry.Add(name, descriptor);

        TYPE::id = descriptor;

        return descriptor;
    }
    else
    {
        n_error("Tried to register property named %s: Cannot register two properties with same name!", name.Value());
    }

    return PropertyId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
inline PropertyId
TypeRegistry::Register(Util::StringAtom name, SizeT typeSize, void const* defaultValue, uint32_t flags)
{
    auto* reg = Instance();
    if (!reg->registry.Contains(name))
    {
        // setup a state description with the default values from the type
        PropertyDescription* desc = n_new(PropertyDescription(name, typeSize, defaultValue, flags));

        PropertyId descriptor = reg->propertyDescriptions.Size();
        reg->propertyDescriptions.Append(desc);
        reg->registry.Add(name, descriptor);
        return descriptor;
    }
    else
    {
        n_error("Tried to register property named %s: Cannot register two properties with same name!", name.Value());
    }

    return PropertyId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
inline PropertyId
TypeRegistry::GetPropertyId(Util::StringAtom name)
{
    auto* reg = Instance();
    IndexT index = reg->registry.FindIndex(name);
    if (index != InvalidIndex)
    {
        return reg->registry.ValueAtIndex(index);
    }

    return PropertyId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
inline PropertyDescription*
TypeRegistry::GetDescription(PropertyId descriptor)
{
    auto* reg = Instance();
    if (descriptor.id >= 0 && descriptor.id < reg->propertyDescriptions.Size())
        return reg->propertyDescriptions[descriptor.id];
    
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
TypeRegistry::TypeSize(PropertyId descriptor)
{
    auto* reg = Instance();
    n_assert(descriptor.id >= 0 && descriptor.id < reg->propertyDescriptions.Size());
    return reg->propertyDescriptions[descriptor.id]->typeSize;
}

//------------------------------------------------------------------------------
/**
*/
inline uint32_t
TypeRegistry::Flags(PropertyId descriptor)
{
    auto* reg = Instance();
    n_assert(descriptor.id >= 0 && descriptor.id < reg->propertyDescriptions.Size());
    return reg->propertyDescriptions[descriptor.id]->externalFlags;
}

//------------------------------------------------------------------------------
/**
*/
inline void const* const
TypeRegistry::DefaultValue(PropertyId descriptor)
{
    auto* reg = Instance();
    n_assert(descriptor.id >= 0 && descriptor.id < reg->propertyDescriptions.Size());
    return reg->propertyDescriptions[descriptor.id]->defVal;
}

//------------------------------------------------------------------------------
/**
*/
inline Util::Array<PropertyDescription*> const&
TypeRegistry::GetAllProperties()
{
    auto* reg = Instance();
    return reg->propertyDescriptions;
}

} // namespace MemDb
