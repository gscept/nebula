#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::FixedPool
    
    Implements a fixed size pool, from which objects of a specific type can be
    allocated and freed for reuse.

    @copyright
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/array.h"
#include <functional>

//------------------------------------------------------------------------------
namespace Util
{
template<class TYPE> class FixedPool
{
public:
    /// define iterator
    typedef TYPE* Iterator;

    /// default constructor
    FixedPool();
    /// constructor with fixed size
    FixedPool(SizeT s, std::function<void(TYPE& val, IndexT idx)> setupFunc);
    /// destructor
    ~FixedPool();

    /// assignment operator
    void operator=(const FixedArray<TYPE>& rhs);
    /// access operator
    TYPE& operator[](const IndexT elem);

    /// allocate an element in the pool
    TYPE& Alloc();
    /// free element allocated from pool
    void Free(const TYPE& elem);
    /// free element using iterator
    void Free(const Iterator iter);

    /// get number of elements
    SizeT Size() const;
    /// get number of free elements
    SizeT NumFree() const;
    /// get number of used elements
    SizeT NumUsed() const;
    /// reset pool and resize pool
    void Resize(SizeT newSize);
    /// returns true if no more free values are available
    bool IsFull() const;
    /// returns true if the pool is empty
    bool IsEmpty() const;
    /// clear all pool values
    void Clear();

    /// resets all used indices without clearing contents
    void Reset();

    /// set optional setup value
    void SetSetupFunc(const std::function<void(TYPE& val, IndexT idx)>& func);

    /// get allocated values as array
    const Util::Array<TYPE>& GetAllocated();

private:
    std::function<void(TYPE& val, IndexT idx)> setupFunc;
    SizeT size;
    Util::Array<TYPE> freeValues;
    Util::Array<TYPE> usedValues;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline
Util::FixedPool<TYPE>::FixedPool() :
    size(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline 
Util::FixedPool<TYPE>::~FixedPool()
{
    this->freeValues.Clear();
    this->usedValues.Clear();
    this->setupFunc = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline
Util::FixedPool<TYPE>::FixedPool(SizeT s, std::function<void(TYPE& val, IndexT idx)> setupFunc) :
    size(s)
{
    this->freeValues.Reserve(s);
    this->usedValues.Reserve(s);
    this->setupFunc = setupFunc;

    IndexT i;
    for (i = 0; i < this->size; i++)
    {
        this->freeValues.Append(TYPE());
        if (this->setupFunc != nullptr) this->setupFunc(this->freeValues[i], i);
    }   
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void
Util::FixedPool<TYPE>::operator=(const FixedArray<TYPE>& rhs)
{
    this->freeValues = rhs.freeValues;
    this->usedValues = rhs.usedValues;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE&
Util::FixedPool<TYPE>::operator[](const IndexT elem)
{
#if NEBULA_BOUNDSCHECKS
    n_assert(this->usedValues.Size() > elem);
#endif
    return this->usedValues[elem];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline TYPE&
Util::FixedPool<TYPE>::Alloc()
{
    n_assert(!this->freeValues.IsEmpty());
    TYPE elem = this->freeValues.PopFront();
    this->usedValues.Append(elem);
    return this->usedValues.Back();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void
Util::FixedPool<TYPE>::Free(const TYPE& elem)
{
    IndexT idx = this->usedValues.FindIndex(elem);
    n_assert(idx != InvalidIndex);
    this->usedValues.EraseIndex(idx);
    this->freeValues.Append(elem);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void
Util::FixedPool<TYPE>::Free(const Iterator iter)
{
    this->usedValues.Erase(iter);
    this->freeValues.Append(&iter);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline SizeT
Util::FixedPool<TYPE>::Size() const
{
    return this->size;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline SizeT
Util::FixedPool<TYPE>::NumFree() const
{
    return this->freeValues.Size();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline SizeT
Util::FixedPool<TYPE>::NumUsed() const
{
    return this->usedValues.Size();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void
Util::FixedPool<TYPE>::Resize(SizeT newSize)
{
    this->size = newSize;
    this->usedValues.Clear();
    this->freeValues.Clear();
    this->freeValues.Reserve(newSize);

    IndexT i;
    for (i = 0; i < this->size; i++)
    {
        this->freeValues.Append(TYPE());
        if (this->setupFunc != nullptr) this->setupFunc(this->freeValues[i], i);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline bool
Util::FixedPool<TYPE>::IsFull() const
{
    return this->freeValues.IsEmpty();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline bool
Util::FixedPool<TYPE>::IsEmpty() const
{
    return this->usedValues.IsEmpty();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void
Util::FixedPool<TYPE>::Clear()
{
    this->usedValues.Clear();
    this->freeValues.Clear();
    this->freeValues.Reserve(this->size);

    IndexT i;
    for (i = 0; i < this->size; i++)
    {
        this->freeValues.Append(TYPE());
        if (this->setupFunc != nullptr) this->setupFunc(this->freeValues[i], i);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void
Util::FixedPool<TYPE>::Reset()
{
    IndexT i;
    for (i = 0; i < this->usedValues.Size(); i++)
    {
        this->freeValues.Append(this->usedValues[i]);
    }
    this->usedValues.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void
Util::FixedPool<TYPE>::SetSetupFunc(const std::function<void(TYPE& val, IndexT idx)>& func)
{
    this->setupFunc = func;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline const Util::Array<TYPE>&
Util::FixedPool<TYPE>::GetAllocated()
{
    return this->usedValues;
}

}
