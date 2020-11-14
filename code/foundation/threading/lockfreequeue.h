#pragma once
//------------------------------------------------------------------------------
/**
    A lock-free queue implements a queue which completely relies on atomic operations

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
    TYPE Dequeue();

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
    : head(nullptr)
    , tail(nullptr)
    , freeHead(nullptr)
    , freeTail(nullptr)
    , size(0)
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
    Node* prev = this->freeHead;
    for (Node& node : this->storage)
    {
        if (prev == nullptr)
            this->freeHead = &node;
        else
            prev->next = &node;

        prev = &node;
    }
    this->freeTail = prev;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void 
LockFreeQueue<TYPE>::Enqueue(const TYPE& item)
{
    // allocate a new node from storage
    Node* newNode = this->Alloc();
    newNode->data = item;
    newNode->next = nullptr;

    Node* oldTail = nullptr;
    while (true)
    {
        oldTail = this->tail;

        // try to change the tail to the new node, retry if tail doesn't match (not our turn)
        if (Interlocked::CompareExchangePointer((void* volatile*)&this->tail, (void*)newNode, (void*)oldTail) == oldTail)
        {
            // we can safetly change oldTail because nobody else can access the node yet
            if (oldTail != nullptr)
                oldTail->next = newNode;
            break;
        }
    }

    // set head to the new node if nullptr
    Interlocked::CompareExchangePointer((void* volatile*)&this->head, (void*)newNode, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline TYPE
LockFreeQueue<TYPE>::Dequeue()
{
    n_assert(this->size != 0);

    // reduce size
    Interlocked::Decrement(this->size);

    Node* oldHead = nullptr;
    while (true)
    {
        oldHead = this->head;

        // repoint head to the next in the chain
        if (Interlocked::CompareExchangePointer((void* volatile*)&this->head, (void*)oldHead->next, (void*)oldHead) == oldHead)
        {
            break;
        }
    }

    // copy return value here, if we dealloc and someone else allocs, they might have time to change the value
    TYPE ret = oldHead->data;

    // make sure to return this node to the free list
    this->Dealloc(oldHead);
    return ret;
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
    Remove node from free list
*/
template<class TYPE>
__forceinline typename LockFreeQueue<TYPE>::Node*
LockFreeQueue<TYPE>::Alloc()
{
    n_assert(this->size != this->capacity);

    // increase size
    Interlocked::Increment(this->size);

    Node* oldHead = nullptr;
    while (true)
    {
        oldHead = this->freeHead;
        if (Interlocked::CompareExchangePointer((void* volatile*)&this->freeHead, (void*)oldHead->next, (void*)oldHead) == oldHead)
        {
            break;
        }
    }

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
    Node* oldTail = nullptr;
    while (true)
    {
        oldTail = this->freeTail;

        if (Interlocked::CompareExchangePointer((void* volatile*)&this->freeTail, (void*)node, (void*)oldTail) == oldTail)
        {
            if (oldTail != nullptr)
                oldTail->next = node;
            break;
        }
    }

    // set head to tail if head is null
    Interlocked::CompareExchangePointer((void* volatile*)&this->freeHead, (void*)node, nullptr);
}

} // namespace Threading
