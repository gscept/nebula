//------------------------------------------------------------------------------
//  shader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/shader.h"

#if __DX11__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::Shader, 'SHDR', Direct3D11::D3D11Shader);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::Shader, 'SHDR', OpenGL4::OGL4Shader);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::Shader, 'SHDR', Vulkan::VkShader);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::Shader, 'SHDR', Direct3D9::D3D9Shader);
}
#else
#error "Shader class not implemented on this platform!"
#endif
