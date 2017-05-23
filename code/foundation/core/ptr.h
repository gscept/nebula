#pragma once
//------------------------------------------------------------------------------
/**
    @class Ptr

    Nebula3's smart pointer class which manages the life time of RefCounted
    objects. Can be used like a normal C++ pointer in most cases.

    NOTE: the Ptr class is not part of the Core namespace for
    convenience reasons.

    (C) 2006 RadonLabs GmbH
    (C) 2013-2014 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

// platform secific stuff for handling/suppress "unused-argument"-warnings
#if NEBULA3_DEBUG
#if (__XBOX360__ || __WIN32__)
#pragma warning( push )
// warning unused param
#pragma warning(disable: 4189)
#endif
#if __PS3__
#define NEBULA3_UNUSED_ATTR __attribute__ ((unused))
#else
#define NEBULA3_UNUSED_ATTR
#endif
#endif // NEBULA3_DEBUG

//------------------------------------------------------------------------------
template<class TYPE>
class Ptr
{
public:
    /// constructor
    Ptr();
    /// construct from C++ pointer
    Ptr(TYPE* p);
    /// construct from smart pointer
    Ptr(const Ptr<TYPE>& p);
    /// destructor
    ~Ptr();
    /// assignment operator
    void operator=(const Ptr<TYPE>& rhs);
    /// assignment operator
    void operator=(TYPE* rhs);
    /// equality operator
    bool operator==(const Ptr<TYPE>& rhs) const;
    /// inequality operator
    bool operator!=(const Ptr<TYPE>& rhs) const;
    /// shortcut equality operator
    bool operator==(const TYPE* rhs) const;
    /// shortcut inequality operator
    bool operator!=(const TYPE* rhs) const;
    /// safe -> operator
    TYPE* operator->() const;
    /// safe dereference operator
    TYPE& operator*() const;
    /// safe pointer cast operator
    operator TYPE*() const;
    /// type-safe downcast operator to other smart pointer
    template<class DERIVED> const Ptr<DERIVED>& downcast() const;
    /// type-safe upcast operator to other smart pointer
    template<class BASE> const Ptr<BASE>& upcast() const;
    /// unsafe(!) cast to anything, unless classes have no inheritance-relationship,
    /// call upcast/downcast instead, they are type-safe
    template<class OTHERTYPE> const Ptr<OTHERTYPE>& cast() const;
    /// check if pointer is valid
    bool isvalid() const;
    /// return direct pointer (asserts if null pointer)
    TYPE* get() const;
    /// return direct pointer (returns null pointer)
    TYPE* get_unsafe() const;

	/// calculate hash code for Util::HashTable (basically just the adress)
	IndexT HashCode() const;

private:
    TYPE* ptr;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Ptr<TYPE>::Ptr() :
    ptr(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Ptr<TYPE>::Ptr(TYPE* p) :
    ptr(p)
{
    if (0 != this->ptr)
    {
        this->ptr->AddRef();
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Ptr<TYPE>::Ptr(const Ptr<TYPE>& p) :
    ptr(p.ptr)
{
    if (0 != this->ptr)
    {
        this->ptr->AddRef();
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Ptr<TYPE>::~Ptr()
{
    if (0 != this->ptr)
    {
        this->ptr->Release();
        this->ptr = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Ptr<TYPE>::operator=(const Ptr<TYPE>& rhs)
{
    if (this->ptr != rhs.ptr)
    {
        if (this->ptr)
        {
            this->ptr->Release();
        }
        this->ptr = rhs.ptr;
        if (this->ptr)
        {
            this->ptr->AddRef();
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Ptr<TYPE>::operator=(TYPE* rhs)
{
    if (this->ptr != rhs)
    {
        if (this->ptr)
        {
            this->ptr->Release();
        }
        this->ptr = rhs;
        if (this->ptr)
        {
            this->ptr->AddRef();
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
Ptr<TYPE>::operator==(const Ptr<TYPE>& rhs) const
{
    return (this->ptr == rhs.ptr);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
Ptr<TYPE>::operator!=(const Ptr<TYPE>& rhs) const
{
    return (this->ptr != rhs.ptr);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
Ptr<TYPE>::operator==(const TYPE* rhs) const
{
    return (this->ptr == rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
Ptr<TYPE>::operator!=(const TYPE* rhs) const
{
    return (this->ptr != rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE*
Ptr<TYPE>::operator->() const
{
    n_assert2(this->ptr, "NULL pointer access in Ptr::operator->()!");
    return this->ptr;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE&
Ptr<TYPE>::operator*() const
{
    n_assert2(this->ptr, "NULL pointer access in Ptr::operator*()!");
    return *this->ptr;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Ptr<TYPE>::operator TYPE*() const
{
    n_assert2(this->ptr, "NULL pointer access in Ptr::operator TYPE*()!");
    return this->ptr;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
template<class DERIVED> const Ptr<DERIVED>&
Ptr<TYPE>::downcast() const
{
#if (NEBULA3_DEBUG == 1)
    // if DERIVED is not a derived class of TYPE, compiler complains here
    // compile-time inheritance-test
    NEBULA3_UNUSED_ATTR const DERIVED *derived = static_cast<const DERIVED*>(this->ptr);
#endif
    return *reinterpret_cast<const Ptr<DERIVED>*>(this);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
template<class BASE> const Ptr<BASE>&
Ptr<TYPE>::upcast() const
{
#if (NEBULA3_DEBUG == 1)
    // if BASE is not a base-class of TYPE, compiler complains here
    // compile-time inheritance-test
    NEBULA3_UNUSED_ATTR const BASE *base = this->ptr;
#endif
    return *reinterpret_cast<const Ptr<BASE>*>(this);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
template<class OTHERTYPE> const Ptr<OTHERTYPE>&
Ptr<TYPE>::cast() const
{
    // note: this is an unsafe cast
    return *reinterpret_cast<const Ptr<OTHERTYPE>*>(this);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
Ptr<TYPE>::isvalid() const
{
    return (0 != this->ptr);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE*
Ptr<TYPE>::get() const
{
    n_assert2(this->ptr, "NULL pointer access in Ptr::get()!");
    return this->ptr;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE*
Ptr<TYPE>::get_unsafe() const
{
    return this->ptr;
}


//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
IndexT
Ptr<TYPE>::HashCode() const
{
	return (IndexT)this->ptr;
}

//------------------------------------------------------------------------------

#if (__XBOX360__ || __WIN32__) && NEBULA3_DEBUG
#pragma warning( pop )
#endif
#ifdef NEBULA3_UNUSED_ATTR
#undef NEBULA3_UNUSED_ATTR
#endif
