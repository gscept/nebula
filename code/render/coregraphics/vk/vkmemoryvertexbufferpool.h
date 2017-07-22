#pragma once
//------------------------------------------------------------------------------
/**
	Implements a memory vertex buffer loader for Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/memoryvertexbufferpoolbase.h"
namespace Vulkan
{
class VkMemoryVertexBufferPool : public Base::MemoryVertexBufferPoolBase
{
	__DeclareClass(VkMemoryVertexBufferPool);
public:

	/// update resource
	LoadStatus UpdateResource(const Resources::ResourceId id, void* info);
	/// unload resource
	void Unload(const Ptr<Resources::Resource>& res);
};
} // namespace Vulkan