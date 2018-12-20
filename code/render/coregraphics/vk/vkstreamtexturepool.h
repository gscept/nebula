#pragma once
//------------------------------------------------------------------------------
/**
	Handles stream-loaded Vulkan textures.

	Handles alloc/dealloc object creation through the memory texture pool
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcestreampool.h"
#include "vktexture.h"
namespace Vulkan
{
class VkStreamTexturePool : public Resources::ResourceStreamPool
{
	__DeclareClass(VkStreamTexturePool);
public:
	/// constructor
	VkStreamTexturePool();
	/// destructor
	virtual ~VkStreamTexturePool();

	/// setup loader
	void Setup();

private:
	/// load shader
	LoadStatus LoadFromStream(const Resources::ResourceId res, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream) override;
	/// unload shader
	void Unload(const Resources::ResourceId id);

	/// allocate object
	Resources::ResourceUnknownId AllocObject() override;
	/// deallocate object
	void DeallocObject(const Resources::ResourceUnknownId id) override;
};

} // namespace Vulkan