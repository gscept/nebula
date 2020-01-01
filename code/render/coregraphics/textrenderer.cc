//------------------------------------------------------------------------------
//  textrenderer.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/textrenderer.h"

namespace CoreGraphics
{
#if __DX11__
__ImplementClass(CoreGraphics::TextRenderer, 'TXRR', Direct3D11::D3D11TextRenderer);
#elif __OGL4__
__ImplementClass(CoreGraphics::TextRenderer, 'TXRR', OpenGL4::OGL4TextRenderer);
#elif __VULKAN__
__ImplementClass(CoreGraphics::TextRenderer, 'TXRR', Vulkan::VkTextRenderer);
#elif __DX9__
__ImplementClass(CoreGraphics::TextRenderer, 'TXRR', Direct3D9::D3D9TextRenderer);
#else
#error "TextRenderer class not implemented on this platform!"
#endif

__ImplementSingleton(CoreGraphics::TextRenderer);

//------------------------------------------------------------------------------
/**
*/
TextRenderer::TextRenderer()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
TextRenderer::~TextRenderer()
{
    __DestructSingleton;
}

} // namespace CoreGraphics
