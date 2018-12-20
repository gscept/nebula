#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::List
  
    Implements a doubly linked list. Since list elements can be all over the
    place in memory, dynamic arrays are often the better choice, unless 
    insert/remove performance is more important then traversal performance.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/    
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Util
{
template<class TYPE> class List
{
private:
    class Node;
public:
    class Iterator;

    /// constructor
    List();
    /// copy constructor
    List(const List<TYPE>& rhs);
    /// move constructor
    List(List<TYPE>&& rhs);
    /// destructor
    ~List();
    /// assignment operator
    void operator=(const List<TYPE>& rhs);
    /// assignment operator
    void operator=(List<TYPE>&& rhs);

    /// return true if the list is empty
    bool IsEmpty() const;
    /// get number of elements in list (slow)
    SizeT Size() const;
    /// clear list
    void Clear();
    /// add contents of other list to this list
    void AddList(const List<TYPE>& l);
    /// add element after given element
    Iterator AddAfter(Iterator iter, const TYPE& e);
    /// add element before given element
    Iterator AddBefore(Iterator iter, const TYPE& e);
    /// add element to beginning of list
    Iterator AddFront(const TYPE& e);
    /// add element to end of list
    Iterator AddBack(const TYPE& e);
    /// remove first element of list
    TYPE RemoveFront();
    /// remove last element of list
    TYPE RemoveBack();
    /// remove given element
    TYPE Remove(Iterator iter);
    /// get first element
    TYPE& Front() const;
    /// get last element
    TYPE& Back() const;
    /// get iterator to first element
    Iterator Begin() const;
    /// get iterator past the last element
    Iterator End() const;
    /// find element in array (slow)
    Iterator Find(const TYPE& e, Iterator start) const;

    /// the list iterator
    class Iterator
    {
    public:
        /// default constructor
        Iterator();
        /// constructor
        Iterator(Node* node);
        /// copy constructor
        Iterator(const Iterator& rhs);
        /// assignment operator
        const Iterator& operator=(const Iterator& rhs);
        /// equality operator
        bool operator==(const Iterator& rhs) const;
        /// inequality operator
        bool operator!=(const Iterator& rhs) const;
        /// pre-increment operator
        const Iterator& operator++();
        /// post-increment operator
        Iterator operator++(int);
        /// pre-decrement operator
        const Iterator& operator--();
        /// post-increment operator
        Iterator operator--(int);
        /// bool operator
        operator bool() const;
        /// safe -> operator
        TYPE* operator->() const;
        /// safe dereference operator
        TYPE& operator*() const;
    private:
        friend class List<TYPE>;

        /// access to node
        Node* GetNode() const;

        Node* node;
    };

private:
    /// a node in the list
    class Node
    {
        friend class List;
        friend class Iterator;

        /// constructor
        Node(const TYPE& v);
        /// destructor
        ~Node();
        /// set pointer to next node
        void SetNext(Node* n);
        /// get pointer to next node
        Node* GetNext() const;
        /// set pointer to previous node
        void SetPrev(Node* p);
        /// get pointer to previous node
        Node* GetPrev() const;
        /// get value reference
        TYPE& Value();

        Node* next;
        Node* prev;
        TYPE value;
    };

    Node* front;
    Node* back;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
List<TYPE>::Node::Node(const TYPE& val) :
    next(0),
    prev(0),
    value(val)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
List<TYPE>::Node::~Node()
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(0 == this->next);
    n_assert(0 == this->prev);
    #endif
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
List<TYPE>::Node::SetNext(Node* n)
{
    this->next = n;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
typename List<TYPE>::Node*
List<TYPE>::Node::GetNext() const
{
    return this->next;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
List<TYPE>::Node::SetPrev(Node* p)
{
    this->prev = p;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
typename List<TYPE>::Node*
List<TYPE>::Node::GetPrev() const
{
    return this->prev;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE&
List<TYPE>::Node::Value()
{
    return this->value;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
List<TYPE>::Iterator::Iterator() :
    node(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
List<TYPE>::Iterator::Iterator(Node* n) :
    node(n)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
List<TYPE>::Iterator::Iterator(const Iterator& rhs) :
    node(rhs.node)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
const typename List<TYPE>::Iterator&
List<TYPE>::Iterator::operator=(const Iterator& rhs)
{
    if (&rhs != this)
    {
        this->node = rhs.node;
    }
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
List<TYPE>::Iterator::operator==(const Iterator& rhs) const
{
    return (this->node == rhs.node);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
List<TYPE>::Iterator::operator!=(const Iterator& rhs) const
{
    return (this->node != rhs.node);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
const typename List<TYPE>::Iterator&
List<TYPE>::Iterator::operator++()
{
    #if NEBULA_BOUNDSCHECKS    
    n_assert(0 != this->node);
    #endif
    this->node = this->node->GetNext();
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
typename List<TYPE>::Iterator
List<TYPE>::Iterator::operator++(int)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(0 != this->node);
    #endif
    Iterator temp(this->node);
    this->node = this->node->GetNext();
    return temp;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
const typename List<TYPE>::Iterator&
List<TYPE>::Iterator::operator--()
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(0 != this->node);
    #endif
    this->node = this->node->GetPred();
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
typename List<TYPE>::Iterator
List<TYPE>::Iterator::operator--(int)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(0 != this->node);
    #endif
    Iterator temp(this->node);    
    this->node = this->node->GetPred();
    return temp;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
List<TYPE>::Iterator::operator bool() const
{
    return (0 != this->node);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE*
List<TYPE>::Iterator::operator->() const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->node);
    #endif
    return &(this->node->Value());
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE&
List<TYPE>::Iterator::operator*() const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->node);
    #endif
    return this->node->Value();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
typename List<TYPE>::Node*
List<TYPE>::Iterator::GetNode() const
{
    return this->node;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
List<TYPE>::List() :
    front(0),
    back(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
List<TYPE>::List(const List<TYPE>& rhs) :
    front(0),
    back(0)
{
    this->AddList(rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
List<TYPE>::List(List<TYPE>&& rhs) :
    front(rhs.front),
    back(rhs.back)
{
    rhs.front = nullptr;
    rhs.back = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
List<TYPE>::~List()
{
    this->Clear();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
List<TYPE>::operator=(const List<TYPE>& rhs)
{
    this->Clear();
    this->AddList(rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
List<TYPE>::IsEmpty() const
{
    return (0 == this->front);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
SizeT
List<TYPE>::Size() const
{
    Iterator iter;
    SizeT size = 0;
    for (iter = this->Begin(); iter != this->End(); ++iter)
    {
        size++;
    }
    return size;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
List<TYPE>::Clear()
{
    while (this->back)
    {
        this->RemoveBack();
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
List<TYPE>::AddList(const List<TYPE>& rhs)
{
    Iterator iter;
    for (iter = rhs.Begin(); iter != rhs.End(); ++iter)
    {
        this->AddBack(*iter);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
typename List<TYPE>::Iterator
List<TYPE>::AddAfter(Iterator iter, const TYPE& e)
{
    Node* node = n_new(Node(e));
    if (0 == iter.GetNode())
    {
        #if NEBULA_BOUNDSCHECKS
        n_assert((0 == this->front) && (0 == this->back));
        #endif
        this->front = node;
        this->back  = node;
    }
    else
    {
        if (iter.GetNode() == this->back)
        {
            this->back = node;
        }
        if (0 != iter.GetNode()->GetNext())
        {
            iter.GetNode()->GetNext()->SetPrev(node);
        }
        node->SetNext(iter.GetNode()->GetNext());
        iter.GetNode()->SetNext(node);
        node->SetPrev(iter.GetNode());
    }
    return Iterator(node);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
typename List<TYPE>::Iterator
List<TYPE>::AddBefore(Iterator iter, const TYPE& e)
{
    Node *node = n_new(Node(e));
    if (0 == iter.GetNode())
    {
        #if NEBULA_BOUNDSCHECKS
        n_assert((0 == this->front) && (0 == this->back));
        #endif
        this->front = node;
        this->back = node;
    }
    else
    {
        if (iter.GetNode() == this->front)
        {
            this->front = node;
        }
        if (0 != iter.GetNode()->GetPrev())
        {
            iter.GetNode()->GetPrev()->SetNext(node);
        }
        node->SetPrev(iter.GetNode()->GetPrev());
        iter.GetNode()->SetPrev(node);
        node->SetNext(iter.GetNode());
    }
    return Iterator(node);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
typename List<TYPE>::Iterator
List<TYPE>::AddFront(const TYPE& e)
{
    return this->AddBefore(this->front, e);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
typename List<TYPE>::Iterator
List<TYPE>::AddBack(const TYPE& e)
{
    return this->AddAfter(this->back, e);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE
List<TYPE>::Remove(Iterator iter)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(iter.GetNode());
    #endif
    Node* node = iter.GetNode();
    if (node->GetPrev())
    {
        node->GetPrev()->SetNext(node->GetNext());
    }
    if (node->GetNext())
    {
        node->GetNext()->SetPrev(node->GetPrev());
    }
    if (node == this->front)
    {
        this->front = node->GetNext();
    }
    if (node == this->back)
    {
        this->back = node->GetPrev();
    }
    node->SetNext(0);
    node->SetPrev(0);
    TYPE elm = node->Value();
    n_delete(node);
    return elm;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE
List<TYPE>::RemoveFront()
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(0 != this->front);
    #endif
    return this->Remove(this->front);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE
List<TYPE>::RemoveBack()
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(0 != this->back);
    #endif
    return this->Remove(this->back);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE&
List<TYPE>::Front() const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(0 != this->front);
    #endif
    return this->front->Value();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE&
List<TYPE>::Back() const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(0 != this->back);
    #endif
    return this->back->Value();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
typename List<TYPE>::Iterator
List<TYPE>::Begin() const
{
    return Iterator(this->front);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
typename List<TYPE>::Iterator
List<TYPE>::End() const
{
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
typename List<TYPE>::Iterator
List<TYPE>::Find(const TYPE& e, Iterator start) const
{
    for (; start != this->End(); ++start)
    {
        if (*start == e)
        {
            return start;
        }
    }
    return 0;
}

} // namespace Util
//------------------------------------------------------------------------------
