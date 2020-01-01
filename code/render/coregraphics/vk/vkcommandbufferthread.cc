//------------------------------------------------------------------------------
// vkcmdbufferthread.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkcommandbufferthread.h"
#include "threading/event.h"
#include "coregraphics/vk/vkgraphicsdevice.h"

namespace Vulkan
{

__ImplementClass(Vulkan::VkCommandBufferThread, 'VCBT', Threading::Thread);
//------------------------------------------------------------------------------
/**
*/
VkCommandBufferThread::VkCommandBufferThread()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkCommandBufferThread::~VkCommandBufferThread()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkCommandBufferThread::EmitWakeupSignal()
{
	this->commands.Signal();
}

//------------------------------------------------------------------------------
/**
*/
void
VkCommandBufferThread::DoWork()
{
	Util::Array<Command> curCommands;
	curCommands.Reserve(1000);
	while (!this->ThreadStopRequested())
	{
		// dequeue all commands, this ensures we don't gain any new commands this thread loop
		this->commands.DequeueAll(curCommands);

		IndexT i;
		for (i = 0; i < curCommands.Size(); i++)
		{
			const Command& cmd = curCommands[i];

			// use the data in the command dependent on what type we have
			switch (cmd.type)
			{
			case BeginCommand:
				this->commandBuffer = cmd.bgCmd.buf;
#if NEBULA_GRAPHICS_DEBUG
				{
					Util::String name = Util::String::Sprintf("%s Generate draws", this->GetMyThreadName());
					Vulkan::CommandBufferBeginMarker(this->commandBuffer, Math::float4(0.8f, 0.6f, 0.6f, 1.0f), name.AsCharPtr());
				}
#endif
				n_assert(vkBeginCommandBuffer(this->commandBuffer, &cmd.bgCmd.info) == VK_SUCCESS);
				break;
			case ResetCommands:
				n_assert(vkResetCommandBuffer(this->commandBuffer, 0) == VK_SUCCESS);
				break;
			case EndCommand:
				n_assert(vkEndCommandBuffer(this->commandBuffer) == VK_SUCCESS);

#if NEBULA_GRAPHICS_DEBUG
				Vulkan::CommandBufferEndMarker(this->commandBuffer);
#endif
				this->commandBuffer = VK_NULL_HANDLE;
				this->pipelineLayout = VK_NULL_HANDLE;
				break;
			case GraphicsPipeline:
				n_assert(this->commandBuffer != VK_NULL_HANDLE);
				this->pipelineLayout = cmd.pipe.layout;
				vkCmdBindPipeline(this->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cmd.pipe.pipeline);
				break;
			case ComputePipeline:
				n_assert(this->commandBuffer != VK_NULL_HANDLE);
				this->pipelineLayout = cmd.pipe.layout;
				vkCmdBindPipeline(this->commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cmd.pipe.pipeline);
				break;
			case InputAssemblyVertex:
				n_assert(this->commandBuffer != VK_NULL_HANDLE);
				vkCmdBindVertexBuffers(this->commandBuffer, cmd.vbo.index, 1, &cmd.vbo.buffer, &cmd.vbo.offset);
				break;
			case InputAssemblyIndex:
				n_assert(this->commandBuffer != VK_NULL_HANDLE);
				vkCmdBindIndexBuffer(this->commandBuffer, cmd.ibo.buffer, cmd.ibo.offset, cmd.ibo.indexType);
				break;
			case Draw:
				n_assert(this->commandBuffer != VK_NULL_HANDLE);
				if (cmd.draw.numIndices > 0)	vkCmdDrawIndexed(this->commandBuffer, cmd.draw.numIndices, cmd.draw.numInstances, cmd.draw.baseIndex, cmd.draw.baseVertex, cmd.draw.baseInstance);
				else							vkCmdDraw(this->commandBuffer, cmd.draw.numVerts, cmd.draw.numInstances, cmd.draw.baseVertex, cmd.draw.baseInstance);
				break;
			case Dispatch:
				n_assert(this->commandBuffer != VK_NULL_HANDLE);
				vkCmdDispatch(this->commandBuffer, cmd.dispatch.numGroupsX, cmd.dispatch.numGroupsY, cmd.dispatch.numGroupsZ);
				break;
			case BindDescriptors:
				n_assert(this->commandBuffer != VK_NULL_HANDLE);
				n_assert(this->pipelineLayout != VK_NULL_HANDLE);
				vkCmdBindDescriptorSets(this->commandBuffer, cmd.descriptor.type, this->pipelineLayout, cmd.descriptor.baseSet, cmd.descriptor.numSets, cmd.descriptor.sets, cmd.descriptor.numOffsets, cmd.descriptor.offsets);
				break;
			case PushRange:
				n_assert(this->commandBuffer != VK_NULL_HANDLE);
				n_assert(this->pipelineLayout != VK_NULL_HANDLE);
				vkCmdPushConstants(this->commandBuffer, this->pipelineLayout, cmd.pushranges.stages, cmd.pushranges.offset, cmd.pushranges.size, cmd.pushranges.data);
				Memory::Free(Memory::ScratchHeap, cmd.pushranges.data);
				break;
			case Viewport:
				n_assert(this->commandBuffer != VK_NULL_HANDLE);
				vkCmdSetViewport(this->commandBuffer, cmd.viewport.index, 1, &cmd.viewport.vp);
				break;
			case ViewportArray:
				n_assert(this->commandBuffer != VK_NULL_HANDLE);
				vkCmdSetViewport(this->commandBuffer, cmd.viewportArray.first, cmd.viewportArray.num, cmd.viewportArray.vps);
				break;
			case ScissorRect:
				n_assert(this->commandBuffer != VK_NULL_HANDLE);
				vkCmdSetScissor(this->commandBuffer, cmd.scissorRect.index, 1, &cmd.scissorRect.sc);
				break;
			case ScissorRectArray:
				n_assert(this->commandBuffer != VK_NULL_HANDLE);
				vkCmdSetScissor(this->commandBuffer, cmd.scissorRectArray.first, cmd.scissorRectArray.num, cmd.scissorRectArray.scs);
				break;
			case UpdateBuffer:
				n_assert(this->commandBuffer != VK_NULL_HANDLE);
				vkCmdUpdateBuffer(this->commandBuffer, cmd.updBuffer.buf, cmd.updBuffer.offset, cmd.updBuffer.size, cmd.updBuffer.data);
				break;
			case SetEvent:
				n_assert(this->commandBuffer != VK_NULL_HANDLE);
				vkCmdSetEvent(this->commandBuffer, cmd.setEvent.event, cmd.setEvent.stages);
				break;
			case ResetEvent:
				n_assert(this->commandBuffer != VK_NULL_HANDLE);
				vkCmdResetEvent(this->commandBuffer, cmd.resetEvent.event, cmd.resetEvent.stages);
				break;
			case WaitForEvent:
				n_assert(this->commandBuffer != VK_NULL_HANDLE);
				vkCmdWaitEvents(this->commandBuffer, cmd.waitEvent.numEvents, cmd.waitEvent.events, cmd.waitEvent.waitingStage, cmd.waitEvent.signalingStage, cmd.waitEvent.memoryBarrierCount, cmd.waitEvent.memoryBarriers, cmd.waitEvent.bufferBarrierCount, cmd.waitEvent.bufferBarriers, cmd.waitEvent.imageBarrierCount, cmd.waitEvent.imageBarriers);
				break;
			case Barrier:
				n_assert(this->commandBuffer != VK_NULL_HANDLE);
				vkCmdPipelineBarrier(this->commandBuffer, cmd.barrier.srcMask, cmd.barrier.dstMask, cmd.barrier.dep, cmd.barrier.memoryBarrierCount, cmd.barrier.memoryBarriers, cmd.barrier.bufferBarrierCount, cmd.barrier.bufferBarriers, cmd.barrier.imageBarrierCount, cmd.barrier.imageBarriers);
				break;
			case Sync:
				cmd.syncEvent->Signal();
				break;
			}
		}

		// reset commands, but don't destroy them
		curCommands.Reset();
		this->commands.Wait();
	}
}

} // namespace Vulkan
