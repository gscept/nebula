#pragma once
//------------------------------------------------------------------------------
/**
	@class CoreGraphics::VertexSignaturePool

	Resource pool for vertex layout signatures

	(C) 2007 Radon Labs GmbH
	(C) 2013-2018 Individual contributors, see AUTHORS file
*/
#if __DX11__
#include "coregraphics/d3d11/d3d11vertexsignaturepool.h"
namespace CoreGraphics
{
class VertexSignaturePool : public Direct3D11::D3D11VertexSignaturePool
{
	__DeclareClass(VertexSignaturePool);
};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4vertexsignaturepool.h"
namespace CoreGraphics
{
class VertexSignaturePool : public OpenGL4::OGL4VertexSignaturePool
{
	__DeclareClass(VertexSignaturePool);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkvertexsignaturepool.h"
namespace CoreGraphics
{
class VertexSignaturePool : public Vulkan::VkVertexSignaturePool
{
	__DeclareClass(VertexSignaturePool);
};
}
#elif __DX9__
#include "coregraphics/d3d9/d3d9vertexsignaturepool.h"
namespace CoreGraphics
{
class VertexSignaturePool : public Direct3D9::D3D9VertexSignaturePool
{
	__DeclareClass(VertexSignaturePool);
};
}
#else
#error "VertexSignaturePool class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------


