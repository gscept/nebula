#pragma once
//------------------------------------------------------------------------------
/**
    This simple Id pool implements a set of free and used consecutive integers.
    The pool can be configured to only release new Ids up to a certain maximum,
    after which it asserts. 

    Essentially, it allocates a certain amount of new Ids when needed 
    (not following the Array method of doubling its size) and can recycle
    the indices for later. Unlike the IdGenerationPool, this one does not
    keep track of how many times an id has been reused, so use this with
    caution!

    Maximum amount of bits produced by this system is 32.
    
    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <cstdint>
#include "core/types.h"
#include "util/array.h"
#include "id.h"
#include "math/scalar.h"
#include <functional>
namespace Ids
{
class IdPool
{
public:
    /// constructor
    IdPool();
    /// constructor with maximum size
    IdPool(const uint max, const uint grow = 512);
    /// destructor
    ~IdPool();

    /// get new id
    uint Alloc();
    /// free id
    void Dealloc(uint id);
    /// reserve ids
    void Reserve(uint numIds);
    /// get number of active ids
    uint GetNumUsed() const;
    /// get number of free elements
    uint GetNumFree() const;
    /// iterate free indices
    void ForEachFree(const std::function<void(uint, uint)> fun, SizeT num);
    /// frees up lhs and erases rhs
    void Move(uint lhs, uint rhs);
    /// get grow
    const uint GetGrow() const;
private:

    Util::Array<uint> free;
    uint maxId;
    uint grow;
};

//------------------------------------------------------------------------------
/**
*/
inline
IdPool::IdPool() :
    maxId(0xFFFFFFFF),      // int max
    grow(512)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
IdPool::IdPool(const uint max, const uint grow) :
    maxId(max),
    grow(grow)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
IdPool::~IdPool()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline uint
IdPool::Alloc()
{
    // if we're out of ids, allocate more, but with a controlled grow (not log2 as per Array)
    if (this->free.Size() == 0)
    {
        // calculate how many more indices we are allowed to get
        SizeT growTo = Math::min(this->maxId, this->grow);
        SizeT oldCapacity = this->free.Capacity();

        // make sure we don't allocate too many indices
        n_assert2((uint)(oldCapacity + growTo) < this->maxId, "Pool is full! Be careful with how much you allocate!\n");

        // reserve more space for new Ids
        this->free.Reserve(oldCapacity + growTo);
        IndexT i;
        for (i = this->free.Capacity()-1; i >= oldCapacity; i--)
        {
            // count backwards from the max id
            this->free.Append(this->maxId - i);
        }
    }

    // if we do an inverse erase, we don't have to move elements, and since we know the max id, subtract it
    uint id = this->maxId - this->free.Back();
    this->free.EraseIndex(this->free.Size() - 1);
    return id;
}

//------------------------------------------------------------------------------
/**
*/
inline void
IdPool::Dealloc(uint id)
{
    this->free.Append(this->maxId - id);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
IdPool::Reserve(uint numIds)
{
    this->maxId = numIds;
    this->free.Reserve(numIds);
    for (int i = numIds - 1; i >= 0; i--)
    {
        this->free.Append(this->maxId - i);
    }
}

//------------------------------------------------------------------------------
/**
*/
inline uint
IdPool::GetNumUsed() const
{
    return this->free.Capacity() - this->free.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline uint
IdPool::GetNumFree() const
{
    return this->free.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline void
IdPool::ForEachFree(const std::function<void(uint, uint)> fun, SizeT num)
{
    SizeT size = this->free.Size();
    for (IndexT i = size - 1; i >= 0; i--)
    {
        const uint id = this->maxId - this->free[i];
        if (id < (uint)num)
        {
            fun(id, i);
            num--;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
IdPool::Move(uint idx, uint id)
{
    this->free.Append(this->maxId - id);
    this->free.EraseIndex(idx);
}

//------------------------------------------------------------------------------
/**
*/
inline const uint
IdPool::GetGrow() const
{
    return this->grow;
}

} // namespace Ids
