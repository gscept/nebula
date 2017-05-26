//------------------------------------------------------------------------------
//  texture.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/texture.h"

#if __DX11__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::Texture, 'TEXR', Direct3D11::D3D11Texture);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::Texture, 'TEXR', OpenGL4::OGL4Texture);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::Texture, 'TEXR', Vulkan::VkTexture);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::Texture, 'TEXR', Direct3D9::D3D9Texture);
}
#else
#error "Texture class not implemented on this platform!"
#endif
