#pragma once
//------------------------------------------------------------------------------
/**
    MemDb::AttributeRegistry

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "attribute.h"
#include "util/stringatom.h"

namespace MemDb
{

class AttributeRegistry
{
public:
    /// register a type (templated)
    template<typename TYPE>
    static AttributeId Register(Util::StringAtom name, TYPE defaultValue, uint32_t flags = 0);

    /// Check if a type is registered
    template <typename TYPE>
    static bool IsRegistered();

    /// register a POD, mem-copyable type
    static AttributeId Register(Util::StringAtom name, SizeT typeSize, void const* defaultValue, uint32_t flags = 0);

    /// get attribute id from name
    static AttributeId GetAttributeId(Util::StringAtom name);
    /// get attribute description by id
    static Attribute* GetAttribute(AttributeId descriptor);
    /// get type size by attribute id
    static SizeT TypeSize(AttributeId descriptor);
    /// get flags by attribute id
    static uint32_t Flags(AttributeId descriptor);
    /// get attribute default value pointer
    static void const* const DefaultValue(AttributeId descriptor);
    /// get an array of all attributes
    static Util::FixedArray<Attribute*> const& GetAllAttributes();

private:
    static AttributeRegistry* Instance();
    static void Destroy();

    AttributeRegistry();
    ~AttributeRegistry();

    static AttributeRegistry* Singleton;

    Util::FixedArray<Attribute*> componentDescriptions;
    Util::Dictionary<Util::StringAtom, AttributeId> registry;
};

/// Generates a new attribute id
uint16_t GenerateNewAttributeId();

//------------------------------------------------------------------------------
/**
*/
template <typename ATTRIBUTE>
AttributeId
GetAttributeId()
{
    static_assert(std::is_trivially_destructible<ATTRIBUTE>());
    static const uint16_t id = MemDb::GenerateNewAttributeId();
    return AttributeId(id);
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline bool
AttributeRegistry::IsRegistered()
{
    auto* reg = Instance();
    AttributeId id = MemDb::GetAttributeId<TYPE>();
    return reg->componentDescriptions[id.id] != nullptr;
}

//------------------------------------------------------------------------------
/**
    TYPE must be trivially copyable and destructible, and also standard layout.
    Essentially a POD type, but we do allow non-trivially-constructible types since
    components are created by copying the default value, not with constructors.
    The reason for this is because it allows us to do value initialization in declarations.
*/
template<typename TYPE>
inline AttributeId
AttributeRegistry::Register(Util::StringAtom name, TYPE defaultValue, uint32_t flags)
{
    // Special case for string atoms since they actually are trivial to copy and destroy
    //if constexpr (!std::is_same<TYPE, Util::StringAtom>())
    //{
    //    static_assert(std::is_trivially_copyable<TYPE>(), "TYPE must be trivially copyable.");
    //    static_assert(std::is_trivially_destructible<TYPE>(), "TYPE must be trivially destructible.");
    //}
    
    static_assert(!std::is_polymorphic<TYPE>(), "TYPE must not be polymorpic.");
    static_assert(!std::is_abstract<TYPE>(), "TYPE must not be abstract.");
    
    auto* reg = Instance();
    if (!reg->registry.Contains(name))
    {
        // setup a state description with the default values from the type
        Attribute* desc = new Attribute(name, defaultValue, flags);

        AttributeId descriptor = MemDb::GetAttributeId<TYPE>();
        if (descriptor.id >= reg->componentDescriptions.Size())
        {
            SizeT prevSize = reg->componentDescriptions.Size();
            reg->componentDescriptions.Resize(descriptor.id + 1); // fixed increment
            SizeT num = reg->componentDescriptions.Size() - prevSize;
            reg->componentDescriptions.Fill(prevSize, num, nullptr); // make sure new entries are null, since we use this to check if attributes are registered or not.
        }

        reg->componentDescriptions[descriptor.id] = desc;
        reg->registry.Add(name, descriptor);

        return descriptor;
    }
    else
    {
        n_error("Tried to register component named %s: Cannot register two components with same name!", name.Value());
    }

    return AttributeId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
inline AttributeId
AttributeRegistry::Register(Util::StringAtom name, SizeT typeSize, void const* defaultValue, uint32_t flags)
{
    auto* reg = Instance();
    if (!reg->registry.Contains(name))
    {
        // setup a state description with the default values from the type
        Attribute* desc = new Attribute(name, typeSize, defaultValue, flags);

        AttributeId descriptor = MemDb::GenerateNewAttributeId();
        if (descriptor.id >= reg->componentDescriptions.Size())
        {
            reg->componentDescriptions.Resize(descriptor.id + 1);

        }
        
        reg->componentDescriptions[descriptor.id] = desc;
        reg->registry.Add(name, descriptor);
        return descriptor;
    }
    else
    {
        n_error("Tried to register component named %s: Cannot register two components with same name!", name.Value());
    }

    return AttributeId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
inline AttributeId
AttributeRegistry::GetAttributeId(Util::StringAtom name)
{
    auto* reg = Instance();
    IndexT index = reg->registry.FindIndex(name);
    if (index != InvalidIndex)
    {
        return reg->registry.ValueAtIndex(index);
    }

    return AttributeId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
inline Attribute*
AttributeRegistry::GetAttribute(AttributeId descriptor)
{
    auto* reg = Instance();
    if (descriptor.id >= 0 && descriptor.id < reg->componentDescriptions.Size())
    {
        n_assert2(
            reg->componentDescriptions[descriptor.id] != NULL, "Trying to get description of attribute that is not registered!"
        );
        return reg->componentDescriptions[descriptor.id];
    }
    
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
AttributeRegistry::TypeSize(AttributeId descriptor)
{
    auto* reg = Instance();
    n_assert(descriptor.id >= 0 && descriptor.id < reg->componentDescriptions.Size());
    return reg->componentDescriptions[descriptor.id]->typeSize;
}

//------------------------------------------------------------------------------
/**
*/
inline uint32_t
AttributeRegistry::Flags(AttributeId descriptor)
{
    auto* reg = Instance();
    n_assert(descriptor.id >= 0 && descriptor.id < reg->componentDescriptions.Size());
    return reg->componentDescriptions[descriptor.id]->externalFlags;
}

//------------------------------------------------------------------------------
/**
*/
inline void const* const
AttributeRegistry::DefaultValue(AttributeId descriptor)
{
    auto* reg = Instance();
    n_assert(descriptor.id >= 0 && descriptor.id < reg->componentDescriptions.Size());
    return reg->componentDescriptions[descriptor.id]->defVal;
}

//------------------------------------------------------------------------------
/**
*/
inline Util::FixedArray<Attribute*> const&
AttributeRegistry::GetAllAttributes()
{
    auto* reg = Instance();
    return reg->componentDescriptions;
}

} // namespace MemDb
