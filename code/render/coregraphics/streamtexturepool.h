#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::StreamTextureLoader
  
    Resource loader for loading texture data from a Nebula stream. Supports
    synchronous and asynchronous loading.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/    
#if __DX11__
#include "coregraphics/d3d11/d3d11streamtextureloader.h"
namespace CoreGraphics
{
class StreamTexturePool : public Direct3D11::D3D11StreamTextureLoader
{
	__DeclareClass(StreamTexturePool);
};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4streamtextureloader.h"
namespace CoreGraphics
{
class StreamTexturePool : public OpenGL4::OGL4StreamTextureLoader
{
	__DeclareClass(StreamTexturePool);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkstreamtexturepool.h"
namespace CoreGraphics
{
class StreamTexturePool : public Vulkan::VkStreamTexturePool
{
	__DeclareClass(StreamTexturePool);
};
}
#elif __DX9__
#include "coregraphics/d3d9/d3d9streamtextureloader.h"
namespace CoreGraphics
{
class StreamTexturePool : public Direct3D9::D3D9StreamTextureLoader
{
    __DeclareClass(StreamTexturePool);
};
}
#else
#error "StreamTextureLoader class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------


