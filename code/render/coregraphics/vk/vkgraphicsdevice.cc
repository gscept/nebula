//------------------------------------------------------------------------------
//  vkgraphicsdevice.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/config.h"
#include "vkgraphicsdevice.h"
#include "coregraphics/commandbuffer.h"
#include "vkshaderprogram.h"
#include "vkpipelinedatabase.h"
#include "vkcommandbuffer.h"
#include "vktransformdevice.h"
#include "vkresourcetable.h"
#include "vkshaderserver.h"
#include "vkpass.h"
#include "vkrendertexture.h"
#include "vkshaderrwtexture.h"
#include "vkshaderrwbuffer.h"
#include "vkbarrier.h"
#include "coregraphics/displaydevice.h"
#include "app/application.h"
#include "io/ioserver.h"
#include "vkevent.h"
#include "vkfence.h"
#include "vktypes.h"
#include "coregraphics/memoryindexbufferpool.h"
#include "coregraphics/memoryvertexbufferpool.h"
#include "coregraphics/vertexsignaturepool.h"
#include "coregraphics/glfw/glfwwindow.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/vk/vkconstantbuffer.h"
#include "coregraphics/vk/vksemaphore.h"
#include "coregraphics/vk/vkfence.h"
#include "coregraphics/vk/vksubmissioncontext.h"
#include "coregraphics/submissioncontext.h"
#include "resources/resourcemanager.h"
namespace Vulkan
{

enum VkPipelineInfoBits
{
	NoInfoSet = 0,
	ShaderInfoSet = 1,
	VertexLayoutInfoSet = 2,
	FramebufferLayoutInfoSet = 4,
	InputLayoutInfoSet = 8,

	AllInfoSet = 15,

	PipelineBuilt = 16
};
__ImplementEnumBitOperators(VkPipelineInfoBits);

struct GraphicsDeviceState : CoreGraphics::GraphicsDeviceState
{
	uint32_t adapter;
	uint32_t frameId;
	VkPhysicalDeviceMemoryProperties memoryProps;

	VkInstance instance;
	VkDescriptorPool descPool;
	VkPipelineCache cache;
	VkAllocationCallbacks alloc;

	VkFence mainCmdDrawFence;
	VkFence mainCmdCmpFence;
	VkFence mainCmdTransFence;

	CoreGraphics::ShaderPipeline currentBindPoint;

	static const SizeT NumDrawThreads = 8;
	IndexT currentDrawThread;
	VkCommandPool dispatchableCmdDrawBufferPool[NumDrawThreads];
	VkCommandBuffer dispatchableDrawCmdBuffers[NumDrawThreads];
	Ptr<VkCommandBufferThread> drawThreads[NumDrawThreads];
	Threading::Event* drawCompletionEvents[NumDrawThreads];

	static const SizeT NumTransferThreads = 1;
	IndexT currentTransThread;
	VkCommandPool dispatchableCmdTransBufferPool[NumTransferThreads];
	VkCommandBuffer dispatchableTransCmdBuffers[NumTransferThreads];
	Ptr<VkCommandBufferThread> transThreads[NumTransferThreads];
	Threading::Event* transCompletionEvents[NumTransferThreads];

	static const SizeT NumComputeThreads = 1;
	IndexT currentComputeThread;
	VkCommandPool dispatchableCmdCompBufferPool[NumComputeThreads];
	VkCommandBuffer dispatchableCompCmdBuffers[NumComputeThreads];
	Ptr<VkCommandBufferThread> compThreads[NumComputeThreads];
	Threading::Event* compCompletionEvents[NumComputeThreads];

	Util::Array<VkCommandBufferThread::Command> propagateDescriptorSets;
	Util::Array<VkCommandBufferThread::Command> threadCmds[NumDrawThreads];
	SizeT numCallsLastFrame;
	SizeT numActiveThreads;
	SizeT numUsedThreads;

	VkCommandBufferInheritanceInfo passInfo;
	VkPipelineInputAssemblyStateCreateInfo inputInfo;
	VkPipelineColorBlendStateCreateInfo blendInfo;
	VkViewport* passViewports;
	uint32_t numVsInputs;

	struct ConstantsRingBuffer
	{
		// handle global constant memory
		uint32_t cboGfxStartAddress[CoreGraphicsGlobalConstantBufferType::NumConstantBufferTypes];
		uint32_t cboGfxEndAddress[CoreGraphicsGlobalConstantBufferType::NumConstantBufferTypes];
		uint32_t cboComputeStartAddress[CoreGraphicsGlobalConstantBufferType::NumConstantBufferTypes];
		uint32_t cboComputeEndAddress[CoreGraphicsGlobalConstantBufferType::NumConstantBufferTypes];
	};

	Util::FixedArray<ConstantsRingBuffer> frameSubmissions;
	uint maxNumFrameSubmissions;
	uint32_t currentFrameSubmission;

	VkExtensionProperties physicalExtensions[64];

	uint32_t usedPhysicalExtensions;
	const char* deviceExtensionStrings[64];

	uint32_t usedExtensions;
	const char* extensions[64];

	uint32_t numQueues;
	VkQueueFamilyProperties queuesProps[64];

	uint32_t drawQueueFamily;
	uint32_t computeQueueFamily;
	uint32_t transferQueueFamily;
	uint32_t sparseQueueFamily;
	uint32_t drawQueueIdx;
	uint32_t computeQueueIdx;
	uint32_t transferQueueIdx;
	uint32_t sparseQueueIdx;
	Util::Set<uint32_t> usedQueueFamilies;
	Util::FixedArray<uint32_t> queueFamilyMap;

	// setup management classes
	VkSubContextHandler subcontextHandler;
	VkPipelineDatabase database;

	// device handling (multi GPU?!?!)
	Util::FixedArray<VkDevice> devices;
	Util::FixedArray<VkPhysicalDevice> physicalDevices;
	Util::FixedArray<VkPhysicalDeviceProperties> deviceProps;
	Util::FixedArray<VkPhysicalDeviceFeatures> deviceFeatures;
	Util::FixedArray<uint32_t> numCaps;
	Util::FixedArray<Util::FixedArray<VkExtensionProperties>> caps;
	Util::FixedArray<Util::FixedArray<const char*>> deviceFeatureStrings;
	IndexT currentDevice;

	CoreGraphics::ShaderProgramId currentShaderProgram;
	CoreGraphics::ShaderFeature::Mask currentShaderMask;

	VkGraphicsPipelineCreateInfo currentPipelineInfo;
	VkPipelineLayout currentPipelineLayout;
	VkPipeline currentPipeline;
	VkPipelineInfoBits currentPipelineBits;

	static const SizeT MaxClipSettings = 8;
	uint32_t numViewports;
	VkViewport viewports[MaxClipSettings];
	uint32_t numScissors;
	VkRect2D scissors[MaxClipSettings];
	bool viewportsDirty[NumDrawThreads];
	bool scissorsDirty[NumDrawThreads];

	uint32_t currentProgram;

