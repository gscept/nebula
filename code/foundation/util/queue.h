#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::Queue
    
    Nebula3's queue class (a FIFO container).
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/array.h"

//------------------------------------------------------------------------------
namespace Util
{
template<class TYPE> class Queue
{
public:
    /// constructor
    Queue();
    /// copy constructor
    Queue(const Queue<TYPE>& rhs);
	/// conversion constructor for array
	Queue(const Array<TYPE>& rhs);

    /// assignment operator
    void operator=(const Queue<TYPE>& rhs);
    /// access element by index, 0 is the frontmost element (next to be dequeued)
    TYPE& operator[](IndexT index) const;
    /// equality operator
    bool operator==(const Queue<TYPE>& rhs) const;
    /// inequality operator
    bool operator!=(const Queue<TYPE>& rhs) const;
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
Queue<TYPE>::Queue()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Queue<TYPE>::Queue(const Queue<TYPE>& rhs)
{
    this->queueArray = rhs.queueArray;
}


//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Util::Queue<TYPE>::Queue( const Array<TYPE>& rhs )
{
	this->queueArray = rhs;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Queue<TYPE>::operator=(const Queue<TYPE>& rhs)
{
    this->queueArray = rhs.queueArray;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE&
Queue<TYPE>::operator[](IndexT index) const
{
    return this->queueArray[index];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
Queue<TYPE>::operator==(const Queue<TYPE>& rhs) const
{
    return this->queueArray == rhs.queueArray;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
Queue<TYPE>::operator!=(const Queue<TYPE>& rhs) const
{
    return this->queueArray != rhs.queueArray;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
Queue<TYPE>::Contains(const TYPE& e) const
{
    return (InvalidIndex != this->queueArray.FindIndex(e));
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Queue<TYPE>::Clear()
{
    this->queueArray.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Queue<TYPE>::Reserve(SizeT num)
{
    this->queueArray.Reserve(num);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
SizeT
Queue<TYPE>::Size() const
{
    return this->queueArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
Queue<TYPE>::IsEmpty() const
{
    return this->queueArray.IsEmpty();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Queue<TYPE>::Enqueue(const TYPE& e)
{
    this->queueArray.Append(e);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE
Queue<TYPE>::Dequeue()
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
Queue<TYPE>::Peek() const
{
    return this->queueArray.Front();
}

} // namespace Util
//------------------------------------------------------------------------------
