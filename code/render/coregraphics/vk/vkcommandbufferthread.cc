//------------------------------------------------------------------------------
// vkcmdbufferthread.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkcommandbufferthread.h"
#include "threading/event.h"
#include "coregraphics/vk/vkgraphicsdevice.h"

namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
*/
DrawThread* CreateDrawThread()
{
	return Vulkan::VkCommandBufferThread::Create();
}

}

namespace Vulkan
{

extern PFN_vkCmdBeginDebugUtilsLabelEXT VkCmdDebugMarkerBegin;
extern PFN_vkCmdEndDebugUtilsLabelEXT VkCmdDebugMarkerEnd;
extern PFN_vkCmdInsertDebugUtilsLabelEXT VkCmdDebugMarkerInsert;


__ImplementClass(Vulkan::VkCommandBufferThread, 'VCBT', Threading::Thread);
//------------------------------------------------------------------------------
/**
*/
VkCommandBufferThread::VkCommandBufferThread() 
	: vkCommandBuffer(VK_NULL_HANDLE)
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
VkCommandBufferThread::DoWork()
{
	Profiling::ProfilingRegisterThread();

	Util::Array<DrawThread::Command> curCommands;
	byte* curCommandBuffer = nullptr;
	size_t curCommandBufferSize = 0;
	while (!this->ThreadStopRequested())
	{
		// lock our resources, which hinders the main thread from pushing more data
		N_MARKER_BEGIN(RecordCopyData, Vulkan);
		this->lock.Enter();

		if (this->commands.Size() > 0)
		{
			// copy command structs from main thread
			curCommands = this->commands;
			this->commands.Reset();

			// copy command data from main thread
			if (this->commandBuffer.size > curCommandBufferSize)
			{
				n_delete_array(curCommandBuffer);
				curCommandBuffer = n_new_array(byte, this->commandBuffer.size);
				curCommandBufferSize = this->commandBuffer.size;
			}
			memcpy(curCommandBuffer, this->commandBuffer.buffer, this->commandBuffer.size);
			this->commandBuffer.Reset();
		}

		// leave lock, this allows the main thread to issue more commands
		this->lock.Leave();
		N_MARKER_END();

		N_MARKER_BEGIN(Record, Vulkan);

		IndexT i;
		for (i = 0; i < curCommands.Size(); i++)
		{
			const DrawThread::Command& cmd = curCommands[i];
			VkCommandBufferThread::VkCommand* data = reinterpret_cast<VkCommandBufferThread::VkCommand*>(curCommandBuffer + cmd.offset);

			// use the data in the command dependent on what type we have
			switch (data->type)
			{
			case BeginCommand:
				n_assert(this->vkCommandBuffer == nullptr);
				this->vkCommandBuffer = data->bgCmd.buf;
#if NEBULA_GRAPHICS_DEBUG
				{
					Util::String name = Util::String::Sprintf("%s Generate draws", this->GetMyThreadName());
					Vulkan::CommandBufferBeginMarker(this->vkCommandBuffer, Math::float4(0.8f, 0.6f, 0.6f, 1.0f), name.AsCharPtr());
				}
#endif
				data->bgCmd.info.pInheritanceInfo = &data->bgCmd.inheritInfo;
				n_assert(vkBeginCommandBuffer(this->vkCommandBuffer, &data->bgCmd.info) == VK_SUCCESS);
				break;
			case ResetCommands:
				n_assert(vkResetCommandBuffer(this->vkCommandBuffer, 0) == VK_SUCCESS);
				break;
			case EndCommand:
				n_assert(this->vkCommandBuffer != nullptr);
				n_assert(vkEndCommandBuffer(this->vkCommandBuffer) == VK_SUCCESS);

#if NEBULA_GRAPHICS_DEBUG
				Vulkan::CommandBufferEndMarker(this->vkCommandBuffer);
#endif
				this->vkCommandBuffer = VK_NULL_HANDLE;
				this->vkPipelineLayout = VK_NULL_HANDLE;
				break;
			case GraphicsPipeline:
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				this->vkPipelineLayout = data->pipe.layout;
				vkCmdBindPipeline(this->vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, data->pipe.pipeline);
				break;
			case ComputePipeline:
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				this->vkPipelineLayout = data->pipe.layout;
				vkCmdBindPipeline(this->vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, data->pipe.pipeline);
				break;
			case InputAssemblyVertex:
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdBindVertexBuffers(this->vkCommandBuffer, data->vbo.index, 1, &data->vbo.buffer, &data->vbo.offset);
				break;
			case InputAssemblyIndex:
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdBindIndexBuffer(this->vkCommandBuffer, data->ibo.buffer, data->ibo.offset, data->ibo.indexType);
				break;
			case Draw:
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				if (data->draw.numIndices > 0)	vkCmdDrawIndexed(this->vkCommandBuffer, data->draw.numIndices, data->draw.numInstances, data->draw.baseIndex, data->draw.baseVertex, data->draw.baseInstance);
				else							vkCmdDraw(this->vkCommandBuffer, data->draw.numVerts, data->draw.numInstances, data->draw.baseVertex, data->draw.baseInstance);
				break;
			case Dispatch:
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdDispatch(this->vkCommandBuffer, data->dispatch.numGroupsX, data->dispatch.numGroupsY, data->dispatch.numGroupsZ);
				break;
			case BindDescriptors:
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				n_assert(this->vkPipelineLayout != VK_NULL_HANDLE);
				vkCmdBindDescriptorSets(this->vkCommandBuffer, data->descriptor.type, this->vkPipelineLayout, data->descriptor.baseSet, data->descriptor.numSets, data->descriptor.sets, data->descriptor.numOffsets, data->descriptor.offsets);
				break;
			case PushRange:
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				n_assert(this->vkPipelineLayout != VK_NULL_HANDLE);
				vkCmdPushConstants(this->vkCommandBuffer, this->vkPipelineLayout, data->pushranges.stages, data->pushranges.offset, data->pushranges.size, data->pushranges.data);
				Memory::Free(Memory::ScratchHeap, data->pushranges.data);
				break;
			case Viewport:
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdSetViewport(this->vkCommandBuffer, data->viewport.index, 1, &data->viewport.vp);
				break;
			case ViewportArray:
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdSetViewport(this->vkCommandBuffer, data->viewportArray.first, data->viewportArray.num, data->viewportArray.vps);
				break;
			case ScissorRect:
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdSetScissor(this->vkCommandBuffer, data->scissorRect.index, 1, &data->scissorRect.sc);
				break;
			case ScissorRectArray:
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdSetScissor(this->vkCommandBuffer, data->scissorRectArray.first, data->scissorRectArray.num, data->scissorRectArray.scs);
				break;
			case UpdateBuffer:
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdUpdateBuffer(this->vkCommandBuffer, data->updBuffer.buf, data->updBuffer.offset, data->updBuffer.size, data->updBuffer.data);
				break;
			case SetEvent:
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdSetEvent(this->vkCommandBuffer, data->setEvent.event, data->setEvent.stages);
				break;
			case ResetEvent:
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdResetEvent(this->vkCommandBuffer, data->resetEvent.event, data->resetEvent.stages);
				break;
			case WaitForEvent:
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdWaitEvents(this->vkCommandBuffer, data->waitEvent.numEvents, data->waitEvent.events, data->waitEvent.waitingStage, data->waitEvent.signalingStage, data->waitEvent.memoryBarrierCount, data->waitEvent.memoryBarriers, data->waitEvent.bufferBarrierCount, data->waitEvent.bufferBarriers, data->waitEvent.imageBarrierCount, data->waitEvent.imageBarriers);
				break;
			case Barrier:
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdPipelineBarrier(this->vkCommandBuffer, data->barrier.srcMask, data->barrier.dstMask, data->barrier.dep, data->barrier.memoryBarrierCount, data->barrier.memoryBarriers, data->barrier.bufferBarrierCount, data->barrier.bufferBarriers, data->barrier.imageBarrierCount, data->barrier.imageBarriers);
				break;
			case BeginMarker:
			{
				VkDebugUtilsLabelEXT info =
				{
					VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
					nullptr,
					data->marker.text,
					{ data->marker.values[0], data->marker.values[1], data->marker.values[2], data->marker.values[3] }
				};
				VkCmdDebugMarkerBegin(this->vkCommandBuffer, &info);
				break;
			}
			case EndMarker:
				VkCmdDebugMarkerEnd(this->vkCommandBuffer);
				break;
			case InsertMarker:
			{
				VkDebugUtilsLabelEXT info =
				{
					VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
					nullptr,
					data->marker.text,
					{ data->marker.values[0], data->marker.values[1], data->marker.values[2], data->marker.values[3] }
				};
				VkCmdDebugMarkerInsert(this->vkCommandBuffer, &info);
				break;
			}

			}
		}

		N_MARKER_END();

		this->lock.Enter();
		if (this->event)
		{
			this->event->Signal();
			this->event = nullptr;
		}
		this->lock.Leave();

		// clear up commands
		if (!curCommands.IsEmpty())
			curCommands.Reset();

		N_SCOPE(RecordIdle, Render);

		// wait for more data
		this->signalEvent.Wait();
	}
}

} // namespace Vulkan
