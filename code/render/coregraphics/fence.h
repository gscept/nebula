#pragma once
//------------------------------------------------------------------------------
/**
	A fence is a CPU-GPU sync object, used to let the CPU wait for the GPU to finish some work.
				
	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
namespace CoreGraphics
{

ID_24_8_TYPE(FenceId);

struct FenceCreateInfo
{
	bool createSignaled : 1;
};

/// create a new fence
FenceId CreateFence(const FenceCreateInfo& info);
/// destroy a fence
void DestroyFence(const FenceId id);

/// insert fence into queue
void FenceInsert(const FenceId id, const CoreGraphicsQueueType queue);
/// peek fence status
bool FencePeek(const FenceId id);
/// reset fence status
void FenceReset(const FenceId id);
/// wait for fence
bool FenceWait(const FenceId id, const uint64 time);

} // namespace CoreGraphics
