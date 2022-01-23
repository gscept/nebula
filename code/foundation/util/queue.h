#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::Queue
    
    Faster queue class that apart from Enqueue has constant time operations.
    Assumes that items can be trivially moved by using memmove.
                
    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "math/scalar.h"
#include "util/round.h"
#include <type_traits>

#if (__cplusplus >= 201703L ) || (_MSVC_LANG >= 201703L)
#define IFCONSTEXPR if constexpr
#else
#define IFCONSTEXPR if
#endif


//------------------------------------------------------------------------------
namespace Util
{
template<class TYPE> class Queue
{
public:
    /// constructor
    Queue();
    /// destructor
    ~Queue();
    /// copy constructor
    Queue(const Queue<TYPE>& rhs);
    /// move constructor
    Queue(Queue<TYPE>&& rhs);

    /// assignment operator
    void operator=(const Queue<TYPE>& rhs);
    /// move assignment operator
    void operator=(Queue<TYPE>&& rhs);
    /// access element by index, 0 is the frontmost element (next to be dequeued)
    TYPE& operator[](IndexT index) const;
    /// equality operator
    bool operator==(const Queue<TYPE>& rhs) const;
    /// inequality operator
    bool operator!=(const Queue<TYPE>& rhs) const;
    /// increase capacity to fit N more elements into the Dequeue (slow)
    void Reserve(SizeT num);
    /// grow Dequeue by internal growing rules (slow)
    void Grow();
    /// returns number of elements in the Dequeue
    SizeT Size() const;
    /// returns allocation of elements in the Dequeue
    SizeT Capacity() const;
    /// return true if Dequeue is empty
    bool IsEmpty() const;
    /// remove all elements from the Dequeue
    void Clear();
    /// return true if Dequeue contains element
    bool Contains(const TYPE& e) const;
    /// erase element at index (slow!!)
    void EraseIndex(const IndexT i);

    /// add element to the back of the Dequeue, can trigger grow
    void Enqueue(const TYPE& e);
    /// remove the element from the front of the Dequeue
    TYPE Dequeue();
    /// access to element at front of Dequeue without removing it
    TYPE& Peek() const;

protected:
    template<typename X>
    __forceinline
    typename std::enable_if<std::is_trivially_destructible<X>::value == false>::type
    DestroyElement(IndexT idx)
    {
        this->data[idx].~X(); 
    }
    
    template<typename X>
    __forceinline
    typename std::enable_if<std::is_trivially_destructible<X>::value == true>::type
    DestroyElement(IndexT idx)
    {
        // empty
    }

    template<typename X>
    __forceinline
        typename std::enable_if<std::is_trivially_destructible<X>::value == false>::type
        ClearAll()
    {
        for (IndexT i = 0; i < this->size; i++)
        {
            this->data[this->MapIndex(i)].~X();
        }        
    }

    template<typename X>
    __forceinline
        typename std::enable_if<std::is_trivially_destructible<X>::value == true>::type
        ClearAll()
    {
        // empty
    }
    
    /// maps index to actual item position using wrapping
    IndexT MapIndex(IndexT index) const;
    TYPE * data;

    static const SizeT MinGrowSize = 16;
    static const SizeT MaxGrowSize = 65536; 
    SizeT grow;
    SizeT start;
    SizeT size;
    SizeT capacity;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Queue<TYPE>::Queue():
    start(0),
    size(0),
    capacity(0),
    grow(16),
    data(nullptr)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Queue<TYPE>::~Queue()
{
    if (this->data)
    {
        this->Clear();
        n_delete_array(this->data);
    }    
    this->data = nullptr;
}


//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Queue<TYPE>::Queue(const Queue<TYPE>& rhs):
    data(nullptr),
    capacity(0)
{
    this->Reserve(rhs.size);
    for (IndexT i = 0; i < rhs.size; i++)
    {
        this->data[i] = rhs[i];
    }    
    this->size = rhs.size;
    this->grow = rhs.grow;
    this->start = 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Queue<TYPE>::Queue(Queue<TYPE>&& rhs) :
    data(rhs.data),
    capacity(rhs.capacity),
    size(rhs.size),
    grow(rhs.grow),
    start(rhs.start)
{
    rhs.data = nullptr;
    rhs.capacity = 0;
    rhs.size = 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Queue<TYPE>::operator=(const Queue<TYPE>& rhs)
{
    if (this != &rhs)
    {
        this->Reserve(rhs.size);
        for (IndexT i = 0; i < rhs.size; i++)
        {
            this->data[i] = rhs[i];
        }
        this->size = rhs.size;
        this->grow = rhs.grow;
        this->start = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Queue<TYPE>::operator=(Queue<TYPE>&& rhs)
{
    if (this != &rhs)
    {
        this->data = rhs.data;
        this->size = rhs.size;
        this->grow = rhs.grow;
        this->capacity = rhs.capacity;
        this->start = rhs.start;
        rhs.data = nullptr;
        rhs.size = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
__forceinline
TYPE&
Queue<TYPE>::operator[](IndexT index) const
{
    n_assert(index < this->size);
    return this->data[MapIndex(index)];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
Queue<TYPE>::operator==(const Queue<TYPE>& rhs) const
{
    if(this->size != rhs.size) return false;

    for(IndexT i= 0; i < this->size; i++)
    {
        if(!((*this)[i] == rhs[i] ))
        {
            return false;
        }
    }
    return true;    
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
Queue<TYPE>::operator!=(const Queue<TYPE>& rhs) const
{
    return ! this->operator==(rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
Queue<TYPE>::Contains(const TYPE& e) const
{
    for(IndexT i = 0 ; i<this->size; i++)
    { 
        if (this->operator[](i) == e)
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Util::Queue<TYPE>::EraseIndex(const IndexT i)
{
    n_assert(i < this->size);

    if (i == 0)
    {
        this->Dequeue();
    }
    else
    {
        IndexT idx = this->MapIndex(i);

        this->DestroyElement<TYPE>(idx);

        // check if wrapped around
        if (idx < this->start)
        {
            for (IndexT j = 0; j < this->size - i; ++j)
            {
                this->data[idx] = this->data[idx+1];
                idx++;
            }
        }
        else
        {
            for (IndexT j = 0; j < i; j++)
            {
                this->data[idx] = this->data[idx-1];
                --idx;
            }
            this->start = this->MapIndex(1);
        }
        --this->size;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Queue<TYPE>::Clear()
{    
    this->ClearAll<TYPE>();
    this->size = 0;
    this->start = 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Queue<TYPE>::Reserve(SizeT num)
{
    if (num > this->capacity)
    {
        // round up to next multiple of 64                
        num = num<64? num: Util::Round::RoundUp(num, 64);

        TYPE * newdata = n_new_array(TYPE, num);

        // check if empty
        if (this->capacity > 0)
        {
            // we could use SFINAE here as well, but as its a single if in a (rare) call its not worth the bother
            IFCONSTEXPR(std::is_trivially_copyable_v<TYPE>)
            {
                if (this->size > 0)
                {
                    SizeT upper = this->capacity - this->start;
                    SizeT lower = this->size - (this->capacity - this->start);

                    if (lower < 0)
                    {
                        Memory::Copy(&this->data[this->start], newdata, this->size * sizeof(TYPE));
                    }
                    else
                    {
                        Memory::Copy(&this->data[this->start], newdata, upper * sizeof(TYPE));
                        Memory::Copy(this->data, &newdata[upper], lower * sizeof(TYPE));
                    }
                }
                if (this->data != nullptr)
                {
                    n_delete_array(this->data);
                }
            }
            else
            {
                for (IndexT i = 0; i < this->size; i++)
                {
                    IndexT idx = this->MapIndex(i);
                    newdata[i] = this->data[idx];
                    this->DestroyElement<TYPE>(idx);
                }
                if (this->data != nullptr)
                {
                    n_delete_array(this->data);
                }
            }
        }
        this->data = newdata;        
        this->capacity = num;
        this->start = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Queue<TYPE>::Grow()
{
    SizeT growToSize;
    if (0 == this->capacity)
    {
        growToSize = this->grow;
    }
    else
    {
        // grow by half of the current capacity, but never more then MaxGrowSize
        SizeT growBy = this->capacity >> 1;
        
        if (growBy == 0)
        {
            growBy = MinGrowSize;
        }
        else if (growBy > MaxGrowSize)
        {
            growBy = MaxGrowSize;
        }
        growToSize = this->capacity + growBy;
    }
    this->Reserve(growToSize);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
__forceinline
SizeT
Queue<TYPE>::Size() const
{
    return this->size;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
__forceinline
SizeT
Queue<TYPE>::Capacity() const
{
    return this->capacity;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
__forceinline
bool
Queue<TYPE>::IsEmpty() const
{
    return this->size == 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
__forceinline
void
Queue<TYPE>::Enqueue(const TYPE& e)
{
    if (this->size == this->capacity)
    {
        this->Grow();
    }
    this->data[this->MapIndex(this->size++)] = e;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
__forceinline
TYPE
Queue<TYPE>::Dequeue()
{    
    n_assert(this->size > 0);

    TYPE t = this->data[this->start];
    #if __cplusplus > 201703L
    if constexpr (!std::is_nothrow_destructible_v<TYPE>)
    {
        (&(this->data[this->start]))->~TYPE();
    }
    #else
    this->DestroyElement<TYPE>(this->start);
    #endif
    this->start = this->MapIndex(1);
    --this->size;
    return t;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
__forceinline
TYPE&
Queue<TYPE>::Peek() const
{
    n_assert(this->size > 0);
    return this->data[this->start];
}

//------------------------------------------------------------------------------
/**
    Maps an index onto the actual array index by wrapping around. can deal
    with negative indices as well
*/

template<class TYPE>
__forceinline
IndexT
Queue<TYPE>::MapIndex(IndexT idx) const
{    
    idx = (idx + this->start) % this->capacity;
    if (idx > 0)
    {
        return idx;
    }
    else
    {
        return (idx + this->capacity) % this->capacity;
    }
}


} // namespace Util
//------------------------------------------------------------------------------
