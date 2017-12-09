#pragma once
//------------------------------------------------------------------------------
/**
    @class Core::RefCounted
    
    The common base class of Nebula3. Implement a strong refcounted
    mechanism and runtime type information. Nebula3 checks at
    application shutdown for propert cleanup of all 
    RefCounted objects. Refcounting leaks will generate a log
    on the debug output.

    FIXME: The RefCounted class uses Interlocked functions and
    a CriticalSection to guarantee thread-safe refcounting and
    destruction, but only some classes need this (mostly messages
    which are passed between threads). If it is guaranteed that an
    object is only manipulated from the same thread then this 
    thread-synchronization adds unnecessary overhead.

    (C) 2006 RadonLabs GmbH
    (C) 2013-2014 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "core/rtti.h"
#include "core/factory.h"
#include "threading/interlocked.h"
#include "core/refcountedlist.h"

#if NEBULA_DEBUG
#include "threading/criticalsection.h"
#include "util/dictionary.h"
#endif

//------------------------------------------------------------------------------
namespace Core
{
class RefCounted
{
    __DeclareClass(RefCounted);
public:
    /// constructor
    RefCounted();
    /// get the current refcount
    int GetRefCount() const;
    /// increment refcount by one
    void AddRef();
    /// decrement refcount and destroy object if refcount is zero
    void Release();
    /// return true if this object is instance of given class
    bool IsInstanceOf(const Rtti& rtti) const;
    /// return true if this object is instance of given class by string
    bool IsInstanceOf(const Util::String& className) const;
    /// return true if this object is instance of given class by fourcc
    bool IsInstanceOf(const Util::FourCC& classFourCC) const;
    /// return true if this object is instance of given class, or a derived class
    bool IsA(const Rtti& rtti) const;
    /// return true if this object is instance of given class, or a derived class, by string
    bool IsA(const Util::String& rttiName) const;
    /// return true if this object is instance of given class, or a derived class, by fourcc
    bool IsA(const Util::FourCC& rttiFourCC) const;
    /// get the class name
    const Util::String& GetClassName() const;
    /// get the class FourCC code
    Util::FourCC GetClassFourCC() const;
    /// dump refcounting leaks, call at end of application (NEBULA_DEBUG builds only!)
    static void DumpRefCountingLeaks();

    #if NEBULA_DEBUG
    struct Stats
    {
        Util::String className;
        Util::FourCC classFourCC;
        SizeT numObjects;
        SizeT overallRefCount;
        SizeT instanceSize;
    };
    /// get overall statistics
    static Util::Dictionary<Util::String,Stats> GetOverallStats();
    #endif

protected:
    /// destructor (called when refcount reaches zero)
    virtual ~RefCounted();

private:
    volatile int refCount;

    #if NEBULA_DEBUG
protected:
        static ThreadLocal bool isInCreate;
        static Threading::CriticalSection criticalSection;
private:        
        static RefCountedList list;
        RefCountedList::Iterator listIterator;
        bool destroyed;

public:
		/// register debug name
		void SetDebugName(const Util::String& name);
    #endif
};

//------------------------------------------------------------------------------
/**
*/
inline
RefCounted::RefCounted() : 
    refCount(0)
{
    #if NEBULA_DEBUG
    n_assert2(this->isInCreate, "RefCounted objects must be created with Create()!");
    this->listIterator = list.AddBack(this);
    this->destroyed = false;
    #endif
}

//------------------------------------------------------------------------------
/**
    Increment the refcount of the object.
*/
inline void
RefCounted::AddRef()
{
    Threading::Interlocked::Increment(this->refCount);
}

//------------------------------------------------------------------------------
/**
    Decrement the refcount and destroy object if refcount is zero.
*/
inline void
RefCounted::Release()
{
    if (0 == Threading::Interlocked::Decrement(this->refCount))
    {
        n_delete(this);
    }
}

//------------------------------------------------------------------------------
/**
    Return the current refcount of the object.
*/
inline int
RefCounted::GetRefCount() const
{
    return this->refCount;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
RefCounted::IsInstanceOf(const Rtti& other) const
{
    return this->GetRtti() == &other;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
RefCounted::IsInstanceOf(const Util::String& other) const
{
    return this->GetRtti()->GetName() == other;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
RefCounted::IsInstanceOf(const Util::FourCC& other) const
{
    return this->GetRtti()->GetFourCC() == other;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
RefCounted::IsA(const Rtti& other) const
{
    return this->GetRtti()->IsDerivedFrom(other);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
RefCounted::IsA(const Util::String& other) const
{
    return this->GetRtti()->IsDerivedFrom(other);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
RefCounted::IsA(const Util::FourCC& other) const
{
    return this->GetRtti()->IsDerivedFrom(other);
}

//------------------------------------------------------------------------------
/**
    Get the class name of the object.
*/
inline const Util::String&
RefCounted::GetClassName() const
{
    return this->GetRtti()->GetName();
}

//------------------------------------------------------------------------------
/**
    Get the class FourCC of the object.
*/
inline Util::FourCC
RefCounted::GetClassFourCC() const
{
    return this->GetRtti()->GetFourCC();
}

#if NEBULA_DEBUG
//------------------------------------------------------------------------------
/**
*/
inline void
RefCounted::SetDebugName(const Util::String& name)
{
	list.SetDebugName(this, name);
}
#endif

} // namespace Core
//------------------------------------------------------------------------------



    
    