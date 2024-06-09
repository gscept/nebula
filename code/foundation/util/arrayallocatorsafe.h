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
#include "types.h"
#include "util/pinnedarray.h"
#include "threading/readwritelock.h"
#include "threading/spinlock.h"
#include <tuple>
#include "threading/interlocked.h"
#include "tupleutility.h"
#include "ids/id.h"

// use this macro to safetly access an array allocator from within a scope
#define __Lock(name, element) auto __allocator_lock_##name##__ = Util::AllocatorLock(&name, element);

// use this macro when we need to retrieve an allocator and lock it with an explicit name
#define __LockName(allocator, name, element) auto __allocator_lock_##name##__ = Util::AllocatorLock(allocator, element);

namespace Util
{

template<class T>
struct AllocatorLock
{
    AllocatorLock(T* allocator, uint32_t element)
        : element(element)
        , allocator(allocator)
    {
        this->didAcquire = this->allocator->Acquire(this->element);
    };

    ~AllocatorLock()
    {
        if (this->didAcquire)
            this->allocator->Release(this->element);
    }

    bool didAcquire;
    uint32_t element;
    T* allocator;
};

template <uint MAX_ALLOCS = 0xFFFF, class ... TYPES>
class ArrayAllocatorSafe
{
public:
    /// constructor
    ArrayAllocatorSafe();

    /// move constructor
    ArrayAllocatorSafe(ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>&& rhs);

    /// copy constructor
    ArrayAllocatorSafe(const ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>& rhs);

    /// destructor
    ~ArrayAllocatorSafe();

    /// assign operator
    void operator=(const ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>& rhs);

    /// move operator
    void operator=(ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>&& rhs);

    /// allocate a new resource
    uint32_t Alloc();

    /// Erase element for each
    void EraseIndex(const uint32_t id);

    /// Erase element for each
    void EraseIndexSwap(const uint32_t id);

    /// get single item from resource
    template <int MEMBER>
    tuple_array_t<MEMBER, TYPES...>& Get(const uint32_t index);

    /// Get const explicitly
    template <int MEMBER>
    const tuple_array_t<MEMBER, TYPES...>& ConstGet(const uint32_t index) const;

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

    /// Spinlock to acquire 
    void TryAcquire(const uint32_t index);
    /// Acquire element, asserts if false and returns true if this call acquired
    bool Acquire(const uint32_t index);
    /// Release an object, the next thread that acquires may use this instance as it fits
    void Release(const uint32_t index);

protected:
     
    uint32_t size;
    std::tuple<Util::PinnedArray<MAX_ALLOCS, TYPES>...> objects;
    Util::Array<Threading::ThreadId> owners;

