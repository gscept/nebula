#pragma once
//------------------------------------------------------------------------------
/**
    @class WeakPtr
    
    A smart pointer which does not change the reference count of the
    target object. Use this to prevent cyclic dependencies between
    objects. NOTE: The weak ptr only has a subset of methods of Ptr<>.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/ptr.h"

//------------------------------------------------------------------------------
template<class TYPE>
class WeakPtr
{
public:
    /// constructor
    WeakPtr();
    /// construct from C++ pointer
    WeakPtr(TYPE* p);
    /// construct from Ptr<> pointer
    WeakPtr(const Ptr<TYPE>& p);
    /// construct from WeakPtr<> pointer
    WeakPtr(const WeakPtr<TYPE>& p);
    /// destructor
    ~WeakPtr();
    /// assignment operator from Ptr<>
    void operator=(const Ptr<TYPE>& rhs);
    /// assignment operator from WeakPtr<>
    void operator=(const WeakPtr<TYPE>& rhs);
    /// assignment operator
    void operator=(TYPE* rhs);
    /// safe -> operator
    TYPE* operator->() const;
    /// safe dereference operator
    TYPE& operator*() const;
    /// safe pointer cast operator
    operator TYPE*() const;
    /// check if pointer is valid
    bool isvalid() const;
    /// return direct pointer (asserts if null pointer)
    TYPE* get() const;
    /// return direct pointer (returns null pointer)
    TYPE* get_unsafe() const;

private:
    TYPE* ptr;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
WeakPtr<TYPE>::WeakPtr() : 
    ptr(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
WeakPtr<TYPE>::WeakPtr(TYPE* p) : 
    ptr(p)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
WeakPtr<TYPE>::WeakPtr(const Ptr<TYPE>& p) : 
    ptr(p.get_unsafe())
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
WeakPtr<TYPE>::WeakPtr(const WeakPtr<TYPE>& p) : 
    ptr(p.ptr)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
WeakPtr<TYPE>::~WeakPtr()
{
    this->ptr = 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
WeakPtr<TYPE>::operator=(const Ptr<TYPE>& rhs)
{
    this->ptr = rhs.get_unsafe();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
WeakPtr<TYPE>::operator=(const WeakPtr<TYPE>& rhs)
{
    this->ptr = rhs.ptr;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
WeakPtr<TYPE>::operator=(TYPE* rhs)
{
    this->ptr = rhs;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE*
WeakPtr<TYPE>::operator->() const
{
    n_assert2(this->ptr, "NULL pointer access in WeakPtr::operator->()!");
    return this->ptr;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE&
WeakPtr<TYPE>::operator*() const
{
    n_assert2(this->ptr, "NULL pointer access in WeakPtr::operator*()!");
    return *this->ptr;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
WeakPtr<TYPE>::operator TYPE*() const
{
    n_assert2(this->ptr, "NULL pointer access in WeakPtr::operator TYPE*()!");
    return this->ptr;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool 
WeakPtr<TYPE>::isvalid() const
{
    return (0 != this->ptr);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE*
WeakPtr<TYPE>::get() const
{
    n_assert2(this->ptr, "NULL pointer access in WeakPtr::get()!");
    return this->ptr;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE*
WeakPtr<TYPE>::get_unsafe() const
{
    return this->ptr;
}
//------------------------------------------------------------------------------
