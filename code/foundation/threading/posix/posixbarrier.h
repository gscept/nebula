#pragma once
#ifndef POSIX_POSIXBARRIER_H
#define POSIX_POSIXBARRIER_H
//------------------------------------------------------------------------------
/**
    @class Posix::PosixBarrier
    
    Implements the 2 macros ReadWriteBarrier and MemoryBarrier.
    
    ReadWriteBarrier prevents the compiler from re-ordering memory
    accesses accross the barrier.

    MemoryBarrier prevents the CPU from reordering memory access across
    the barrier (all memory access will be finished before the barrier
    is crossed).
    
    (C) 2007 Oleg Khryptul (Haron)
*/

//------------------------------------------------------------------------------
inline void ReadWriteBarrier() {asm volatile("": : :"memory");}
inline void MemoryBarrier() {asm volatile("mfence":::"memory");}
//------------------------------------------------------------------------------
#endif
