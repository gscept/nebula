#pragma once
//------------------------------------------------------------------------------
/**
    Material special version of variant

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/config.h"
namespace Materials
{

struct MaterialVariant
{
    struct TextureHandleTuple;

    /// Nullptr constructor
    MaterialVariant()
        : mem(nullptr)
        , needsDeref(false)
        , size(0)
    {};

    /// Copy constructor
    MaterialVariant(const MaterialVariant& rhs)
        : needsDeref(false)
        , size(0)
    {
        this->mem = rhs.mem;
    }

    /// Assign operator
    void operator=(const MaterialVariant& rhs)
    {
        this->mem = rhs.mem;
    }

    /// Move constructor
    MaterialVariant(MaterialVariant&& rhs)
        : needsDeref(false)
        , size(0)
    {
        this->mem = rhs.mem;
        rhs.mem = nullptr;
    }

    /// Move operator
    void operator=(MaterialVariant&& rhs)
    {
        this->mem = rhs.mem;
        rhs.mem = nullptr;
    }

    /// Nullptr constructor
    MaterialVariant(std::nullptr_t)
        : mem(nullptr)
        , needsDeref(false)
        , size(0)
    {}

    bool needsDeref;
    void* mem;
    SizeT size;

    /// Get pointer
    const void* Get() const
    {
        // If the data is stored in a pointer, and not the pointer value it self, we need to deref the pointer
        return this->needsDeref ? this->mem : reinterpret_cast<const void*>(&this->mem);
    }

    /// Get
    template <typename T> const T& Get() const
    {
        return this->needsDeref ? *reinterpret_cast<T*>(this->mem) : reinterpret_cast<const T&>(this->mem);
    }

    /// Set
    template <typename T> void Set(const T& data, void* mem)
    {
        this->size = sizeof(T);
        this->mem = mem;
        this->needsDeref = this->size > sizeof(void*);
        memcpy(this->needsDeref ? this->mem : reinterpret_cast<void*>(&this->mem), &data, this->size);
    }
};

} // namespace Materials
