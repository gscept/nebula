#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::TextRenderer
  
    A simple text renderer for drawing text on screen.
    Only for debug purposes.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#if __VULKAN__
#include "coregraphics/vk/vktextrenderer.h"
namespace CoreGraphics
{
class TextRenderer : public Vulkan::VkTextRenderer
{
    __DeclareClass(TextRenderer);
    __DeclareSingleton(TextRenderer);
public:
    /// constructor
    TextRenderer();
    /// destructor
    virtual ~TextRenderer();
};
} // namespace CoreGraphics
#else
#error "TextRenderer class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

