//------------------------------------------------------------------------------
//  streamtexturepool.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/streamtexturepool.h"

#if __DX11__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::StreamTexturePool, 'STXP', Direct3D11::D3D11StreamTextureLoader);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::StreamTexturePool, 'STXP', OpenGL4::OGL4StreamTextureLoader);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::StreamTexturePool, 'STXP', Vulkan::VkStreamTexturePool);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::StreamTexturePool, 'STXP', Direct3D9::D3D9StreamTextureLoader);
}
#else
#error "StreamTextureLoader class not implemented on this platform!"
#endif
