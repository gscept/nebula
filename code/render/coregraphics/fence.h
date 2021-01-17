#pragma once
//------------------------------------------------------------------------------
/**
    A fence is a CPU-GPU sync object, used to let the CPU wait for the GPU to finish some work.
                
    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "coregraphics/config.h"

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

/// peek fence status
bool FencePeek(const FenceId id);
/// reset fence status
bool FenceReset(const FenceId id);
/// wait for fence
bool FenceWait(const FenceId id, const uint64 time);
/// wait for fence and reset
bool FenceWaitAndReset(const FenceId id, const uint64 time);

} // namespace CoreGraphics
