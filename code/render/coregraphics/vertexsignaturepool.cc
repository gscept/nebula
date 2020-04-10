//------------------------------------------------------------------------------
//  vertexsignaturepool.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/vertexsignaturepool.h"

#if __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::VertexSignaturePool, 'STXL', Vulkan::VkVertexSignaturePool);
}
#else
#error "VertexSignaturePool class not implemented on this platform!"
#endif
