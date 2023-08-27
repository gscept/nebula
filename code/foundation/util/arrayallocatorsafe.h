#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::ArrayAllocatorSafe
    
    The ArrayAllocatorSafe provides a thread safe variadic list of types which is to be contained in the allocator
    and fetching each value by providing the index into the list of types, which means the members
    are nameless.

    Note that this is not a container for multiple arrays, but a object allocator that uses
    multiple arrays to store their variables parallelly, and should be treated as such.

    There are two versions of this type, an unsafe and a safe one. Both are implemented
    in the same way.

    The thread safe allocator requires the Get-methods to be within an Enter/Leave
    lockstep phase. 

    @see    arrayallocator.h

    @copyright
    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "util/array.h"
#include "threading/readwritelock.h"
#include <tuple>
#include "tupleutility.h"
#include "ids/id.h"

// use this macro to safetly access an array allocator from within a scope
#define __Lock(name, access) auto __allocator_lock_##name##__ = Util::AllocatorLock(&name, access);

// use this macro when we need to retrieve an allocator and lock it with an explicit name
#define __LockName(allocator, name, access) auto __allocator_lock_##name##__ = Util::AllocatorLock(allocator, access);

namespace Util
{

enum class ArrayAllocatorAccess
{
    Read,
    Write
};

template<class T>
struct AllocatorLock
{
    AllocatorLock(T* allocator, ArrayAllocatorAccess access)
        : allocator(allocator)
        , access(access)
    {
        this->allocator->Lock(this->access);
    };

    ~AllocatorLock()
    {
        this->allocator->Unlock(this->access);
    }

    ArrayAllocatorAccess access;
    T* allocator;
};

template <class ... TYPES>
class ArrayAllocatorSafe
{
public:
    /// constructor
    ArrayAllocatorSafe();

    /// move constructor
    ArrayAllocatorSafe(ArrayAllocatorSafe<TYPES...>&& rhs);

    /// copy constructor
    ArrayAllocatorSafe(const ArrayAllocatorSafe<TYPES...>& rhs);

    /// destructor
    ~ArrayAllocatorSafe();

    /// assign operator
    void operator=(const ArrayAllocatorSafe<TYPES...>& rhs);

    /// move operator
    void operator=(ArrayAllocatorSafe<TYPES...>&& rhs);

    /// allocate a new resource
    uint32_t Alloc();

    /// Erase element for each
    void EraseIndex(const uint32_t id);

    /// Erase element for each
    void EraseIndexSwap(const uint32_t id);

    /// get single item from resource
    template <int MEMBER>
    tuple_array_t<MEMBER, TYPES...>& Get(const uint32_t index);

    /// same as 32 bit get, but const
    template <int MEMBER>
    const tuple_array_t<MEMBER, TYPES...>& Get(const uint32_t index) const;

    /// set single item
    template <int MEMBER>
    void Set(const uint32_t index, const tuple_array_t<MEMBER, TYPES...>& type);

    /// get array const reference
    template <int MEMBER>
    const Util::Array<tuple_array_t<MEMBER, TYPES...>>& GetArray() const;

    /// get array
    template <int MEMBER>
    Util::Array<tuple_array_t<MEMBER, TYPES...>>& GetArray();

    /// set for each in tuple
    void Set(const uint32_t index, TYPES...);

    /// get number of used indices
    const uint32_t Size() const;

    /// grow capacity of arrays to size
    void Reserve(uint32_t size);

    /// clear entire allocator and start from scratch.
    void Clear();

    /// Any reserve and direct array access might mess with the size.
    /// This will update the size to reflect the first member array size in objects.
    void UpdateSize();

    /// Lock allocator
    void Lock(const ArrayAllocatorAccess access = ArrayAllocatorAccess::Write);
    /// Unlock allocator
    void Unlock(const ArrayAllocatorAccess access = ArrayAllocatorAccess::Write);

    /// get single item unsafe (use with extreme caution)
    template<int MEMBER> Util::tuple_array_t<MEMBER, TYPES...>& GetUnsafe(const uint32_t index);
    /// get single item unsafe (use with extreme caution)
    template<int MEMBER> void SetUnsafe(const uint32_t index, const tuple_array_t<MEMBER, TYPES...>& type);

protected:

