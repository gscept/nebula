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
        this->defVal = Memory::Alloc(Memory::HeapType::ObjectHeap, sizeof(T));
        Memory::Copy(&defaultValue, this->defVal, sizeof(T));
        this->typeSize = sizeof(T);
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

        rhs.typeSize = 0;
        rhs.defVal = nullptr;
    }
    void operator=(StateDescription& rhs)
    {
        this->typeSize = rhs.typeSize;
        this->defVal = rhs.defVal;

        rhs.typeSize = 0;
        rhs.defVal = nullptr;
    }

    SizeT typeSize;
    void* defVal;
};

} // namespace Db

} // namespace Game
