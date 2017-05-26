//------------------------------------------------------------------------------
//  shaderinstance.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/shaderstate.h"

#if __DX11__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderState, 'SINS', Direct3D11::D3D11ShaderInstance);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderState, 'SINS', OpenGL4::OGL4ShaderInstance);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderState, 'SINS', Vulkan::VkShaderState);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderState, 'SINS', Direct3D9::D3D9ShaderInstance);
}
#else
#error "ShaderInstance class not implemented on this platform!"
#endif
