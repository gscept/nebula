#pragma once
//------------------------------------------------------------------------------
/**
	Implements a memory fence in Vulkan.

	Uses two command queues, one which waits, and one which signals.
	
	For example, a signaling command buffer could be for example the main draw buffer,
	and the waiting buffer is a command buffer created for a transfer, which should update
	a buffer, but wait for some specific draw to finish before doing so. 

	DRAW ----------- Draw ---- Draw ---- Draw ---- Signal ---- Draw ---- Draw ---- Signal
	TRANSFER ------- Update Wait ----------------- Update Wait ------------------- Update

	Since Vulkan supports the vkCmdUpdateBuffer which copies data to the command buffer,
	we can do all our buffer updates in a command buffer.

	One should be careful that the wait command buffer and signal command buffer doesn't belong on the same queue.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/bufferlockbase.h"
namespace Vulkan
{
class VkCpuSyncFence : public Base::BufferLockBase
{
	__DeclareClass(VkCpuSyncFence);
public:
	/// constructor
	VkCpuSyncFence();
	/// destructor
	virtual ~VkCpuSyncFence();

	/// set the command buffer the sync should be put in
	void SetWaitCommandBuffer(VkCommandBuffer cmd, VkPipelineStageFlags flags);
	/// set the command buffer the sync should be put in
	void SetSignalCommandBuffer(VkCommandBuffer cmd, VkPipelineStageFlags flags);

	/// lock a buffer
	void LockBuffer(IndexT index);
	/// wait for a buffer to finish
	void WaitForBuffer(IndexT index);
	/// lock a range
	void LockRange(IndexT startIndex, SizeT length);
	/// wait for a locked range
	void WaitForRange(IndexT startIndex, SizeT length);
private:
	/// perform waiting
	void Wait(VkEvent sync);
	/// cleanup
	void Cleanup(VkEvent sync);

	VkPipelineStageFlags waitStage;
	VkPipelineStageFlags signalStage;
	VkCommandBuffer waitCmd;
	VkCommandBuffer signalCmd;
	Util::Dictionary<Base::BufferRange, VkEvent> rangeFences;
	Util::Dictionary<IndexT, VkEvent> indexFences;
};

//------------------------------------------------------------------------------
/**
*/
inline void
VkCpuSyncFence::SetWaitCommandBuffer(VkCommandBuffer cmd, VkPipelineStageFlags flags)
{
	this->waitCmd = cmd;
	this->waitStage = flags;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkCpuSyncFence::SetSignalCommandBuffer(VkCommandBuffer cmd, VkPipelineStageFlags flags)
{
	this->signalCmd = cmd;
	this->signalStage = flags;
}
} // namespace Vulkan