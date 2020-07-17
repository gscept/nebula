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
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
#define __DeclareSingleton(type) \
public: \
    thread_local static type * Singleton; \
    static type * Instance() { n_assert(nullptr != Singleton); return Singleton; }; \
    static bool HasInstance() { return nullptr != Singleton; }; \
private:

#define __DeclareInterfaceSingleton(type) \
public: \
    static type * Singleton; \
    static type * Instance() { n_assert(nullptr != Singleton); return Singleton; }; \
    static bool HasInstance() { return nullptr != Singleton; }; \
private:

#define __ImplementSingleton(type) \
    thread_local type * type::Singleton = nullptr;

#define __ImplementInterfaceSingleton(type) \
    type * type::Singleton = nullptr;

#define __ConstructSingleton \
    n_assert(nullptr == Singleton); Singleton = this;

#define __ConstructInterfaceSingleton \
    n_assert(nullptr == Singleton); Singleton = this;

#define __DestructSingleton \
    n_assert(Singleton); Singleton = nullptr;

#define __DestructInterfaceSingleton \
    n_assert(Singleton); Singleton = nullptr;
//------------------------------------------------------------------------------
