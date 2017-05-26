#pragma once
//------------------------------------------------------------------------------
/**
	Implements a texture loader into a Vulkan texture.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/streamresourceloader.h"
namespace Vulkan
{
class VkStreamTextureLoader : public Resources::StreamResourceLoader
{
	__DeclareClass(VkStreamTextureLoader);
public:
	/// constructor
	VkStreamTextureLoader();
	/// destructor
	virtual ~VkStreamTextureLoader();
private:
	/// setup the texture from a Nebula3 stream
	virtual bool SetupResourceFromStream(const Ptr<IO::Stream>& stream);
};
} // namespace Vulkan