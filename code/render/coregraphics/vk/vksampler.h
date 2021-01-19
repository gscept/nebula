#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan sampler implemention

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/sampler.h"
#include "ids/idallocator.h"
#include "vkloader.h"

namespace Vulkan
{

typedef Ids::IdAllocator<
    VkDevice,
    VkSampler
> VkSamplerAllocator;
extern VkSamplerAllocator samplerAllocator;

/// get vulkan sampler
const VkSampler& SamplerGetVk(const CoreGraphics::SamplerId& id);

} // namespace Vulkan

namespace CoreGraphics
{

/// use vulkan sampler create info to create Nebula sampler create info
SamplerCreateInfo ToNebulaSamplerCreateInfo(const VkSamplerCreateInfo& info);

}
