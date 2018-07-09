#pragma once
//------------------------------------------------------------------------------
/**
	A fence is a CPU-GPU sync object, used to let the CPU wait for the GPU to finish
				
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
void DestroyFence(const FenceId& id);

} // namespace CoreGraphics
