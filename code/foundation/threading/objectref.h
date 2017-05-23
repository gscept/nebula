#pragma once
//------------------------------------------------------------------------------
/**
    @class Threading::ObjectRef
    
    A thread-safe reference to a shared object. Object refs are used
    with the messaging system to reference opaque objects created 
    and manipulated in other threads. ObjectRef objects must be
    created on the heap (thus they are ref-counted) because the 
    "client-side" owner-object may be discarded before the target
    object in the other thread is destroyed.
    
    (C) 2010 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"

//------------------------------------------------------------------------------
namespace Threading
{
class ObjectRef : public Core::RefCounted
{
    __DeclareClass(ObjectRef);
public:
    /// constructor
    ObjectRef();
    /// destructor
    virtual ~ObjectRef();
    
    /// return true if the ObjectRef points to a valid object
    bool IsValid() const;
    /// validate the ref with a pointer to a target object (must be RefCounted)
    template<class TYPE> void Validate(TYPE* obj);
    /// invalidate the ref
    void Invalidate();
    /// get pointer to object
    template<class TYPE> TYPE* Ref() const;

private:
    Core::RefCounted* volatile ref;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
ObjectRef::IsValid() const
{
    return (0 != this->ref);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> inline void
ObjectRef::Validate(TYPE* obj)
{
    n_assert(0 != obj);
    n_assert(obj->IsA(TYPE::RTTI));
    n_assert(0 == this->ref);
    obj->AddRef();
    Interlocked::ExchangePointer((void* volatile*)&this->ref, obj);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ObjectRef::Invalidate()
{
    n_assert(0 != this->ref);
    this->ref->Release();
    Interlocked::ExchangePointer((void* volatile*)&this->ref, 0);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> inline TYPE*
ObjectRef::Ref() const
{
    n_assert(0 != this->ref);
    n_assert(this->ref->IsA(TYPE::RTTI));
    return (TYPE*) this->ref;
}

} // namespace Threading
//------------------------------------------------------------------------------
    