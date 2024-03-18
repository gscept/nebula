#pragma once
//------------------------------------------------------------------------------
/**
    @class Ids::IdAllocator

    An ID allocator associates an id with a slice in an N number of arrays.

    There are two versions of this type, an unsafe and a safe one. Both are implemented
    in the same way, providing a variadic list of types which is to be contained in the allocator
    and fetching each value by providing the index into the list of types, which means the members
    are nameless. 

    The thread safe allocator requires the Get-methods to be within an Enter/Leave
    lockstep phase. 
    
    @class Ids::IdAllocatorSafe
    
    @see Ids::IdAllocator

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "id.h"
#include "idpool.h"
#include "util/array.h"
#include "threading/criticalsection.h"
#include <tuple>
#include <utility>
#include "util/tupleutility.h"
#include "util/arrayallocator.h"
#include "util/arrayallocatorsafe.h"
#include "idgenerationpool.h"

namespace Ids
{

template<class ... TYPES>
class IdAllocator : public Util::ArrayAllocator<TYPES...>
{
public:
    /// constructor
    IdAllocator(uint32_t maxid = 0xFFFFFFFF) : maxId(maxid) {};
    
    /// Allocate an object. 
    Ids::Id32 Alloc()
    {
        /// @note   This purposefully hides the default allocation method and should definetly not be virtual!
        Ids::Id32 id;
        if (this->pool.Allocate(id))
        {
            Util::ArrayAllocator<TYPES...>::Alloc();
            n_assert2(this->maxId > Ids::Index(id), "max amount of allocations exceeded!\n");
        }

        return id;
    }

    /// Deallocate an object. Just places it in freeids array for recycling
    void Dealloc(Ids::Id32 id)
    {
        // TODO: We could possibly get better performance when defragging if we insert it in reverse order (high to low)
        this->pool.Deallocate(id);
    }

    /// Set element
    template<int MEMBER>
    inline void Set(const Ids::Id32 id, const Util::tuple_array_t<MEMBER, TYPES...>& type)
    {
        Util::ArrayAllocator<TYPES...>::Set<MEMBER>(Ids::Index(id), type);
    }

    /// Set elements
    inline void Set(const Ids::Id32 id, TYPES... values)
    {
        Util::ArrayAllocator<TYPES...>::Set(Ids::Index(id), values...);
    }

    /// Get element
    template<int MEMBER>
    inline Util::tuple_array_t<MEMBER, TYPES...>& Get(const Ids::Id32 id)
    {
        return Util::ArrayAllocator<TYPES...>::Get<MEMBER>(Ids::Index(id));
    }

    /// Const get element
    template<int MEMBER>
    inline const Util::tuple_array_t<MEMBER, TYPES...>& ConstGet(const Ids::Id32 id) const
    {
        return Util::ArrayAllocator<TYPES...>::ConstGet<MEMBER>(Ids::Index(id));
    }

    /// Get the free ids list from the pool
    inline Util::Queue<Id32>& FreeIds()
    {
        return this->pool.FreeIds();
    }

    /// return number of allocated ids
    const uint32_t Size() const
    {
        return this->size;
    }

private:
    uint32_t maxId = 0xFFFFFFFF;
    Ids::IdGenerationPool pool;
};

#define _DECL_ACQUIRE_RELEASE(ty) \
    bool ty##Acquire(const ty id); \
    void ty##Release(const ty id); \
    struct ty##Lock \
    { \
        ty##Lock(const ty element) : element(element) { this->didAcquire = ty##Acquire(this->element); } \
        ~ty##Lock() { if (this->didAcquire) ty##Release(this->element); } \
    private: \
        bool didAcquire; \
        ty element; \
    };

#define _IMPL_ACQUIRE_RELEASE(ty, allocator) \
    bool ty##Acquire(const ty id) { return allocator.Acquire(id.id); } \
    void ty##Release(const ty id) { allocator.Release(id.id); }

template<int MAX_ALLOCS, class... TYPES>
class IdAllocatorSafe : public Util::ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>
{
public:
    /// constructor
    IdAllocatorSafe()
    {
    };

    /// Allocate an object. 
    Ids::Id32 Alloc()
    {
        /// @note   This purposefully hides the default allocation method and should definitely not be virtual!
        this->allocationLock.Lock();
        Ids::Id32 id;
        if (this->pool.Allocate(id))
        {
            alloc_for_each_in_tuple(this->objects);
            this->owners.Append(Threading::Thread::GetMyThreadId());
            n_assert2(MAX_ALLOCS > Ids::Index(id), "max amount of allocations exceeded!\n");
        }
        else
        {
            this->owners[Ids::Index(id)] = Threading::Thread::GetMyThreadId();
        }
        this->allocationLock.Unlock();

        return id;
    }

    /// Set element
    template<int MEMBER>
    inline void Set(const Ids::Id32 id, const Util::tuple_array_t<MEMBER, TYPES...>& type)
    {
        Util::ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::Set<MEMBER>(Ids::Index(id), type);
    }

    /// Set elements
    inline void Set(const Ids::Id32 id, TYPES... values)
    {
        Util::ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::Set(Ids::Index(id), values...);
    }

    /// Get element
    template<int MEMBER>
    inline Util::tuple_array_t<MEMBER, TYPES...>& Get(const Ids::Id32 id)
    {
        return Util::ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::Get<MEMBER>(Ids::Index(id));
    }

    /// Const get element
    template<int MEMBER>
    inline const Util::tuple_array_t<MEMBER, TYPES...>& ConstGet(const Ids::Id32 id) const
    {
        return Util::ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::ConstGet<MEMBER>(Ids::Index(id));
    }

    /// Get the free ids list from the pool
    inline Util::Queue<Id32>& FreeIds()
    {
        return this->pool.FreeIds();
    }

    /// Spinlock to acquire 
    void TryAcquire(const Ids::Id32 id)
    {
        Util::ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::TryAcquire(Ids::Index(id));
    }
    /// Acquire element, asserts if false and returns true if this call acquired
    bool Acquire(const Ids::Id32 id)
    {
        return Util::ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::Acquire(Ids::Index(id));
    }
    /// Release an object, the next thread that acquires may use this instance as it fits
    void Release(const Ids::Id32 id)
    {
        Util::ArrayAllocatorSafe<MAX_ALLOCS, TYPES...>::Release(Ids::Index(id));
    }

    /// Deallocate an object. Just places it in freeids array for recycling
    void Dealloc(Ids::Id32 id)
    {
        // TODO: We could possibly get better performance when defragging if we insert it in reverse order (high to low)
        this->allocationLock.Lock();
        this->pool.Deallocate(id);
        this->allocationLock.Unlock();
    }

private:
    Ids::IdGenerationPool pool;
};

} // namespace Ids
