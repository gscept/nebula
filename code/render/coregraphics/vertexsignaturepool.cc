//------------------------------------------------------------------------------
//  vertexsignaturepool.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/vertexsignaturepool.h"

#if __DX11__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::VertexSignaturePool, 'STXL', Direct3D11::D3D11VertexSignaturePool);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::VertexSignaturePool, 'STXL', OpenGL4::OGL4SVertexSignaturePool);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::VertexSignaturePool, 'STXL', Vulkan::VkVertexSignaturePool);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::VertexSignaturePool, 'STXL', Direct3D9::D3D9VertexSignaturePool);
}
#else
#error "VertexSignaturePool class not implemented on this platform!"
#endif
