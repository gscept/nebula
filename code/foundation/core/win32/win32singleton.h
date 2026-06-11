#pragma once
//------------------------------------------------------------------------------
/**
    @file core/win32/win32singleton.h

    Provides helper macros to implement singleton objects:
    
    - __DeclareSingleton      put this into class declaration
    - __ImplementSingleton    put this into the implemention file
    - __ConstructSingleton    put this into the constructor
    - __DestructSingleton     put this into the destructor

    Get a pointer to a singleton object using the static Instance() method:

    Core::Server* coreServer = Core::Server::Instance();
    
    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Core
{

void* SingletonGet(const char* key, bool threadLocal);
void SingletonSet(const char* key, bool threadLocal, void* ptr);

// this is a wrapper to deal with windows dll export/import of singletons
template<class TYPE>
class SingletonProxy
{
public:
    constexpr SingletonProxy(const char* inKey = nullptr, bool inThreadLocal = false)
        : key(inKey)
        , threadLocal(inThreadLocal)
    {
    }

    TYPE* Get() const
    {
        return reinterpret_cast<TYPE*>(Core::SingletonGet(this->key, this->threadLocal));
    }

    void Set(TYPE* ptr)
    {
        Core::SingletonSet(this->key, this->threadLocal, ptr);
    }

    SingletonProxy& operator=(TYPE* ptr)
    {
        this->Set(ptr);
        return *this;
    }

    operator TYPE*() const
    {
        return this->Get();
    }

    TYPE* operator->() const
    {
        return this->Get();
    }

private:
    const char* key;
    bool threadLocal;
};

} // namespace Core

//------------------------------------------------------------------------------
#define __DeclareSingleton(type) \
public: \
    static Core::SingletonProxy<type> Singleton; \
    static type * Instance() { type* ptr = Singleton; n_assert(nullptr != ptr); return ptr; }; \
    static bool HasInstance() { return nullptr != Singleton; }; \
private:

#define __DeclareInterfaceSingleton(type) \
public: \
    static Core::SingletonProxy<type> Singleton; \
    static type * Instance() { type* ptr = Singleton; n_assert(nullptr != ptr); return ptr; }; \
    static bool HasInstance() { return nullptr != Singleton; }; \
private:

#define __ImplementSingleton(type) \
    Core::SingletonProxy<type> type::Singleton(__FILE__ ":" #type, true);

#define __ImplementInterfaceSingleton(type) \
    Core::SingletonProxy<type> type::Singleton(__FILE__ ":" #type, false);

#define __ConstructSingleton \
    n_assert(nullptr == Singleton); Singleton = this;

#define __ConstructInterfaceSingleton \
    n_assert(nullptr == Singleton); Singleton = this;

#define __DestructSingleton \
    n_assert(Singleton); Singleton = nullptr;

#define __DestructInterfaceSingleton \
    n_assert(Singleton); Singleton = nullptr;
//------------------------------------------------------------------------------
