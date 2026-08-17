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
    VkDevice dev;

    // Pass needed for pass related resource tables
    CoreGraphics::PassId pass;
    CoreGraphics::RenderPassId renderPass;
};

enum
{
    Pipeline_Object,
    Pipeline_CreateInfo
};

typedef Ids::IdAllocator<
    Pipeline,
    CoreGraphics::PipelineCreateInfo
> PipelineAllocator;
extern PipelineAllocator pipelineAllocator;

/// Get device used to create pipeline
VkDevice PipelineGetVkDevice(const CoreGraphics::PipelineId id);
/// Get vk pipeline
VkPipeline PipelineGetVkPipeline(const CoreGraphics::PipelineId id);

} // namespace Vulkan
