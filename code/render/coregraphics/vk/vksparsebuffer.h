#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan sparse buffer implementation

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/sparsebuffer.h"
#include "ids/idallocator.h"

namespace Vulkan
{

/// get vulkan buffer
VkBuffer SparseBufferGetVk(const CoreGraphics::SparseBufferId id);

} // namespace Vulkan
