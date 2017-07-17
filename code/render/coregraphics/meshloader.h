#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::StreamMeshLoader
    
    Resource loader to setup a Mesh object from a stream.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __DX11__
#include "coregraphics/d3d11/d3d11streammeshloader.h"
namespace CoreGraphics
{
class MeshLoader : public Direct3D11::D3D11StreamMeshLoader
{
	__DeclareClass(MeshLoader);
};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4streammeshloader.h"
namespace CoreGraphics
{
class MeshLoader : public OpenGL4::OGL4StreamMeshLoader
{
	__DeclareClass(MeshLoader);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkmeshloader.h"
namespace CoreGraphics
{
class MeshLoader : public Vulkan::VkMeshLoader
{
	__DeclareClass(MeshLoader);
};
}
#elif __DX9__
#include "coregraphics/win360/d3d9streammeshloader.h"
namespace CoreGraphics
{
class MeshLoader : public Direct3D9::D3D9StreamMeshLoader
{
    __DeclareClass(MeshLoader);
};
}
#else
#error "StreamMeshLoader class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

    