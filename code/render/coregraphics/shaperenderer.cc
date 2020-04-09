//------------------------------------------------------------------------------
//  shaperenderer.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/shaperenderer.h"

namespace CoreGraphics
{
#if __VULKAN__
__ImplementClass(CoreGraphics::ShapeRenderer, 'SHPR', Vulkan::VkShapeRenderer);
#else
#error "ShapeRenderer class not implemented on this platform!"
#endif
__ImplementSingleton(CoreGraphics::ShapeRenderer);

//------------------------------------------------------------------------------
/**
*/
ShapeRenderer::ShapeRenderer()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ShapeRenderer::~ShapeRenderer()
{
    __DestructSingleton;
}

} // namespace CoreGraphics
    
