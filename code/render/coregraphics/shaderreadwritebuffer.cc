//------------------------------------------------------------------------------
//  shaderreadwritebuffer.cc
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/shaderreadwritebuffer.h"

#if __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderReadWriteBuffer, 'SHBU', OpenGL4::OGL4ShaderStorageBuffer);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderReadWriteBuffer, 'SHBU', Vulkan::VkShaderStorageBuffer);
}
#else
#error "ShaderBuffer class not implemented on this platform!"
#endif

