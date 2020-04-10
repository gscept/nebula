//------------------------------------------------------------------------------
//  streamtexturepool.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/streamtexturepool.h"

#if __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::StreamTexturePool, 'STXP', Vulkan::VkStreamTexturePool);
}
#else
#error "StreamTextureLoader class not implemented on this platform!"
#endif
