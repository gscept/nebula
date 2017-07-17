//------------------------------------------------------------------------------
//  streamshaderloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/shaderloader.h"

#if __DX11__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderLoader, 'SSDL', Direct3D11::D3D11StreamShaderLoader);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderLoader, 'SSDL', OpenGL4::OGL4StreamShaderLoader);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderLoader, 'SSDL', Vulkan::VkShaderLoader);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderLoader, 'SSDL', Direct3D9::D3D9StreamShaderLoader);
}
#else
#error "StreamShaderLoader class not implemented on this platform!"
#endif
