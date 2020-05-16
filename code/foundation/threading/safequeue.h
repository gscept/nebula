#pragma once
//------------------------------------------------------------------------------
/**
    @class Threading::SafeQueue
    
    Thread-safe version of Util::Queue. The SafeQueue is normally configured
    to signal an internal Event object when an element is enqueued, so that
    a worker-thread can wait for new elements to arrive. This is the default
    behaviour. This doesn't make sense for a continously running thread 
    (i.e. a rendering thread), thus this behaviour can be disabled
    using the SetSignalOnEnqueueEnabled(). In this case, the
    Enqueue() method won't signal the internal event, and the Wait()
    method will return immediately without ever waiting.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "util/queue.h"
#include "threading/criticalsection.h"
#include "threading/event.h"

//------------------------------------------------------------------------------
namespace Threading
{
template<class TYPE> class SafeQueue
{
public:
    /// constructor
    SafeQueue();
    /// copy constructor
    SafeQueue(const SafeQueue<TYPE>& rhs);

    /// assignment operator
    void operator=(const SafeQueue<TYPE>& rhs);
    /// enable/disable signalling on Enqueue() (default is enabled)
    void SetSignalOnEnqueueEnabled(bool b);
    /// return signalling-on-Enqueue() flag
    bool IsSignalOnEnqueueEnabled() const;
    /// returns number of elements in the queue
    SizeT Size() const;
    /// return true if queue is empty
    bool IsEmpty() const;
    /// remove all elements from the queue
    void Clear();
    /// add element to the back of the queue
    void Enqueue(const TYPE& e);
    /// enqueue an array of elements
    void EnqueueArray(const Util::Array<TYPE>& a);
    /// remove the element from the front of the queue
    TYPE Dequeue();
    /// dequeue all events (only requires one lock)
    void DequeueAll(Util::Array<TYPE>& outArray);
    /// access to element at front of queue without removing it
    TYPE Peek() const;
    /// wait until queue contains at least one element
    void Wait();
    /// wait until queue contains at least one element, or time-out happens
    void WaitTimeout(int ms);
    /// signal the internal event, so that Wait() will return
    void Signal();
    /// erase all matching elements
    void EraseMatchingElements(const TYPE& e);

protected:
    CriticalSection criticalSection;
    Event enqueueEvent;
    bool signalOnEnqueueEnabled;
    Util::Queue<TYPE> queue;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
SafeQueue<TYPE>::SafeQueue() :
    signalOnEnqueueEnabled(true)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
SafeQueue<TYPE>::SafeQueue(const SafeQueue<TYPE>& rhs)
{
    this->criticalSection.Enter();
    this->queue = rhs.queue;
    this->signalOnEnqueueEnabled = rhs.signalOnEnqueueEnabled;
    this->criticalSection.Leave();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
SafeQueue<TYPE>::operator=(const SafeQueue<TYPE>& rhs)
{
    this->criticalSection.Enter();
    this->queue = rhs.queue;
    this->signalOnEnqueueEnabled = rhs.signalOnEnqueueEnabled;
    this->criticalSection.Leave();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
SafeQueue<TYPE>::SetSignalOnEnqueueEnabled(bool b)
{
    this->criticalSection.Enter();
    this->signalOnEnqueueEnabled = b;
    this->criticalSection.Leave();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
SafeQueue<TYPE>::Clear()
{
    this->criticalSection.Enter();
    this->queue.Clear();
    this->criticalSection.Leave();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> SizeT
SafeQueue<TYPE>::Size() const
{
    return this->queue.Size();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> bool
SafeQueue<TYPE>::IsEmpty() const
{
    this->criticalSection.Enter();
    bool isEmpty = this->queue.IsEmpty();
    this->criticalSection.Leave();
    return isEmpty;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
SafeQueue<TYPE>::Enqueue(const TYPE& e)
{
    this->criticalSection.Enter();
    this->queue.Enqueue(e);
    this->criticalSection.Leave();
    if (this->signalOnEnqueueEnabled)
    {
        this->enqueueEvent.Signal();
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
SafeQueue<TYPE>::EnqueueArray(const Util::Array<TYPE>& a)
{
    this->criticalSection.Enter();
    for (auto &i : a)
    {
        this->queue.Enqueue(i);
    }    
    this->criticalSection.Leave();
    if (this->signalOnEnqueueEnabled)
    {
        this->enqueueEvent.Signal();
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> TYPE
SafeQueue<TYPE>::Dequeue()
{
    this->criticalSection.Enter();
    TYPE e = this->queue.Dequeue();    
    this->criticalSection.Leave();
    return e;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
SafeQueue<TYPE>::DequeueAll(Util::Array<TYPE>& outArray)
{
    this->criticalSection.Enter();
#if NEBULA_BOUNDSCHECKS
	n_warn_fmt(outArray.Capacity() >= this->queue.Size(), "SafeQueue::DequeueAll(): (PERFORMANCE) Output array is too small (%d), requires (%d), array will have to grow.\n", outArray.Capacity(), this->queue.Size());
#endif
    outArray.Clear();
    for (IndexT i = 0; i < this->queue.Size(); i++)
    {
        outArray.Append(this->queue[i]);
    }
    this->queue.Clear();    
    this->criticalSection.Leave();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> TYPE
SafeQueue<TYPE>::Peek() const
{
    this->criticalSection.Enter();
    TYPE e = this->queue.Peek();
    this->criticalSection.Leave();
    return e;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
SafeQueue<TYPE>::Wait()
{
    if (this->signalOnEnqueueEnabled && this->IsEmpty())
    {
        this->enqueueEvent.Wait();
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
SafeQueue<TYPE>::WaitTimeout(int ms)
{
    if (this->signalOnEnqueueEnabled && this->IsEmpty())
    {
        this->enqueueEvent.WaitTimeout(ms);
    }
}

//------------------------------------------------------------------------------
/**
    This signals the internal event object, on which Wait() may be waiting.
    This method may be useful to wake up a thread waiting for events
    when it should stop.
*/
template<class TYPE> void
SafeQueue<TYPE>::Signal()
{
    this->enqueueEvent.Signal();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
SafeQueue<TYPE>::EraseMatchingElements(const TYPE& e)
{
    this->criticalSection.Enter();
    IndexT i;
    for (i = this->queue.Size() - 1; i >= 0; i--)
    {
        if (e == this->queue[i])
        {
            this->queue.EraseIndex(i);
        }
    }
    this->criticalSection.Leave();
}

} // namespace Threading
//------------------------------------------------------------------------------

    