	_declare_counter(NumImageBytesAllocated);
	_declare_counter(NumBufferBytesAllocated);
	_declare_counter(NumBytesAllocated);
	_declare_counter(NumPipelinesBuilt);
	_declare_counter(GraphicsDeviceNumComputes);
	_declare_counter(GraphicsDeviceNumPrimitives);
	_declare_counter(GraphicsDeviceNumDrawCalls);
	_declare_timer(DebugTimer);

} state;

VkDebugUtilsMessengerEXT VkDebugMessageHandle = nullptr;
PFN_vkCreateDebugUtilsMessengerEXT VkCreateDebugMessenger = nullptr;
PFN_vkDestroyDebugUtilsMessengerEXT VkDestroyDebugMessenger = nullptr;

PFN_vkSetDebugUtilsObjectNameEXT VkDebugObjectName = nullptr;
PFN_vkSetDebugUtilsObjectTagEXT VkDebugObjectTag = nullptr;
PFN_vkQueueBeginDebugUtilsLabelEXT VkQueueBeginLabel = nullptr;
PFN_vkQueueEndDebugUtilsLabelEXT VkQueueEndLabel = nullptr;
PFN_vkQueueInsertDebugUtilsLabelEXT VkQueueInsertLabel = nullptr;
PFN_vkCmdBeginDebugUtilsLabelEXT VkCmdDebugMarkerBegin = nullptr;
PFN_vkCmdEndDebugUtilsLabelEXT VkCmdDebugMarkerEnd = nullptr;
PFN_vkCmdInsertDebugUtilsLabelEXT VkCmdDebugMarkerInsert = nullptr;
//------------------------------------------------------------------------------
/**
*/
void
SetupAdapter()
{
	// retrieve available GPUs
	uint32_t gpuCount;
	VkResult res;
	res = vkEnumeratePhysicalDevices(state.instance, &gpuCount, NULL);
	n_assert(res == VK_SUCCESS);

	state.devices.Resize(gpuCount);
	state.physicalDevices.Resize(gpuCount);
	state.numCaps.Resize(gpuCount);
	state.caps.Resize(gpuCount);
	state.deviceFeatureStrings.Resize(gpuCount);
	state.deviceProps.Resize(gpuCount);
	state.deviceFeatures.Resize(gpuCount);

	if (gpuCount > 0)
	{
		res = vkEnumeratePhysicalDevices(state.instance, &gpuCount, state.physicalDevices.Begin());
		n_assert(res == VK_SUCCESS);

		if (gpuCount > 1)
			n_message("Found %d GPUs, which is more than 1! Perhaps the Render Device should be able to use it?\n", gpuCount);

		IndexT i;
		for (i = 0; i < (IndexT)gpuCount; i++)
		{
			res = vkEnumerateDeviceExtensionProperties(state.physicalDevices[i], nullptr, &state.numCaps[i], nullptr);
			n_assert(res == VK_SUCCESS);

			if (state.numCaps[i] > 0)
			{
				state.caps[i].Resize(state.numCaps[i]);
				state.deviceFeatureStrings[i].Resize(state.numCaps[i]);

				res = vkEnumerateDeviceExtensionProperties(state.physicalDevices[i], nullptr, &state.numCaps[i], state.caps[i].Begin());
				n_assert(res == VK_SUCCESS);

				uint32_t newNumCaps = 0;
				for (uint32_t j = 0; j < state.numCaps[i]; j++)
				{
					Util::String str(state.caps[i][j].extensionName);

					// only load khronos extensions
					if (str.BeginsWithString("VK_KHR_"))
						state.deviceFeatureStrings[i][newNumCaps++] = state.caps[i][j].extensionName;
				}
				state.numCaps[i] = newNumCaps;
				state.deviceFeatureStrings[i].Resize(newNumCaps);
			}

			// get device props and features
			vkGetPhysicalDeviceProperties(state.physicalDevices[0], &state.deviceProps[i]);
			vkGetPhysicalDeviceFeatures(state.physicalDevices[0], &state.deviceFeatures[i]);
		}
	}
	else
	{
		n_error("VkGraphicsDevice::SetupAdapter(): No GPU available.\n");
	}
}

//------------------------------------------------------------------------------
/**
*/
VkInstance
GetInstance()
{
	return state.instance;
}

//------------------------------------------------------------------------------
/**
*/
VkDevice
GetCurrentDevice()
{
	return state.devices[state.currentDevice];
}

//------------------------------------------------------------------------------
/**
*/
VkPhysicalDevice
GetCurrentPhysicalDevice()
{
	return state.physicalDevices[state.currentDevice];
}

//------------------------------------------------------------------------------
/**
*/
VkPhysicalDeviceProperties
GetCurrentProperties()
{
	return state.deviceProps[state.currentDevice];
}

//------------------------------------------------------------------------------
/**
*/
VkPhysicalDeviceFeatures 
GetCurrentFeatures()
{
	return state.deviceFeatures[state.currentDevice];;
}

//------------------------------------------------------------------------------
/**
*/
VkPipelineCache 
GetPipelineCache()
{
	return state.cache;
}

//------------------------------------------------------------------------------
/**
*/
VkPhysicalDeviceMemoryProperties
GetMemoryProperties()
{
	return state.memoryProps;
}

//------------------------------------------------------------------------------
/**
*/
VkCommandBuffer 
GetMainBuffer(const CoreGraphicsQueueType queue)
{
	switch (queue)
	{
	case GraphicsQueueType: return CommandBufferGetVk(state.gfxCmdBuffer);
	case ComputeQueueType: return CommandBufferGetVk(state.computeCmdBuffer);
	}
	return VK_NULL_HANDLE;
}

//------------------------------------------------------------------------------
/**
*/
VkSemaphore 
GetPresentSemaphore()
{
	return SemaphoreGetVk(state.gfxSemaphore);
}

//------------------------------------------------------------------------------
/**
*/
VkFence 
GetPresentFence()
{
	return FenceGetVk(state.gfxFence);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Set<uint32_t>& 
GetQueueFamilies()
{
	return state.usedQueueFamilies;
}

//------------------------------------------------------------------------------
/**
*/
const uint32_t 
GetQueueFamily(const CoreGraphicsQueueType type)
{
	return state.queueFamilyMap[type];
}

//------------------------------------------------------------------------------
/**
*/
const VkQueue 
GetQueue(const CoreGraphicsQueueType type, const IndexT index)
{
	switch (type)
	{
	case GraphicsQueueType:
		return state.subcontextHandler.drawQueues[index];
		break;
	case ComputeQueueType:
		return state.subcontextHandler.computeQueues[index];
		break;
	case TransferQueueType:
		return state.subcontextHandler.transferQueues[index];
		break;
	case SparseQueueType:
		return state.subcontextHandler.sparseQueues[index];
		break;
	}
	return VK_NULL_HANDLE;
}

//------------------------------------------------------------------------------
/**
*/
const VkQueue 
GetCurrentQueue(const CoreGraphicsQueueType type)
{
	return state.subcontextHandler.GetQueue(type);
}

//------------------------------------------------------------------------------
/**
*/
void 
Copy(const VkImage from, Math::rectangle<SizeT> fromRegion, const VkImage to, Math::rectangle<SizeT> toRegion)
{
	n_assert(!state.inBeginPass);
	VkImageCopy region;
	region.dstOffset = { fromRegion.left, fromRegion.top, 0 };
	region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	region.extent = { (uint32_t)toRegion.width(), (uint32_t)toRegion.height(), 1 };
	region.srcOffset = { toRegion.left, toRegion.top, 0 };
	region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	vkCmdCopyImage(GetMainBuffer(GraphicsQueueType), from, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, to, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

//------------------------------------------------------------------------------
/**
*/
void 
Blit(const VkImage from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const VkImage to, Math::rectangle<SizeT> toRegion, IndexT toMip)
{
	n_assert(!state.inBeginPass);
	VkImageBlit blit;
	blit.srcOffsets[0] = { fromRegion.left, fromRegion.top, 0 };
	blit.srcOffsets[1] = { fromRegion.right, fromRegion.bottom, 1 };
	blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)fromMip, 0, 1 };
	blit.dstOffsets[0] = { toRegion.left, toRegion.top, 0 };
	blit.dstOffsets[1] = { toRegion.right, toRegion.bottom, 1 };
	blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)toMip, 0, 1 };
	vkCmdBlitImage(GetMainBuffer(GraphicsQueueType), from, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, to, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
}

//------------------------------------------------------------------------------
/**
*/
void 
BindDescriptorsGraphics(const VkDescriptorSet* descriptors, uint32_t baseSet, uint32_t setCount, const uint32_t* offsets, uint32_t offsetCount, bool propagate)
{
	// if we are setting descriptors before we have a pipeline, add them for later submission
	if (state.currentProgram == -1)
	{
		VkCommandBufferThread::Command cmd;
		cmd.type = VkCommandBufferThread::BindDescriptors;
		cmd.descriptor.baseSet = baseSet;
		cmd.descriptor.numSets = setCount;
		cmd.descriptor.sets = descriptors;
		cmd.descriptor.numOffsets = offsetCount;
		cmd.descriptor.offsets = offsets;
		cmd.descriptor.type = VK_PIPELINE_BIND_POINT_GRAPHICS;
		state.propagateDescriptorSets.Append(cmd);
	}
	else
	{
		// if batching, draws goes to thread
		n_assert(state.currentProgram != -1);
		if (state.inBeginBatch)
		{
			VkCommandBufferThread::Command cmd;
			cmd.type = VkCommandBufferThread::BindDescriptors;
			cmd.descriptor.baseSet = baseSet;
			cmd.descriptor.numSets = setCount;
			cmd.descriptor.sets = descriptors;
			cmd.descriptor.numOffsets = offsetCount;
			cmd.descriptor.offsets = offsets;
			cmd.descriptor.type = VK_PIPELINE_BIND_POINT_GRAPHICS;
			PushToThread(cmd, state.currentDrawThread);
		}
		else
		{
			// otherwise they go on the main draw
			vkCmdBindDescriptorSets(GetMainBuffer(GraphicsQueueType), VK_PIPELINE_BIND_POINT_GRAPHICS, state.currentPipelineLayout, baseSet, setCount, descriptors, offsetCount, offsets);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
BindDescriptorsCompute(const VkDescriptorSet* descriptors, uint32_t baseSet, uint32_t setCount, const uint32_t* offsets, uint32_t offsetCount)
{
	n_assert(state.inBeginFrame);
	vkCmdBindDescriptorSets(GetMainBuffer(GraphicsQueueType), VK_PIPELINE_BIND_POINT_COMPUTE, state.currentPipelineLayout, baseSet, setCount, descriptors, offsetCount, offsets);
}

//------------------------------------------------------------------------------
/**
*/
void 
UpdatePushRanges(const VkShaderStageFlags& stages, const VkPipelineLayout& layout, uint32_t offset, uint32_t size, void* data)
{
	if (state.inBeginBatch)
	{
		VkCommandBufferThread::Command cmd;
		cmd.type = VkCommandBufferThread::PushRange;
		cmd.pushranges.layout = layout;
		cmd.pushranges.offset = offset;
		cmd.pushranges.size = size;
		cmd.pushranges.stages = stages;

		// copy data here, will be deleted in the thread
		cmd.pushranges.data = Memory::Alloc(Memory::ScratchHeap, size);
		memcpy(cmd.pushranges.data, data, size);
		PushToThread(cmd, state.currentDrawThread);
	}
	else
	{
		vkCmdPushConstants(GetMainBuffer(GraphicsQueueType), layout, stages, offset, size, data);
	}
	
}

//------------------------------------------------------------------------------
/**
*/
void 
BindGraphicsPipelineInfo(const VkGraphicsPipelineCreateInfo& shader, const uint32_t programId)
{
	if (state.currentProgram != programId || !(state.currentPipelineBits & ShaderInfoSet))
	{
		state.database.SetShader(programId, shader);
		state.currentPipelineBits |= ShaderInfoSet;

		state.blendInfo.pAttachments = shader.pColorBlendState->pAttachments;
		memcpy(state.blendInfo.blendConstants, shader.pColorBlendState->blendConstants, sizeof(float) * 4);
		state.blendInfo.logicOp = shader.pColorBlendState->logicOp;
		state.blendInfo.logicOpEnable = shader.pColorBlendState->logicOpEnable;

		state.currentPipelineInfo.pDepthStencilState = shader.pDepthStencilState;
		state.currentPipelineInfo.pRasterizationState = shader.pRasterizationState;
		state.currentPipelineInfo.pMultisampleState = shader.pMultisampleState;
		state.currentPipelineInfo.pDynamicState = shader.pDynamicState;
		state.currentPipelineInfo.pTessellationState = shader.pTessellationState;
		state.currentPipelineInfo.stageCount = shader.stageCount;
		state.currentPipelineInfo.pStages = shader.pStages;
		state.currentPipelineInfo.layout = shader.layout;
		state.currentPipelineBits &= ~PipelineBuilt;
		state.currentProgram = programId;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
SetVertexLayoutPipelineInfo(VkPipelineVertexInputStateCreateInfo* vertexLayout)
{
	if (state.currentPipelineInfo.pVertexInputState != vertexLayout || !(state.currentPipelineBits & VertexLayoutInfoSet))
	{
		state.database.SetVertexLayout(vertexLayout);
		state.currentPipelineBits |= VertexLayoutInfoSet;
		state.currentPipelineInfo.pVertexInputState = vertexLayout;

		state.currentPipelineBits &= ~PipelineBuilt;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
SetFramebufferLayoutInfo(const VkGraphicsPipelineCreateInfo& framebufferLayout)
{
	state.currentPipelineBits |= FramebufferLayoutInfoSet;
	state.currentPipelineInfo.renderPass = framebufferLayout.renderPass;
	state.currentPipelineInfo.subpass = framebufferLayout.subpass;
	state.currentPipelineInfo.pViewportState = framebufferLayout.pViewportState;
	state.currentPipelineBits &= ~PipelineBuilt;
}

//------------------------------------------------------------------------------
/**
*/
void 
SetInputLayoutInfo(VkPipelineInputAssemblyStateCreateInfo* inputLayout)
{
	if (state.currentPipelineInfo.pInputAssemblyState != inputLayout || !(state.currentPipelineBits & InputLayoutInfoSet))
	{
		state.database.SetInputLayout(inputLayout);
		state.currentPipelineBits |= InputLayoutInfoSet;
		state.currentPipelineInfo.pInputAssemblyState = inputLayout;
		state.currentPipelineBits &= ~PipelineBuilt;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
CreateAndBindGraphicsPipeline()
{
	VkPipeline pipeline = state.database.GetCompiledPipeline();
	_incr_counter(state.NumPipelinesBuilt, 1);

	if (state.inBeginBatch)
	{ 
		VkCommandBufferThread::Command cmd;

		// send pipeline bind command, this is the first step in our procedure, so we use this as a trigger to switch threads
		cmd.type = VkCommandBufferThread::GraphicsPipeline;
		cmd.pipe.pipeline = pipeline;
		cmd.pipe.layout = state.currentPipelineLayout;
		PushToThread(cmd, state.currentDrawThread);
		
		// bind textures and camera descriptors
		VkShaderServer::Instance()->BindTextureDescriptorSetsGraphics();
		VkTransformDevice::Instance()->BindCameraDescriptorSetsGraphics();

		// push propagation descriptors
		for (IndexT i = 0; i < state.propagateDescriptorSets.Size(); i++)
			PushToThread(state.propagateDescriptorSets[i], state.currentDrawThread);

		uint32_t i;
		for (i = 0; i < state.numScissors; i++)
		{
			cmd.type = VkCommandBufferThread::ScissorRect;
			cmd.scissorRect.sc = state.scissors[i];
			cmd.scissorRect.index = i;
			PushToThread(cmd, state.currentDrawThread);
		}

		for (i = 0; i < state.numViewports; i++)
		{
			cmd.type = VkCommandBufferThread::Viewport;
			cmd.viewport.vp = state.viewports[i];
			cmd.viewport.index = i;
			PushToThread(cmd, state.currentDrawThread);
		}
		state.viewportsDirty[state.currentDrawThread] = false;
	}
	else
	{
		// bind pipeline
		vkCmdBindPipeline(GetMainBuffer(GraphicsQueueType), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	}
	
}

//------------------------------------------------------------------------------
/**
*/
void 
BindComputePipeline(const VkPipeline& pipeline, const VkPipelineLayout& layout)
{
	// bind compute pipeline
	state.currentBindPoint = CoreGraphics::ComputePipeline;

	// bind pipeline
	vkCmdBindPipeline(GetMainBuffer(GraphicsQueueType), VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

	// bind shared descriptors
	VkShaderServer::Instance()->BindTextureDescriptorSetsCompute();
	VkTransformDevice::Instance()->BindCameraDescriptorSetsCompute();
}

//------------------------------------------------------------------------------
/**
*/
void 
UnbindPipeline()
{
	state.currentBindPoint = CoreGraphics::InvalidPipeline;
	state.currentPipelineBits &= ~ShaderInfoSet;
}

//------------------------------------------------------------------------------
/**
*/
void
SetViewports(VkViewport* viewports, SizeT num)
{
	n_assert(num < state.MaxClipSettings);
	memcpy(state.viewports, viewports, sizeof(VkViewport) * num);
	state.numViewports = num;
	if (state.currentProgram != -1)
	{
		if (state.inBeginBatch)
		{
			VkCommandBufferThread::Command cmd;
			cmd.type = VkCommandBufferThread::ViewportArray;
			cmd.viewportArray.first = 0;
			cmd.viewportArray.num = num;
			cmd.viewportArray.vps = viewports;
			PushToThread(cmd, state.currentDrawThread);
		}
		else
		{
			// activate this code when we have main thread secondary buffers
			//vkCmdSetViewport(CommandBufferGetVk(state.mainCmdDrawBuffer), 0, num, viewports);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
SetScissorRects(VkRect2D* scissors, SizeT num)
{
	n_assert(num < state.MaxClipSettings);
	memcpy(state.scissors, scissors, sizeof(VkRect2D) * num);
	state.numScissors = num;
	if (state.currentProgram != -1)
	{
		if (state.inBeginBatch)
		{
			VkCommandBufferThread::Command cmd;
			cmd.type = VkCommandBufferThread::ScissorRectArray;
			cmd.scissorRectArray.first = 0;
			cmd.scissorRectArray.num = num;
			cmd.scissorRectArray.scs = scissors;
			PushToThread(cmd, state.currentDrawThread);
		}
		else
		{
			// activate this code when we have main thread secondary buffers
			//vkCmdSetScissor(CommandBufferGetVk(state.mainCmdDrawBuffer), 0, num, scissors);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmitToQueue(VkQueue queue, VkPipelineStageFlags flags, uint32_t numBuffers, VkCommandBuffer* buffers)
{
	uint32_t i;
	for (i = 0; i < numBuffers; i++)
	{
		VkResult res = vkEndCommandBuffer(buffers[i]);
		n_assert(res == VK_SUCCESS);
	}

	// submit to queue
	const VkSubmitInfo submitInfo =
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		NULL,
		0,
		NULL,
		&flags,
		numBuffers,
		buffers,
		0,
		NULL
	};

	// submit to queue
	VkResult res = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmitToQueue(VkQueue queue, VkFence fence)
{
	// submit to queue
	VkResult res = vkQueueSubmit(queue, 0, VK_NULL_HANDLE, fence);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void 
WaitForFences(VkFence* fences, uint32_t numFences, bool waitForAll)
{
	VkResult res = vkWaitForFences(state.devices[state.currentDevice], numFences, fences, waitForAll, UINT_MAX);
	n_assert(res == VK_SUCCESS);
	res = vkResetFences(state.devices[state.currentDevice], numFences, fences);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void 
BeginDrawThread()
{
	n_assert(state.numActiveThreads < state.NumDrawThreads);

	// allocate command buffer
	VkCommandBufferAllocateInfo info =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		NULL,
		state.dispatchableCmdDrawBufferPool[state.currentDrawThread],
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,
		1
	};
	vkAllocateCommandBuffers(state.devices[state.currentDevice], &info, &state.dispatchableDrawCmdBuffers[state.currentDrawThread]);

	// tell thread to begin command buffer recording
	VkCommandBufferBeginInfo begin =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		NULL,
		VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
		&state.passInfo
	};

	VkCommandBufferThread::Command cmd;
	cmd.type = VkCommandBufferThread::BeginCommand;
	cmd.bgCmd.buf = state.dispatchableDrawCmdBuffers[state.currentDrawThread];
	cmd.bgCmd.info = begin;
	PushToThread(cmd, state.currentDrawThread);

	// run begin command buffer pass
	state.numActiveThreads++;
}

//------------------------------------------------------------------------------
/**
*/
void 
EndDrawThreads()
{
	if (state.numActiveThreads > 0)
	{
		IndexT i;
		for (i = 0; i < state.numActiveThreads; i++)
		{
			// push remaining cmds to thread
			FlushToThread(i);

			// end thread
			VkCommandBufferThread::Command cmd;
			cmd.type = VkCommandBufferThread::EndCommand;
			PushToThread(cmd, i, false);

			cmd.type = VkCommandBufferThread::Sync;
			cmd.syncEvent = state.drawCompletionEvents[i];
			PushToThread(cmd, i, false);
			state.drawCompletionEvents[i]->Wait();
			state.drawCompletionEvents[i]->Reset();
		}

		// execute commands
		vkCmdExecuteCommands(GetMainBuffer(GraphicsQueueType), state.numActiveThreads, state.dispatchableDrawCmdBuffers);

		// get current submission
		Vulkan::GraphicsDeviceState::ConstantsRingBuffer& sub = state.frameSubmissions[state.currentFrameSubmission];
		VkDevice dev = state.devices[state.currentDevice];

		// destroy command buffers
		for (i = 0; i < state.numActiveThreads; i++)
		{
			Vulkan::SubmissionContextFreeCommandBuffer(state.gfxSubmission, dev, state.dispatchableCmdDrawBufferPool[i], state.dispatchableDrawCmdBuffers[i]);
		}
		state.currentDrawThread = state.NumDrawThreads - 1;
		state.numActiveThreads = 0;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
PushToThread(const VkCommandBufferThread::Command& cmd, const IndexT& index, bool allowStaging)
{
	//this->threadCmds[index].Append(cmd);
	if (allowStaging)
	{
		state.threadCmds[index].Append(cmd);
		if (state.threadCmds[index].Size() == 1500)
		{
			state.drawThreads[index]->PushCommands(state.threadCmds[index]);
			state.threadCmds[index].Reset();
		}
	}
	else
	{
		state.drawThreads[index]->PushCommand(cmd);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
FlushToThread(const IndexT& index)
{
	if (!state.threadCmds[index].IsEmpty())
	{
		state.drawThreads[index]->PushCommands(state.threadCmds[index]);
		state.threadCmds[index].Clear();
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
BindSharedDescriptorSets()
{

}

#if NEBULA_GRAPHICS_DEBUG
//------------------------------------------------------------------------------
/**
*/
void 
CommandBufferBeginMarker(VkCommandBuffer buf, const Math::float4& color, const char* name)
{
	alignas(16) float col[4];
	color.store(col);
	VkDebugUtilsLabelEXT info =
	{
		VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		nullptr,
		name,
		{ col[0], col[1], col[2], col[3] }
	};
	VkCmdDebugMarkerBegin(buf, &info);
}

//------------------------------------------------------------------------------
/**
*/
void 
CommandBufferEndMarker(VkCommandBuffer buf)
{
	VkCmdDebugMarkerEnd(buf);
}
#endif

} // namespace Vulkan

//------------------------------------------------------------------------------
/**
*/
VKAPI_ATTR VkBool32 VKAPI_CALL
NebulaVulkanDebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
	void* userData)
{
	const int32_t ignore[] =
	{
		61 // unused descriptors 
	};

	for (IndexT i = 0; i < sizeof(ignore) / sizeof(int32_t); i++)
	{
		if (callbackData->messageIdNumber == ignore[i]) 
			return VK_FALSE;
	}

	if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		n_warning("VULKAN ERROR: %s\n", callbackData->pMessage);
	}
	else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		n_warning("VULKAN WARNING: %s\n", callbackData->pMessage);
	}
	return VK_FALSE;
}


namespace CoreGraphics
{
using namespace Vulkan;

#if NEBULA_GRAPHICS_DEBUG
template<> void ObjectSetName(const CoreGraphics::CommandBufferId id, const Util::String& name);
#endif

//------------------------------------------------------------------------------
/**
*/
bool 
CreateGraphicsDevice(const GraphicsDeviceCreateInfo& info)
{
	DisplayDevice* displayDevice = DisplayDevice::Instance();
	n_assert(displayDevice->IsOpen());

	state.enableValidation = info.enableValidation;

	// create result
	VkResult res;

	// setup application
	VkApplicationInfo appInfo =
	{
		VK_STRUCTURE_TYPE_APPLICATION_INFO,
		NULL,
		App::Application::Instance()->GetAppTitle().AsCharPtr(),
		2,																// application version
		"Nebula Trifid",												// engine name
		2,																// engine version
		VK_MAKE_VERSION(1, 1, VK_HEADER_VERSION)						// API version
	};

	state.usedExtensions = 0;
	uint32_t requiredExtensionsNum;
	const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsNum);
	uint32_t i;
	for (i = 0; i < (uint32_t)requiredExtensionsNum; i++)
	{
		state.extensions[state.usedExtensions++] = requiredExtensions[i];
	}

	const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
	int numLayers = 0;
	const char** usedLayers = nullptr;

#if NEBULA_GRAPHICS_DEBUG
	if (info.enableValidation)
	{
		usedLayers = &layers[0];
		numLayers = 1;
	}
	else
	{
		// don't use any layers, but still load the debug utils so we can put markers
		numLayers = 0;
	}
	state.extensions[state.usedExtensions++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#else
	if (info.enableValidation)
	{
		usedLayers = &layers[0];
		numLayers = 1;
		state.extensions[state.usedExtensions++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	}
#endif

	// setup instance
	VkInstanceCreateInfo instanceInfo =
	{
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,		// type of struct
		NULL,										// pointer to next
		0,											// flags
		&appInfo,									// application
		numLayers,
		usedLayers,
		state.usedExtensions,
		state.extensions
	};

	// create instance
	res = vkCreateInstance(&instanceInfo, NULL, &state.instance);
	if (res == VK_ERROR_INCOMPATIBLE_DRIVER)
	{
		n_error("VkGraphicsDevice::OpenVulkanContext(): Your GPU driver is not compatible with Vulkan.\n");
	}
	else if (res == VK_ERROR_EXTENSION_NOT_PRESENT)
	{
		n_error("VkGraphicsDevice::OpenVulkanContext(): Vulkan extension failed to load.\n");
	}
	else if (res == VK_ERROR_LAYER_NOT_PRESENT)
	{
		n_error("VkGraphicsDevice::OpenVulkanContext(): Vulkan layer failed to load.\n");
	}
	n_assert(res == VK_SUCCESS);

	// setup adapter
	SetupAdapter();
	state.currentDevice = 0;

#if NEBULA_GRAPHICS_DEBUG
#else
	if (info.enableValidation)
#endif
	{
		VkCreateDebugMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(state.instance, "vkCreateDebugUtilsMessengerEXT");
		VkDestroyDebugMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(state.instance, "vkDestroyDebugUtilsMessengerEXT");
		VkDebugUtilsMessengerCreateInfoEXT dbgInfo;
		dbgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		dbgInfo.flags = 0;
		dbgInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		dbgInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		dbgInfo.pNext = nullptr;
		dbgInfo.pfnUserCallback = NebulaVulkanDebugCallback;
		dbgInfo.pUserData = nullptr;
		res = VkCreateDebugMessenger(state.instance, &dbgInfo, NULL, &VkDebugMessageHandle);
		n_assert(res == VK_SUCCESS);
	}

#if NEBULA_GRAPHICS_DEBUG
	VkDebugObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(state.instance, "vkSetDebugUtilsObjectNameEXT");
	VkDebugObjectTag = (PFN_vkSetDebugUtilsObjectTagEXT)vkGetInstanceProcAddr(state.instance, "vkSetDebugUtilsObjectTagEXT");
	VkQueueBeginLabel = (PFN_vkQueueBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(state.instance, "vkQueueBeginDebugUtilsLabelEXT");
	VkQueueEndLabel = (PFN_vkQueueEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(state.instance, "vkQueueEndDebugUtilsLabelEXT");
	VkQueueInsertLabel = (PFN_vkQueueInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(state.instance, "vkQueueInsertDebugUtilsLabelEXT");
	VkCmdDebugMarkerBegin = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(state.instance, "vkCmdBeginDebugUtilsLabelEXT");
	VkCmdDebugMarkerEnd = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(state.instance, "vkCmdEndDebugUtilsLabelEXT");
	VkCmdDebugMarkerInsert = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(state.instance, "vkCmdInsertDebugUtilsLabelEXT");
#endif

	vkGetPhysicalDeviceQueueFamilyProperties(state.physicalDevices[state.currentDevice], &state.numQueues, NULL);
	n_assert(state.numQueues > 0);

	// now get queues from device
	vkGetPhysicalDeviceQueueFamilyProperties(state.physicalDevices[state.currentDevice], &state.numQueues, state.queuesProps);
	vkGetPhysicalDeviceMemoryProperties(state.physicalDevices[state.currentDevice], &state.memoryProps);

	state.drawQueueIdx = UINT32_MAX;
	state.computeQueueIdx = UINT32_MAX;
	state.transferQueueIdx = UINT32_MAX;
	state.sparseQueueIdx = UINT32_MAX;

	// create three queues for each family
	Util::FixedArray<uint> indexMap;
	indexMap.Resize(state.numQueues);
	indexMap.Fill(0);
	for (i = 0; i < state.numQueues; i++)
	{

		if (state.queuesProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && state.drawQueueIdx == UINT32_MAX)
		{
			if (state.queuesProps[i].queueCount == indexMap[i]) continue;
			state.drawQueueFamily = i;
			state.drawQueueIdx = indexMap[i]++;
		}

		// compute queues may not support graphics
		if (state.queuesProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT && state.computeQueueIdx == UINT32_MAX)
		{
			if (state.queuesProps[i].queueCount == indexMap[i]) continue;
			state.computeQueueFamily = i;
			state.computeQueueIdx = indexMap[i]++;
		}

		// transfer queues may not support compute or graphics
		if (state.queuesProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT &&
			!(state.queuesProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
			!(state.queuesProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
			state.transferQueueIdx == UINT32_MAX)
		{
			if (state.queuesProps[i].queueCount == indexMap[i]) continue;
			state.transferQueueFamily = i;
			state.transferQueueIdx = indexMap[i]++;
		}

		// sparse queues may not support compute or graphics
		if (state.queuesProps[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT &&
			!(state.queuesProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
			!(state.queuesProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
			state.sparseQueueIdx == UINT32_MAX)
		{
			if (state.queuesProps[i].queueCount == indexMap[i]) continue;
			state.sparseQueueFamily = i;
			state.sparseQueueIdx = indexMap[i]++;
		}
	}

	if (state.transferQueueIdx == UINT32_MAX)
	{
		// assert the draw queue can do both transfers and computes
		n_assert(state.queuesProps[state.drawQueueFamily].queueFlags & VK_QUEUE_TRANSFER_BIT);
		n_assert(state.queuesProps[state.drawQueueFamily].queueFlags & VK_QUEUE_COMPUTE_BIT);

		// this is actually sub-optimal, but on my AMD card, using the compute queue transfer or the sparse queue doesn't work
		state.transferQueueFamily = state.drawQueueFamily;
		state.transferQueueIdx = state.drawQueueIdx;
		//state.transferQueueFamily = 2;
		//state.transferQueueIdx = indexMap[2]++;
	}

	if (state.drawQueueFamily == UINT32_MAX)		n_error("VkDisplayDevice: Could not find a queue for graphics and present.\n");
	if (state.computeQueueFamily == UINT32_MAX)		n_error("VkDisplayDevice: Could not find a queue for compute.\n");
	if (state.transferQueueFamily == UINT32_MAX)	n_error("VkDisplayDevice: Could not find a queue for transfers.\n");
	if (state.sparseQueueFamily == UINT32_MAX)		n_warning("VkDisplayDevice: Could not find a queue for sparse binding.\n");

	// create device
	Util::FixedArray<Util::FixedArray<float>> prios;
	Util::Array<VkDeviceQueueCreateInfo> queueInfos;
	prios.Resize(state.numQueues);

	for (i = 0; i < state.numQueues; i++)
	{
		if (indexMap[i] == 0) continue;
		prios[i].Resize(indexMap[i]);
		prios[i].Fill(1.0f);
		queueInfos.Append(
		{
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			NULL,
			0,
			i,
			indexMap[i],
			&prios[i][0]
		});
	}

	// get physical device features
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(state.physicalDevices[state.currentDevice], &features);

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(state.physicalDevices[state.currentDevice], &props);

	VkDeviceCreateInfo deviceInfo =
	{
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		NULL,
		0,
		(uint32_t)queueInfos.Size(),
		&queueInfos[0],
		numLayers,
		layers,
		state.numCaps[state.currentDevice],
		state.deviceFeatureStrings[state.currentDevice].Begin(),
		&features
	};

	// create device
	res = vkCreateDevice(state.physicalDevices[state.currentDevice], &deviceInfo, NULL, &state.devices[state.currentDevice]);
	n_assert(res == VK_SUCCESS);

	// setup queue handler
	Util::FixedArray<uint> families(4);
	families[GraphicsQueueType] = state.drawQueueFamily;
	families[ComputeQueueType] = state.computeQueueFamily;
	families[TransferQueueType] = state.transferQueueFamily;
	families[SparseQueueType] = state.sparseQueueFamily;
	state.subcontextHandler.Setup(state.devices[state.currentDevice], indexMap, families);

	state.usedQueueFamilies.Add(state.drawQueueFamily);
	state.usedQueueFamilies.Add(state.computeQueueFamily);
	state.usedQueueFamilies.Add(state.transferQueueFamily);
	state.usedQueueFamilies.Add(state.sparseQueueFamily);
	state.queueFamilyMap.Resize(NumQueueTypes);
	state.queueFamilyMap[GraphicsQueueType] = state.drawQueueFamily;
	state.queueFamilyMap[ComputeQueueType] = state.computeQueueFamily;
	state.queueFamilyMap[TransferQueueType] = state.transferQueueFamily;
	state.queueFamilyMap[SparseQueueType] = state.sparseQueueFamily;

	VkPipelineCacheCreateInfo cacheInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
		NULL,
		0,
		0,
		NULL
	};

	// create cache
	res = vkCreatePipelineCache(state.devices[state.currentDevice], &cacheInfo, NULL, &state.cache);
	n_assert(res == VK_SUCCESS);

	// setup our own pipeline database
	state.database.Setup(state.devices[state.currentDevice], state.cache);

	// setup the empty descriptor set
	SetupEmptyDescriptorSetLayout();

	// setup pools (from VkCmdBuffer.h)
	SetupVkPools(state.devices[state.currentDevice], state.drawQueueFamily, state.computeQueueFamily, state.transferQueueFamily, state.sparseQueueFamily);

	CommandBufferCreateInfo cmdCreateInfo =
	{
		false,
		false,
		true,
		InvalidCommandUsage
	};

	state.frameSubmissions.Resize(info.numBufferedFrames);

#ifdef CreateSemaphore
#pragma push_macro("CreateSemaphore")
#undef CreateSemaphore
#endif

	for (i = 0; i < info.numBufferedFrames; i++)
	{
		Vulkan::GraphicsDeviceState::ConstantsRingBuffer& sub = state.frameSubmissions[i];

		IndexT j;
		for (j = 0; j < NumConstantBufferTypes; j++)
		{
			sub.cboComputeStartAddress[j] = sub.cboComputeEndAddress[j] = 0;
			sub.cboGfxStartAddress[j] = sub.cboGfxEndAddress[j] = 0;
		}
	}

	for (i = 0; i < NumConstantBufferTypes; i++)
	{
		static const Util::String threadName[] = { "Main Thread ", "Visibility Thread " };
		static const Util::String queueName[] = { "Graphics Constant Buffer", "Compute Constant Buffer" };
		ConstantBufferCreateInfo cboInfo =
		{
			"",
			-1,
			0,
			CoreGraphics::ConstantBufferUpdateMode::ManualFlush
		};

		cboInfo.name = threadName[i] + queueName[0];
		cboInfo.size = info.globalGraphicsConstantBufferMemorySize[i] * info.numBufferedFrames;
		if (cboInfo.size > 0)
		{
			state.globalGraphicsConstantBuffer[i] = CreateConstantBuffer(cboInfo);
			state.globalGraphicsConstantBufferMaxValue[i] = info.globalGraphicsConstantBufferMemorySize[i];
		}	

		cboInfo.name = threadName[i] + queueName[1];
		cboInfo.size = info.globalComputeConstantBufferMemorySize[i] * info.numBufferedFrames;
		if (cboInfo.size > 0)
		{
			state.globalComputeConstantBuffer[i] = CreateConstantBuffer(cboInfo);
			state.globalComputeConstantBufferMaxValue[i] = info.globalComputeConstantBufferMemorySize[i];
		}	
	}

	cmdCreateInfo.usage = CommandGfx;
	state.gfxSubmission = CreateSubmissionContext({ cmdCreateInfo, info.numBufferedFrames, true });
	state.gfxCmdBuffer = CommandBufferId::Invalid();
	state.gfxSemaphore = SemaphoreId::Invalid();

	cmdCreateInfo.usage = CommandCompute;
	state.computeSubmission = CreateSubmissionContext({ cmdCreateInfo, info.numBufferedFrames, true });
	state.computeCmdBuffer = CommandBufferId::Invalid();
	state.computeSemaphore = SemaphoreId::Invalid();

	CommandBufferBeginInfo beginInfo{ true, false, false };

	// create setup submission and transfer submission
	cmdCreateInfo.usage = CommandTransfer;
	state.resourceSubmissionContext = CreateSubmissionContext({ cmdCreateInfo, info.numBufferedFrames, true });
	state.resourceSubmissionFence = SubmissionContextNextCycle(state.resourceSubmissionContext);
	SubmissionContextNewBuffer(state.resourceSubmissionContext, state.resourceSubmissionCmdBuffer, state.resourceSubmissionSemaphore);

	// start the buffer, but the actual, end record - new cycle - start record, stuff will take place in frame end
	CommandBufferBeginRecord(state.resourceSubmissionCmdBuffer, beginInfo);

	cmdCreateInfo.usage = CommandGfx;
	state.setupSubmissionContext = CreateSubmissionContext({ cmdCreateInfo, info.numBufferedFrames, true });
	state.setupSubmissionFence = SubmissionContextNextCycle(state.setupSubmissionContext);
	SubmissionContextNewBuffer(state.setupSubmissionContext, state.setupSubmissionCmdBuffer, state.setupSubmissionSemaphore);

	// start the buffer, but the actual, end record - new cycle - start record, stuff will take place in frame end
	CommandBufferBeginRecord(state.setupSubmissionCmdBuffer, beginInfo);

#pragma pop_macro("CreateSemaphore")

	state.maxNumFrameSubmissions = info.numBufferedFrames;

	Util::String threadName;
	for (i = 0; i < state.NumDrawThreads; i++)
	{
		threadName.Format("DrawCmdBufferThread%d", i);
		state.drawThreads[i] = VkCommandBufferThread::Create();
		state.drawThreads[i]->SetPriority(Threading::Thread::High);
		state.drawThreads[i]->SetThreadAffinity(System::Cpu::Core5);
		state.drawThreads[i]->SetName(threadName);
		state.drawThreads[i]->Start();

		state.drawCompletionEvents[i] = n_new(Threading::Event(true));

		VkCommandPoolCreateInfo info =
		{
			VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			nullptr,
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
			state.drawQueueFamily
		};
		VkResult res = vkCreateCommandPool(state.devices[state.currentDevice], &info, nullptr, &state.dispatchableCmdDrawBufferPool[i]);
		n_assert(res == VK_SUCCESS);
	}

	// setup compute threads
	for (i = 0; i < state.NumComputeThreads; i++)
	{
		threadName.Format("ComputeCmdBufferThread%d", i);
		state.compThreads[i] = VkCommandBufferThread::Create();
		state.compThreads[i]->SetPriority(Threading::Thread::Low);
		state.compThreads[i]->SetThreadAffinity(System::Cpu::Core5);
		state.compThreads[i]->SetName(threadName);
		state.compThreads[i]->Start();

		state.compCompletionEvents[i] = n_new(Threading::Event(true));

		VkCommandPoolCreateInfo info =
		{
			VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			nullptr,
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
			state.computeQueueFamily
		};
		VkResult res = vkCreateCommandPool(state.devices[state.currentDevice], &info, nullptr, &state.dispatchableCmdCompBufferPool[i]);
		n_assert(res == VK_SUCCESS);
	}

	// setup transfer threads
	for (i = 0; i < state.NumTransferThreads; i++)
	{
		threadName.Format("TransferCmdBufferThread%d", i);
		state.transThreads[i] = VkCommandBufferThread::Create();
		state.transThreads[i]->SetPriority(Threading::Thread::Low);
		state.transThreads[i]->SetThreadAffinity(System::Cpu::Core5);
		state.transThreads[i]->SetName(threadName);
		state.transThreads[i]->Start();

		state.transCompletionEvents[i] = n_new(Threading::Event(true));

		VkCommandPoolCreateInfo info =
		{
			VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			nullptr,
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
			state.transferQueueFamily
		};
		VkResult res = vkCreateCommandPool(state.devices[state.currentDevice], &info, nullptr, &state.dispatchableCmdTransBufferPool[i]);
		n_assert(res == VK_SUCCESS);
	}

	VkFenceCreateInfo fenceInfo =
	{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		nullptr,
		0
	};

	res = vkCreateFence(state.devices[state.currentDevice], &fenceInfo, nullptr, &state.mainCmdDrawFence);
	n_assert(res == VK_SUCCESS);
	res = vkCreateFence(state.devices[state.currentDevice], &fenceInfo, nullptr, &state.mainCmdCmpFence);
	n_assert(res == VK_SUCCESS);
	res = vkCreateFence(state.devices[state.currentDevice], &fenceInfo, nullptr, &state.mainCmdTransFence);
	n_assert(res == VK_SUCCESS);

	state.passInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		nullptr,
		0,
	};

	// setup pipeline construction struct
	state.currentPipelineInfo.pNext = nullptr;
	state.currentPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	state.currentPipelineInfo.flags = 0;
	state.currentPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	state.currentPipelineInfo.basePipelineIndex = -1;
	state.currentPipelineInfo.pColorBlendState = &state.blendInfo;

	state.inputInfo.pNext = nullptr;
	state.inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	state.inputInfo.flags = 0;

	state.blendInfo.pNext = nullptr;
	state.blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	state.blendInfo.flags = 0;

	// reset state
	state.inputInfo.topology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	state.currentProgram = -1;
	state.currentPipelineInfo.pVertexInputState = nullptr;
	state.currentPipelineInfo.pInputAssemblyState = nullptr;

	state.inBeginAsyncCompute = false;
	state.inBeginBatch = false;
	state.inBeginCompute = false;
	state.inBeginFrame = false;
	state.inBeginPass = false;

	state.currentDrawThread = 0;
	state.numActiveThreads = 0;

	_setup_timer(state.DebugTimer);
	_setup_counter(state.NumImageBytesAllocated);
	_begin_counter(state.NumImageBytesAllocated);
	_setup_counter(state.NumBufferBytesAllocated);
	_begin_counter(state.NumBufferBytesAllocated);
	_setup_counter(state.NumBytesAllocated);
	_begin_counter(state.NumBytesAllocated);
	_setup_counter(state.NumPipelinesBuilt);
	_begin_counter(state.NumPipelinesBuilt);
    _setup_counter(state.GraphicsDeviceNumComputes);
    _begin_counter(state.GraphicsDeviceNumComputes);
    _setup_counter(state.GraphicsDeviceNumPrimitives);
    _begin_counter(state.GraphicsDeviceNumPrimitives);
    _setup_counter(state.GraphicsDeviceNumDrawCalls);
    _begin_counter(state.GraphicsDeviceNumDrawCalls);

	// yay, Vulkan!
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void 
DestroyGraphicsDevice()
{
	_discard_timer(state.DebugTimer);
    _end_counter(state.GraphicsDeviceNumDrawCalls);
    _discard_counter(state.GraphicsDeviceNumDrawCalls);
    _end_counter(state.GraphicsDeviceNumPrimitives);
    _discard_counter(state.GraphicsDeviceNumPrimitives);
    _end_counter(state.GraphicsDeviceNumComputes);
    _discard_counter(state.GraphicsDeviceNumComputes);
	_end_counter(state.NumImageBytesAllocated);
	_discard_counter(state.NumImageBytesAllocated);
	_end_counter(state.NumBufferBytesAllocated);
	_discard_counter(state.NumBufferBytesAllocated);
	_end_counter(state.NumBytesAllocated);
	_discard_counter(state.NumBytesAllocated);
	_end_counter(state.NumPipelinesBuilt);
	_discard_counter(state.NumPipelinesBuilt);

	// save pipeline cache
	size_t size;
	vkGetPipelineCacheData(state.devices[0], state.cache, &size, nullptr);
	uint8_t* data = (uint8_t*)Memory::Alloc(Memory::ScratchHeap, size);
	vkGetPipelineCacheData(state.devices[0], state.cache, &size, data);
	Util::String path = Util::String::Sprintf("bin:%s_vkpipelinecache", App::Application::Instance()->GetAppTitle().AsCharPtr());
	Ptr<IO::Stream> cachedData = IO::IoServer::Instance()->CreateStream(path);
	cachedData->SetAccessMode(IO::Stream::WriteAccess);
	if (cachedData->Open())
	{
		cachedData->Write(data, (IO::Stream::Size)size);
		cachedData->Close();
	}

	IndexT i;
	for (i = 0; i < state.NumDrawThreads; i++)
	{
		state.drawThreads[i]->Stop();
		state.drawThreads[i] = nullptr;
		n_delete(state.drawCompletionEvents[i]);
	}

	for (i = 0; i < state.NumTransferThreads; i++)
	{
		state.transThreads[i]->Stop();
		state.transThreads[i] = nullptr;
		n_delete(state.transCompletionEvents[i]);
	}

	for (i = 0; i < state.NumComputeThreads; i++)
	{
		state.compThreads[i]->Stop();
		state.compThreads[i] = nullptr;
		n_delete(state.compCompletionEvents[i]);
	}

	// wait for all commands to be done first
	state.subcontextHandler.WaitIdle(GraphicsQueueType);
	state.subcontextHandler.WaitIdle(ComputeQueueType);
	state.subcontextHandler.WaitIdle(TransferQueueType);
	state.subcontextHandler.WaitIdle(SparseQueueType);

	// wait for queues and run all pending commands
	state.subcontextHandler.Discard();

	// destroy command pools
	for (i = 0; i < state.NumDrawThreads; i++)
		vkDestroyCommandPool(state.devices[state.currentDevice], state.dispatchableCmdDrawBufferPool[i], nullptr);
	for (i = 0; i < state.NumTransferThreads; i++)
		vkDestroyCommandPool(state.devices[state.currentDevice], state.dispatchableCmdTransBufferPool[i], nullptr);
	for (i = 0; i < state.NumComputeThreads; i++)
		vkDestroyCommandPool(state.devices[state.currentDevice], state.dispatchableCmdCompBufferPool[i], nullptr);

	state.database.Discard();

	// destroy pipeline
	vkDestroyPipelineCache(state.devices[state.currentDevice], state.cache, nullptr);

	// free our main buffers, our secondary buffers should be fine so the pools should be free to destroy
	DestroyVkPools(state.devices[0]);

	vkDestroyFence(state.devices[0], state.mainCmdDrawFence, nullptr);
	vkDestroyFence(state.devices[0], state.mainCmdCmpFence, nullptr);
	vkDestroyFence(state.devices[0], state.mainCmdTransFence, nullptr);

#if NEBULA_VULKAN_DEBUG
	VkDestroyDebugMessenger(state.instance, VkDebugMessageHandle, nullptr);
#endif

	vkDestroyDevice(state.devices[0], nullptr);
	vkDestroyInstance(state.instance, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
SizeT 
GetNumBufferedFrames()
{
	return state.maxNumFrameSubmissions;
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
GetBufferedFrameIndex()
{
	return state.currentFrameSubmission;
}

//------------------------------------------------------------------------------
/**
*/
void 
AttachEventHandler(const Ptr<CoreGraphics::RenderEventHandler>& h)
{
	n_assert(h.isvalid());
	n_assert(InvalidIndex == state.eventHandlers.FindIndex(h));
	n_assert(!state.inNotifyEventHandlers);
	state.eventHandlers.Append(h);
	h->OnAttach();
}

//------------------------------------------------------------------------------
/**
*/
void 
RemoveEventHandler(const Ptr<CoreGraphics::RenderEventHandler>& h)
{
	n_assert(h.isvalid());
	n_assert(!state.inNotifyEventHandlers);
	IndexT index = state.eventHandlers.FindIndex(h);
	n_assert(InvalidIndex != index);
	state.eventHandlers.EraseIndex(index);
	h->OnRemove();
}

//------------------------------------------------------------------------------
/**
*/
bool 
NotifyEventHandlers(const CoreGraphics::RenderEvent& e)
{
	n_assert(!state.inNotifyEventHandlers);
	bool handled = false;
	state.inNotifyEventHandlers = true;
	IndexT i;
	for (i = 0; i < state.eventHandlers.Size(); i++)
	{
		handled |= state.eventHandlers[i]->PutEvent(e);
	}
	state.inNotifyEventHandlers = false;
	return handled;
}

//------------------------------------------------------------------------------
/**
*/
bool
BeginFrame(IndexT frameIndex)
{
	n_assert(!state.inBeginFrame);
	n_assert(!state.inBeginPass);
	n_assert(!state.inBeginBatch);

	if (frameIndex != state.currentFrameIndex)
	{
		_begin_counter(state.GraphicsDeviceNumComputes);
		_begin_counter(state.GraphicsDeviceNumPrimitives);
		_begin_counter(state.GraphicsDeviceNumDrawCalls);
	}
	state.inBeginFrame = true;

	// update current state submission
	Vulkan::GraphicsDeviceState::ConstantsRingBuffer& sub = state.frameSubmissions[state.currentFrameSubmission];

	// set to next frame submission
	state.currentFrameSubmission = (state.currentFrameSubmission + 1) % state.maxNumFrameSubmissions;

	// cycle submissions, will wait for the fence to finish
	Vulkan::GraphicsDeviceState::ConstantsRingBuffer& nextSub = state.frameSubmissions[state.currentFrameSubmission];
	state.gfxFence = CoreGraphics::SubmissionContextNextCycle(state.gfxSubmission);
	state.computeFence = CoreGraphics::SubmissionContextNextCycle(state.computeSubmission);

	IndexT i;
	for (i = 0; i < CoreGraphicsGlobalConstantBufferType::NumConstantBufferTypes; i++)
	{
		nextSub.cboGfxStartAddress[i] = state.globalGraphicsConstantBufferMaxValue[i] * state.currentFrameSubmission;
		nextSub.cboGfxEndAddress[i] = state.globalGraphicsConstantBufferMaxValue[i] * state.currentFrameSubmission;
		nextSub.cboComputeStartAddress[i] = state.globalComputeConstantBufferMaxValue[i] * state.currentFrameSubmission;
		nextSub.cboComputeEndAddress[i] = state.globalComputeConstantBufferMaxValue[i] * state.currentFrameSubmission;
	}

	state.gfxPrevSemaphore = SemaphoreId::Invalid();
	state.computePrevSemaphore = SemaphoreId::Invalid();

	// update bindless texture descriptors
	VkShaderServer::Instance()->SubmitTextureDescriptorChanges();

	// reset current thread
	state.currentDrawThread = state.NumDrawThreads - 1;
	state.currentPipelineBits = NoInfoSet;

	return true;
}

//------------------------------------------------------------------------------
/**
*/
void 
BeginSubmission(CoreGraphicsQueueType queue, CoreGraphicsQueueType waitQueue)
{
	n_assert(state.inBeginFrame);

	if (queue == GraphicsQueueType)
	{
		// generate new buffer and semaphore
		CoreGraphics::SubmissionContextNewBuffer(state.gfxSubmission, state.gfxCmdBuffer, state.gfxSemaphore);

		// begin recording the new buffer
		const CommandBufferBeginInfo cmdInfo =
		{
			true, false, false
		};
		CommandBufferBeginRecord(state.gfxCmdBuffer, cmdInfo);

		if (waitQueue == ComputeQueueType)
			state.gfxWaitSemaphore = state.computeSemaphore;
	}
	else if (queue == ComputeQueueType)
	{
		// generate new buffer and semaphore
		CoreGraphics::SubmissionContextNewBuffer(state.computeSubmission, state.computeCmdBuffer, state.computeSemaphore);

		// begin recording the new buffer
		const CommandBufferBeginInfo cmdInfo =
		{
			true, false, false
		};
		CommandBufferBeginRecord(state.computeCmdBuffer, cmdInfo);

		if (waitQueue == GraphicsQueueType)
			state.computeWaitSemaphore = state.gfxSemaphore;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
BeginPass(const CoreGraphics::PassId pass)
{
	n_assert(state.inBeginFrame);
	n_assert(!state.inBeginPass);
	n_assert(!state.inBeginBatch);
	n_assert(state.pass == PassId::Invalid());
	state.inBeginPass = true;

	state.pass = pass;

	// set info
	SetFramebufferLayoutInfo(PassGetVkFramebufferInfo(pass));
	state.database.SetPass(pass);
	state.database.SetSubpass(0);

	const VkRenderPassBeginInfo& info = PassGetVkRenderPassBeginInfo(pass);
	vkCmdBeginRenderPass(GetMainBuffer(GraphicsQueueType), &info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	state.passInfo.framebuffer = info.framebuffer;
	state.passInfo.renderPass = info.renderPass;
	state.passInfo.subpass = 0;
	state.passInfo.pipelineStatistics = 0;
	state.passInfo.queryFlags = 0;
	state.passInfo.occlusionQueryEnable = VK_FALSE;
}

//------------------------------------------------------------------------------
/**
*/
void 
SetToNextSubpass()
{
	n_assert(state.inBeginFrame);
	n_assert(state.inBeginPass);
	n_assert(!state.inBeginBatch);
	n_assert(state.pass != PassId::Invalid());
	SetFramebufferLayoutInfo(PassGetVkFramebufferInfo(state.pass));
	state.database.SetSubpass(state.currentPipelineInfo.subpass);
	state.passInfo.subpass = state.currentPipelineInfo.subpass;
	vkCmdNextSubpass(GetMainBuffer(GraphicsQueueType), VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}

//------------------------------------------------------------------------------
/**
*/
void 
BeginBatch(Frame::FrameBatchType::Code batchType)
{
	n_assert(state.inBeginPass);
	n_assert(!state.inBeginBatch);
	n_assert(state.pass != PassId::Invalid());

	state.inBeginBatch = true;
	PassBeginBatch(state.pass, batchType);

	// set to first sub-batch which starts a new draw thread
	SetToNextSubBatch();
}

//------------------------------------------------------------------------------
/**
*/
void 
SetToNextSubBatch()
{
	n_assert(state.inBeginBatch);

	// begin new draw thread
	state.currentDrawThread = (state.currentDrawThread + 1) % state.NumDrawThreads;
	if (state.numActiveThreads < state.NumDrawThreads)
	{
		// if we want a new thread, make one, then bind shared descriptor sets
		BeginDrawThread();
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ResetClipSettings()
{
	PassApplyClipSettings(state.pass);
}

//------------------------------------------------------------------------------
/**
*/
void 
SetStreamVertexBuffer(IndexT streamIndex, const CoreGraphics::VertexBufferId& vb, IndexT offsetVertexIndex)
{
	if (state.inBeginBatch)
	{
		VkCommandBufferThread::Command cmd;
		cmd.type = VkCommandBufferThread::InputAssemblyVertex;
		cmd.vbo.buffer = CoreGraphics::vboPool->allocator.Get<1>(vb.resourceId).buf;
		cmd.vbo.index = streamIndex;
		cmd.vbo.offset = offsetVertexIndex;
		PushToThread(cmd, state.currentDrawThread);
	}
	else
	{
		vkCmdBindVertexBuffers(GetMainBuffer(GraphicsQueueType), streamIndex, 1, &CoreGraphics::vboPool->allocator.Get<1>(vb.resourceId).buf, (VkDeviceSize*)&offsetVertexIndex);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
SetVertexLayout(const CoreGraphics::VertexLayoutId& vl)
{
	n_assert(state.currentShaderProgram != CoreGraphics::ShaderProgramId::Invalid());
	VkPipelineVertexInputStateCreateInfo* info = CoreGraphics::layoutPool->GetDerivativeLayout(vl, state.currentShaderProgram);
	SetVertexLayoutPipelineInfo(info);
}

//------------------------------------------------------------------------------
/**
*/
void 
SetIndexBuffer(const CoreGraphics::IndexBufferId& ib, IndexT offsetIndex)
{
	if (state.inBeginBatch)
	{
		VkCommandBufferThread::Command cmd;
		cmd.type = VkCommandBufferThread::InputAssemblyIndex;
		cmd.ibo.buffer = CoreGraphics::iboPool->allocator.Get<1>(ib.resourceId).buf;
		cmd.ibo.indexType = CoreGraphics::iboPool->allocator.Get<1>(ib.resourceId).type == IndexType::Index16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
		cmd.ibo.offset = offsetIndex;
		PushToThread(cmd, state.currentDrawThread);
	}
	else
	{
		vkCmdBindIndexBuffer(GetMainBuffer(GraphicsQueueType), CoreGraphics::iboPool->allocator.Get<1>(ib.resourceId).buf, offsetIndex, CoreGraphics::iboPool->allocator.Get<1>(ib.resourceId).type == IndexType::Index16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
SetPrimitiveTopology(const CoreGraphics::PrimitiveTopology::Code topo)
{
	state.primitiveTopology = topo;
	VkPrimitiveTopology comp = VkTypes::AsVkPrimitiveType(topo);
	if (state.inputInfo.topology != comp)
	{
		state.inputInfo.topology = comp;
		state.inputInfo.primitiveRestartEnable = VK_FALSE;
		SetInputLayoutInfo(&state.inputInfo);
	}
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveTopology::Code& 
GetPrimitiveTopology()
{
	return state.primitiveTopology;
}

//------------------------------------------------------------------------------
/**
*/
void 
SetPrimitiveGroup(const CoreGraphics::PrimitiveGroup& pg)
{
	state.primitiveGroup = pg;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveGroup& 
GetPrimitiveGroup()
{
	return state.primitiveGroup;
}

//------------------------------------------------------------------------------
/**
*/
void
SetShaderProgram(const CoreGraphics::ShaderProgramId& pro)
{
	const VkShaderProgramRuntimeInfo& info = CoreGraphics::shaderPool->shaderAlloc.Get<3>(pro.shaderId).Get<2>(pro.programId);
	state.currentShaderProgram = pro;
	state.currentPipelineLayout = info.layout;

	// if we are compute, we can set the pipeline straight away, otherwise we have to accumulate the infos
	if (info.type == ComputePipeline)		Vulkan::BindComputePipeline(info.pipeline, info.layout);
	else if (info.type == GraphicsPipeline)
	{
		// setup pipeline information regarding the shader state
		VkGraphicsPipelineCreateInfo ginfo =
		{
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			NULL,
			VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT,
			info.stageCount,
			info.shaderInfos,
			&info.vertexInfo,			// we only save how many vs inputs we allow here
			NULL,						// this is input type related (triangles, patches etc)
			&info.tessInfo,
			NULL,						// this is our viewport and is setup by the framebuffer
			&info.rasterizerInfo,
			&info.multisampleInfo,
			&info.depthStencilInfo,
			&info.colorBlendInfo,
			&info.dynamicInfo,
			info.layout,
			NULL,							// pass specific stuff, keep as NULL, handled by the framebuffer
			0,
			VK_NULL_HANDLE, 0				// base pipeline is kept as NULL too, because this is the base for all derivatives
		};
		Vulkan::BindGraphicsPipelineInfo(ginfo, info.uniqueId);
	}
	else
		Vulkan::UnbindPipeline();
}

//------------------------------------------------------------------------------
/**
*/
void 
SetShaderProgram(const CoreGraphics::ShaderId shaderId, const CoreGraphics::ShaderFeature::Mask mask)
{
	VkShaderPool::VkShaderRuntimeInfo& runtime = CoreGraphics::shaderPool->shaderAlloc.Get<2>(shaderId.resourceId);
	ShaderProgramId& programId = runtime.activeShaderProgram;
	VkShaderProgramAllocator& programs = CoreGraphics::shaderPool->shaderAlloc.Get<3>(shaderId.resourceId);

	// change variation if it's actually changed
	if (state.currentShaderProgram != programId && runtime.activeMask != mask)
	{
		programId = runtime.programMap[mask];
		runtime.activeMask = mask;
	}
	SetShaderProgram(programId);
}

//------------------------------------------------------------------------------
/**
*/
void
SetResourceTable(const CoreGraphics::ResourceTableId table, const IndexT slot, ShaderPipeline pipeline, const Util::FixedArray<uint>& offsets)
{
	switch (pipeline)
	{
	case GraphicsPipeline:
		Vulkan::BindDescriptorsGraphics(&ResourceTableGetVkDescriptorSet(table),
			slot,
			1,
			offsets.IsEmpty() ? nullptr : offsets.Begin(),
			offsets.Size());
		break;
	case ComputePipeline:
		Vulkan::BindDescriptorsCompute(&ResourceTableGetVkDescriptorSet(table),
			slot,
			1,
			offsets.IsEmpty() ? nullptr : offsets.Begin(),
			offsets.Size());
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
SetResourceTable(const CoreGraphics::ResourceTableId table, const IndexT slot, ShaderPipeline pipeline, uint32 numOffsets, uint32* offsets)
{
	switch (pipeline)
	{
		case GraphicsPipeline:
		Vulkan::BindDescriptorsGraphics(&ResourceTableGetVkDescriptorSet(table),
										slot,
										1,
										offsets,
										numOffsets);
		break;
		case ComputePipeline:
		Vulkan::BindDescriptorsCompute(&ResourceTableGetVkDescriptorSet(table),
									   slot,
									   1,
									   offsets,
									   numOffsets);
		break;
	}
}
//------------------------------------------------------------------------------
/**
*/
void 
SetResourceTablePipeline(const CoreGraphics::ResourcePipelineId layout)
{
	n_assert(layout != CoreGraphics::ResourcePipelineId::Invalid());
	state.currentPipelineLayout = ResourcePipelineGetVk(layout);
}

//------------------------------------------------------------------------------
/**
*/
void
PushConstants(ShaderPipeline pipeline, uint offset, uint size, byte* data)
{
	switch (pipeline)
	{
	case GraphicsPipeline:
		Vulkan::UpdatePushRanges(VK_SHADER_STAGE_ALL_GRAPHICS, state.currentPipelineLayout, offset, size, data);
		break;
	case ComputePipeline:
		Vulkan::UpdatePushRanges(VK_SHADER_STAGE_COMPUTE_BIT, state.currentPipelineLayout, offset, size, data);
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
uint 
AllocateGraphicsConstantBufferMemory(CoreGraphicsGlobalConstantBufferType type, uint size)
{
	Vulkan::GraphicsDeviceState::ConstantsRingBuffer& sub = state.frameSubmissions[state.currentFrameSubmission];

	// no matter how we spin it
	uint ret = sub.cboGfxEndAddress[type];
	uint newEnd = Math::n_align(ret + size, state.deviceProps[state.currentDevice].limits.minUniformBufferOffsetAlignment);

	// if we have to wrap around, or we are fingering on the range of the next frame submission buffer...
	if (newEnd > state.globalGraphicsConstantBufferMaxValue[type] * (state.currentFrameSubmission + 1))
	{
		n_error("Over allocation of graphics constant memory! Memory will be overwritten!\n");

		// return the beginning of the buffer, will definitely stomp the memory!
		ret = state.globalGraphicsConstantBufferMaxValue[type] * state.currentFrameSubmission;
		newEnd = Math::n_align(ret + size, state.deviceProps[state.currentDevice].limits.minUniformBufferOffsetAlignment);
	}

	// just bump the current frame submission pointer
	sub.cboGfxEndAddress[type] = newEnd;

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ConstantBufferId 
GetGraphicsConstantBuffer(CoreGraphicsGlobalConstantBufferType type)
{
	return state.globalGraphicsConstantBuffer[type];
}

//------------------------------------------------------------------------------
/**
*/
uint 
AllocateComputeConstantBufferMemory(CoreGraphicsGlobalConstantBufferType type, uint size)
{
	Vulkan::GraphicsDeviceState::ConstantsRingBuffer& sub = state.frameSubmissions[state.currentFrameSubmission];

	// no matter how we spin it
	uint ret = sub.cboComputeEndAddress[type];
	uint newEnd = Math::n_align(ret + size, state.deviceProps[state.currentDevice].limits.minUniformBufferOffsetAlignment);

	// if we have to wrap around, or we are fingering on the range of the next frame submission buffer...
	if (newEnd > state.globalComputeConstantBufferMaxValue[type] * (state.currentFrameSubmission + 1))
	{
		n_error("Over allocation of compute constant memory! Memory will be overwritten!\n");

		// return the beginning of the buffer, will definitely stomp the memory!
		ret = state.globalComputeConstantBufferMaxValue[type] * state.currentFrameSubmission;
		newEnd = Math::n_align(ret + size, state.deviceProps[state.currentDevice].limits.minUniformBufferOffsetAlignment);
	}

	// just bump the current frame submission pointer
	sub.cboComputeEndAddress[type] = newEnd;

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ConstantBufferId 
GetComputeConstantBuffer(CoreGraphicsGlobalConstantBufferType type)
{
	return state.globalComputeConstantBuffer[type];
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::SubmissionContextId 
GetResourceSubmissionContext()
{
	return state.resourceSubmissionContext;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::SubmissionContextId 
GetSetupSubmissionContext()
{
	return state.setupSubmissionContext;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::CommandBufferId 
GetGfxCommandBuffer()
{
	return state.gfxCmdBuffer;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::CommandBufferId 
GetComputeCommandBuffer()
{
	return state.computeCmdBuffer;
}

//------------------------------------------------------------------------------
/**
*/
void 
SetGraphicsPipeline()
{
	n_assert((state.currentPipelineBits & AllInfoSet) != 0);
	state.currentBindPoint = CoreGraphics::GraphicsPipeline;
	if ((state.currentPipelineBits & PipelineBuilt) == 0)
	{
		CreateAndBindGraphicsPipeline();
		state.currentPipelineBits |= PipelineBuilt;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ReloadShaderProgram(const CoreGraphics::ShaderProgramId& pro)
{
	state.database.Reload(pro);
}

//------------------------------------------------------------------------------
/**
*/
void 
InsertBarrier(const CoreGraphics::BarrierId barrier, const CoreGraphicsQueueType queue)
{
	VkBarrierInfo& info = barrierAllocator.Get<0>(barrier.id24);
	if (queue == GraphicsQueueType && state.inBeginPass)
	{
		if (state.inBeginBatch)
		{
			VkCommandBufferThread::Command cmd;
			cmd.type = VkCommandBufferThread::Barrier;
			cmd.barrier.dep = info.dep;
			cmd.barrier.srcMask = info.srcFlags;
			cmd.barrier.dstMask = info.dstFlags;
			cmd.barrier.memoryBarrierCount = info.numMemoryBarriers;
			cmd.barrier.memoryBarriers = info.memoryBarriers;
			cmd.barrier.bufferBarrierCount = info.numBufferBarriers;
			cmd.barrier.bufferBarriers = info.bufferBarriers;
			cmd.barrier.imageBarrierCount = info.numImageBarriers;
			cmd.barrier.imageBarriers = info.imageBarriers;
			PushToThread(cmd, state.currentDrawThread);
		}
		else
		{
			vkCmdPipelineBarrier(GetMainBuffer(GraphicsQueueType),
				info.srcFlags, info.dstFlags, 
				info.dep, 
				info.numMemoryBarriers, info.memoryBarriers, 
				info.numBufferBarriers, info.bufferBarriers, 
				info.numImageBarriers, info.imageBarriers);
		}
	}
	else
	{
		VkCommandBuffer buf = GetMainBuffer(queue);
		vkCmdPipelineBarrier(buf,
			info.srcFlags,
			info.dstFlags,
			info.dep,
			info.numMemoryBarriers, info.memoryBarriers,
			info.numBufferBarriers, info.bufferBarriers,
			info.numImageBarriers, info.imageBarriers);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
SignalEvent(const CoreGraphics::EventId ev, const CoreGraphicsQueueType queue)
{
	VkEventInfo& info = eventAllocator.Get<1>(ev.id24);
	if (queue == GraphicsQueueType && state.inBeginPass)
	{
		if (state.inBeginBatch)
		{
			VkCommandBufferThread::Command cmd;
			cmd.type = VkCommandBufferThread::SetEvent;
			cmd.setEvent.event = info.event;
			cmd.setEvent.stages = info.leftDependency;
			PushToThread(cmd, state.currentDrawThread);
		}
		else
		{
			vkCmdSetEvent(GetMainBuffer(GraphicsQueueType), info.event, info.leftDependency);
		}
	}
	else
	{
		VkCommandBuffer buf = GetMainBuffer(queue);
		vkCmdSetEvent(buf, info.event, info.leftDependency);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
WaitEvent(const CoreGraphics::EventId ev, const CoreGraphicsQueueType queue)
{
	VkEventInfo& info = eventAllocator.Get<1>(ev.id24);
	if (queue == GraphicsQueueType && state.inBeginPass)
	{
		if (state.inBeginBatch)
		{
			VkCommandBufferThread::Command cmd;
			cmd.type = VkCommandBufferThread::WaitForEvent;
			cmd.waitEvent.events = &info.event;
			cmd.waitEvent.numEvents = 1;
			cmd.waitEvent.memoryBarrierCount = info.numMemoryBarriers;
			cmd.waitEvent.memoryBarriers = info.memoryBarriers;
			cmd.waitEvent.bufferBarrierCount = info.numBufferBarriers;
			cmd.waitEvent.bufferBarriers = info.bufferBarriers;
			cmd.waitEvent.imageBarrierCount = info.numImageBarriers;
			cmd.waitEvent.imageBarriers = info.imageBarriers;
			cmd.waitEvent.waitingStage = info.leftDependency;
			cmd.waitEvent.signalingStage = info.rightDependency;
			PushToThread(cmd, state.currentDrawThread);
		}
		else
		{
			vkCmdWaitEvents(GetMainBuffer(GraphicsQueueType), 1, &info.event,
				info.leftDependency,
				info.rightDependency,
				info.numMemoryBarriers,
				info.memoryBarriers,
				info.numBufferBarriers,
				info.bufferBarriers,
				info.numImageBarriers,
				info.imageBarriers);
		}
	}
	else
	{
		VkCommandBuffer buf = GetMainBuffer(queue);
		vkCmdWaitEvents(buf, 1, &info.event,
			info.leftDependency,
			info.rightDependency,
			info.numMemoryBarriers,
			info.memoryBarriers,
			info.numBufferBarriers,
			info.bufferBarriers,
			info.numImageBarriers,
			info.imageBarriers);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ResetEvent(const CoreGraphics::EventId ev, const CoreGraphicsQueueType queue)
{
	VkEventInfo& info = eventAllocator.Get<1>(ev.id24);
	if (queue == GraphicsQueueType && state.inBeginPass)	
	{
		if (state.inBeginBatch)
		{
			VkCommandBufferThread::Command cmd;
			cmd.type = VkCommandBufferThread::ResetEvent;
			cmd.setEvent.event = info.event;
			cmd.setEvent.stages = info.rightDependency;
			PushToThread(cmd, state.currentDrawThread);
		}
		else
		{
			vkCmdResetEvent(GetMainBuffer(GraphicsQueueType), info.event, info.rightDependency);
		}
	}
	else
	{
		VkCommandBuffer buf = GetMainBuffer(queue);
		vkCmdResetEvent(buf, info.event, info.rightDependency);
	}
}


//------------------------------------------------------------------------------
/**
*/
void 
Draw()
{
	n_assert(state.inBeginPass);

	if (state.inBeginBatch)
	{
		VkCommandBufferThread::Command cmd;
		cmd.type = VkCommandBufferThread::Draw;
		cmd.draw.baseIndex = state.primitiveGroup.GetBaseIndex();
		cmd.draw.baseVertex = state.primitiveGroup.GetBaseVertex();
		cmd.draw.baseInstance = 0;
		cmd.draw.numIndices = state.primitiveGroup.GetNumIndices();
		cmd.draw.numVerts = state.primitiveGroup.GetNumVertices();
		cmd.draw.numInstances = 1;
		PushToThread(cmd, state.currentDrawThread);
	}
	else
	{
		vkCmdDraw(GetMainBuffer(GraphicsQueueType), state.primitiveGroup.GetNumVertices(), 1, state.primitiveGroup.GetBaseVertex(), 0);
	}

	// go to next thread
	_incr_counter(state.GraphicsDeviceNumDrawCalls, 1);
	_incr_counter(state.GraphicsDeviceNumPrimitives, state.primitiveGroup.GetNumVertices() / 3);
}

//------------------------------------------------------------------------------
/**
*/
void 
DrawIndexedInstanced(SizeT numInstances, IndexT baseInstance)
{
	n_assert(state.inBeginPass);

	if (state.inBeginBatch)
	{
		VkCommandBufferThread::Command cmd;
		cmd.type = VkCommandBufferThread::Draw;
		cmd.draw.baseIndex = state.primitiveGroup.GetBaseIndex();
		cmd.draw.baseVertex = state.primitiveGroup.GetBaseVertex();
		cmd.draw.baseInstance = baseInstance;
		cmd.draw.numIndices = state.primitiveGroup.GetNumIndices();
		cmd.draw.numVerts = state.primitiveGroup.GetNumVertices();
		cmd.draw.numInstances = numInstances;
		PushToThread(cmd, state.currentDrawThread);
	}
	else
	{
		vkCmdDrawIndexed(GetMainBuffer(GraphicsQueueType), state.primitiveGroup.GetNumIndices(), numInstances, state.primitiveGroup.GetBaseIndex(), state.primitiveGroup.GetBaseVertex(), baseInstance);
	}

	// go to next thread
	_incr_counter(state.GraphicsDeviceNumDrawCalls, 1);
	_incr_counter(state.GraphicsDeviceNumPrimitives, state.primitiveGroup.GetNumIndices() * numInstances / 3);
}

//------------------------------------------------------------------------------
/**
*/
void 
Compute(int dimX, int dimY, int dimZ)
{
	n_assert(!state.inBeginPass);
	vkCmdDispatch(GetMainBuffer(GraphicsQueueType), dimX, dimY, dimZ);
}

//------------------------------------------------------------------------------
/**
*/
void
EndBatch()
{
	n_assert(state.inBeginBatch);
	n_assert(state.pass != PassId::Invalid());

	state.currentProgram = -1;
	state.inBeginBatch = false;
	PassEndBatch(state.pass);

	// end draw threads, if any are remaining
	EndDrawThreads();
}

//------------------------------------------------------------------------------
/**
*/
void
EndPass()
{
	n_assert(state.inBeginPass);
	n_assert(state.pass != PassId::Invalid());

	state.pass = PassId::Invalid();
	state.inBeginPass = false;

	//this->currentPipelineBits = 0;
	state.propagateDescriptorSets.Clear();
	state.currentProgram = -1;

	// end render pass
	vkCmdEndRenderPass(GetMainBuffer(GraphicsQueueType));
}

//------------------------------------------------------------------------------
/**
*/
void 
EndSubmission(CoreGraphicsQueueType queue, bool endOfFrame)
{
	if (queue == GraphicsQueueType)
	{
		// if end of frame, insert setup submission 
		if (endOfFrame)
		{
			// finish up the resource submission and setup submissions
			CommandBufferEndRecord(state.setupSubmissionCmdBuffer);

			// submit the setup stuff, but allow the graphics queue to submit the fence
			state.subcontextHandler.AppendSubmission(GraphicsQueueType,
				CommandBufferGetVk(state.setupSubmissionCmdBuffer),
				VK_NULL_HANDLE, VK_PIPELINE_STAGE_TRANSFER_BIT,
				SemaphoreGetVk(state.setupSubmissionSemaphore));
		}

		// stop recording
		CommandBufferEndRecord(state.gfxCmdBuffer);

		// submit to graphics
		state.subcontextHandler.AppendSubmission(GraphicsQueueType,
			CommandBufferGetVk(state.gfxCmdBuffer),
			SemaphoreGetVk(state.gfxPrevSemaphore), VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
			SemaphoreGetVk(state.gfxSemaphore));

		// add wait semaphore for the last command buffer
		if (endOfFrame)
		{
			// add wait semaphore and add this semaphore to free
			state.subcontextHandler.AddWaitSemaphore(GraphicsQueueType, SemaphoreGetVk(state.setupSubmissionSemaphore), VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
		}

		// if we should wait for the compute, add a semaphore
		if (state.gfxWaitSemaphore != SemaphoreId::Invalid())
		{
			state.subcontextHandler.AddWaitSemaphore(GraphicsQueueType, SemaphoreGetVk(state.gfxWaitSemaphore), VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

#if NEBULA_GRAPHICS_DEBUG
			CoreGraphics::QueueBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Graphics-Compute Sync Submission");
#endif

			// flush submissions
			state.subcontextHandler.FlushSubmissions(GraphicsQueueType, VK_NULL_HANDLE, false);

#if NEBULA_GRAPHICS_DEBUG
			CoreGraphics::QueueEndMarker(GraphicsQueueType);
#endif
			
		}

		// set prev to current semaphore
		state.gfxPrevSemaphore = state.gfxSemaphore;
		state.gfxWaitSemaphore = SemaphoreId::Invalid();
	}
	else if (queue == ComputeQueueType)
	{
		if (endOfFrame)
		{
			// finish up the resource submission and setup submissions
			CommandBufferEndRecord(state.setupSubmissionCmdBuffer);

			// submit the setup stuff, but allow the graphics queue to submit the fence
			state.subcontextHandler.AppendSubmission(ComputeQueueType,
				CommandBufferGetVk(state.setupSubmissionCmdBuffer),
				VK_NULL_HANDLE, VK_PIPELINE_STAGE_TRANSFER_BIT,
				SemaphoreGetVk(state.setupSubmissionSemaphore));
		}

		// stop recording
		CommandBufferEndRecord(state.computeCmdBuffer);

		// submit to compute
		state.subcontextHandler.AppendSubmission(ComputeQueueType,
			CommandBufferGetVk(state.computeCmdBuffer),
			SemaphoreGetVk(state.computePrevSemaphore), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			SemaphoreGetVk(state.computeSemaphore));

		// add wait semaphore for the last command buffer
		if (endOfFrame)
		{
			// add wait semaphore and add this semaphore to free
			state.subcontextHandler.AddWaitSemaphore(ComputeQueueType, SemaphoreGetVk(state.setupSubmissionSemaphore), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		}

		// if we should wait for the graphics, add a semaphore
		if (state.computeWaitSemaphore == GraphicsQueueType)
		{
			state.subcontextHandler.AddWaitSemaphore(ComputeQueueType, SemaphoreGetVk(state.computeWaitSemaphore), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

#if NEBULA_GRAPHICS_DEBUG
			CoreGraphics::QueueBeginMarker(ComputeQueueType, NEBULA_MARKER_BLUE, "Compute-Graphics Sync Submission");
#endif

			// flush submissions
			state.subcontextHandler.FlushSubmissions(ComputeQueueType, VK_NULL_HANDLE, false);

#if NEBULA_GRAPHICS_DEBUG
			CoreGraphics::QueueEndMarker(ComputeQueueType);
#endif

		}

		// set previous semaphore
		state.computePrevSemaphore = state.computeSemaphore;
		state.computeWaitSemaphore = SemaphoreId::Invalid();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
EndFrame(IndexT frameIndex)
{
	n_assert(state.inBeginFrame);

	if (state.currentFrameIndex != frameIndex)
	{
		_end_counter(state.GraphicsDeviceNumComputes);
		_end_counter(state.GraphicsDeviceNumPrimitives);
		_end_counter(state.GraphicsDeviceNumDrawCalls);
		state.currentFrameIndex = frameIndex;
	}

	state.inBeginFrame = false;

	Vulkan::GraphicsDeviceState::ConstantsRingBuffer& sub = state.frameSubmissions[state.currentFrameSubmission];
	VkDevice dev = state.devices[state.currentDevice];

	// flush constant buffer memory
	VkMappedMemoryRange flushMapRanges[2];
	flushMapRanges[0] =
	{
		VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		nullptr,
		VK_NULL_HANDLE,
		0, 0
	};

	IndexT i;
	for (i = 0; i < NumConstantBufferTypes; i++)
	{
		uint size = sub.cboGfxEndAddress[i] - sub.cboGfxStartAddress[i];
		uint flushIndex = 0;
		if (size > 0)
		{
			flushMapRanges[flushIndex].memory = ConstantBufferGetVkMemory(state.globalGraphicsConstantBuffer[i]);
			flushMapRanges[flushIndex].offset = sub.cboGfxStartAddress[i];
			flushMapRanges[flushIndex].size = size;
			flushIndex++;
		}
		
		size = sub.cboComputeEndAddress[i] - sub.cboComputeStartAddress[i];
		if (size > 0)
		{
			flushMapRanges[flushIndex] = flushMapRanges[0];
			flushMapRanges[flushIndex].memory = ConstantBufferGetVkMemory(state.globalComputeConstantBuffer[i]);
			flushMapRanges[flushIndex].offset = sub.cboComputeStartAddress[i];
			flushMapRanges[flushIndex].size = size;
			flushIndex++;
		}	

		// be sure to flush all constant memory before we submit the command buffers with the data
		if (flushIndex > 0)
			vkFlushMappedMemoryRanges(dev, flushIndex, flushMapRanges);
	}

	// wait for resource manager first, since it will write to the transfer cmd buffer
	Resources::WaitForLoaderThread();

	// finish up the resource submission and setup submissions
	CommandBufferEndRecord(state.resourceSubmissionCmdBuffer);

#if NEBULA_GRAPHICS_DEBUG
	CoreGraphics::QueueBeginMarker(TransferQueueType, NEBULA_MARKER_BLUE, "Resource Transfer Submission");
#endif

	// submit resource stuff
	state.subcontextHandler.AppendSubmission(TransferQueueType,
		CommandBufferGetVk(state.resourceSubmissionCmdBuffer),
		VK_NULL_HANDLE, VK_PIPELINE_STAGE_TRANSFER_BIT,
		SemaphoreGetVk(state.resourceSubmissionSemaphore));

	state.subcontextHandler.FlushSubmissions(TransferQueueType, 
		FenceGetVk(state.resourceSubmissionFence), 
		false);

#if NEBULA_GRAPHICS_DEBUG
	CoreGraphics::QueueEndMarker(TransferQueueType);
#endif

#if NEBULA_GRAPHICS_DEBUG
	CoreGraphics::QueueBeginMarker(ComputeQueueType, NEBULA_MARKER_BLUE, "Compute Commands Submission");
#endif

	// submit compute, wait for this frames resource submissions
	state.subcontextHandler.FlushSubmissions(ComputeQueueType, 
		FenceGetVk(state.computeFence),
		false);

#if NEBULA_GRAPHICS_DEBUG
	CoreGraphics::QueueEndMarker(ComputeQueueType);
	CoreGraphics::QueueBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Graphics Commands Submission");
#endif

	state.subcontextHandler.AddWaitSemaphore(GraphicsQueueType, SemaphoreGetVk(state.resourceSubmissionSemaphore), VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

	// submit graphics, wait for this frames resource submissions, if we decide to use the compute queue for presentation, we have to change this
	state.subcontextHandler.FlushSubmissions(GraphicsQueueType, 
		VK_NULL_HANDLE, // use no fence, it will be used by the present queue
		false);

	state.subcontextHandler.SubmitFence(GraphicsQueueType, FenceGetVk(state.setupSubmissionFence));

#if NEBULA_GRAPHICS_DEBUG
	CoreGraphics::QueueEndMarker(GraphicsQueueType);
#endif

	// we don't need to sync, because the beginning of the frame will have waited for the previous fence for the setup
	state.setupSubmissionFence = SubmissionContextNextCycle(state.setupSubmissionContext);
	SubmissionContextNewBuffer(state.setupSubmissionContext, state.setupSubmissionCmdBuffer, state.setupSubmissionSemaphore);

	// cycle the resource and setup submissions, no need to get the fence again since we use 1 buffer
	state.resourceSubmissionFence = SubmissionContextNextCycle(state.resourceSubmissionContext);
	SubmissionContextNewBuffer(state.resourceSubmissionContext, state.resourceSubmissionCmdBuffer, state.resourceSubmissionSemaphore);

	// start recording 
	CommandBufferBeginInfo beginInfo{ true, false, false };
	CommandBufferBeginRecord(state.resourceSubmissionCmdBuffer, beginInfo);
	CommandBufferBeginRecord(state.setupSubmissionCmdBuffer, beginInfo);

	// reset state
	state.inputInfo.topology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	state.currentProgram = -1;
	state.currentPipelineInfo.pVertexInputState = nullptr;
	state.currentPipelineInfo.pInputAssemblyState = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
bool
IsInBeginFrame()
{
	return state.inBeginFrame;
}

//------------------------------------------------------------------------------
/**
*/
void 
WaitForQueue(CoreGraphicsQueueType queue)
{
	state.subcontextHandler.WaitIdle(queue);
}

//------------------------------------------------------------------------------
/**
*/
void 
WaitForAllQueues()
{
	state.subcontextHandler.WaitIdle(GraphicsQueueType);
	state.subcontextHandler.WaitIdle(ComputeQueueType);
	state.subcontextHandler.WaitIdle(TransferQueueType);
	state.subcontextHandler.WaitIdle(SparseQueueType);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ImageFileFormat::Code
SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream)
{
	return CoreGraphics::ImageFileFormat::InvalidImageFileFormat;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ImageFileFormat::Code 
SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream, const Math::rectangle<int>& rect, int x, int y)
{
	return CoreGraphics::ImageFileFormat::InvalidImageFileFormat;
}

//------------------------------------------------------------------------------
/**
*/
bool 
GetVisualizeMipMaps()
{
	return state.visualizeMipMaps;
}

//------------------------------------------------------------------------------
/**
*/
void 
SetVisualizeMipMaps(bool val)
{
	state.visualizeMipMaps = val;
}

//------------------------------------------------------------------------------
/**
*/
bool
GetRenderWireframe()
{
	return state.renderWireframe;
}

//------------------------------------------------------------------------------
/**
*/
void 
SetRenderWireframe(bool b)
{
	state.renderWireframe = b;
}

//------------------------------------------------------------------------------
/**
*/
void 
Copy(const CoreGraphics::TextureId from, Math::rectangle<SizeT> fromRegion, const CoreGraphics::TextureId to, Math::rectangle<SizeT> toRegion)
{
	n_assert(from != CoreGraphics::TextureId::Invalid() && to != CoreGraphics::TextureId::Invalid());
	n_assert(!state.inBeginPass);
	Vulkan::Copy(TextureGetVkImage(from), fromRegion, TextureGetVkImage(to), toRegion);
}

//------------------------------------------------------------------------------
/**
*/
void 
Copy(const CoreGraphics::RenderTextureId from, Math::rectangle<SizeT> fromRegion, const CoreGraphics::RenderTextureId to, Math::rectangle<SizeT> toRegion)
{
	n_assert(from != CoreGraphics::RenderTextureId::Invalid() && to != CoreGraphics::RenderTextureId::Invalid());
	n_assert(!state.inBeginPass);
	Vulkan::Copy(RenderTextureGetVkImage(from), fromRegion, RenderTextureGetVkImage(to), toRegion);
}

//------------------------------------------------------------------------------
/**
*/
void 
Blit(const CoreGraphics::TextureId from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const CoreGraphics::TextureId to, Math::rectangle<SizeT> toRegion, IndexT toMip)
{
	n_assert(from != CoreGraphics::TextureId::Invalid() && to != CoreGraphics::TextureId::Invalid());
	Vulkan::Blit(TextureGetVkImage(from), fromRegion, fromMip, TextureGetVkImage(to), toRegion, toMip);
}

//------------------------------------------------------------------------------
/**
*/
void
Blit(const CoreGraphics::RenderTextureId from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const CoreGraphics::RenderTextureId to, Math::rectangle<SizeT> toRegion, IndexT toMip)
{
	n_assert(from != CoreGraphics::RenderTextureId::Invalid() && to != CoreGraphics::RenderTextureId::Invalid());
	Vulkan::Blit(RenderTextureGetVkImage(from), fromRegion, fromMip, RenderTextureGetVkImage(to), toRegion, toMip);
}

//------------------------------------------------------------------------------
/**
*/
void 
SetUsePatches(bool b)
{
	state.usePatches = b;
}

//------------------------------------------------------------------------------
/**
*/
bool 
GetUsePatches()
{
	return state.usePatches;
}

//------------------------------------------------------------------------------
/**
*/
void 
SetViewport(const Math::rectangle<int>& rect, int index)
{
	// copy here is on purpose, because we don't want to modify the state viewports (they are pointers to the pass)
	VkViewport& vp = state.viewports[index];
	vp.width = (float)rect.width();
	vp.height = (float)rect.height();
	vp.x = (float)rect.left;
	vp.y = (float)rect.top;

	// only apply to batch or command buffer if we have a program bound
	if (state.currentProgram != -1)
	{
		if (state.inBeginBatch)
		{
			VkCommandBufferThread::Command cmd;
			cmd.type = VkCommandBufferThread::Viewport;
			cmd.viewport.index = index;
			cmd.viewport.vp = vp;
			PushToThread(cmd, state.currentDrawThread);
		}
		else
		{
			vkCmdSetViewport(GetMainBuffer(GraphicsQueueType), index, 1, &state.viewports[index]);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
SetScissorRect(const Math::rectangle<int>& rect, int index)
{
	// copy here is on purpose, because we don't want to modify the state scissors (they are pointers to the pass)
	VkRect2D& sc = state.scissors[index];
	sc.extent.width = rect.width();
	sc.extent.height = rect.height();
	sc.offset.x = rect.left;
	sc.offset.y = rect.top;

	if (state.currentProgram != -1)
	{
		if (state.inBeginBatch)
		{
			VkCommandBufferThread::Command cmd;
			cmd.type = VkCommandBufferThread::ScissorRect;
			cmd.scissorRect.index = index;
			cmd.scissorRect.sc = sc;
			PushToThread(cmd, state.currentDrawThread);
		}
		else
		{
			vkCmdSetScissor(GetMainBuffer(GraphicsQueueType), index, 1, &state.scissors[index]);
		}
	}
}


//------------------------------------------------------------------------------
/**
*/
void 
RegisterRenderTexture(const Util::StringAtom& name, const CoreGraphics::RenderTextureId id)
{
	state.renderTextures.Add(name, id);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::RenderTextureId 
GetRenderTexture(const Util::StringAtom & name)
{
	IndexT i = state.renderTextures.FindIndex(name);
	if (i == InvalidIndex)		return CoreGraphics::RenderTextureId::Invalid();
	else						return state.renderTextures[name];
}

//------------------------------------------------------------------------------
/**
*/
void 
RegisterShaderRWTexture(const Util::StringAtom& name, const CoreGraphics::ShaderRWTextureId id)
{
	state.shaderRWTextures.Add(name, id);
}


//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderRWTextureId 
GetShaderRWTexture(const Util::StringAtom & name)
{
	IndexT i = state.shaderRWTextures.FindIndex(name);
	if (i == InvalidIndex)		return CoreGraphics::ShaderRWTextureId::Invalid();
	else						return state.shaderRWTextures[name];
}

//------------------------------------------------------------------------------
/**
*/
void 
RegisterShaderRWBuffer(const Util::StringAtom& name, const CoreGraphics::ShaderRWBufferId id)
{
	state.shaderRWBuffers.Add(name, id);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderRWBufferId 
GetShaderRWBuffer(const Util::StringAtom& name)
{
	IndexT i = state.shaderRWBuffers.FindIndex(name);
	if (i == InvalidIndex)		return CoreGraphics::ShaderRWBufferId::Invalid();
	else						return state.shaderRWBuffers[name];
}

#if NEBULA_GRAPHICS_DEBUG

//------------------------------------------------------------------------------
/**
*/
template<>
void
ObjectSetName(const CoreGraphics::ConstantBufferId id, const Util::String& name)
{
	VkDebugUtilsObjectNameInfoEXT info =
	{
		VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		nullptr,
		VK_OBJECT_TYPE_IMAGE,
		(uint64_t)Vulkan::ConstantBufferGetVk(id),
		name.AsCharPtr()
	};
	VkDevice dev = GetCurrentDevice();
	VkResult res = VkDebugObjectName(dev, &info);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ObjectSetName(const CoreGraphics::TextureId id, const Util::String& name)
{
	VkDebugUtilsObjectNameInfoEXT info =
	{
		VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		nullptr,
		VK_OBJECT_TYPE_IMAGE,
		(uint64_t)Vulkan::TextureGetVkImage(id),
		name.AsCharPtr()
	};
	VkDevice dev = GetCurrentDevice();
	VkResult res = VkDebugObjectName(dev, &info);
	n_assert(res == VK_SUCCESS);

	info.objectHandle = (uint64_t)Vulkan::TextureGetVkImageView(id);
	info.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
	Util::String str = Util::String::Sprintf("%s - View", name.AsCharPtr());
	info.pObjectName = str.AsCharPtr();
	res = VkDebugObjectName(dev, &info);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ObjectSetName(const CoreGraphics::RenderTextureId id, const Util::String& name)
{
	VkDebugUtilsObjectNameInfoEXT info =
	{
		VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		nullptr,
		VK_OBJECT_TYPE_IMAGE,
		(uint64_t)Vulkan::RenderTextureGetVkImage(id),
		name.AsCharPtr()
	};
	VkDevice dev = GetCurrentDevice();
	VkResult res = VkDebugObjectName(dev, &info);
	n_assert(res == VK_SUCCESS);

	info.objectHandle = (uint64_t)Vulkan::RenderTextureGetVkImageView(id);
	info.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
	Util::String str = Util::String::Sprintf("%s - View", name.AsCharPtr());
	info.pObjectName = str.AsCharPtr();
	res = VkDebugObjectName(dev, &info);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ObjectSetName(const CoreGraphics::ShaderRWTextureId id, const Util::String& name)
{
	VkDebugUtilsObjectNameInfoEXT info =
	{
		VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		nullptr,
		VK_OBJECT_TYPE_IMAGE,
		(uint64_t)Vulkan::ShaderRWTextureGetVkImage(id),
		name.AsCharPtr()
	};
	VkDevice dev = GetCurrentDevice();
	VkResult res = VkDebugObjectName(dev, &info);
	n_assert(res == VK_SUCCESS);

	info.objectHandle = (uint64_t)Vulkan::ShaderRWTextureGetVkImageView(id);
	info.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
	Util::String str = Util::String::Sprintf("%s - View", name.AsCharPtr());
	info.pObjectName = str.AsCharPtr();
	res = VkDebugObjectName(dev, &info);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ObjectSetName(const CoreGraphics::ShaderRWBufferId id, const Util::String& name)
{
	VkDebugUtilsObjectNameInfoEXT info =
	{
		VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		nullptr,
		VK_OBJECT_TYPE_BUFFER,
		(uint64_t)Vulkan::ShaderRWBufferGetVkBuffer(id),
		name.AsCharPtr()
	};
	VkDevice dev = GetCurrentDevice();
	VkResult res = VkDebugObjectName(dev, &info);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ObjectSetName(const CoreGraphics::ResourceTableLayoutId id, const Util::String& name)
{
	VkDebugUtilsObjectNameInfoEXT info =
	{
		VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		nullptr,
		VK_OBJECT_TYPE_IMAGE,
		(uint64_t)Vulkan::ResourceTableLayoutGetVk(id),
		name.AsCharPtr()
	};
	VkDevice dev = GetCurrentDevice();
	VkResult res = VkDebugObjectName(dev, &info);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ObjectSetName(const CoreGraphics::ResourcePipelineId id, const Util::String& name)
{
	VkDebugUtilsObjectNameInfoEXT info =
	{
		VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		nullptr,
		VK_OBJECT_TYPE_IMAGE,
		(uint64_t)Vulkan::ResourcePipelineGetVk(id),
		name.AsCharPtr()
	};
	VkDevice dev = GetCurrentDevice();
	VkResult res = VkDebugObjectName(dev, &info);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ObjectSetName(const CoreGraphics::CommandBufferId id, const Util::String& name)
{
	VkDebugUtilsObjectNameInfoEXT info =
	{
		VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		nullptr,
		VK_OBJECT_TYPE_COMMAND_BUFFER,
		(uint64_t)Vulkan::CommandBufferGetVk(id),
		name.AsCharPtr()
	};
	VkDevice dev = GetCurrentDevice();
	VkResult res = VkDebugObjectName(dev, &info);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ObjectSetName(const VkShaderModule id, const Util::String& name)
{
	VkDebugUtilsObjectNameInfoEXT info =
	{
		VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		nullptr,
		VK_OBJECT_TYPE_SHADER_MODULE,
		(uint64_t)id,
		name.AsCharPtr()
	};
	VkDevice dev = GetCurrentDevice();
	VkResult res = VkDebugObjectName(dev, &info);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ObjectSetName(const SemaphoreId id, const Util::String& name)
{
	VkDebugUtilsObjectNameInfoEXT info =
	{
		VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		nullptr,
		VK_OBJECT_TYPE_SHADER_MODULE,
		(uint64_t)SemaphoreGetVk(id.id24),
		name.AsCharPtr()
	};
	VkDevice dev = GetCurrentDevice();
	VkResult res = VkDebugObjectName(dev, &info);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void 
QueueBeginMarker(const CoreGraphicsQueueType queue, const Math::float4& color, const Util::String& name)
{
	VkQueue vkqueue = state.subcontextHandler.GetQueue(queue);
	alignas(16) float col[4];
	color.store(col);
	VkDebugUtilsLabelEXT info =
	{
		VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		nullptr,
		name.AsCharPtr(),
		{ col[0], col[1], col[2], col[3] }
	};
	VkQueueBeginLabel(vkqueue, &info);
}

//------------------------------------------------------------------------------
/**
*/
void 
QueueEndMarker(const CoreGraphicsQueueType queue)
{
	VkQueue vkqueue = state.subcontextHandler.GetQueue(queue);
	VkQueueEndLabel(vkqueue);
}

//------------------------------------------------------------------------------
/**
*/
void 
QueueInsertMarker(const CoreGraphicsQueueType queue, const Math::float4& color, const Util::String& name)
{
	VkQueue vkqueue = state.subcontextHandler.GetQueue(queue);
	alignas(16) float col[4];
	color.store(col);
	VkDebugUtilsLabelEXT info =
	{
		VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		nullptr,
		name.AsCharPtr(),
		{ col[0], col[1], col[2], col[3] }
	};
	VkQueueInsertLabel(vkqueue, &info);
}

//------------------------------------------------------------------------------
/**
*/
void 
CommandBufferBeginMarker(const CoreGraphicsQueueType queue, const Math::float4& color, const Util::String& name)
{
	VkCommandBuffer buf = GetMainBuffer(queue);
	alignas(16) float col[4];
	color.store(col);
	VkDebugUtilsLabelEXT info =
	{
		VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		nullptr,
		name.AsCharPtr(),
		{ col[0], col[1], col[2], col[3] }
	};
	VkCmdDebugMarkerBegin(buf, &info);
}

//------------------------------------------------------------------------------
/**
*/
void 
CommandBufferEndMarker(const CoreGraphicsQueueType queue)
{
	VkCommandBuffer buf = GetMainBuffer(queue);
	VkCmdDebugMarkerEnd(buf);
}

//------------------------------------------------------------------------------
/**
*/
void 
CommandBufferInsertMarker(const CoreGraphicsQueueType queue, const Math::float4& color, const Util::String& name)
{
	VkCommandBuffer buf = GetMainBuffer(queue);
	alignas(16) float col[4];
	color.store(col);
	VkDebugUtilsLabelEXT info =
	{
		VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		nullptr,
		name.AsCharPtr(),
		{ col[0], col[1], col[2], col[3] }
	};
	VkCmdDebugMarkerInsert(buf, &info);
}
#endif

} // namespace CoreGraphics
