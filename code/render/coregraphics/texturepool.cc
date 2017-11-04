//------------------------------------------------------------------------------
//  texturepool.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/texturepool.h"

#if __DX11__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::TexturePool, 'STXL', Direct3D11::D3D11StreamTextureLoader);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::TexturePool, 'STXL', OpenGL4::OGL4StreamTextureLoader);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::TexturePool, 'STXL', Vulkan::VkTexturePool);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::TexturePool, 'STXL', Direct3D9::D3D9StreamTextureLoader);
}
#else
#error "StreamTextureLoader class not implemented on this platform!"
#endif
