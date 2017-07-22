#pragma once
//------------------------------------------------------------------------------
/**
	Implements an index buffer loader for Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/memoryindexbufferpoolbase.h"
namespace Vulkan
{
class VkMemoryIndexBufferPool : public Base::MemoryIndexBufferPoolBase
{
	__DeclareClass(VkMemoryIndexBufferPool);
public:

	/// update resource
	LoadStatus UpdateResource(const Resources::ResourceId id, void* info);

	/// perform actual load, override in subclass
	LoadStatus Load(const Ptr<Resources::Resource>& res, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);
	/// unload resource
	void Unload(const Ptr<Resources::Resource>& res);

	/// called by resource when a load is requested
	virtual bool OnLoadRequested();
};
} // namespace Vulkan