    Threading::ReadWriteLock lock;
    volatile int numReaders;
    volatile int writer;
    uint32_t size;
    std::tuple<Util::Array<TYPES>...> objects;
};

//------------------------------------------------------------------------------
/**
*/
template<class ... TYPES>
inline ArrayAllocatorSafe<TYPES...>::ArrayAllocatorSafe() :
    size(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline ArrayAllocatorSafe<TYPES...>::ArrayAllocatorSafe(ArrayAllocatorSafe<TYPES...>&& rhs)
{
    this->lock.LockWrite();
    rhs.lock.LockRead();
    this->objects = rhs.objects;
    this->size = rhs.size;
    this->lock.UnlockWrite();
    rhs.lock.UnlockRead();
    
    rhs.lock.LockWrite();
    rhs.Clear();
    rhs.lock.UnlockWrite();
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline ArrayAllocatorSafe<TYPES...>::ArrayAllocatorSafe(const ArrayAllocatorSafe<TYPES...>& rhs)
{
    this->lock.LockWrite();
    this->objects = rhs.objects;
    this->size = rhs.size;
    this->lock.UnlockWrite();
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline ArrayAllocatorSafe<TYPES...>::~ArrayAllocatorSafe()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void
ArrayAllocatorSafe<TYPES...>::operator=(const ArrayAllocatorSafe<TYPES...>& rhs)
{
    this->lock.LockWrite();
    this->objects = rhs.objects;
    this->size = rhs.size;
    this->lock.UnlockWrite();
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void
ArrayAllocatorSafe<TYPES...>::operator=(ArrayAllocatorSafe<TYPES...>&& rhs)
{
    this->lock.LockWrite();
    rhs.lock.LockRead();
    this->objects = rhs.objects;
    this->size = rhs.size;
    this->lock.UnlockWrite();
    rhs.lock.UnlockRead();

    rhs.lock.LockWrite();
    rhs.Clear();
    rhs.lock.UnlockWrite();
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline uint32_t ArrayAllocatorSafe<TYPES...>::Alloc()
{
    this->lock.LockWrite();
    alloc_for_each_in_tuple(this->objects);
    auto i = this->size++;
    this->lock.UnlockWrite();
    return i;
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void ArrayAllocatorSafe<TYPES...>::EraseIndex(const uint32_t id)
{
    this->lock.LockWrite();
    erase_index_for_each_in_tuple(this->objects, id);
    this->size--;
    this->lock.UnlockWrite();
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void ArrayAllocatorSafe<TYPES...>::EraseIndexSwap(const uint32_t id)
{
    this->lock.LockWrite();
    erase_index_swap_for_each_in_tuple(this->objects, id);
    this->size--;
    this->lock.UnlockWrite();
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline const uint32_t
ArrayAllocatorSafe<TYPES...>::Size() const
{
    n_assert2(this->numReaders > 0, "Size requires a read lock");
    return this->size;
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void
ArrayAllocatorSafe<TYPES...>::Reserve(uint32_t num)
{
    this->lock.LockWrite();
    reserve_for_each_in_tuple(this->objects, num);
    this->lock.UnlockWrite();
    // Size is still the same.
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void
ArrayAllocatorSafe<TYPES...>::Clear()
{
    this->lock.LockWrite();
    clear_for_each_in_tuple(this->objects);
    this->size = 0;
    this->lock.UnlockWrite();
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
template<int MEMBER>
inline tuple_array_t<MEMBER, TYPES...>&
ArrayAllocatorSafe<TYPES...>::Get(const uint32_t index)
{
    n_assert2(this->writer == 1, "Non-const Get requires a write lock");
    return std::get<MEMBER>(this->objects)[index];
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
template<int MEMBER>
inline const tuple_array_t<MEMBER, TYPES...>&
ArrayAllocatorSafe<TYPES...>::Get(const uint32_t index) const
{
    n_assert2(this->numReaders > 0, "Const Get requires a read lock");
    return std::get<MEMBER>(this->objects)[index];
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
template<int MEMBER>
inline void 
ArrayAllocatorSafe<TYPES...>::Set(const uint32_t index, const tuple_array_t<MEMBER, TYPES...>& type)
{
    n_assert2(this->writer == 1, "Set requires a write lock");
    std::get<MEMBER>(this->objects)[index] = type;
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
template<int MEMBER>
inline const Util::Array<tuple_array_t<MEMBER, TYPES...>>&
ArrayAllocatorSafe<TYPES...>::GetArray() const
{
    n_assert2(this->numReaders > 0, "const GetArray requires a read lock");
    return std::get<MEMBER>(this->objects);
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
template<int MEMBER>
inline Util::Array<tuple_array_t<MEMBER, TYPES...>>&
ArrayAllocatorSafe<TYPES...>::GetArray()
{
    n_assert2(this->writer == 1, "Non-const GetArray requires a write lock");
    return std::get<MEMBER>(this->objects);
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES> void
ArrayAllocatorSafe<TYPES...>::Set(const uint32_t index, TYPES... values)
{
    n_assert(this->writer == 1);
    set_for_each_in_tuple(this->objects, index, values...);
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES> void
ArrayAllocatorSafe<TYPES...>::UpdateSize()
{
    n_assert(this->writer == 1);
    this->size = this->GetArray<0>().Size();
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void 
ArrayAllocatorSafe<TYPES...>::Lock(const ArrayAllocatorAccess access)
{
    if (access == ArrayAllocatorAccess::Read)
    {
        this->lock.LockRead();
        Threading::Interlocked::Increment(&this->numReaders);
    }
    else
    {
        this->lock.LockWrite();
        Threading::Interlocked::Exchange(&this->writer, 1);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void 
ArrayAllocatorSafe<TYPES...>::Unlock(const ArrayAllocatorAccess access)
{
    if (access == ArrayAllocatorAccess::Read)
    {
        this->lock.UnlockRead();
        Threading::Interlocked::Decrement(&this->numReaders);
    }
    else
    {
        this->lock.UnlockWrite();
        Threading::Interlocked::Exchange(&this->writer, 1);
    }
}

//------------------------------------------------------------------------------
/**
    Get single item unsafetly, use with caution
*/
template<class ... TYPES>
template<int MEMBER>
Util::tuple_array_t<MEMBER, TYPES...>&
ArrayAllocatorSafe<TYPES...>::GetUnsafe(const uint32_t index)
{
    return std::get<MEMBER>(this->objects)[index];
}

//------------------------------------------------------------------------------
/**
    Get single item unsafetly, use with caution
*/
template<class ... TYPES>
template<int MEMBER>
void
ArrayAllocatorSafe<TYPES...>::SetUnsafe(const uint32_t index, const tuple_array_t<MEMBER, TYPES...>& type)
{
    std::get<MEMBER>(this->objects)[index] = type;
}

} // namespace Util
