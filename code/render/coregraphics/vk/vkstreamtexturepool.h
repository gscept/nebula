#pragma once
//------------------------------------------------------------------------------
/**
	Handles stream-loaded Vulkan textures.

	Handles alloc/dealloc object creation through the memory texture pool
	
	(C) 2016 Individual contributors, see AUTHORS file
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
	LoadStatus LoadFromStream(const Ids::Id24 res, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream) override;
	/// unload shader
	void Unload(const Ids::Id24 id);

	/// allocate object
	Ids::Id32 AllocObject() override;
	/// deallocate object
	void DeallocObject(const Ids::Id32 id) override;
};


} // namespace Vulkan