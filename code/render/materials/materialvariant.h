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
        : needsDeref(false)
        , mem(nullptr)
        , size(0)
    {};

    /// Copy constructor
    MaterialVariant(const MaterialVariant& rhs)
        : needsDeref(rhs.needsDeref)
        , mem(rhs.mem)
        , size(rhs.size)
    {
    }

    /// Assign operator
    void operator=(const MaterialVariant& rhs)
    {
        this->mem = rhs.mem;
        this->needsDeref = rhs.needsDeref;
        this->size = rhs.size;
    }

    /// Move constructor
    MaterialVariant(MaterialVariant&& rhs)
        : needsDeref(rhs.needsDeref)
        , mem(rhs.mem)
        , size(rhs.size)
    {
        rhs.mem = nullptr;
        rhs.needsDeref = false;
        rhs.size = 0;
    }

    /// Move operator
    void operator=(MaterialVariant&& rhs)
    {
        this->mem = rhs.mem;
        this->needsDeref = rhs.needsDeref;
        this->size = rhs.size;

        rhs.mem = nullptr;
        rhs.needsDeref = false;
        rhs.size = 0;
    }

    /// Nullptr constructor
    MaterialVariant(std::nullptr_t)
        : needsDeref(false)
        , mem(nullptr)
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

    /// Const get
    template <typename T> const T& ConstGet() const
    {
        return this->needsDeref ? *reinterpret_cast<T*>(this->mem) : reinterpret_cast<const T&>(this->mem);
    }

    /// Mutable get
    template <typename T> T& Get()
    {
        return this->needsDeref ? *reinterpret_cast<T*>(this->mem) : reinterpret_cast<T&>(this->mem);
    }

    /// Set
    template <typename T> void Set(const T& data, void* mem)
    {
        this->size = sizeof(T);
        this->mem = mem;
        this->needsDeref = this->size > sizeof(void*);
        memcpy(this->needsDeref ? this->mem : reinterpret_cast<void*>(&this->mem), &data, this->size);
    }

    /// Set from raw memory
    template <typename T> void Set(void* mem)
    {
        this->size = sizeof(T);
        this->mem = mem;
        this->needsDeref = true;
    }
};

} // namespace Materials
