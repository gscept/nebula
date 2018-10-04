#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::Stack
  
    Nebula's stack class (a FILO container).
      
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/array.h"

//------------------------------------------------------------------------------
namespace Util
{
template<class TYPE> class Stack
{
public:
    /// constructor
    Stack();
    /// copy constructor
    Stack(const Stack<TYPE>& rhs);
 
    /// assignment operator
    void operator=(const Stack<TYPE>& rhs);
    /// access element by index, 0 is the topmost element
    TYPE& operator[](IndexT index) const;
    /// equality operator
    bool operator==(const Stack<TYPE>& rhs) const;
    /// inequality operator
    bool operator!=(const Stack<TYPE>& rhs) const;
    /// returns number of elements on stack
    SizeT Size() const;
    /// returns true if stack is empty
    bool IsEmpty() const;
    /// remove all elements from the stack
    void Clear();
    /// return true if stack contains element
    bool Contains(const TYPE& e) const;
	/// erase element at index
	void EraseIndex(const IndexT i);

    /// push an element on the stack
    void Push(const TYPE& e);
    /// get reference of topmost element of stack, without removing it
    TYPE& Peek() const;
    /// get topmost element of stack, remove element
    TYPE Pop();

private:
    Array<TYPE> stackArray;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Stack<TYPE>::Stack()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Stack<TYPE>::Stack(const Stack<TYPE>& rhs)
{
    this->stackArray = rhs.stackArray;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Stack<TYPE>::operator=(const Stack<TYPE>& rhs)
{
    this->stackArray = rhs.stackArray;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE&
Stack<TYPE>::operator[](IndexT index) const
{
    return this->stackArray[this->stackArray.Size() - 1 - index];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
Stack<TYPE>::operator==(const Stack<TYPE>& rhs) const
{
    return this->stackArray == rhs.stackArray;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
Stack<TYPE>::operator!=(const Stack<TYPE>& rhs) const
{
    return this->stackArray != rhs.stackArray;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
Stack<TYPE>::Contains(const TYPE& e) const
{
    return (InvalidIndex != this->stackArray.FindIndex(e));
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Stack<TYPE>::Clear()
{
    this->stackArray.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
SizeT
Stack<TYPE>::Size() const
{
    return this->stackArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
Stack<TYPE>::IsEmpty() const
{
    return this->stackArray.IsEmpty();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Util::Stack<TYPE>::EraseIndex(const IndexT i)
{
	this->stackArray.EraseIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Stack<TYPE>::Push(const TYPE& e)
{
    this->stackArray.Append(e);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE&
Stack<TYPE>::Peek() const
{
    return this->stackArray.Back();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE
Stack<TYPE>::Pop()
{
    TYPE e = this->stackArray.Back();
    this->stackArray.EraseIndex(this->stackArray.Size() - 1);
    return e;
}


} // namespace Util
//------------------------------------------------------------------------------
