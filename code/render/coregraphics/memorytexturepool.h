#pragma once
//------------------------------------------------------------------------------
/**
    Platform-wrapper for memory texture loader
    
    (C) 2012 Johannes Hirche
	(C) 2012-2020 Individual contributors, see AUTHORS file
*/
#if __DX11__
#include "coregraphics/d3d11/d3d11memorytextureloader.h"
namespace CoreGraphics
{
class MemoryTexturePool : public Direct3D11::D3D11MemoryTextureLoader
{
	__DeclareClass(MemoryTexturePool);
};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4memorytextureloader.h"
namespace CoreGraphics
{
class MemoryTexturePool : public OpenGL4::OGL4MemoryTextureLoader
{
	__DeclareClass(MemoryTexturePool);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkmemorytexturepool.h"
namespace CoreGraphics
{
class MemoryTexturePool : public Vulkan::VkMemoryTexturePool
{
	__DeclareClass(MemoryTexturePool);
};
}
#elif __DX9__
#include "coregraphics/d3d9/d3d9memorytextureloader.h"
namespace CoreGraphics
{
class MemoryTexturePool : public Direct3D9::D3D9MemoryTextureLoader
{
	__DeclareClass(MemoryTexturePool);
};
}
#else
#error "memorytextureloader is not implemented on this configuration"
#endif