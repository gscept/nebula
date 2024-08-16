#pragma once
//------------------------------------------------------------------------------
/**
    @file coregraphics/semaphore.h
    
    A semaphore is an inter-GPU queue synchronization primitive

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "barrier.h"

#ifdef CreateSemaphore
#pragma push_macro("CreateSemaphore")
#undef CreateSemaphore
#endif

namespace CoreGraphics
{

ID_24_8_TYPE(SemaphoreId);

enum SemaphoreType
{
    Binary,         // binary semaphore means it can only be signaled or unsignaled
    Timeline        // a timeline semaphore
};

struct SemaphoreCreateInfo
{
#if NEBULA_GRAPHICS_DEBUG
    const char* name = nullptr;
#endif
    SemaphoreType type;
};

/// create semaphore
SemaphoreId CreateSemaphore(const SemaphoreCreateInfo& info);
/// destroy semaphore
void DestroySemaphore(const SemaphoreId& semaphore);

/// get semaphore index
uint64 SemaphoreGetValue(const SemaphoreId& semaphore);
/// signal semaphore
void SemaphoreSignal(const SemaphoreId& semaphore);
/// reset semaphore
void SemaphoreReset(const SemaphoreId& semaphore);

} // namespace CoreGraphics

#pragma pop_macro("CreateSemaphore")
