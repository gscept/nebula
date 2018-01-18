//------------------------------------------------------------------------------
// vkrenderdevice.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/config.h"
#include "vkrenderdevice.h"
#include "coregraphics/displaydevice.h"
#include "app/application.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "system/cpu.h"
#include "vktypes.h"
#include "vktransformdevice.h"
#include "vkshaderserver.h"
#include "coregraphics/pass.h"
#include "vkpass.h"
#include "io/ioserver.h"

using namespace CoreGraphics;
//------------------------------------------------------------------------------
/**
*/
VKAPI_ATTR VkBool32 VKAPI_CALL
NebulaVulkanDebugCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objectType, uint64_t src, size_t location, int32_t msgCode, const char* layerPrefix, const char* msg, void* userData)
{

#if NEBULAT_VULKAN_DEBUG
	bool ret = true;
#else
	bool ret = false;
#endif

	if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		n_error("VULKAN ERROR: [%s], code %d : %s\n", layerPrefix, msgCode, msg);
	}
	else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		n_warning("VULKAN WARNING: [%s], code %d : %s\n", layerPrefix, msgCode, msg);
	} 
	return ret;
}

namespace Vulkan
{
__ImplementClass(Vulkan::VkRenderDevice, 'VURD', Base::RenderDeviceBase);
__ImplementSingleton(Vulkan::VkRenderDevice);

VkDescriptorPool VkRenderDevice::descPool;
VkInstance VkRenderDevice::instance;
VkPipelineCache VkRenderDevice::cache;

CoreGraphics::CmdBufferId VkRenderDevice::mainCmdDrawBuffer;
CoreGraphics::CmdBufferId VkRenderDevice::mainCmdComputeBuffer;
CoreGraphics::CmdBufferId VkRenderDevice::mainCmdTransferBuffer;
CoreGraphics::CmdBufferId VkRenderDevice::mainCmdSparseBuffer;

//------------------------------------------------------------------------------
/**
*/
VkRenderDevice::VkRenderDevice() :
	frameId(0),
	drawQueueIdx(-1),
	computeQueueIdx(-1),
	transferQueueIdx(-1),
	currentDrawThread(NumDrawThreads-1),
	numActiveThreads(0),
	currentTransThread(0),
	currentProgram(0)
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
VkRenderDevice::~VkRenderDevice()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool
VkRenderDevice::Open()
{
	n_assert(!this->IsOpen());
	bool success = false;
	if (this->OpenVulkanContext())
	{
		// hand to parent class, this will notify event handlers
		success = RenderDeviceBase::Open();
	}
	return success;
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::Close()
{
	n_assert(this->IsOpen());
	this->CloseVulkanDevice();
	RenderDeviceBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
bool
VkRenderDevice::OpenVulkanContext()
{
	DisplayDevice* displayDevice = DisplayDevice::Instance();
	n_assert(displayDevice->IsOpen());

	// create result
	VkResult res;

	// setup application
	VkApplicationInfo appInfo =
	{
		VK_STRUCTURE_TYPE_APPLICATION_INFO,
		NULL,
		App::Application::Instance()->GetAppTitle().AsCharPtr(),
		2,					// application version
		"Nebula Trifid",	// engine name
		2,					// engine version
		//VK_MAKE_VERSION(1, 0, 4)
		0		// API version
	};

	this->usedExtensions = 0;
	uint32_t requiredExtensionsNum;
	const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsNum);
	uint32_t i;
	for (i = 0; i < (uint32_t)requiredExtensionsNum; i++)
	{
		this->extensions[this->usedExtensions++] = requiredExtensions[i];
	}
	
	const char* layers[] = { "VK_LAYER_LUNARG_standard_validation" };
#if NEBULAT_VULKAN_DEBUG
	this->extensions[this->usedExtensions++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
	const int numLayers = 0;//sizeof(layers) / sizeof(const char*);
#else
	const int numLayers = 0;
#endif

	// setup instance
	VkInstanceCreateInfo instanceInfo =
	{
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,		// type of struct
		NULL,										// pointer to next
		0,											// flags
		&appInfo,									// application
		numLayers,
		layers,
		this->usedExtensions,
		this->extensions
	};
	
	// create instance
	res = vkCreateInstance(&instanceInfo, NULL, &this->instance);
	if (res == VK_ERROR_INCOMPATIBLE_DRIVER)
	{
		n_error("VkRenderDevice::OpenVulkanContext(): Your GPU driver is not compatible with Vulkan.\n");
	}
	else if (res == VK_ERROR_EXTENSION_NOT_PRESENT)
	{
		n_error("VkRenderDevice::OpenVulkanContext(): Vulkan extension failed to load.\n");
	}
	else if (res == VK_ERROR_LAYER_NOT_PRESENT)
	{
		n_error("VkRenderDevice::OpenVulkanContext(): Vulkan layer failed to load.\n");
	}
	n_assert(res == VK_SUCCESS);

	// setup adapter
	this->SetupAdapter();
	this->currentDevice = 0;

#if NEBULAT_VULKAN_DEBUG
	this->debugCallbackPtr = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(this->instance, "vkCreateDebugReportCallbackEXT");
	VkDebugReportCallbackCreateInfoEXT dbgInfo;
	dbgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	dbgInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	dbgInfo.pNext = NULL;
	dbgInfo.pfnCallback = NebulaVulkanDebugCallback;
	dbgInfo.pUserData = NULL;
	res = this->debugCallbackPtr(this->instance, &dbgInfo, NULL, &this->debugCallbackHandle);
	n_assert(res == VK_SUCCESS);
#endif

	vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevices[this->currentDevice], &this->numQueues, NULL);
	n_assert(this->numQueues > 0);

	// now get queues from device
	vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevices[this->currentDevice], &this->numQueues, this->queuesProps);
	vkGetPhysicalDeviceMemoryProperties(this->physicalDevices[this->currentDevice], &this->memoryProps);

	this->drawQueueIdx = UINT32_MAX;
	this->computeQueueIdx = UINT32_MAX;
	this->transferQueueIdx = UINT32_MAX;
	this->sparseQueueIdx = UINT32_MAX;

	// create three queues for each family
	Util::FixedArray<uint> indexMap;
	indexMap.Resize(numQueues);
	indexMap.Fill(0);
	for (i = 0; i < numQueues; i++)
	{
		
		if (this->queuesProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && this->drawQueueIdx == UINT32_MAX)
		{
			if (this->queuesProps[i].queueCount == indexMap[i]) continue;
			this->drawQueueFamily = i;
			this->drawQueueIdx = indexMap[i]++;
		}

		// compute queues may not support graphics
		if (this->queuesProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT && this->computeQueueIdx == UINT32_MAX)
		{
			if (this->queuesProps[i].queueCount == indexMap[i]) continue;
			this->computeQueueFamily = i;
			this->computeQueueIdx = indexMap[i]++;
		}

		// transfer queues may not support compute or graphics
		if (this->queuesProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT && 
			!(this->queuesProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
			!(this->queuesProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
			this->transferQueueIdx == UINT32_MAX)
		{
			if (this->queuesProps[i].queueCount == indexMap[i]) continue;
			this->transferQueueFamily = i;
			this->transferQueueIdx = indexMap[i]++;
		}

		// sparse queues may not support compute or graphics
		if (this->queuesProps[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT &&
			!(this->queuesProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
			!(this->queuesProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
			this->transferQueueIdx == UINT32_MAX)
		{
			if (this->queuesProps[i].queueCount == indexMap[i]) continue;
			this->sparseQueueFamily = i;
			this->sparseQueueIdx = indexMap[i]++;
		}
	}

	if (this->transferQueueIdx == UINT32_MAX)
	{
		// assert the draw queue can do both transfers and computes
		n_assert(this->queuesProps[this->drawQueueFamily].queueFlags & VK_QUEUE_TRANSFER_BIT);
		n_assert(this->queuesProps[this->drawQueueFamily].queueFlags & VK_QUEUE_COMPUTE_BIT);

		// this is actually sub-optimal, but on my AMD card, using the compute queue transfer or the sparse queue doesn't work
		this->transferQueueFamily = this->drawQueueFamily;
		this->transferQueueIdx = this->drawQueueIdx;
		//this->transferQueueFamily = 2;
		//this->transferQueueIdx = indexMap[2]++;
	}	

	if (this->drawQueueFamily == UINT32_MAX)		n_error("VkDisplayDevice: Could not find a queue for graphics and present.\n");
	if (this->computeQueueFamily == UINT32_MAX)		n_error("VkDisplayDevice: Could not find a queue for compute.\n");
	if (this->transferQueueFamily == UINT32_MAX)	n_error("VkDisplayDevice: Could not find a queue for transfers.\n");
	if (this->sparseQueueFamily == UINT32_MAX)		n_warning("VkDisplayDevice: Could not find a queue for sparse binding.\n");

	// create device
	Util::FixedArray<Util::FixedArray<float>> prios;
	Util::Array<VkDeviceQueueCreateInfo> queueInfos;
	prios.Resize(numQueues);

	for (i = 0; i < numQueues; i++)
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
	vkGetPhysicalDeviceFeatures(this->physicalDevices[this->currentDevice], &features);

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(this->physicalDevices[this->currentDevice], &props);
	
	VkDeviceCreateInfo deviceInfo =
	{
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		NULL,
		0,
		(uint32_t)queueInfos.Size(),
		&queueInfos[0],
		numLayers,
		layers,
		this->usedPhysicalExtensions,
		this->deviceExtensionStrings,
		&features
	};

	// create device
	res = vkCreateDevice(this->physicalDevices[this->currentDevice], &deviceInfo, NULL, &this->devices[0]);
	n_assert(res == VK_SUCCESS);

	// setup queue handler
	Util::FixedArray<uint> families(4);
	families[VkSubContextHandler::DrawContextType] = this->drawQueueFamily;
	families[VkSubContextHandler::ComputeContextType] = this->computeQueueFamily;
	families[VkSubContextHandler::TransferContextType] = this->transferQueueFamily;
	families[VkSubContextHandler::SparseContextType] = this->sparseQueueFamily;
	this->subcontextHandler.Setup(this->devices[this->currentDevice], indexMap, families);

	VkPipelineCacheCreateInfo cacheInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
		NULL,
		0,
		0,
		NULL
	};

	// create cache
	res = vkCreatePipelineCache(this->devices[this->currentDevice], &cacheInfo, NULL, &this->cache);
	n_assert(res == VK_SUCCESS);

	// setup our own pipeline database
	this->database.Setup(this->devices[this->currentDevice], this->cache);

	uint32_t descCounts[] =
	{
		256,
		256,
		262140,
		512,
		32,
		32,
		32,
		32,
		65535,
		256,
		2048
	};

	VkDescriptorType types[] =
	{
		VK_DESCRIPTOR_TYPE_SAMPLER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
		VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
	};

	for (i = 0; i < 11; i++)
	{
		this->poolSizes[i].descriptorCount = descCounts[i];
		this->poolSizes[i].type = types[i];
	}

	this->RequestPool();
	this->currentPool = 0;

	// setup pools (from VkCmdBuffer.h)
	SetupVkPools(this->devices[this->currentDevice], this->drawQueueFamily, this->computeQueueFamily, this->transferQueueFamily, this->sparseQueueFamily);

	CmdBufferCreateInfo cmdCreateInfo =
	{
		false,
		true,
		false,
		InvalidCmdUsage
	};
	cmdCreateInfo.usage = CmdDraw;
	mainCmdDrawBuffer = CreateCmdBuffer(cmdCreateInfo);

	cmdCreateInfo.usage = CmdCompute;
	mainCmdComputeBuffer = CreateCmdBuffer(cmdCreateInfo);

	cmdCreateInfo.usage = CmdTransfer;
	mainCmdTransferBuffer = CreateCmdBuffer(cmdCreateInfo);

	cmdCreateInfo.usage = CmdSparse;
	mainCmdSparseBuffer = CreateCmdBuffer(cmdCreateInfo);
	
	// setup draw threads
	Util::String threadName;
	for (i = 0; i < NumDrawThreads; i++)
	{
		threadName.Format("DrawCmdBufferThread%d", i);
		this->drawThreads[i] = VkCmdBufferThread::Create();
		this->drawThreads[i]->SetPriority(Threading::Thread::High);
		this->drawThreads[i]->SetCoreId(System::Cpu::RenderThreadFirstCore + i);
		this->drawThreads[i]->SetName(threadName);
		this->drawThreads[i]->Start();

		this->drawCompletionEvents[i] = n_new(Threading::Event(true));
	}

	// setup transfer threads
	for (i = 0; i < NumTransferThreads; i++)
	{
		threadName.Format("TransferCmdBufferThread%d", i);
		this->transThreads[i] = VkCmdBufferThread::Create();
		this->transThreads[i]->SetPriority(Threading::Thread::Low);
		this->transThreads[i]->SetCoreId(System::Cpu::RenderThreadFirstCore + NumDrawThreads + i);
		this->transThreads[i]->SetName(threadName);
		this->transThreads[i]->Start();

		this->transCompletionEvents[i] = n_new(Threading::Event(true));
	}

	// setup compute threads
	for (i = 0; i < NumComputeThreads; i++)
	{
		threadName.Format("ComputeCmdBufferThread%d", i);
		this->compThreads[i] = VkCmdBufferThread::Create();
		this->compThreads[i]->SetPriority(Threading::Thread::Low);
		this->compThreads[i]->SetCoreId(System::Cpu::RenderThreadFirstCore + NumDrawThreads + NumTransferThreads + i);
		this->compThreads[i]->SetName(threadName);
		this->compThreads[i]->Start();

		this->compCompletionEvents[i] = n_new(Threading::Event(true));
	}

	VkFenceCreateInfo fenceInfo =
	{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		NULL,
		0
	};

	res = vkCreateFence(this->devices[this->currentDevice], &fenceInfo, NULL, &this->mainCmdDrawFence);
	n_assert(res == VK_SUCCESS);
	res = vkCreateFence(this->devices[this->currentDevice], &fenceInfo, NULL, &this->mainCmdCmpFence);
	n_assert(res == VK_SUCCESS);
	res = vkCreateFence(this->devices[this->currentDevice], &fenceInfo, NULL, &this->mainCmdTransFence);
	n_assert(res == VK_SUCCESS);

	VkEventCreateInfo eventInfo = 
	{
		VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
		NULL,
		0
	};
	res = vkCreateEvent(this->devices[this->currentDevice], &eventInfo, NULL, &this->mainCmdDrawEvent);
	n_assert(res == VK_SUCCESS);
	res = vkCreateEvent(this->devices[this->currentDevice], &eventInfo, NULL, &this->mainCmdCmpEvent);
	n_assert(res == VK_SUCCESS);
	res = vkCreateEvent(this->devices[this->currentDevice], &eventInfo, NULL, &this->mainCmdTransEvent);
	n_assert(res == VK_SUCCESS);

	this->passInfo = 
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		NULL,
		0,
	};

	// setup pipeline construction struct
	this->currentPipelineInfo.pNext = NULL;
	this->currentPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	this->currentPipelineInfo.flags = 0;
	this->currentPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	this->currentPipelineInfo.basePipelineIndex = -1;
	this->currentPipelineInfo.pColorBlendState = &this->blendInfo;

	this->inputInfo.pNext = NULL;
	this->inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	this->inputInfo.flags = 0;

	this->blendInfo.pNext = NULL;
	this->blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	this->blendInfo.flags = 0;

	// reset state
	this->inputInfo.topology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	this->currentProgram = 0;
	this->currentPipelineInfo.pVertexInputState = nullptr;
	this->currentPipelineInfo.pInputAssemblyState = nullptr;

	_setup_timer(DebugTimer);
	_setup_counter(NumImageBytesAllocated);
	_begin_counter(NumImageBytesAllocated);
	_setup_counter(NumBufferBytesAllocated);
	_begin_counter(NumBufferBytesAllocated);
	_setup_counter(NumBytesAllocated);
	_begin_counter(NumBytesAllocated);
	_setup_counter(NumPipelinesBuilt);
	_begin_counter(NumPipelinesBuilt);

	// yay, Vulkan!
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::CloseVulkanDevice()
{
	_discard_timer(DebugTimer);
	_end_counter(NumImageBytesAllocated);
	_discard_counter(NumImageBytesAllocated);
	_end_counter(NumBufferBytesAllocated);
	_discard_counter(NumBufferBytesAllocated);
	_end_counter(NumBytesAllocated);
	_discard_counter(NumBytesAllocated);
	_end_counter(NumPipelinesBuilt);
	_discard_counter(NumPipelinesBuilt);

	size_t size;
	vkGetPipelineCacheData(this->devices[0], this->cache, &size, NULL);
	uint8_t* data = (uint8_t*)Memory::Alloc(Memory::ScratchHeap, size);
	vkGetPipelineCacheData(this->devices[0], this->cache, &size, data);
	Util::String path = Util::String::Sprintf("bin:%s_vkpipelinecache", App::Application::Instance()->GetAppTitle().AsCharPtr());
	Ptr<IO::Stream> cachedData = IO::IoServer::Instance()->CreateStream(path);
	cachedData->SetAccessMode(IO::Stream::WriteAccess);
	if (cachedData->Open())
	{
		cachedData->Write(data, size);
		cachedData->Close();
	}

	// wait for all commands to be done first
	this->subcontextHandler.WaitIdle(VkSubContextHandler::DrawContextType);
	this->subcontextHandler.WaitIdle(VkSubContextHandler::ComputeContextType);
	this->subcontextHandler.WaitIdle(VkSubContextHandler::TransferContextType);
	this->subcontextHandler.WaitIdle(VkSubContextHandler::SparseContextType);

	IndexT i;
	for (i = 0; i < NumDrawThreads; i++)
	{
		this->drawThreads[i]->Stop();
		this->drawThreads[i] = nullptr;
		n_delete(this->drawCompletionEvents[i]);

		vkDestroyCommandPool(this->devices[0], this->dispatchableCmdDrawBufferPool[i], NULL);
	}

	for (i = 0; i < NumTransferThreads; i++)
	{
		this->transThreads[i]->Stop();
		this->transThreads[i] = nullptr;
		n_delete(this->transCompletionEvents[i]);

		vkDestroyCommandPool(this->devices[0], this->dispatchableCmdTransBufferPool[i], NULL);
	}

	for (i = 0; i < NumComputeThreads; i++)
	{
		this->compThreads[i]->Stop();
		this->compThreads[i] = nullptr;
		n_delete(this->compCompletionEvents[i]);

		vkDestroyCommandPool(this->devices[0], this->dispatchableCmdCompBufferPool[i], NULL);
	}

	// free our main buffers, our secondary buffers should be fine so the pools should be free to destroy
	DestroyCmdBuffer(this->mainCmdDrawBuffer);
	DestroyCmdBuffer(this->mainCmdComputeBuffer);
	DestroyCmdBuffer(this->mainCmdTransferBuffer);
	DestroyCmdBuffer(this->mainCmdSparseBuffer);
	DestroyVkPools(this->devices[0]);

	vkDestroyFence(this->devices[0], this->mainCmdDrawFence, NULL);
	vkDestroyFence(this->devices[0], this->mainCmdCmpFence, NULL);
	vkDestroyFence(this->devices[0], this->mainCmdTransFence, NULL);

	vkDestroyDevice(this->devices[0], NULL);
	vkDestroyInstance(this->instance, NULL);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::SetupAdapter()
{
	// retrieve available GPUs
	uint32_t gpuCount;
	VkResult res;
	res = vkEnumeratePhysicalDevices(this->instance, &gpuCount, NULL);
	n_assert(res == VK_SUCCESS);

	this->devices.Resize(gpuCount);
	this->physicalDevices.Resize(gpuCount);
	this->numCaps.Resize(gpuCount);
	this->caps.Resize(gpuCount);
	this->deviceProps.Resize(gpuCount);
	this->deviceFeatures.Resize(gpuCount);

	if (gpuCount > 0)
	{
		res = vkEnumeratePhysicalDevices(this->instance, &gpuCount, this->physicalDevices.Begin());
		n_assert(res == VK_SUCCESS);

		if (gpuCount > 1)
			n_message("Found %d GPUs, which is more than 1! Perhaps the Render Device should be able to use it?\n", gpuCount);

		IndexT i;
		for (i = 0; i < gpuCount; i++)
		{
			res = vkEnumerateDeviceExtensionProperties(physicalDevices[i], nullptr, &this->numCaps[i], nullptr);
			n_assert(res == VK_SUCCESS);

			if (this->numCaps[i] > 0)
			{
				this->caps[i].Resize(this->numCaps[i]);
				res = vkEnumerateDeviceExtensionProperties(physicalDevices[i], NULL, &this->usedPhysicalExtensions, this->caps[i].Begin());
				n_assert(res == VK_SUCCESS);
			}

			// get device props and features
			vkGetPhysicalDeviceProperties(this->physicalDevices[0], &this->deviceProps[i]);
			vkGetPhysicalDeviceFeatures(this->physicalDevices[0], &this->deviceFeatures[i]);
		}
	}
	else
	{
		n_error("VkRenderDevice::SetupAdapter(): No GPU available.\n");
	}
}

//------------------------------------------------------------------------------
/**
*/
bool
VkRenderDevice::BeginFrame(IndexT frameIndex)
{
	const CmdBufferBeginInfo cmdInfo = 
	{
		true, false, false
	};
	CmdBufferBeginRecord(this->mainCmdDrawBuffer, cmdInfo);
	CmdBufferBeginRecord(this->mainCmdComputeBuffer, cmdInfo);
	CmdBufferBeginRecord(this->mainCmdTransferBuffer, cmdInfo);
	CmdBufferBeginRecord(this->mainCmdSparseBuffer, cmdInfo);

	// all commands are put on the main command buffer
	this->currentCommandState = MainState;

	this->scheduler.Begin();
	this->scheduler.ExecuteCommandPass(VkScheduler::OnHandleTransferFences);
	this->scheduler.ExecuteCommandPass(VkScheduler::OnHandleDrawFences);
	this->scheduler.ExecuteCommandPass(VkScheduler::OnHandleComputeFences);
	this->scheduler.ExecuteCommandPass(VkScheduler::OnBeginFrame);

	// reset current thread
	this->currentDrawThread = NumDrawThreads - 1;
	this->currentPipelineBits = 0;

	return RenderDeviceBase::BeginFrame(frameIndex);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::SetStreamVertexBuffer(IndexT streamIndex, const VkBuffer vb, IndexT offsetVertexIndex)
{
	// hmm, build pipeline before we start setting this stuff
	this->BuildRenderPipeline();

	//RenderDeviceBase::SetStreamVertexBuffer(streamIndex, vb, offsetVertexIndex);
	VkCmdBufferThread::Command cmd;
	cmd.type = VkCmdBufferThread::InputAssemblyVertex;
	cmd.vbo.buffer = vb;
	cmd.vbo.index = streamIndex;
	cmd.vbo.offset = offsetVertexIndex;
	this->PushToThread(cmd, this->currentDrawThread);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::SetVertexLayout(const VkPipelineVertexInputStateCreateInfo* vl)
{
	n_assert(this->currentProgram != 0);
	this->SetVertexLayoutPipelineInfo(vl);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::SetIndexBuffer(const VkBuffer ib, IndexT offsetIndex, const IndexType::Code type)
{
	// hmm, build pipeline before we start setting this stuff
	this->BuildRenderPipeline();

	VkCmdBufferThread::Command cmd;
	cmd.type = VkCmdBufferThread::InputAssemblyIndex;
	cmd.ibo.buffer = ib;
	cmd.ibo.indexType = type == IndexType::Index16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
	cmd.ibo.offset = offsetIndex;
	this->PushToThread(cmd, this->currentDrawThread);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::SetPrimitiveTopology(const CoreGraphics::PrimitiveTopology::Code& topo)
{
	VkPrimitiveTopology comp = VkTypes::AsVkPrimitiveType(topo);
	if (this->inputInfo.topology != comp)
	{
		this->inputInfo.topology = comp;
		this->inputInfo.primitiveRestartEnable = VK_FALSE;
		this->SetInputLayoutInfo(&this->inputInfo);
	}	
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::Compute(int dimX, int dimY, int dimZ)
{
	RenderDeviceBase::Compute(dimX, dimY, dimZ);
	vkCmdDispatch(CommandBufferGetVk(this->mainCmdDrawBuffer), dimX, dimY, dimZ);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::BeginPass(const CoreGraphics::PassId pass)
{
	RenderDeviceBase::BeginPass(pass);
	this->currentCommandState = SharedState;

	// set info
	this->SetFramebufferLayoutInfo(PassGetVkFramebufferInfo(pass));
	this->database.SetPass(pass);
	this->database.SetSubpass(0);

	const VkRenderPassBeginInfo& info = PassGetVkRenderPassBeginInfo(pass);
	vkCmdBeginRenderPass(CommandBufferGetVk(this->mainCmdDrawBuffer), &info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	// run this phase for scheduler
	this->scheduler.ExecuteCommandPass(VkScheduler::OnBeginPass);

	this->passInfo.framebuffer = info.framebuffer;
	this->passInfo.renderPass = info.renderPass;
	this->passInfo.subpass = 0;
	this->passInfo.pipelineStatistics = 0;
	this->passInfo.queryFlags = 0;
	this->passInfo.occlusionQueryEnable = VK_FALSE;

	const Util::FixedArray<VkRect2D>& scissors = PassGetVkRects(pass);
	this->numScissors = scissors.Size();
	this->scissors = scissors.Begin();
	const Util::FixedArray<VkViewport>& viewports = PassGetVkViewports(pass);
	this->numViewports = viewports.Size();
	this->viewports = viewports.Begin();
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::SetToNextSubpass()
{
	RenderDeviceBase::SetToNextSubpass();

	// end this batch of draw threads
	this->EndDrawThreads();

	// run this phase for scheduler
	this->scheduler.ExecuteCommandPass(VkScheduler::OnNextSubpass);

	// progress to next subpass
	this->passInfo.subpass++;
	this->currentPipelineInfo.subpass++;
	this->currentPipelineBits &= ~PipelineBuilt; // hmm, this is really ugly, is it avoidable?
	this->database.SetSubpass(this->passInfo.subpass);
	vkCmdNextSubpass(CommandBufferGetVk(this->mainCmdDrawBuffer), VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	const Util::FixedArray<VkRect2D>& scissors = PassGetVkRects(this->pass);
	this->numScissors = scissors.Size();
	this->scissors = scissors.Begin();
	const Util::FixedArray<VkViewport>& viewports = PassGetVkViewports(this->pass);
	this->numViewports = viewports.Size();
	this->viewports = viewports.Begin();
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::BeginBatch(CoreGraphics::FrameBatchType::Code batchType)
{
	RenderDeviceBase::BeginBatch(batchType);

	// set to the shared state
	this->currentCommandState = SharedState;
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::Draw()
{
	VkCmdBufferThread::Command cmd;
	cmd.type = VkCmdBufferThread::Draw;
	cmd.draw.baseIndex = this->primitiveGroup.GetBaseIndex();
	cmd.draw.baseVertex = this->primitiveGroup.GetBaseVertex();
	cmd.draw.baseInstance = 0;
	cmd.draw.numIndices = this->primitiveGroup.GetNumIndices();
	cmd.draw.numVerts = this->primitiveGroup.GetNumVertices();
	cmd.draw.numInstances = 1;
	this->PushToThread(cmd, this->currentDrawThread);

	// go to next thread
	_incr_counter(RenderDeviceNumDrawCalls, 1);
	_incr_counter(RenderDeviceNumPrimitives, this->primitiveGroup.GetNumVertices()/3);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::DrawIndexedInstanced(SizeT numInstances, IndexT baseInstance)
{
	VkCmdBufferThread::Command cmd;
	cmd.type = VkCmdBufferThread::Draw;
	cmd.draw.baseIndex = this->primitiveGroup.GetBaseIndex();
	cmd.draw.baseVertex = this->primitiveGroup.GetBaseVertex();
	cmd.draw.baseInstance = baseInstance;
	cmd.draw.numIndices = this->primitiveGroup.GetNumIndices();
	cmd.draw.numVerts = this->primitiveGroup.GetNumVertices();
	cmd.draw.numInstances = numInstances;
	this->PushToThread(cmd, this->currentDrawThread);

	// go to next thread
	_incr_counter(RenderDeviceNumDrawCalls, 1);
	_incr_counter(RenderDeviceNumPrimitives, this->primitiveGroup.GetNumIndices() * numInstances / 3);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::EndBatch()
{
	// transition back to the shared state
	this->currentCommandState = SharedState;

	// when ending a batch, flush all threads
	if (this->numActiveThreads > 0)
	{
		IndexT i;
		for (i = 0; i < this->numActiveThreads; i++) this->FlushToThread(i);
	}

	RenderDeviceBase::EndBatch();
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::EndPass()
{
	//this->currentPipelineBits = 0;
	this->sharedDescriptorSets.Clear();
	this->currentCommandState = SharedState;

	// end draw threads, if any are remaining
	this->EndDrawThreads();

	// tell scheduler pass is ending
	this->scheduler.ExecuteCommandPass(VkScheduler::OnEndPass);

	// end render pass
	vkCmdEndRenderPass(CommandBufferGetVk(this->mainCmdDrawBuffer));
	RenderDeviceBase::EndPass();
}

//------------------------------------------------------------------------------
/**
	Ideally implement some way of using double-triple-N buffering here, by having more than one fence per frame.
*/
void
VkRenderDevice::EndFrame(IndexT frameIndex)
{
	RenderDeviceBase::EndFrame(frameIndex);

	// set back to the main state
	this->currentCommandState = MainState;

	// kick transfer commands
	this->subcontextHandler.InsertCommandBuffer(VkSubContextHandler::TransferContextType, CommandBufferGetVk(this->mainCmdTransferBuffer));
	this->subcontextHandler.Submit(VkSubContextHandler::TransferContextType, this->mainCmdTransFence, true);
	
	this->scheduler.ExecuteCommandPass(VkScheduler::OnMainTransferSubmitted);
	this->scheduler.EndTransfers();

	// submit compute stuff
	this->subcontextHandler.InsertCommandBuffer(VkSubContextHandler::ComputeContextType, CommandBufferGetVk(this->mainCmdComputeBuffer));
	this->subcontextHandler.Submit(VkSubContextHandler::ComputeContextType, this->mainCmdCmpFence, true);

	this->scheduler.ExecuteCommandPass(VkScheduler::OnMainComputeSubmitted);
	this->scheduler.EndComputes();
	
	// submit draw stuff
	this->subcontextHandler.InsertCommandBuffer(VkSubContextHandler::DrawContextType, CommandBufferGetVk(this->mainCmdDrawBuffer));
	this->subcontextHandler.Submit(VkSubContextHandler::DrawContextType, this->mainCmdDrawFence, true);

	this->scheduler.ExecuteCommandPass(VkScheduler::OnMainDrawSubmitted);
	this->scheduler.EndDraws();

	static VkFence fences[] = { this->mainCmdDrawFence, this->mainCmdTransFence, this->mainCmdCmpFence };
	this->WaitForFences(fences, 3, true);

	CmdBufferClearInfo clearInfo =
	{
		false
	};
	CmdBufferClear(this->mainCmdTransferBuffer, clearInfo);
	CmdBufferClear(this->mainCmdComputeBuffer, clearInfo);
	CmdBufferClear(this->mainCmdDrawBuffer, clearInfo);
	CmdBufferClear(this->mainCmdSparseBuffer, clearInfo);

	// run end-of-frame commands
	this->scheduler.ExecuteCommandPass(VkScheduler::OnEndFrame);

	// reset state
	this->inputInfo.topology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	this->vertexLayout = nullptr;
	this->currentProgram = 0;
	this->currentPipelineInfo.pVertexInputState = nullptr;
	this->currentPipelineInfo.pInputAssemblyState = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::Present()
{
	this->frameId++; 

// 	// get active window
// 	Ptr<CoreGraphics::Window> wnd = CoreGraphics::DisplayDevice::Instance()->GetCurrentWindow();
// 
// 	// submit a sync point
// 	VkPipelineStageFlags flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
// 	const VkSubmitInfo submitInfo = 
// 	{
// 		VK_STRUCTURE_TYPE_SUBMIT_INFO,
// 		NULL,
// 		1,
// 		&wnd->displaySemaphore,
// 		&flags, 
// 		0,
// 		NULL,
// 		0,
// 		NULL
// 	};
// 	VkResult res = vkQueueSubmit(this->drawQueue, 1, &submitInfo, NULL);
// 	n_assert(res == VK_SUCCESS);
// 
// 	// present
// 	VkResult presentResults;
// 	const VkPresentInfoKHR info =
// 	{
// 		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
// 		NULL,
// 		0,
// 		NULL,
// 		1,
// 		&wnd->swapchain,
// 		&wnd->currentBackbuffer,
// 		&presentResults
// 	};
// 	res = vkQueuePresentKHR(this->drawQueue, &info);
// 
// 
// 	if (res == VK_ERROR_OUT_OF_DATE_KHR)
// 	{
// 		// window has been resized!
// 		n_printf("Window resized!");
// 	}
// 	else
// 	{
// 		n_assert(res == VK_SUCCESS);
// 	}

	//res = vkQueueWaitIdle(this->drawQueue);
	//n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::SetScissorRect(const Math::rectangle<int>& rect, int index)
{
	VkRect2D sc;
	sc.extent.height = rect.height();
	sc.extent.width = rect.width();
	sc.offset.x = rect.left;
	sc.offset.y = rect.top;
	if (this->currentCommandState == LocalState)
	{
		VkCmdBufferThread::Command cmd;
		cmd.type = VkCmdBufferThread::ScissorRect;
		cmd.scissorRect.index = index;
		cmd.scissorRect.sc = sc;
		this->PushToThread(cmd, this->currentDrawThread);
	}	
	else
	{
		this->scissors[index] = sc;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::SetViewport(const Math::rectangle<int>& rect, int index)
{
	VkViewport vp;
	vp.width = (float)rect.width();
	vp.height = (float)rect.height();
	vp.x = (float)rect.left;
	vp.y = (float)rect.top;
	if (this->currentCommandState == LocalState)
	{
		VkCmdBufferThread::Command cmd;
		cmd.type = VkCmdBufferThread::Viewport;
		cmd.viewport.index = index;
		cmd.viewport.vp = vp;
		this->PushToThread(cmd, this->currentDrawThread);
	}
	else
	{
		this->viewports[index] = vp;
	}
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ImageFileFormat::Code
VkRenderDevice::SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream)
{
	return CoreGraphics::ImageFileFormat::InvalidImageFileFormat;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ImageFileFormat::Code
VkRenderDevice::SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream, const Math::rectangle<int>& rect, int x, int y)
{
	return CoreGraphics::ImageFileFormat::InvalidImageFileFormat;
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::RequestPool()
{
	VkDescriptorPoolCreateInfo poolInfo =
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		nullptr,
		VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		INT_MAX,
		11,
		this->poolSizes
	};

	VkDescriptorPool pool;
	vkCreateDescriptorPool(this->devices[this->currentDevice], &poolInfo, nullptr, &pool);
	this->descriptorPools.Append(pool);
}

//------------------------------------------------------------------------------
/**
*/
const VkQueue
VkRenderDevice::GetQueue(const VkSubContextHandler::SubContextType type, const IndexT index)
{
	switch (type)
	{
	case VkSubContextHandler::DrawContextType:
		return this->subcontextHandler.drawQueues[index];
		break;
	case VkSubContextHandler::ComputeContextType:
		return this->subcontextHandler.computeQueues[index];
		break;
	case VkSubContextHandler::TransferContextType:
		return this->subcontextHandler.transferQueues[index];
		break;
	case VkSubContextHandler::SparseContextType:
		return this->subcontextHandler.sparseQueues[index];
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::DisplayResized(SizeT width, SizeT height)
{
	// TODO: implement me!
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::BindGraphicsPipelineInfo(const VkGraphicsPipelineCreateInfo& shader, const uint32_t programId)
{
	if (this->currentProgram != programId || !(this->currentPipelineBits & ShaderInfoSet))
	{
		this->database.SetShader(programId);
		this->currentPipelineBits |= ShaderInfoSet;

		this->blendInfo.pAttachments = shader.pColorBlendState->pAttachments;
		memcpy(this->blendInfo.blendConstants, shader.pColorBlendState->blendConstants, sizeof(float) * 4);
		this->blendInfo.logicOp = shader.pColorBlendState->logicOp;
		this->blendInfo.logicOpEnable = shader.pColorBlendState->logicOpEnable;

		this->currentPipelineInfo.pDepthStencilState = shader.pDepthStencilState;
		this->currentPipelineInfo.pRasterizationState = shader.pRasterizationState;
		this->currentPipelineInfo.pMultisampleState = shader.pMultisampleState;
		this->currentPipelineInfo.pDynamicState = shader.pDynamicState;
		this->currentPipelineInfo.pTessellationState = shader.pTessellationState;
		this->currentPipelineInfo.stageCount = shader.stageCount;
		this->currentPipelineInfo.pStages = shader.pStages;
		this->currentPipelineInfo.layout = shader.layout;
		this->currentPipelineBits &= ~PipelineBuilt;
		this->currentProgram = programId;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::SetVertexLayoutPipelineInfo(const VkPipelineVertexInputStateCreateInfo* vertexLayout)
{
	if (this->currentPipelineInfo.pVertexInputState != vertexLayout || !(this->currentPipelineBits & VertexLayoutInfoSet))
	{
		this->database.SetVertexLayout(vertexLayout);
		this->currentPipelineBits |= VertexLayoutInfoSet;
		this->currentPipelineInfo.pVertexInputState = vertexLayout;
		//this->vertexInfo = vertexLayout;

		this->currentPipelineBits &= ~PipelineBuilt;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::SetFramebufferLayoutInfo(const VkGraphicsPipelineCreateInfo& framebufferLayout)
{
	this->currentPipelineBits |= FramebufferLayoutInfoSet;
	this->currentPipelineInfo.renderPass = framebufferLayout.renderPass;
	this->currentPipelineInfo.subpass = framebufferLayout.subpass;
	this->currentPipelineInfo.pViewportState = framebufferLayout.pViewportState;
	this->currentPipelineBits &= ~PipelineBuilt;
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::SetInputLayoutInfo(VkPipelineInputAssemblyStateCreateInfo* inputLayout)
{
	if (this->currentPipelineInfo.pInputAssemblyState != inputLayout || !(this->currentPipelineBits & InputLayoutInfoSet))
	{
		this->database.SetInputLayout(inputLayout);
		this->currentPipelineBits |= InputLayoutInfoSet;
		this->currentPipelineInfo.pInputAssemblyState = inputLayout;
		this->currentPipelineBits &= ~PipelineBuilt;
	}	
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::BindDescriptorsGraphics(const VkDescriptorSet* descriptors, const VkPipelineLayout& layout, uint32_t baseSet, uint32_t setCount, const uint32_t* offsets, uint32_t offsetCount, bool shared)
{
	// if we are in the local state, push directly to thread
	if (this->currentCommandState == LocalState)
	{
		VkCmdBufferThread::Command cmd;
		cmd.type = VkCmdBufferThread::BindDescriptors;
		cmd.descriptor.baseSet = baseSet;
		cmd.descriptor.numSets = setCount;
		cmd.descriptor.sets = descriptors;
		cmd.descriptor.layout = layout;
		cmd.descriptor.numOffsets = offsetCount;
		cmd.descriptor.offsets = offsets;
		cmd.descriptor.type = VK_PIPELINE_BIND_POINT_GRAPHICS;
		this->PushToThread(cmd, this->currentDrawThread);
	}
	else
	{
		// if the state should be applied in a shared manner, or we are in the shared phase (inside a Pass or Batch), push to all threads
		if (shared || this->currentCommandState == SharedState)
		{
			VkCmdBufferThread::Command cmd;
			cmd.type = VkCmdBufferThread::BindDescriptors;
			cmd.descriptor.baseSet = baseSet;
			cmd.descriptor.numSets = setCount;
			cmd.descriptor.sets = descriptors;
			cmd.descriptor.layout = layout;
			cmd.descriptor.numOffsets = offsetCount;
			cmd.descriptor.offsets = offsets;
			cmd.descriptor.type = VK_PIPELINE_BIND_POINT_GRAPHICS;
			this->sharedDescriptorSets.Append(cmd);
		}
		else // if we are in the 'main state', we just delay the descriptor binds until we have threads
		{
			VkDeferredCommand cmd;
			cmd.del.type = VkDeferredCommand::BindDescriptorSets;
			cmd.del.descSetBind.baseSet = baseSet;
			cmd.del.descSetBind.numSets = setCount;
			cmd.del.descSetBind.sets = descriptors;
			cmd.del.descSetBind.layout = layout;
			cmd.del.descSetBind.numOffsets = offsetCount;
			cmd.del.descSetBind.offsets = offsets;
			cmd.del.descSetBind.type = VK_PIPELINE_BIND_POINT_GRAPHICS;
			cmd.dev = this->dev;
			this->scheduler.PushCommand(cmd, VkScheduler::OnBindGraphicsPipeline);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::BindDescriptorsCompute(const VkDescriptorSet* descriptors, const VkPipelineLayout& layout, uint32_t baseSet, uint32_t setCount, const uint32_t* offsets, uint32_t offsetCount)
{
	if (this->inBeginFrame)
	{
		vkCmdBindDescriptorSets(CommandBufferGetVk(this->mainCmdDrawBuffer), VK_PIPELINE_BIND_POINT_COMPUTE, layout, baseSet, setCount, descriptors, offsetCount, offsets);
	}
	else
	{
		VkDeferredCommand cmd;
		cmd.del.type = VkDeferredCommand::BindDescriptorSets;
		cmd.del.descSetBind.baseSet = baseSet;
		cmd.del.descSetBind.layout = layout;
		cmd.del.descSetBind.numOffsets = offsetCount;
		cmd.del.descSetBind.offsets = offsets;
		cmd.del.descSetBind.numSets = setCount;
		cmd.del.descSetBind.sets = descriptors;
		cmd.del.descSetBind.type = VK_PIPELINE_BIND_POINT_COMPUTE;
		cmd.dev = this->dev;
		this->scheduler.PushCommand(cmd, VkScheduler::OnBindComputePipeline);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::UpdatePushRanges(const VkShaderStageFlags& stages, const VkPipelineLayout& layout, uint32_t offset, uint32_t size, void* data)
{
	VkCmdBufferThread::Command cmd;
	cmd.type = VkCmdBufferThread::PushRange;
	cmd.pushranges.layout = layout;
	cmd.pushranges.offset = offset;
	cmd.pushranges.size = size;
	cmd.pushranges.stages = stages;

	// copy data here, will be deleted in the thread
	cmd.pushranges.data = Memory::Alloc(Memory::ScratchHeap, size);
	memcpy(cmd.pushranges.data, data, size);
	this->PushToThread(cmd, this->currentDrawThread);
}

//------------------------------------------------------------------------------
/**
*/
void
Vulkan::VkRenderDevice::SubmitToQueue(VkQueue queue, VkPipelineStageFlags flags, uint32_t numBuffers, VkCommandBuffer* buffers)
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
VkRenderDevice::SubmitToQueue(VkQueue queue, VkFence fence)
{
	// submit to queue
	VkResult res = vkQueueSubmit(queue, 0, VK_NULL_HANDLE, fence);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::BindComputePipeline(const VkPipeline& pipeline, const VkPipelineLayout& layout)
{
	// bind compute pipeline
	this->currentBindPoint = VkShaderProgram::Compute;

	// bind shared descriptors
	VkShaderServer::Instance()->BindTextureDescriptorSets();
	VkTransformDevice::Instance()->BindCameraDescriptorSets();

	// bind pipeline
	vkCmdBindPipeline(CommandBufferGetVk(this->mainCmdDrawBuffer), VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

	// run command pass
	this->scheduler.ExecuteCommandPass(VkScheduler::OnBindComputePipeline);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::UnbindPipeline()
{
	this->currentBindPoint = VkShaderProgram::InvalidType;
	this->currentPipelineBits &= ~ShaderInfoSet;
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::CreateAndBindGraphicsPipeline()
{
	VkPipeline pipeline = this->database.GetCompiledPipeline();
	_incr_counter(NumPipelinesBuilt, 1);

	// all future commands are put on the local state
	this->currentCommandState = LocalState;
	
	// begin new draw thread
	this->currentDrawThread = (this->currentDrawThread + 1) % NumDrawThreads;
	if (this->numActiveThreads < NumDrawThreads)
	{
		// if we want a new thread, make one, then bind shared descriptor sets
		this->BeginDrawThread();
		this->BindSharedDescriptorSets();
	}

	VkCmdBufferThread::Command cmd;

	// send pipeline bind command, this is the first step in our procedure, so we use this as a trigger to switch threads
	cmd.type = VkCmdBufferThread::GraphicsPipeline;
	cmd.pipeline = pipeline;
	this->PushToThread(cmd, this->currentDrawThread);

	uint32_t i;
	for (i = 0; i < this->numScissors; i++)
	{
		cmd.type = VkCmdBufferThread::ScissorRect;
		cmd.scissorRect.sc = this->scissors[i];
		cmd.scissorRect.index = i;
		this->PushToThread(cmd, this->currentDrawThread);
	}
	
	for (i = 0; i < this->numViewports; i++)
	{
		cmd.type = VkCmdBufferThread::Viewport;
		cmd.viewport.vp = this->viewports[i];
		cmd.viewport.index = i;
		this->PushToThread(cmd, this->currentDrawThread);
	}
	this->viewportsDirty[this->currentDrawThread] = false;
	this->scheduler.ExecuteCommandPass(VkScheduler::OnBindGraphicsPipeline);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::BuildRenderPipeline()
{
	n_assert((this->currentPipelineBits & AllInfoSet) != 0);
	n_assert(this->inBeginBatch);
	this->currentBindPoint = VkShaderProgram::Graphics;
	if ((this->currentPipelineBits & PipelineBuilt) == 0)
	{
		this->CreateAndBindGraphicsPipeline();
		this->currentPipelineBits |= PipelineBuilt;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::InsertBarrier(const BarrierId barrier)
{
	if (this->numActiveThreads > 0)
	{
		VkCmdBufferThread::Command cmd;
		cmd.type = VkCmdBufferThread::Barrier;
		cmd.barrier.dep = barrier->vkDep;
		cmd.barrier.srcMask = barrier->vkSrcFlags;
		cmd.barrier.dstMask = barrier->vkDstFlags;
		cmd.barrier.memoryBarrierCount = barrier->vkNumMemoryBarriers;
		cmd.barrier.memoryBarriers = barrier->vkMemoryBarriers;
		cmd.barrier.bufferBarrierCount = barrier->vkNumBufferBarriers;
		cmd.barrier.bufferBarriers = barrier->vkBufferBarriers;
		cmd.barrier.imageBarrierCount = barrier->vkNumImageBarriers;
		cmd.barrier.imageBarriers = barrier->vkImageBarriers;
		this->PushToThread(cmd, currentDrawThread);
	}
	else
	{
		vkCmdPipelineBarrier(CommandBufferGetVk(this->mainCmdDrawBuffer),
			barrier->vkSrcFlags, 
			barrier->vkDstFlags, 
			barrier->vkDep,
			barrier->vkNumMemoryBarriers, barrier->vkMemoryBarriers,
			barrier->vkNumBufferBarriers, barrier->vkBufferBarriers,
			barrier->vkNumImageBarriers, barrier->vkImageBarriers);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::WaitForFences(VkFence* fences, uint32_t numFences, bool waitAll)
{
	VkResult res = vkWaitForFences(this->dev, numFences, fences, waitAll, UINT_MAX);
	n_assert(res == VK_SUCCESS);
	res = vkResetFences(this->dev, numFences, fences);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::BindSharedDescriptorSets()
{
	VkTransformDevice::Instance()->BindCameraDescriptorSets();
	VkShaderServer::Instance()->BindTextureDescriptorSets();

	IndexT i;
	for (i = 0; i < this->sharedDescriptorSets.Size(); i++)
	{
		this->PushToThread(this->sharedDescriptorSets[i], this->currentDrawThread);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::BeginDrawThread()
{
	n_assert(this->numActiveThreads < NumDrawThreads);

	// allocate command buffer
	VkCommandBufferAllocateInfo info =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		NULL,
		this->dispatchableCmdDrawBufferPool[this->currentDrawThread],
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,
		1
	};
	vkAllocateCommandBuffers(this->dev, &info, &this->dispatchableDrawCmdBuffers[this->currentDrawThread]);

	// tell thread to begin command buffer recording
	VkCommandBufferBeginInfo begin =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		NULL,
		VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
		&this->passInfo
	};

	VkCmdBufferThread::Command cmd;
	cmd.type = VkCmdBufferThread::BeginCommand;
	cmd.bgCmd.buf = this->dispatchableDrawCmdBuffers[this->currentDrawThread];
	cmd.bgCmd.info = begin;
	this->PushToThread(cmd, this->currentDrawThread);

	// run begin command buffer pass
	this->scheduler.ExecuteCommandPass(VkScheduler::OnBeginDrawThread);
	this->numActiveThreads++;
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::EndDrawThreads()
{
	if (this->numActiveThreads > 0)
	{
		IndexT i;
		for (i = 0; i < this->numActiveThreads; i++)
		{
			// push remaining cmds to thread
			this->FlushToThread(i);

			// end thread
			VkCmdBufferThread::Command cmd;
			cmd.type = VkCmdBufferThread::EndCommand;
			this->PushToThread(cmd, i, false);

			cmd.type = VkCmdBufferThread::Sync;
			cmd.syncEvent = this->drawCompletionEvents[i];
			this->PushToThread(cmd, i, false);
			this->drawCompletionEvents[i]->Wait();
			this->drawCompletionEvents[i]->Reset();
		}

		// run end-of-threads pass
		this->scheduler.ExecuteCommandPass(VkScheduler::OnDrawThreadsSubmitted);

		// execute commands
		vkCmdExecuteCommands(CommandBufferGetVk(this->mainCmdDrawBuffer), this->numActiveThreads, this->dispatchableDrawCmdBuffers);

		// destroy command buffers
		for (i = 0; i < this->numActiveThreads; i++)
		{
			VkDeferredCommand cmd;
			cmd.del.type = VkDeferredCommand::FreeCmdBuffers;
			cmd.del.cmdbufferfree.buffers[0] = this->dispatchableDrawCmdBuffers[i];
			cmd.del.cmdbufferfree.numBuffers = 1;
			cmd.del.cmdbufferfree.pool = this->dispatchableCmdDrawBufferPool[i];
			cmd.dev = this->dev;
			this->scheduler.PushCommand(cmd, VkScheduler::OnHandleDrawFences);
		}
		this->currentDrawThread = NumDrawThreads-1;
		this->numActiveThreads = 0;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::NextThread()
{
	this->currentDrawThread = (this->currentDrawThread + 1) % NumDrawThreads;
	this->numUsedThreads = Math::n_min(this->numUsedThreads + 1, NumDrawThreads);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::PushToThread(const VkCmdBufferThread::Command& cmd, const IndexT& index, bool allowStaging)
{
	//this->threadCmds[index].Append(cmd);
	if (allowStaging)
	{
		this->threadCmds[index].Append(cmd);
		if (this->threadCmds[index].Size() == 250)
		{
			this->drawThreads[index]->PushCommands(this->threadCmds[index]);
			this->threadCmds[index].Reset();
		}
	}
	else
	{
		this->drawThreads[index]->PushCommand(cmd);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::FlushToThread(const IndexT& index)
{
	if (!this->threadCmds[index].IsEmpty())
	{
		this->drawThreads[index]->PushCommands(this->threadCmds[index]);
		this->threadCmds[index].Clear();
	}
}

//------------------------------------------------------------------------------
/**
*/
bool
VkRenderDevice::AsyncTransferSupported()
{
	return VkRenderDevice::transferQueue != VkRenderDevice::drawQueue;
}

//------------------------------------------------------------------------------
/**
*/
bool
VkRenderDevice::AsyncComputeSupported()
{
	return VkRenderDevice::computeQueue != VkRenderDevice::drawQueue;
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::Copy(const CoreGraphics::TextureId from, Math::rectangle<SizeT> fromRegion, const CoreGraphics::TextureId to, Math::rectangle<SizeT> toRegion)
{
	RenderDeviceBase::Copy(from, fromRegion, to, toRegion);
	n_assert(!this->inBeginPass);
	VkImageCopy region;
	region.dstOffset = { fromRegion.left, fromRegion.top, 0 };
	region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	region.extent = { (uint32_t)toRegion.width(), (uint32_t)toRegion.height(), 1 };
	region.srcOffset = { toRegion.left, toRegion.top, 0 };
	region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	vkCmdCopyImage(CommandBufferGetVk(this->mainCmdDrawBuffer), TextureGetVk(from), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, TextureGetVk(to), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::Blit(const CoreGraphics::RenderTextureId from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const CoreGraphics::RenderTextureId to, Math::rectangle<SizeT> toRegion, IndexT toMip)
{
	
	this->Blit(TextureGetVk(from), fromRegion, fromMip, to->GetTexture(), toRegion, toMip);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::Blit(const Ptr<CoreGraphics::Texture>& from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const Ptr<CoreGraphics::Texture>& to, Math::rectangle<SizeT> toRegion, IndexT toMip)
{
	n_assert(!this->inBeginPass);
	VkImageBlit blit;
	blit.srcOffsets[0] = { fromRegion.left, fromRegion.top, 0 };
	blit.srcOffsets[1] = { fromRegion.right, fromRegion.bottom, 1 };
	blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)fromMip, 0, 1 };
	blit.dstOffsets[0] = { toRegion.left, toRegion.top, 0 };
	blit.dstOffsets[1] = { toRegion.right, toRegion.bottom, 1 };
	blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)toMip, 0, 1 };
	vkCmdBlitImage(CommandBufferGetVk(this->mainCmdDrawBuffer), from->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, to->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
}

} // namespace Vulkan
