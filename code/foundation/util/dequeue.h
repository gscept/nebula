#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::DeQueue
    
    Faster queue class that apart from Enqueue has constant time operations.
    Assumes that items can be trivially moved by using memmove.
                
    (C) 2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "math/scalar.h"

//------------------------------------------------------------------------------
namespace Util
{
template<class TYPE> class DeQueue
{
public:
    /// constructor
    DeQueue();
    /// destructor
    ~DeQueue();
    /// copy constructor
    DeQueue(const DeQueue<TYPE>& rhs);
	
    /// assignment operator
    void operator=(const DeQueue<TYPE>& rhs);
    /// access element by index, 0 is the frontmost element (next to be deDequeued)
    TYPE& operator[](IndexT index) const;
    /// equality operator
    bool operator==(const DeQueue<TYPE>& rhs) const;
    /// inequality operator
    bool operator!=(const DeQueue<TYPE>& rhs) const;
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
DeQueue<TYPE>::DeQueue():
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
DeQueue<TYPE>::~DeQueue()
{
    if (this->data)
    {
        n_delete_array(this->data);
    }    
    this->data = nullptr;
}


//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
DeQueue<TYPE>::DeQueue(const DeQueue<TYPE>& rhs)
    : data(nullptr)
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
void
DeQueue<TYPE>::operator=(const DeQueue<TYPE>& rhs)
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
TYPE&
DeQueue<TYPE>::operator[](IndexT index) const
{
    return this->data[MapIndex(index)];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
DeQueue<TYPE>::operator==(const DeQueue<TYPE>& rhs) const
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
DeQueue<TYPE>::operator!=(const DeQueue<TYPE>& rhs) const
{
    return ! this->operator==(rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
DeQueue<TYPE>::Contains(const TYPE& e) const
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
Util::DeQueue<TYPE>::EraseIndex(const IndexT i)
{    
    n_error("fixme");
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
DeQueue<TYPE>::Clear()
{    
    this->size = 0;
    this->start = 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
DeQueue<TYPE>::Reserve(SizeT num)
{
    if (num > this->capacity)
    {
        TYPE * newdata = n_new_array(TYPE, num);
        if (this->size > 0)
        {            
            Memory::Copy(&this->data[this->start], newdata, Math::n_min(this->capacity - this->start, this->size) * sizeof(TYPE));

            IndexT wrap = (this->start + this->size) % this->capacity;
            if (wrap < this->start)
            {
                Memory::Copy(this->data, &newdata[this->capacity - this->start], wrap * sizeof(TYPE));
            }
        }
        if (this->data != nullptr)
        {
            n_delete_array(this->data);
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
DeQueue<TYPE>::Grow()
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
SizeT
DeQueue<TYPE>::Size() const
{
    return this->size;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
SizeT
DeQueue<TYPE>::Capacity() const
{
    return this->capacity;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
DeQueue<TYPE>::IsEmpty() const
{
    return this->size == 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
DeQueue<TYPE>::Enqueue(const TYPE& e)
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
TYPE
DeQueue<TYPE>::Dequeue()
{    
    this->start = this->MapIndex(1);
    --this->size;
    return this->operator[](-1);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE&
DeQueue<TYPE>::Peek() const
{
    return this->data[this->start];
}

//------------------------------------------------------------------------------
/**
    Maps an index onto the actual array index by wrapping around. can deal
    with negative indices as well
*/
template<class TYPE>
IndexT
DeQueue<TYPE>::MapIndex(IndexT idx) const
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
