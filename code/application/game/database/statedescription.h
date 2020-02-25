#pragma once
//------------------------------------------------------------------------------
/**
    StateDescription

    Describes a context state's type and default value.

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"

namespace Game
{

namespace Db
{

class StateDescription
{
public:
    template<typename T>
    explicit StateDescription(T const& defaultValue)
    {
        static_assert(std::is_standard_layout<T>(), "State needs to be a standard layout type!");
        static_assert(std::is_trivially_copyable<T>(), "State needs to be trivially copyable type!");
        static_assert(std::is_trivially_destructible<T>(), "Type is not trivially destructible!");
        static_assert(T::ID, "States require a hash. Make sure you declare your states with the __DeclareState macro!\n");

        this->defVal = Memory::Alloc(Memory::HeapType::ObjectHeap, sizeof(T));
        Memory::Copy(&defaultValue, this->defVal, sizeof(T));
        this->typeSize = sizeof(T);
        this->fourcc = T::ID;
    }
    StateDescription() : defVal(nullptr), typeSize(0)
    {
        // empty
    }
    StateDescription(StateDescription&& desc) : defVal(desc.defVal), typeSize(desc.typeSize)
    {
        desc.defVal = nullptr;
        desc.typeSize = 0;
    }
    ~StateDescription()
    {
        if (this->defVal != nullptr)
        {
            Memory::Free(Memory::HeapType::ObjectHeap, this->defVal);
        }
    }
    void operator=(StateDescription&& rhs)
    {
        this->typeSize = rhs.typeSize;
        this->defVal = rhs.defVal;
        this->fourcc = rhs.fourcc;

        rhs.typeSize = 0;
        rhs.defVal = nullptr;
        rhs.fourcc = 0;
    }
    void operator=(StateDescription& rhs)
    {
        this->typeSize = rhs.typeSize;
        this->defVal = rhs.defVal;
        this->fourcc = rhs.fourcc;

        rhs.typeSize = 0;
        rhs.defVal = nullptr;
        rhs.fourcc = 0;
    }

    Util::FourCC fourcc;
    SizeT typeSize;
    void* defVal;
};

} // namespace Db

} // namespace Game
