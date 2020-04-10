//------------------------------------------------------------------------------
//  streamshaderloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/shaderpool.h"

#if __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderPool, 'SSDL', Vulkan::VkShaderPool);
}
#else
#error "StreamShaderLoader class not implemented on this platform!"
#endif
