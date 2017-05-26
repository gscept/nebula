//------------------------------------------------------------------------------
//  shadervariation.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/shadervariation.h"

#if __DX11__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderVariation, 'SHVR', Direct3D11::D3D11ShaderVariation);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderVariation, 'SHVR', OpenGL4::OGL4ShaderProgram);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderVariation, 'SHVR', Vulkan::VkShaderProgram);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderVariation, 'SHVR', Direct3D9::D3D9ShaderVariation);
}
#else
#error "ShaderVariation class not implemented on this platform!"
#endif
