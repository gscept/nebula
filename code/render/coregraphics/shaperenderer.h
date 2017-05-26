#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::ShapeRenderer
    
    Render shapes for debug visualizations.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __DX11__
#include "coregraphics/d3d11/d3d11shaperenderer.h"
namespace CoreGraphics
{
class ShapeRenderer : public Direct3D11::D3D11ShapeRenderer
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
#elif __DX9__
#include "coregraphics/d3d9/d3d9shaperenderer.h"
namespace CoreGraphics
{
class ShapeRenderer : public Direct3D9::D3D9ShapeRenderer
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
#elif __OGL4__
#include "coregraphics/ogl4/ogl4shaperenderer.h"
namespace CoreGraphics
{
class ShapeRenderer : public OpenGL4::OGL4ShapeRenderer
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
#elif __VULKAN__
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

