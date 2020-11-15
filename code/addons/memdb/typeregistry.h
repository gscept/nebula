#pragma once
//------------------------------------------------------------------------------
/**
    MemDb::TypeRegistry

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
    /// register a type
    template<typename TYPE>
    static PropertyId Register(Util::StringAtom name, TYPE defaultValue);

    /// get property id from name
    static PropertyId GetPropertyId(Util::StringAtom name);
    /// get property description by id
    static PropertyDescription* GetDescription(PropertyId descriptor);

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
*/
template<typename TYPE>
inline PropertyId
TypeRegistry::Register(Util::StringAtom name, TYPE defaultValue)
{
    if constexpr (!std::is_same<TYPE, Util::StringAtom>())
    {
        static_assert(std::is_trivially_copyable<TYPE>(), "TYPE must be trivially copyable.");
        static_assert(std::is_trivially_destructible<TYPE>(), "TYPE must be trivially destructible.");
    }
    
    static_assert(std::is_standard_layout<TYPE>(), "TYPE must be standard layout.");
    
    auto* reg = Instance();
    if (!reg->registry.Contains(name))
    {
        // setup a state description with the default values from the type
        PropertyDescription* desc = n_new(PropertyDescription(name, defaultValue));

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

} // namespace MemDb
