#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::StreamTextureLoader
  
    Resource loader for loading texture data from a Nebula3 stream. Supports
    synchronous and asynchronous loading.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#if __DX11__
#include "coregraphics/d3d11/d3d11streamtextureloader.h"
namespace CoreGraphics
{
class TextureLoader : public Direct3D11::D3D11StreamTextureLoader
{
	__DeclareClass(TextureLoader);
};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4streamtextureloader.h"
namespace CoreGraphics
{
class TextureLoader : public OpenGL4::OGL4StreamTextureLoader
{
	__DeclareClass(TextureLoader);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vktextureloader.h"
namespace CoreGraphics
{
class TextureLoader : public Vulkan::VkTextureLoader
{
	__DeclareClass(TextureLoader);
};
}
#elif __DX9__
#include "coregraphics/d3d9/d3d9streamtextureloader.h"
namespace CoreGraphics
{
class TextureLoader : public Direct3D9::D3D9StreamTextureLoader
{
    __DeclareClass(TextureLoader);
};
}
#else
#error "StreamTextureLoader class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------


