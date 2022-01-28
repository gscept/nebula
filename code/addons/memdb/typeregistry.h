#pragma once
//------------------------------------------------------------------------------
/**
    MemDb::TypeRegistry

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "componentdescription.h"
#include "util/stringatom.h"

namespace MemDb
{

class TypeRegistry
{
public:
    /// register a type (templated)
    template<typename TYPE>
    static ComponentId Register(Util::StringAtom name, TYPE defaultValue, uint32_t flags = 0);

    /// register a POD, mem-copyable type
    static ComponentId Register(Util::StringAtom name, SizeT typeSize, void const* defaultValue, uint32_t flags = 0);

    /// get component id from name
    static ComponentId GetComponentId(Util::StringAtom name);
    /// get component description by id
    static ComponentDescription* GetDescription(ComponentId descriptor);
    /// get type size by component id
    static SizeT TypeSize(ComponentId descriptor);
    /// get flags by component id
    static uint32_t Flags(ComponentId descriptor);
    /// get component default value pointer
    static void const* const DefaultValue(ComponentId descriptor);
    /// get an array of all component descriptions
    static Util::Array<ComponentDescription*> const& GetAllComponents();

private:
    static TypeRegistry* Instance();
    static void Destroy();

    TypeRegistry();
    ~TypeRegistry();

    static TypeRegistry* Singleton;

    Util::Array<ComponentDescription*> componentDescriptions;
    Util::Dictionary<Util::StringAtom, ComponentId> registry;
};

//------------------------------------------------------------------------------
/**
    TYPE must be trivially copyable and destructible, and also standard layout.
    Essentially a POD type, but we do allow non-trivially-constructible types since
    components are created by copying the default value, not with constructors.
    The reason for this is because it allows us to do value initialization in declarations.
*/
template<typename TYPE>
inline ComponentId
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
        ComponentDescription* desc = n_new(ComponentDescription(name, defaultValue, flags));

        ComponentId descriptor = reg->componentDescriptions.Size();
        reg->componentDescriptions.Append(desc);
        reg->registry.Add(name, descriptor);

        TYPE::id = descriptor;

        return descriptor;
    }
    else
    {
        n_error("Tried to register component named %s: Cannot register two components with same name!", name.Value());
    }

    return ComponentId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
inline ComponentId
TypeRegistry::Register(Util::StringAtom name, SizeT typeSize, void const* defaultValue, uint32_t flags)
{
    auto* reg = Instance();
    if (!reg->registry.Contains(name))
    {
        // setup a state description with the default values from the type
        ComponentDescription* desc = n_new(ComponentDescription(name, typeSize, defaultValue, flags));

        ComponentId descriptor = reg->componentDescriptions.Size();
        reg->componentDescriptions.Append(desc);
        reg->registry.Add(name, descriptor);
        return descriptor;
    }
    else
    {
        n_error("Tried to register component named %s: Cannot register two components with same name!", name.Value());
    }

    return ComponentId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
inline ComponentId
TypeRegistry::GetComponentId(Util::StringAtom name)
{
    auto* reg = Instance();
    IndexT index = reg->registry.FindIndex(name);
    if (index != InvalidIndex)
    {
        return reg->registry.ValueAtIndex(index);
    }

    return ComponentId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
inline ComponentDescription*
TypeRegistry::GetDescription(ComponentId descriptor)
{
    auto* reg = Instance();
    if (descriptor.id >= 0 && descriptor.id < reg->componentDescriptions.Size())
        return reg->componentDescriptions[descriptor.id];
    
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
TypeRegistry::TypeSize(ComponentId descriptor)
{
    auto* reg = Instance();
    n_assert(descriptor.id >= 0 && descriptor.id < reg->componentDescriptions.Size());
    return reg->componentDescriptions[descriptor.id]->typeSize;
}

//------------------------------------------------------------------------------
/**
*/
inline uint32_t
TypeRegistry::Flags(ComponentId descriptor)
{
    auto* reg = Instance();
    n_assert(descriptor.id >= 0 && descriptor.id < reg->componentDescriptions.Size());
    return reg->componentDescriptions[descriptor.id]->externalFlags;
}

//------------------------------------------------------------------------------
/**
*/
inline void const* const
TypeRegistry::DefaultValue(ComponentId descriptor)
{
    auto* reg = Instance();
    n_assert(descriptor.id >= 0 && descriptor.id < reg->componentDescriptions.Size());
    return reg->componentDescriptions[descriptor.id]->defVal;
}

//------------------------------------------------------------------------------
/**
*/
inline Util::Array<ComponentDescription*> const&
TypeRegistry::GetAllComponents()
{
    auto* reg = Instance();
    return reg->componentDescriptions;
}

} // namespace MemDb
