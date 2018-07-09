#pragma once
//------------------------------------------------------------------------------
/**
	A semaphore is an inter-GPU queue synchronization primitive

	(C) 2018 Individual contributors, see AUTHORS file
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

struct SemaphoreCreateInfo
{
	CoreGraphics::BarrierDependency dependency;
};

/// create semaphore
SemaphoreId CreateSemaphore(const SemaphoreCreateInfo& info);
/// destroy semaphore
void DestroySemaphore(const SemaphoreId& semaphore);

} // namespace CoreGraphics

#pragma pop_macro("CreateSemaphore")