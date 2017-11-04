#pragma once
//------------------------------------------------------------------------------
/**
	Handles stream-loaded Vulkan textures.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcestreampool.h"
#include "coregraphics/base/texturebase.h"
#include "vktexture.h"
namespace Vulkan
{
class VkTexturePool : public Resources::ResourceStreamPool
{
	__DeclareClass(VkTexturePool);
public:
	/// constructor
	VkTexturePool();
	/// destructor
	virtual ~VkTexturePool();

	/// setup loader
	void Setup();

private:
	/// load shader
	LoadStatus Load(const Ids::Id24 res, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);
	/// unload shader
	void Unload(const Ids::Id24 id);

	__ImplementResourceAllocatorSafe(VkTexture::textureAllocator);
};


//------------------------------------------------------------------------------
/**
*/
inline void
VkTexturePool::Unload(const Ids::Id24 id)
{
	this->EnterGet();
	VkTexture::LoadInfo& loadInfo = this->Get<1>(id);
	VkTexture::RuntimeInfo& runtimeInfo = this->Get<0>(id);
	VkTexture::Unload(loadInfo.mem, loadInfo.img, runtimeInfo.view);
	VkShaderServer::Instance()->UnregisterTexture(runtimeInfo.bind, runtimeInfo.type);
	this->LeaveGet();
}

} // namespace Vulkan