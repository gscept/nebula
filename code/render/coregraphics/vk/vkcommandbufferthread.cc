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

		byte* commandBuf = curCommandBuffer;
		IndexT i;
		for (i = 0; i < curCommands.Size(); i++)
		{
			const DrawThread::Command& cmd = curCommands[i];

			// use the data in the command dependent on what type we have
			switch (cmd.type)
			{
			case BeginCommand:
			{
				VkCommandBufferBeginCommand* vkcmd = reinterpret_cast<VkCommandBufferBeginCommand*>(commandBuf);
				n_assert(this->vkCommandBuffer == nullptr);
				this->vkCommandBuffer = vkcmd->buf;
#if NEBULA_GRAPHICS_DEBUG
				{
					Util::String name = Util::String::Sprintf("%s Generate draws", this->GetMyThreadName());
					Vulkan::CommandBufferBeginMarker(this->vkCommandBuffer, Math::float4(0.8f, 0.6f, 0.6f, 1.0f), name.AsCharPtr());
				}
#endif
				vkcmd->info.pInheritanceInfo = &vkcmd->inheritInfo;
				n_assert(vkBeginCommandBuffer(this->vkCommandBuffer, &vkcmd->info) == VK_SUCCESS);
				break;
			}				
			case ResetCommand:
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
			{
				VkGfxPipelineBindCommand* vkcmd = reinterpret_cast<VkGfxPipelineBindCommand*>(commandBuf);
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				this->vkPipelineLayout = vkcmd->layout;
				vkCmdBindPipeline(this->vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkcmd->pipeline);
				break;
			}				
			case ComputePipeline:
			{
				VkComputePipelineBindCommand* vkcmd = reinterpret_cast<VkComputePipelineBindCommand*>(commandBuf);
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				this->vkPipelineLayout = vkcmd->layout;
				vkCmdBindPipeline(this->vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vkcmd->pipeline);
				break;
			}				
			case InputAssemblyVertex:
			{
				VkVertexBufferCommand* vkcmd = reinterpret_cast<VkVertexBufferCommand*>(commandBuf);
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdBindVertexBuffers(this->vkCommandBuffer, vkcmd->index, 1, &vkcmd->buffer, &vkcmd->offset);
				break;
			}
			case InputAssemblyIndex:
			{
				VkIndexBufferCommand* vkcmd = reinterpret_cast<VkIndexBufferCommand*>(commandBuf);
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdBindIndexBuffer(this->vkCommandBuffer, vkcmd->buffer, vkcmd->offset, vkcmd->indexType);
				break;
			}				
			case Draw:
			{
				VkDrawCommand* vkcmd = reinterpret_cast<VkDrawCommand*>(commandBuf);
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				if (vkcmd->numIndices > 0)	vkCmdDrawIndexed(this->vkCommandBuffer, vkcmd->numIndices, vkcmd->numInstances, vkcmd->baseIndex, vkcmd->baseVertex, vkcmd->baseInstance);
				else						vkCmdDraw(this->vkCommandBuffer, vkcmd->numVerts, vkcmd->numInstances, vkcmd->baseVertex, vkcmd->baseInstance);
				break;
			}
			case Dispatch:
			{
				VkDispatchCommand* vkcmd = reinterpret_cast<VkDispatchCommand*>(commandBuf);
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdDispatch(this->vkCommandBuffer, vkcmd->numGroupsX, vkcmd->numGroupsY, vkcmd->numGroupsZ);
				break;
			}
			case BindDescriptors:
			{
				VkDescriptorsCommand* vkcmd = reinterpret_cast<VkDescriptorsCommand*>(commandBuf);
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				n_assert(this->vkPipelineLayout != VK_NULL_HANDLE);
				vkCmdBindDescriptorSets(this->vkCommandBuffer, vkcmd->type, this->vkPipelineLayout, vkcmd->baseSet, vkcmd->numSets, vkcmd->sets, vkcmd->numOffsets, vkcmd->offsets);
				break;
			}				
			case PushRange:
			{
				VkPushConstantsCommand* vkcmd = reinterpret_cast<VkPushConstantsCommand*>(commandBuf);
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				n_assert(this->vkPipelineLayout != VK_NULL_HANDLE);
				vkCmdPushConstants(this->vkCommandBuffer, this->vkPipelineLayout, vkcmd->stages, vkcmd->offset, vkcmd->size, vkcmd->data);
				break;
			}				
			case Viewport:
			{
				VkViewportCommand* vkcmd = reinterpret_cast<VkViewportCommand*>(commandBuf);
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdSetViewport(this->vkCommandBuffer, vkcmd->index, 1, &vkcmd->vp);
				break;
			}				
			case ViewportArray:
			{
				VkViewportArrayCommand* vkcmd = reinterpret_cast<VkViewportArrayCommand*>(commandBuf);
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdSetViewport(this->vkCommandBuffer, vkcmd->first, vkcmd->num, vkcmd->vps);
				break;
			}				
			case ScissorRect:
			{
				VkScissorRectCommand* vkcmd = reinterpret_cast<VkScissorRectCommand*>(commandBuf);
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdSetScissor(this->vkCommandBuffer, vkcmd->index, 1, &vkcmd->sc);
				break;
			}				
			case ScissorRectArray:
			{
				VkScissorRectArrayCommand* vkcmd = reinterpret_cast<VkScissorRectArrayCommand*>(commandBuf);
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdSetScissor(this->vkCommandBuffer, vkcmd->first, vkcmd->num, vkcmd->scs);
				break;
			}				
			case UpdateBuffer:
			{
				VkUpdateBufferCommand* vkcmd = reinterpret_cast<VkUpdateBufferCommand*>(commandBuf);
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdUpdateBuffer(this->vkCommandBuffer, vkcmd->buf, vkcmd->offset, vkcmd->size, vkcmd->data);
				break;
			}				
			case SetEvent:
			{
				VkSetEventCommand* vkcmd = reinterpret_cast<VkSetEventCommand*>(commandBuf);
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdSetEvent(this->vkCommandBuffer, vkcmd->event, vkcmd->stages);
				break;
			}				
			case ResetEvent:
			{
				VkResetEventCommand* vkcmd = reinterpret_cast<VkResetEventCommand*>(commandBuf);
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdResetEvent(this->vkCommandBuffer, vkcmd->event, vkcmd->stages);
				break;
			}				
			case WaitForEvent:
			{
				VkWaitForEventCommand* vkcmd = reinterpret_cast<VkWaitForEventCommand*>(commandBuf);
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdWaitEvents(this->vkCommandBuffer, 1, &vkcmd->event, vkcmd->waitingStage, vkcmd->signalingStage, vkcmd->memoryBarrierCount, vkcmd->memoryBarriers, vkcmd->bufferBarrierCount, vkcmd->bufferBarriers, vkcmd->imageBarrierCount, vkcmd->imageBarriers);
				break;
			}				
			case Barrier:
			{
				VkBarrierCommand* vkcmd = reinterpret_cast<VkBarrierCommand*>(commandBuf);
				n_assert(this->vkCommandBuffer != VK_NULL_HANDLE);
				vkCmdPipelineBarrier(this->vkCommandBuffer, vkcmd->srcMask, vkcmd->dstMask, vkcmd->dep, vkcmd->memoryBarrierCount, vkcmd->memoryBarriers, vkcmd->bufferBarrierCount, vkcmd->bufferBarriers, vkcmd->imageBarrierCount, vkcmd->imageBarriers);
				break;
			}				
			case BeginMarker:
			{
				VkBeginMarkerCommand* vkcmd = reinterpret_cast<VkBeginMarkerCommand*>(commandBuf);
				VkDebugUtilsLabelEXT info =
				{
					VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
					nullptr,
					vkcmd->text,
					{ vkcmd->values[0], vkcmd->values[1], vkcmd->values[2], vkcmd->values[3] }
				};
				VkCmdDebugMarkerBegin(this->vkCommandBuffer, &info);
				break;
			}
			case EndMarker:
				VkCmdDebugMarkerEnd(this->vkCommandBuffer);
				break;
			case InsertMarker:
			{
				VkInsertMarkerCommand* vkcmd = reinterpret_cast<VkInsertMarkerCommand*>(commandBuf);
				VkDebugUtilsLabelEXT info =
				{
					VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
					nullptr,
					vkcmd->text,
					{ vkcmd->values[0], vkcmd->values[1], vkcmd->values[2], vkcmd->values[3] }
				};
				VkCmdDebugMarkerInsert(this->vkCommandBuffer, &info);
				break;
			}

			case Sync:
			{
				SyncCommand* vkcmd = reinterpret_cast<SyncCommand*>(commandBuf);
				vkcmd->event->Signal();
				break;
			}

			}

			commandBuf += cmd.size;
		}

		N_MARKER_END();

		// clear up commands
		if (!curCommands.IsEmpty())
			curCommands.Reset();

		N_SCOPE(RecordIdle, Render);

		// wait for more data
		this->signalEvent.Wait();
	}
}

} // namespace Vulkan
