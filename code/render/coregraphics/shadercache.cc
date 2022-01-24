//------------------------------------------------------------------------------
//  streamshaderloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/shadercache.h"

#if __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderCache, 'SSDL', Vulkan::VkShaderCache);
}
#else
#error "StreamShaderLoader class not implemented on this platform!"
#endif
