#pragma once
//------------------------------------------------------------------------------
/**
	Implements an index buffer loader for Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/memoryindexbufferloaderbase.h"
namespace Vulkan
{
class VkMemoryIndexBufferLoader : public Base::MemoryIndexBufferLoaderBase
{
	__DeclareClass(VkMemoryIndexBufferLoader);
public:

	/// perform actual load, override in subclass
	LoadStatus Load(const Ptr<Resources::Resource>& res, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);
	/// unload resource
	void Unload(const Ptr<Resources::Resource>& res);

	/// called by resource when a load is requested
	virtual bool OnLoadRequested();
};
} // namespace Vulkan