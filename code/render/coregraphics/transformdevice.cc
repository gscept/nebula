//------------------------------------------------------------------------------
//  transformdevice.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/transformdevice.h"

namespace CoreGraphics
{
#if __DX11__
__ImplementClass(CoreGraphics::TransformDevice, 'TRDV', Direct3D11::D3D11TransformDevice);
#elif __OGL4__
__ImplementClass(CoreGraphics::TransformDevice, 'TRDV', OpenGL4::OGL4TransformDevice);
#elif __VULKAN__
__ImplementClass(CoreGraphics::TransformDevice, 'TRDV', Vulkan::VkTransformDevice);
#elif __DX9__
__ImplementClass(CoreGraphics::TransformDevice, 'TRDV', Direct3D9::D3D9TransformDevice);
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
