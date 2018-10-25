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
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
#define __DeclareSingleton(type) \
public: \
    ThreadLocal static type * Singleton; \
    static type * Instance() { n_assert(0 != Singleton); return Singleton; }; \
    static bool HasInstance() { return 0 != Singleton; }; \
private:

#define __DeclareInterfaceSingleton(type) \
public: \
    static type * Singleton; \
    static type * Instance() { n_assert(0 != Singleton); return Singleton; }; \
    static bool HasInstance() { return 0 != Singleton; }; \
private:

#define __ImplementSingleton(type) \
    ThreadLocal type * type::Singleton = 0;

#define __ImplementInterfaceSingleton(type) \
    type * type::Singleton = 0;

#define __ConstructSingleton \
    n_assert(0 == Singleton); Singleton = this;

#define __ConstructInterfaceSingleton \
    n_assert(0 == Singleton); Singleton = this;

#define __DestructSingleton \
    n_assert(Singleton); Singleton = 0;

#define __DestructInterfaceSingleton \
    n_assert(Singleton); Singleton = 0;
//------------------------------------------------------------------------------
