#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::Texture
    
    Front-end class for texture objects.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __DX11__
#include "coregraphics/d3d11/d3d11texture.h"
namespace CoreGraphics
{
class Texture : public Direct3D11::D3D11Texture
{
	__DeclareClass(Texture);
};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4texture.h"
namespace CoreGraphics
{
class Texture : public OpenGL4::OGL4Texture
{
	__DeclareClass(Texture);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vktexture.h"
namespace CoreGraphics
{
class Texture : public Vulkan::VkTexture
{
	__DeclareClass(Texture);
};
}
#elif __DX9__
#include "coregraphics/d3d9/d3d9texture.h"
namespace CoreGraphics
{
class Texture : public Direct3D9::D3D9Texture
{
    __DeclareClass(Texture);
};
}
#else
#error "Texture class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

