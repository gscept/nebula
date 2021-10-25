#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::VertexSignatureCache

    Resource pool for vertex layout signatures

    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#if __VULKAN__
#include "coregraphics/vk/vkvertexsignaturecache.h"
namespace CoreGraphics
{
class VertexSignatureCache : public Vulkan::VkVertexSignatureCache
{
    __DeclareClass(VertexSignatureCache);
};
}
#else
#error "VertexSignatureCache class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------


