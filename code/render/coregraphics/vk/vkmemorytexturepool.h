#pragma once
//------------------------------------------------------------------------------
/**
	Handles memory-loaded Vulkan textures.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcememorypool.h"
#include "coregraphics/pixelformat.h"
#include <array>
#include "coregraphics/base/texturebase.h"
#include "vktexture.h"
#include "vkshaderserver.h"

namespace Vulkan
{
class VkMemoryTexturePool : public Resources::ResourceMemoryPool
{
	__DeclareClass(VkMemoryTexturePool);
public:

	struct VkMemoryTextureInfo
	{
		const void* buffer;
		CoreGraphics::PixelFormat::Code format;
		SizeT width, height;
	};

	/// update resource
	LoadStatus UpdateResource(const Ids::Id24 id, void* info);
	/// unload resource
	void Unload(const Ids::Id24 id);
private:

	__ImplementResourceAllocatorSafe(VkTexture::textureAllocator);
};

//------------------------------------------------------------------------------
/**
*/
inline void
VkMemoryTexturePool::Unload(const Ids::Id24 id)
{
	this->EnterGet();
	VkTexture::LoadInfo& loadInfo = this->Get<1>(id);
	VkTexture::RuntimeInfo& runtimeInfo = this->Get<0>(id);
	VkTexture::Unload(loadInfo.mem, loadInfo.img, runtimeInfo.view);
	VkShaderServer::Instance()->UnregisterTexture(runtimeInfo.bind, runtimeInfo.type);
	this->LeaveGet();
}

} // namespace Vulkan