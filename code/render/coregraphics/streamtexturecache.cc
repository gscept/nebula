//------------------------------------------------------------------------------
//  streamtexturecache.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/streamtexturecache.h"

#if __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::StreamTextureCache, 'STXP', Vulkan::VkStreamTextureCache);
}
#else
#error "StreamTextureCache class not implemented on this platform!"
#endif
