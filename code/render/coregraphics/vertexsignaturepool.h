#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::VertexSignaturePool

    Resource pool for vertex layout signatures

    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#if __VULKAN__
#include "coregraphics/vk/vkvertexsignaturepool.h"
namespace CoreGraphics
{
class VertexSignaturePool : public Vulkan::VkVertexSignaturePool
{
    __DeclareClass(VertexSignaturePool);
};
}
#else
#error "VertexSignaturePool class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------


