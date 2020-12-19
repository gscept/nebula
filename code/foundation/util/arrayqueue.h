#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::Queue
    
    Nebula's queue class (a FIFO container).
    
    @todo   This is extremely slow and should probably use a list instead.

    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/array.h"

//------------------------------------------------------------------------------
namespace Util
{
template<class TYPE> class ArrayQueue
{
public:
    /// constructor
    ArrayQueue();
    /// copy constructor
    ArrayQueue(const ArrayQueue<TYPE>& rhs);
    /// move constructor
    ArrayQueue(ArrayQueue<TYPE>&& rhs);
    /// conversion constructor for array
    ArrayQueue(const Array<TYPE>& rhs);

    /// assignment operator
    void operator=(const ArrayQueue<TYPE>& rhs);
    /// move assignment operator
    void operator=(ArrayQueue<TYPE>&& rhs);
    /// access element by index, 0 is the frontmost element (next to be dequeued)
    TYPE& operator[](IndexT index) const;
    /// equality operator
    bool operator==(const ArrayQueue<TYPE>& rhs) const;
    /// inequality operator
    bool operator!=(const ArrayQueue<TYPE>& rhs) const;
    /// increase capacity to fit N more elements into the queue
    void Reserve(SizeT num);
    /// returns number of elements in the queue
    SizeT Size() const;
    /// return true if queue is empty
    bool IsEmpty() const;
    /// remove all elements from the queue
    void Clear();
    /// return true if queue contains element
    bool Contains(const TYPE& e) const;
    /// erase element at index
    void EraseIndex(const IndexT i);

    /// add element to the back of the queue
    void Enqueue(const TYPE& e);
    /// remove the element from the front of the queue
    TYPE Dequeue();
    /// access to element at front of queue without removing it
    TYPE& Peek() const;

protected:
    Array<TYPE> queueArray;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
ArrayQueue<TYPE>::ArrayQueue()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
ArrayQueue<TYPE>::ArrayQueue(const ArrayQueue<TYPE>& rhs)
{
    this->queueArray = rhs.queueArray;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
ArrayQueue<TYPE>::ArrayQueue(ArrayQueue<TYPE>&& rhs):
    queueArray(std::move(rhs.queueArray))
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Util::ArrayQueue<TYPE>::ArrayQueue(const Array<TYPE>& rhs)
{
    this->queueArray = rhs;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
ArrayQueue<TYPE>::operator=(const ArrayQueue<TYPE>& rhs)
{
    this->queueArray = rhs.queueArray;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
ArrayQueue<TYPE>::operator=(ArrayQueue<TYPE>&& rhs)
{
    this->queueArray = std::move(rhs.queueArray);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE&
ArrayQueue<TYPE>::operator[](IndexT index) const
{
    return this->queueArray[index];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
ArrayQueue<TYPE>::operator==(const ArrayQueue<TYPE>& rhs) const
{
    return this->queueArray == rhs.queueArray;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
ArrayQueue<TYPE>::operator!=(const ArrayQueue<TYPE>& rhs) const
{
    return this->queueArray != rhs.queueArray;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
ArrayQueue<TYPE>::Contains(const TYPE& e) const
{
    return (InvalidIndex != this->queueArray.FindIndex(e));
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Util::ArrayQueue<TYPE>::EraseIndex(const IndexT i)
{
    this->queueArray.EraseIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
ArrayQueue<TYPE>::Clear()
{
    this->queueArray.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
ArrayQueue<TYPE>::Reserve(SizeT num)
{
    this->queueArray.Reserve(num);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
SizeT
ArrayQueue<TYPE>::Size() const
{
    return this->queueArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
ArrayQueue<TYPE>::IsEmpty() const
{
    return this->queueArray.IsEmpty();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
ArrayQueue<TYPE>::Enqueue(const TYPE& e)
{
    this->queueArray.Append(e);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE
ArrayQueue<TYPE>::Dequeue()
{
    TYPE e = this->queueArray.Front();
    this->queueArray.EraseIndex(0);
    return e;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE&
ArrayQueue<TYPE>::Peek() const
{
    return this->queueArray.Front();
}

} // namespace Util
//------------------------------------------------------------------------------
