#pragma once
//------------------------------------------------------------------------------
/**
	Types used for Vulkan vertex buffers, 
	see the MemoryVertexBufferPool for the loader code.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/vertexbufferbase.h"
namespace Vulkan
{
class VkVertexBuffer
{
public:

	struct LoadInfo
	{
		VkDeviceMemory mem;
		CoreGraphics::GpuBufferTypes::SetupFlags gpuResInfo;
		Base::VertexBufferBase::VertexBufferBaseInfo vboInfo;
	};

};

} // namespace Vulkan