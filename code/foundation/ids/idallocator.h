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

namespace Ids
{

template<class ... TYPES>
class IdAllocator : public Util::ArrayAllocator<TYPES...>
{
public:
    /// constructor
    IdAllocator(Ids::Id32 maxid = 0xFFFFFFFF) : maxId(maxid) {};
    
    /// Allocate an object. 
    /// @note   This purposefully hides the default allocation method and should not be virtual!
    Ids::Id32 Alloc()
    {
        Ids::Id32 index;
        if (this->freeIds.Size() > 0)
        {
            index = this->freeIds.Back();
            this->freeIds.EraseBack();
        }
        else
        {
            index = Util::ArrayAllocator<TYPES...>::Alloc();
            n_assert2(this->maxId > index, "max amount of allocations exceeded!\n");
        }

        return index;
    }

    /// Deallocate an object. Just places it in freeids array for recycling
    void Dealloc(Ids::Id32 index)
    {
        // TODO: We could possibly get better performance when defragging if we insert it in reverse order (high to low)
        this->freeIds.Append(index);
    }

    /// Returns the list of free ids.
    Util::Array<Ids::Id32>& FreeIds()
    {
        return this->freeIds;
    }

    /// return number of allocated ids
    const Ids::Id32 Size() const
    {
        return this->size;
    }

private:
    Ids::Id32 maxId = 0xFFFFFFFF;
    Util::Array<Ids::Id32> freeIds;
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
        Ids::Id32 index;
        if (this->freeIds.Size() > 0)
        {
            index = this->freeIds.Back();
            this->owners[index] = Threading::Thread::GetMyThreadId();
            this->freeIds.EraseBack();
        }
        else
        {
            alloc_for_each_in_tuple(this->objects);
            index = this->size++;
            this->owners.Append(Threading::Thread::GetMyThreadId());
            this->generations.Append(0);
            n_assert2(MAX_ALLOCS > index, "max amount of allocations exceeded!\n");
        }
        this->allocationLock.Unlock();
        Ids::Id8 generation = this->generations[index];
        return index;
    }

    /// Deallocate an object. Just places it in freeids array for recycling
    void Dealloc(Ids::Id32 index)
    {
        // TODO: We could possibly get better performance when defragging if we insert it in reverse order (high to low)
        this->allocationLock.Lock();
        this->freeIds.Append(index);
        this->generations[index]++;
        this->allocationLock.Unlock();
    }

private:
    Util::Array<Ids::Id32> freeIds;
    Util::Array<Ids::Id8> generations;
};

} // namespace Ids
