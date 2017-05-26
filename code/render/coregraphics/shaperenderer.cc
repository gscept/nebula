//------------------------------------------------------------------------------
//  shaperenderer.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/shaperenderer.h"

namespace CoreGraphics
{
#if __DX11__
__ImplementClass(CoreGraphics::ShapeRenderer, 'SHPR', Direct3D11::D3D11ShapeRenderer);
#elif __DX9__
__ImplementClass(CoreGraphics::ShapeRenderer, 'SHPR', Win360::D3D9ShapeRenderer);
#elif __OGL4__
__ImplementClass(CoreGraphics::ShapeRenderer, 'SHPR', OpenGL4::OGL4ShapeRenderer);
#elif __VULKAN__
__ImplementClass(CoreGraphics::ShapeRenderer, 'SHPR', Vulkan::VkShapeRenderer);
#elif __WII__
__ImplementClass(CoreGraphics::ShapeRenderer, 'SHPR', Wii::WiiShapeRenderer);
#elif __PS3__
__ImplementClass(CoreGraphics::ShapeRenderer, 'SHPR', PS3::PS3ShapeRenderer);
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
    
