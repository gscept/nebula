#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::TextRenderer
  
    A simple text renderer for drawing text on screen.
    Only for debug purposes.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#if __DX11__
#include "coregraphics/d3d11/d3d11textrenderer.h"
namespace CoreGraphics
{
class TextRenderer : public Direct3D11::D3D11TextRenderer
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
#elif __OGL4__
#include "coregraphics/ogl4/ogl4textrenderer.h"
namespace CoreGraphics
{
class TextRenderer : public OpenGL4::OGL4TextRenderer
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
#elif __VULKAN__
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
#elif __DX9__
#include "coregraphics/d3d9/d3d9textrenderer.h"
namespace CoreGraphics
{
class TextRenderer : public Direct3D9::D3D9TextRenderer
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

