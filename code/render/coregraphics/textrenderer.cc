//------------------------------------------------------------------------------
//  textrenderer.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/textrenderer.h"

namespace CoreGraphics
{
#if __VULKAN__
__ImplementClass(CoreGraphics::TextRenderer, 'TXRR', Vulkan::VkTextRenderer);
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
