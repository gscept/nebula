#pragma once
//------------------------------------------------------------------------------
/**
    ColumnDescription

    Describes a context state's type and default value.

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "util/fourcc.h"
#include "util/stringatom.h"
#include "util/hashtable.h"
#include "table.h"

namespace MemDb
{

class ColumnDescription
{
public:
    template<typename T>
    explicit ColumnDescription(Util::StringAtom name, T const& defaultValue)
    {
        this->typeSize = sizeof(T);
        this->name = name;
        this->defVal = Memory::Alloc(Memory::HeapType::ObjectHeap, sizeof(T));

        if constexpr (std::is_trivially_copyable<T>() && std::is_trivially_destructible<T>())
        {
            Memory::Copy(&defaultValue, this->defVal, sizeof(T));
        }
        else
        {
            // if the type is trivially copyable and trivially destructible, we never have to worry about
            // constructors since we're "constructing" our objects by copy.
            // However, if they are not, we need to store function pointers to the various operations required.
            // This means we need to wrap our constructors, destructors, copy operators and move/assign operators in additional function calls.
            // We remove a bunch of overhead by doing all of this in bulk as much as possible.
            this->fTable.Create = [](void* buffer, uint64_t const size) -> void*
            {
                return new(buffer) T[size];
            };

            this->fTable.Destroy = [](void* buffer, uint64_t const size) -> void
            {
                for (uint64_t i = 0; i < size; ++i)
                {
                    ((T*)buffer)[i].~T();
                }
            };

            if constexpr (std::is_trivially_move_assignable<T>() || std::is_move_assignable<T>())
            {
                this->fTable.Copy = [](void* s, void* d, uint64_t const numInstances)
                {
                    T* src = (T*)s;
                    T* dst = (T*)d;
                    for (uint64_t i = 0; i < numInstances; ++i)
                    {
                        dst[i] = std::move(src[i]);
                    }
                };
            }
            else
            {
                this->fTable.Copy = [](void* s, void* d, uint64_t const numInstances)
                {
                    T* src = (T*)s;
                    T* dst = (T*)d;
                    for (uint64_t i = 0; i < numInstances; ++i)
                    {
                        dst[i] = src[i];
                    }
                };
            }

            this->fTable.Assign = [](void* buffer, void* defVal, uint64_t const numInstances)
            {
                T* p = (T*)buffer;
                T* defaultValue = (T*)defVal;
                for (uint64_t i = 0; i < numInstances; ++i)
                {
                    p[i] = *defaultValue;
                }
            };

            // placement new. Remember to destroy before freeing buffer!
            T* d = new(defVal) T(defaultValue);
            this->trivialType = false;
        }
    }
    ColumnDescription()
    {
        // empty
    }
    ColumnDescription(ColumnDescription&& desc) : defVal(desc.defVal), typeSize(desc.typeSize), name(desc.name), fTable(desc.fTable), trivialType(desc.trivialType)
    {
        desc.defVal = nullptr;
        desc.name = nullptr;
        desc.typeSize = 0;
    }
    ~ColumnDescription()
    {
        if (this->defVal != nullptr)
        {
            if (!this->trivialType)
            {
                this->fTable.Destroy(this->defVal, 1);
            }
            else
            {
                Memory::Free(Memory::HeapType::ObjectHeap, this->defVal);
            }
        }
    }
    void operator=(ColumnDescription&& rhs) noexcept
    {
        this->typeSize = rhs.typeSize;
        this->defVal = rhs.defVal;
        this->name = rhs.name;
        this->trivialType = rhs.trivialType;
        this->fTable = rhs.fTable;

        rhs.typeSize = 0;
        rhs.defVal = nullptr;
        rhs.name = nullptr;
    }
    void operator=(ColumnDescription& rhs)
    {
        this->typeSize = rhs.typeSize;
        this->defVal = rhs.defVal;
        this->name = rhs.name;
        this->fTable = rhs.fTable;
        this->trivialType = rhs.trivialType;

        rhs.typeSize = 0;
        rhs.defVal = nullptr;
        rhs.name = nullptr;
    }

    Util::StringAtom name;
    SizeT typeSize = 0;
    void* defVal = nullptr;
    bool trivialType = true;
    using TableRegistry = Util::HashTable<TableId, void**, 32, 1>;
    // direct access to all buffers within tables
    TableRegistry tableRegistry;

    struct FunctionTable
    {
        void* (*Create)(void*, uint64_t const);
        void (*Destroy)(void*, uint64_t const);
        void (*Copy)(void*, void*, uint64_t const);
        void (*Assign)(void*, void*, uint64_t const);
    } fTable;

};

} // namespace MemDb
