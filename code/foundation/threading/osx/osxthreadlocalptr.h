#pragma once
//------------------------------------------------------------------------------
/**
    @class OSX::OSXThreadLocalPtr

    GCC doesn't implement the __thread modifier on OSX. Instead we
    use pthread keys to emulate the behaviour.
 
    @todo: Performance? Would it actually be better to allocate one
    pointer lookup-table and associate this with a single
    thread-local key as like on the Wii? At the moment every Ptr
    has its own key.
 
    @todo: If this object will be used for anything else then singleton,
    it might make sense to change the interface to look like a normal
    C-pointer.
    
    (C) 2010 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace OSX
{
template<typename TYPE>
class OSXThreadLocalPtr
{
public:
    /// default constructor
    OSXThreadLocalPtr();
    /// destructor
    ~OSXThreadLocalPtr();
    /// set content
    void set(TYPE* p);
    /// get content
    TYPE* get() const;
    /// test if content is valid
    bool isvalid() const;
private:
    pthread_key_t key;
};
    
//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
OSXThreadLocalPtr<TYPE>::OSXThreadLocalPtr() :
    key(0)
{
    int res = pthread_key_create(&this->key, NULL);
    n_assert(0 == res);
    pthread_setspecific(this->key, 0);
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
OSXThreadLocalPtr<TYPE>::~OSXThreadLocalPtr()
{
    int res = pthread_key_delete(this->key);
    n_assert(0 == res);
    this->key = 0;
}
    
//------------------------------------------------------------------------------
/**
*/
template<typename TYPE> void
OSXThreadLocalPtr<TYPE>::set(TYPE* p)
{
    pthread_setspecific(this->key, p);
}
    
//------------------------------------------------------------------------------
/**
*/
template<typename TYPE> TYPE*
OSXThreadLocalPtr<TYPE>::get() const
{
    return (TYPE*) pthread_getspecific(this->key);
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE> bool
OSXThreadLocalPtr<TYPE>::isvalid() const
{
    return (0 != pthread_getspecific(this->key));
}
            
} // namespace OSX
//------------------------------------------------------------------------------
