//------------------------------------------------------------------------------
//  @file vkaccelerationstructure.cc
//  @copyright (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "coregraphics/accelerationstructure.h"
#include "vkaccelerationstructure.h"
#include "vkgraphicsdevice.h"

namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
*/
BottomLevelAccelerationId
CreateBottomLevelAcceleration(const BottomLevelAccelerationCreateInfo& info)
{
    VkDevice dev = Vulkan::GetCurrentDevice();
    auto type = VkAccelerationStructureBuildTypeKHR::VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
}

} // namespace CoreGraphics


namespace Vulkan
{

} // namespace Vulkan
