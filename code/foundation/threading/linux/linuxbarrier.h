#pragma once
//------------------------------------------------------------------------------
/**
    @class Linux::LinuxBarrier
    
    MemoryBarrier prevents the CPU from reordering memory access across
    the barrier (all memory access will be finished before the barrier
    is crossed).
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
#define ReadWriteBarrier _ReadWriterBarrier
// NOTE: MemoryBarrier is defined by windows.h (Win32) or ppcintrinsics.h (Xbox360)
//------------------------------------------------------------------------------
