#pragma once
//------------------------------------------------------------------------------
/**
    @class Threading::SafePriorityQueue
    
    A thread-safe priority-sorted queue which protects itself with critical 
    sections. Offers a method to wait for new elements to be added. Useful 
    for inter-thread communications.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "util/queue.h"
#include "threading/criticalsection.h"
#include "threading/event.h"
    
//------------------------------------------------------------------------------
namespace Threading
{
template<class PRITYPE, class TYPE> class SafePriorityQueue : protected Util::Queue<Util::KeyValuePair<PRITYPE, TYPE> >
{
public:
    /// constructor
    SafePriorityQueue();
    /// copy constructor
    SafePriorityQueue(const SafePriorityQueue<PRITYPE,TYPE>& rhs);

    /// assignment operator
    void operator=(const SafePriorityQueue<PRITYPE,TYPE>& rhs);
    /// returns number of elements in the queue
    SizeT Size() const;
    /// return true if queue is empty
    bool IsEmpty() const;
    /// remove all elements from the queue
    void Clear();

    /// add element to the back of the queue
    void Insert(PRITYPE pri, const TYPE& e);
    /// erase all matching elements
    void EraseMatchingElements(const TYPE& e);
    /// remove the element from the front of the queue
    TYPE Dequeue();
    /// get copy of element at front of queue without removing it
    TYPE Peek() const;
    /// wait until queue contains at least one element
    void Wait();
    /// signal the internal event, so that Wait() will return
    void Signal();

protected:
    CriticalSection criticalSection;
    Event enqueueEvent;
};

//------------------------------------------------------------------------------
/**
*/
template<class PRITYPE, class TYPE>
SafePriorityQueue<PRITYPE,TYPE>::SafePriorityQueue()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class PRITYPE, class TYPE>
SafePriorityQueue<PRITYPE,TYPE>::SafePriorityQueue(const SafePriorityQueue<PRITYPE,TYPE>& rhs)
{
    this->criticalSection.Enter();
    this->queueArray = rhs.queueArray;
    this->criticalSection.Leave();
}

//------------------------------------------------------------------------------
/**
*/
template<class PRITYPE, class TYPE> void
SafePriorityQueue<PRITYPE,TYPE>::operator=(const SafePriorityQueue<PRITYPE,TYPE>& rhs)
{
    this->criticalSection.Enter();
    this->queueArray = rhs.queueArray;
    this->criticalSection.Leave();
}

//------------------------------------------------------------------------------
/**
*/
template<class PRITYPE, class TYPE> void
SafePriorityQueue<PRITYPE,TYPE>::Clear()
{
    this->criticalSection.Enter();
    this->queueArray.Clear();
    this->criticalSection.Leave();
}

//------------------------------------------------------------------------------
/**
*/
template<class PRITYPE, class TYPE> SizeT
SafePriorityQueue<PRITYPE,TYPE>::Size() const
{
    return this->queueArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
template<class PRITYPE, class TYPE> bool
SafePriorityQueue<PRITYPE,TYPE>::IsEmpty() const
{
    return this->queueArray.IsEmpty();
}

//------------------------------------------------------------------------------
/**
*/
template<class PRITYPE, class TYPE> void
SafePriorityQueue<PRITYPE,TYPE>::Insert(PRITYPE pri, const TYPE& e)
{
    Util::KeyValuePair<PRITYPE, TYPE> kvp(pri, e);
    this->criticalSection.Enter();
    this->queueArray.InsertSorted(kvp);
    this->criticalSection.Leave();
    this->enqueueEvent.Signal();
}

//------------------------------------------------------------------------------
/**
*/
template<class PRITYPE, class TYPE> void
SafePriorityQueue<PRITYPE,TYPE>::EraseMatchingElements(const TYPE& e)
{
    this->criticalSection.Enter();
    //Util::Array<Util::KeyValuePair<PRITYPE,TYPE> >::Iterator iter; 
    for (auto iter = this->queueArray.Begin(); iter != this->queueArray.End();)
    {
        if ((*iter).Value() == e)
        {
            iter = this->queueArray.Erase(iter);
        }
        else
        {
            iter++;
        }
    }
    this->criticalSection.Leave();
}

//------------------------------------------------------------------------------
/**
*/
template<class PRITYPE, class TYPE> TYPE
SafePriorityQueue<PRITYPE,TYPE>::Dequeue()
{
    this->criticalSection.Enter();
    TYPE value = this->queueArray.Front().Value();
    this->queueArray.EraseIndex(0);
    this->criticalSection.Leave();
    return value;
}

//------------------------------------------------------------------------------
/**
*/
template<class PRITYPE, class TYPE> TYPE
SafePriorityQueue<PRITYPE,TYPE>::Peek() const
{
    this->criticalSection.Enter();
    TYPE value = this->queueArray.Front().Value();
    this->criticalSection.Leave();
    return value;
}

//------------------------------------------------------------------------------
/**
*/
template<class PRITYPE, class TYPE> void
SafePriorityQueue<PRITYPE,TYPE>::Wait()
{
    if (this->queueArray.Size() == 0)
    {
        this->enqueueEvent.Wait();
    }
}

//------------------------------------------------------------------------------
/**
    This signals the internal event object, on which Wait() may be waiting.
    This method may be useful to wake up a thread waiting for events
    when it should stop.
*/
template<class PRITYPE, class TYPE> void
SafePriorityQueue<PRITYPE,TYPE>::Signal()
{
    this->enqueueEvent.Signal();
}

} // namespace Threading
//------------------------------------------------------------------------------
