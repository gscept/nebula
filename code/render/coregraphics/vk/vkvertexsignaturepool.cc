//------------------------------------------------------------------------------
// vkvertexsignaturepool.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkvertexsignaturepool.h"
#include "coregraphics/vertexcomponent.h"

namespace Vulkan
{

__ImplementClass(Vulkan::VkVertexSignaturePool, 'VVSP', Resources::ResourceMemoryPool);
//------------------------------------------------------------------------------
/**
*/
VkVertexSignaturePool::VkVertexSignaturePool()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkVertexSignaturePool::~VkVertexSignaturePool()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkVertexSignaturePool::BindVertexLayout(const Resources::ResourceId id)
{
	VkVertexLayout* layout = this->GetResource(id);
	layout->Apply();
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
VkVertexSignaturePool::UpdateResource(const Resources::ResourceId id, void* info)
{
	Util::Array<CoreGraphics::VertexComponent>* components = static_cast<Util::Array<CoreGraphics::VertexComponent>*>(info);
	VkVertexLayout* layout = this->GetResource(id);
	layout->Setup(*components);
}

//------------------------------------------------------------------------------
/**
*/
VkPipelineVertexInputStateCreateInfo*
VkVertexSignaturePool::GetDerivativeLayout(const Resources::ResourceId layout)
{

}

} // namespace Vertex
