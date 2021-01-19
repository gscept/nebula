#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::ShapeRenderer
    
    Render shapes for debug visualizations.
    
    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#if __VULKAN__
#include "coregraphics/vk/vkshaperenderer.h"
namespace CoreGraphics
{
class ShapeRenderer : public Vulkan::VkShapeRenderer
{
    __DeclareClass(ShapeRenderer);
    __DeclareSingleton(ShapeRenderer);
public:
    /// constructor
    ShapeRenderer();
    /// destructor
    virtual ~ShapeRenderer();
};
} // namespace CoreGraphics
#else
#error "ShapeRenderer class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

