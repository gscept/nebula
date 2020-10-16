#pragma once
//------------------------------------------------------------------------------
/**
    MemDb::TypeRegistry

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "columndescription.h"
#include "util/stringatom.h"

namespace MemDb
{

class TypeRegistry
{
public:
    static TypeRegistry* Instance();
    static void Destroy();

    template<typename TYPE>
    static ColumnDescriptor Register(Util::StringAtom name, TYPE defaultValue);

    static ColumnDescriptor GetDescriptor(Util::StringAtom name);
    static ColumnDescription* GetDescription(ColumnDescriptor descriptor);

private:
    TypeRegistry();
    ~TypeRegistry();

    static TypeRegistry* Singleton;

    Util::Array<ColumnDescription*> columnDescriptions;
    Util::Dictionary<Util::StringAtom, ColumnDescriptor> columnRegistry;
};

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline ColumnDescriptor
TypeRegistry::Register(Util::StringAtom name, TYPE defaultValue)
{
    static_assert(std::is_trivially_copyable<TYPE>(), "TYPE must be trivially copyable.");
    static_assert(std::is_trivially_destructible<TYPE>(), "TYPE must be trivially destructible.");
    static_assert(std::is_standard_layout<TYPE>(), "TYPE must be standard layout.");
    
    auto* reg = Instance();
    // setup a state description with the default values from the type
    ColumnDescription* desc = n_new(ColumnDescription(name, defaultValue));

    ColumnDescriptor descriptor = reg->columnDescriptions.Size();
    reg->columnDescriptions.Append(desc);
    reg->columnRegistry.Add(name, descriptor);
    return descriptor;
}

//------------------------------------------------------------------------------
/**
*/
inline ColumnDescriptor
TypeRegistry::GetDescriptor(Util::StringAtom name)
{
    auto* reg = Instance();
    IndexT index = reg->columnRegistry.FindIndex(name);
    if (index != InvalidIndex)
    {
        return reg->columnRegistry.ValueAtIndex(index);
    }

    return ColumnDescriptor::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
inline ColumnDescription*
TypeRegistry::GetDescription(ColumnDescriptor descriptor)
{
    auto* reg = Instance();
    if (descriptor.id >= 0 && descriptor.id < reg->columnDescriptions.Size())
        return reg->columnDescriptions[descriptor.id];
    
    return nullptr;
}

} // namespace MemDb