    Threading::Spinlock allocationLock;
};

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES> 
inline 
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::ArrayAllocatorSafe() :
    size(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES> 
inline 
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::ArrayAllocatorSafe(ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>&& rhs)
{
    this->allocationLock.Lock();
    rhs.allocationLock.Lock();
    this->objects = rhs.objects;
    this->size = rhs.size;
    rhs.Clear();

    this->allocationLock.Unlock();
    rhs.allocationLock.Unlock();
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES> 
inline
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::ArrayAllocatorSafe(const ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>& rhs)
{
    this->allocationLock.Lock();
    this->objects = rhs.objects;
    this->size = rhs.size;
    this->allocationLock.Unlock();
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES> 
inline
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::~ArrayAllocatorSafe()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES> 
inline void 
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::operator=(const ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>& rhs)
{
    this->allocationLock.Lock();
    this->objects = rhs.objects;
    this->size = rhs.size;
    this->allocationLock.Unlock();
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES>
inline void
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::operator=(ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>&& rhs)
{
    this->allocationLock.Lock();
    rhs.allocationLock.Lock();
    this->objects = rhs.objects;
    this->size = rhs.size;
    rhs.Clear();

    this->allocationLock.Unlock();
    rhs.allocationLock.Unlock();
}

//------------------------------------------------------------------------------
/**
    Allocs an object AND acquires it
*/
template<uint MAX_ALLOCS, class ...TYPES>
inline uint32_t
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::Alloc()
{
    this->allocationLock.Lock();
    alloc_for_each_in_tuple(this->objects);
    auto i = this->size++;
    this->owners.Append(Threading::Thread::GetMyThreadId());
    this->allocationLock.Unlock();
    return i;
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES>
inline void
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::EraseIndex(const uint32_t id)
{
    n_assert(this->owners[id] == Threading::Thread::GetMyThreadId());
    this->allocationLock.Lock();
    erase_index_for_each_in_tuple(this->objects, id);
    this->allocationLock.Unlock();
    this->size--;
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES>
inline void
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::EraseIndexSwap(const uint32_t id)
{
    n_assert(this->owners[id] == Threading::Thread::GetMyThreadId());
    this->allocationLock.Lock();
    erase_index_swap_for_each_in_tuple(this->objects, id);
    this->allocationLock.Unlock();
    this->size--;
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES>
inline const uint32_t
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::Size() const
{
    n_assert2(this->numReaders > 0, "Size requires a read lock");
    return this->size;
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES>
inline void
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::Reserve(uint32_t num)
{
    this->allocationLock.Lock();
    reserve_for_each_in_tuple(this->objects, num);
    this->allocationLock.Unlock();
    // Size is still the same.
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES>
inline void
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::Clear()
{
    this->allocationLock.Enter();
    clear_for_each_in_tuple(this->objects);
    this->size = 0;
    this->allocationLock.Leave();
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES>
template<int MEMBER>
inline tuple_array_t<MEMBER, TYPES...>&
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::Get(const uint32_t index)
{
    n_assert(this->owners[index] == Threading::Thread::GetMyThreadId());
    return std::get<MEMBER>(this->objects)[index];
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES>
template<int MEMBER>
inline const tuple_array_t<MEMBER, TYPES...>&
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::ConstGet(const uint32_t index) const
{
    // Allow const get when no thread is owning the element as well
    n_assert(this->owners[index] == Threading::Thread::GetMyThreadId() || this->owners[index] == Threading::InvalidThreadId);
    return std::get<MEMBER>(this->objects)[index];
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES>
template<int MEMBER>
inline const tuple_array_t<MEMBER, TYPES...>&
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::Get(const uint32_t index) const
{
    // Allow const get when no thread is owning the element as well
    n_assert(this->owners[index] == Threading::Thread::GetMyThreadId() || this->owners[index] == Threading::InvalidThreadId);
    return std::get<MEMBER>(this->objects)[index];
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES>
template<int MEMBER>
inline void 
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::Set(const uint32_t index, const tuple_array_t<MEMBER, TYPES...>& type)
{
    n_assert(this->owners[index] == Threading::Thread::GetMyThreadId());
    std::get<MEMBER>(this->objects)[index] = type;
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES>
template<int MEMBER>
inline const Util::Array<tuple_array_t<MEMBER, TYPES...>>&
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::GetArray() const
{
    return std::get<MEMBER>(this->objects);
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES>
template<int MEMBER>
inline Util::Array<tuple_array_t<MEMBER, TYPES...>>&
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::GetArray()
{
    return std::get<MEMBER>(this->objects);
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES> 
void
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::Set(const uint32_t index, TYPES... values)
{
    set_for_each_in_tuple(this->objects, index, values...);
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES>
void
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::UpdateSize()
{
    this->size = this->GetArray<0>().Size();
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES>
void 
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::TryAcquire(const uint32_t index)
{
    Threading::ThreadId myThread = Threading::Thread::GetMyThreadId();
    Threading::ThreadId currentThread = Threading::Interlocked::CompareExchange((volatile Threading::ThreadIdStorage*)&this->owners[index], myThread, Threading::InvalidThreadId);
    n_assert(currentThread == Threading::InvalidThreadId);
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES>
bool 
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::Acquire(const uint32_t index)
{
    Threading::ThreadId myThread = Threading::Thread::GetMyThreadId();
    if (this->owners[index] == myThread)
        return false;

    // Spinlock
    while (Threading::Interlocked::CompareExchange((volatile Threading::ThreadIdStorage*)&this->owners[index], myThread, Threading::InvalidThreadId) != Threading::InvalidThreadId)
    {
        Threading::Thread::YieldThread();
    };
    return true;
}

//------------------------------------------------------------------------------
/**
*/
template<uint MAX_ALLOCS, class ...TYPES>
void 
ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::Release(const uint32_t index)
{
    n_assert(this->owners[index] == Threading::Thread::GetMyThreadId());
    Threading::Interlocked::Exchange((volatile Threading::ThreadIdStorage*)&this->owners[index], Threading::InvalidThreadId);
}

} // namespace Util
