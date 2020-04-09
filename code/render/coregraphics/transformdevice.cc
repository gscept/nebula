//------------------------------------------------------------------------------
//  transformdevice.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/transformdevice.h"

namespace CoreGraphics
{
#if __VULKAN__
__ImplementClass(CoreGraphics::TransformDevice, 'TRDV', Vulkan::VkTransformDevice);
#else
#error "TransformDevice class not implemented on this platform!"
#endif
__ImplementSingleton(CoreGraphics::TransformDevice);

//------------------------------------------------------------------------------
/**
*/
TransformDevice::TransformDevice()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
TransformDevice::~TransformDevice()
{
    __DestructSingleton;
}

} // namespace CoreGraphics
