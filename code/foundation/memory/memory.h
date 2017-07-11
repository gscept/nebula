#pragma once
//------------------------------------------------------------------------------
/**
    @class Memory::Memory
  
    Implements a private heap.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/config.h"

#if (__WIN32__ || __XBOX360__)
#include "memory/win360/win360memory.h"
#elif __WII__
#include "memory/wii/wiimemory.h"
#elif __PS3__
#include "memory/ps3/ps3memory.h"
#elif ( __OSX__ || __APPLE__ || __linux__ )
#include "memory/posix/posixmemory.h"
#else
#error "UNKNOWN PLATFORM"
#endif
