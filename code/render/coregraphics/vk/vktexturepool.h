#pragma once
//------------------------------------------------------------------------------
/**
	Implements a texture loader into a Vulkan texture.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcestreampool.h"
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
private:
	/// setup the texture from a Nebula3 stream
	virtual bool SetupResourceFromStream(const Ptr<IO::Stream>& stream);
};
} // namespace Vulkan