#pragma once
//------------------------------------------------------------------------------
/**
	Types used for Vulkan index buffers,
	see the MemoryVertexBufferPool for the loader code.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/indexbufferbase.h"
#include "coregraphics/base/gpuresourcebase.h"
namespace Vulkan
{
class VkIndexBuffer
{
public:

	struct LoadInfo
	{
		VkDeviceMemory mem;
		Base::GpuResourceBase::GpuResourceBaseInfo gpuResInfo;
		Base::IndexBufferBase::IndexBufferBaseInfo iboInfo;
	};
	struct RuntimeInfo
	{
		VkBuffer buf;
		CoreGraphics::IndexType::Code type;
	};
};

} // namespace Vulkan