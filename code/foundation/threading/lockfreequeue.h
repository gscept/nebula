#pragma once
//------------------------------------------------------------------------------
/**
    A lock-free queue implements a queue which completely relies on atomic operations.

    All operations first performs a detachment of a node, followed by a modification of that node.

    Removal:

    1. Get the node you want to pop.
    2. If non-null, get the next node from 1.
    3. Do an interlocked comparison-swap, if we won the race, set head to the next node.
    4. Otherwise, go back to 1.
    5. oldHead is now a node which is detached and can not be reached by any other thread.

    Node* oldHead;
    Node* newHead;
    while (true)
    {
        oldHead = this->head;
        if (oldHead == nullptr)
            return false;

        newHead = oldHead->next;
        if (CompareAndSwap(&this->head, oldHead, newHead))
            break;
    }

    Insertion:

    1. Get the value you expect to modify (head, tail, free head, free tail)
    2. Do an interlocked comparison exchange, check if value from 1. matches current value of what you expect, exchange values.
    3. If not, go back to 1.
    4. oldTail is now detached and can not reached by any other thread.

    Example:

    Node* oldTail = nullptr;
    while (true)
    {
        oldTail = this->tail;

        // if we won the race (oldTail == this->tail), swap this->tail to newNode
        if (CompareAndSwap(&this->tail, oldTail, newNode)
            break;
    }

    if (oldTail != nullptr)
        oldTail->next = newNode;

    This queue implements two linked lists. One for the resident nodes and one for the free nodes.
    The free nodes and resident nodes follow identical patterns, howeven the resident nodes will
    attempt to allocate from the free nodes (and assert that it worked) or return a node to the free
    list. 

    TODO: 
        Add support for ABA - make a new type of node which accompanies the node pointer with an integer
        allowing for knowing if a node has been modified even if it's state looks identical.
        For example, we might be Thread A running an Enqueue operation, and in the CompareAndSwap
        we assume we wont the race because this->tail is equivalent to oldTail, BUT! 
        What can also happen is that between where we get oldTail and to the CompareAndSwap (CAS) 
        operation, Thread B comes in and does Enqueue, Dequeue, leaving us in the same state but
        with changes having happened. If we instead accompany each node with an atomic counter, and
        increase it for every time we make a modification, we can safetly do the CAS and know for sure
        if the node has been tampered with or not. 

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "interlocked.h"
#include "util/fixedarray.h"
namespace Threading
{

template <class TYPE>
class LockFreeQueue
{
public:
    /// constructor
    LockFreeQueue();
    /// destructor
    ~LockFreeQueue();

    /// resizes the queue to hold N elements
    void Resize(const SizeT size);

    /// enqueue item
    void Enqueue(const TYPE& item);
    /// dequeue item
    bool Dequeue(TYPE& item);

    /// get size
    SizeT Size();
    /// returns true if empty
    bool IsEmpty();

private:

    struct Node
    {
        TYPE data;
        Node* next;

        Node()
            : data(TYPE())
            , next(nullptr)
        {};
    };

    Node* head;
    Node* tail;
    volatile SizeT size;

    /// allocates a new node from the storage
    Node* Alloc();
    /// deallocs a node and puts it back into storage
    void Dealloc(Node* node);

    /// does a compare-and-swap
    bool CompareAndSwap(Node** currentValue, Node* expectValue, Node* newValue);

    Node* freeHead;
    Node* freeTail;
    Util::FixedArray<Node> storage;
    SizeT capacity;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline 
LockFreeQueue<TYPE>::LockFreeQueue()
    : size(0)
{
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline 
LockFreeQueue<TYPE>::~LockFreeQueue()
{
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void 
LockFreeQueue<TYPE>::Resize(const SizeT size)
{
    this->capacity = size;
    this->storage.Resize(size);

    // setup free list
    Node* prev = nullptr;
    for (IndexT i = 0; i < this->storage.Size(); i++)
    {
        if (prev != nullptr)
            prev->next = &this->storage[i];
        prev = &this->storage[i];
    }
    this->freeHead = this->storage.Begin();
    this->freeTail = this->storage.End() - 1;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void 
LockFreeQueue<TYPE>::Enqueue(const TYPE& item)
{
    // increase size
    int size = Interlocked::Increment(this->size);
    n_assert(size <= this->capacity);

    // allocate a new node from storage
    Node* newNode = this->Alloc();
    newNode->data = item;
    newNode->next = nullptr;

    Node* oldTail = nullptr;
    while (true)
    {
        oldTail = this->tail;

        if (CompareAndSwap(&this->tail, oldTail, newNode))
            break;
    }

    if (oldTail != nullptr)
        oldTail->next = newNode;

    // if head is null, set head to new node
    CompareAndSwap(&this->head, nullptr, newNode);
    CompareAndSwap(&this->tail, nullptr, newNode);

    /*
    CompareAndSwap(this->tail, oldTail, newNode);
    */
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline bool
LockFreeQueue<TYPE>::Dequeue(TYPE& item)
{
    Node* oldHead;
    Node* newHead;
    while (true)
    {
        oldHead = this->head;
        if (oldHead == nullptr)
            return false;

        newHead = oldHead->next;
        if (CompareAndSwap(&this->head, oldHead, newHead))
            break;
    }

    // set tail to nullptr if head became tail
    oldHead->next = nullptr;

    // copy return value here, if we dealloc and someone else allocs, they might have time to change the value
    item = oldHead->data;

    // make sure to return this node to the free list
    this->Dealloc(oldHead);

    int size = Interlocked::Decrement(this->size);
    n_assert(size >= 0);

    return true;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline SizeT 
LockFreeQueue<TYPE>::Size()
{
    return this->size;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline bool 
LockFreeQueue<TYPE>::IsEmpty()
{
    return this->size == 0;
}

//------------------------------------------------------------------------------
/**
    
*/
template<class TYPE>
__forceinline typename LockFreeQueue<TYPE>::Node*
LockFreeQueue<TYPE>::Alloc()
{
    Node* oldHead = nullptr;
    Node* newHead = nullptr;
    while (true)
    {
        oldHead = this->freeHead;
        if (oldHead == nullptr)
            continue;
        newHead = oldHead->next;

        if (CompareAndSwap(&this->freeHead, oldHead, newHead))
            break;
    }

    n_assert(oldHead != nullptr);
    oldHead->next = nullptr;

    return oldHead;
}

//------------------------------------------------------------------------------
/**
    Add node to free list
*/
template<class TYPE>
__forceinline void 
LockFreeQueue<TYPE>::Dealloc(LockFreeQueue<TYPE>::Node* node)
{
    n_assert(node != nullptr);
    Node* oldTail = nullptr;
    while (true)
    {
        oldTail = this->freeTail;

        if (CompareAndSwap(&this->freeTail, oldTail, node))
            break;
    }

    if (oldTail != nullptr)
        oldTail->next = node;

    // if null, set both iterators to the same node
    CompareAndSwap(&this->freeHead, nullptr, node);
    CompareAndSwap(&this->freeTail, nullptr, node);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline bool 
LockFreeQueue<TYPE>::CompareAndSwap(Node** currentValue, Node* expectValue, Node* newValue)
{
    return Interlocked::CompareExchangePointer((void* volatile*)currentValue, (void*)newValue, (void*)expectValue) == expectValue;
}

} // namespace Threading
