#pragma once
//------------------------------------------------------------------------------
/**
	Implements a Vulkan pipeline barrier
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/barrierbase.h"
namespace Vulkan
{
class VkBarrier : public Base::BarrierBase
{
	__DeclareClass(VkBarrier);
public:
	/// constructor
	VkBarrier();
	/// destructor
	virtual ~VkBarrier();

	/// setup
	void Setup();
	/// discard
	void Discard();
private:
	friend class VkRenderDevice;
	friend class VkCmdEvent;
	VkPipelineStageFlags vkSrcFlags;
	VkPipelineStageFlags vkDstFlags;
	VkDependencyFlags vkDep;
	uint32_t vkNumMemoryBarriers;
	VkMemoryBarrier vkMemoryBarriers[32];
	uint32_t vkNumBufferBarriers;
	VkBufferMemoryBarrier vkBufferBarriers[32];
	uint32_t vkNumImageBarriers;
	VkImageMemoryBarrier vkImageBarriers[32];
};
} // namespace Vulkan