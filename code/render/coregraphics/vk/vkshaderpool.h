#pragma once
//------------------------------------------------------------------------------
/**
	Implements a shader loader from stream into a Vulkan shader.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcestreampool.h"
namespace Vulkan
{
class VkShaderPool : public Resources::ResourceStreamPool
{
	__DeclareClass(VkShaderPool);
public:
	/// return true if asynchronous loading is supported
	virtual bool CanLoadAsync() const;

private:
	/// setup the shader from a Nebula3 stream
	virtual bool SetupResourceFromStream(const Ptr<IO::Stream>& stream);
};
} // namespace Vulkan