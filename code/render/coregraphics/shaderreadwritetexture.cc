//------------------------------------------------------------------------------
//  shaderreadwritetexture.cc
//  (C) 2013-2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/shaderreadwritetexture.h"

#if __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderReadWriteTexture, 'SRWT', OpenGL4::OGL4ShaderImage);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderReadWriteTexture, 'SRWT', Vulkan::VkShaderImage);
}
#else
#error "ShaderBuffer class not implemented on this platform!"
#endif

