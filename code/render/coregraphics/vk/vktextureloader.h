#pragma once
//------------------------------------------------------------------------------
/**
	Implements a texture loader into a Vulkan texture.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourceloader.h"
namespace Vulkan
{
class VkTextureLoader : public Resources::ResourceLoader
{
	__DeclareClass(VkTextureLoader);
public:
	/// constructor
	VkTextureLoader();
	/// destructor
	virtual ~VkTextureLoader();
private:
	/// setup the texture from a Nebula3 stream
	virtual bool SetupResourceFromStream(const Ptr<IO::Stream>& stream);
};
} // namespace Vulkan