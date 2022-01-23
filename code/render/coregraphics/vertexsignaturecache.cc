//------------------------------------------------------------------------------
//  vertexsignaturecache.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/vertexsignaturecache.h"

#if __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::VertexSignatureCache, 'STXL', Vulkan::VkVertexSignatureCache);
}
#else
#error "VertexSignatureCache class not implemented on this platform!"
#endif
