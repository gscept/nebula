#pragma once
//------------------------------------------------------------------------------
/**
	Implements a memory vertex buffer loader for Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/memoryvertexbufferloaderbase.h"
namespace Vulkan
{
class VkMemoryVertexBufferLoader : public Base::MemoryVertexBufferLoaderBase
{
	__DeclareClass(VkMemoryVertexBufferLoader);
public:

	LoadStatus Load(const Resources::ResourceId id);

	/// called by resource when a load is requested
	virtual bool OnLoadRequested();
};
} // namespace Vulkan