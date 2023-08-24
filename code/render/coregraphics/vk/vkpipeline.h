#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan implementation of CoreGraphics::PipelineId

    @copyright
    (C) 2023 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"

namespace Vulkan
{

struct Pipeline
{
    VkPipeline pipeline;
    VkPipelineLayout layout;

    // Pass needed for pass related resource tables
    CoreGraphics::PassId pass;
};

enum
{
    Pipeline_Object
};

extern Ids::IdAllocator<Pipeline> pipelineAllocator;
} // namespace Vulkan
