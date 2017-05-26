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

VkDevice VkRenderDevice::dev;
VkDescriptorPool VkRenderDevice::descPool;
VkQueue VkRenderDevice::drawQueue;
VkQueue VkRenderDevice::computeQueue;
VkQueue VkRenderDevice::transferQueue;
Util::FixedArray<VkQueue> VkRenderDevice::queues;
VkInstance VkRenderDevice::instance;
VkPhysicalDevice VkRenderDevice::physicalDev;
VkPipelineCache VkRenderDevice::cache;

VkCommandPool VkRenderDevice::mainCmdCmpPool;
VkCommandPool VkRenderDevice::mainCmdTransPool;
VkCommandPool VkRenderDevice::mainCmdDrawPool;
VkCommandPool VkRenderDevice::immediateCmdDrawPool;
VkCommandPool VkRenderDevice::immediateCmdTransPool;
VkCommandBuffer VkRenderDevice::mainCmdDrawBuffer;
VkCommandBuffer VkRenderDevice::mainCmdCmpBuffer;
VkCommandBuffer VkRenderDevice::mainCmdTransBuffer;

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

	// get device props and features
	vkGetPhysicalDeviceProperties(this->physicalDev, &this->deviceProps);
	vkGetPhysicalDeviceFeatures(this->physicalDev, &this->deviceFeatures);

	// get number of queues
	vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDev, &this->numQueues, NULL);
	n_assert(this->numQueues > 0);

	// now get queues from device
	vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDev, &this->numQueues, this->queuesProps);
	vkGetPhysicalDeviceMemoryProperties(this->physicalDev, &this->memoryProps);

	// resize queues list
	VkRenderDevice::queues.Resize(numQueues);

	this->drawQueueIdx = UINT32_MAX;
	this->computeQueueIdx = UINT32_MAX;
	this->transferQueueIdx = UINT32_MAX;

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
			/*
			uint32_t j;
			for (j = 0; j < numQueues; j++)
			{
				if (canPresent[i] == VK_TRUE)
				{
					
					
					break;
				}
			}
			*/
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
	vkGetPhysicalDeviceFeatures(this->physicalDev, &features);

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(this->physicalDev, &props);
	
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
	res = vkCreateDevice(this->physicalDev, &deviceInfo, NULL, &this->dev);
	n_assert(res == VK_SUCCESS);

	vkGetDeviceQueue(this->dev, this->drawQueueFamily, this->drawQueueIdx, &VkRenderDevice::queues[this->drawQueueFamily + this->drawQueueIdx]);
	vkGetDeviceQueue(this->dev, this->computeQueueFamily, this->computeQueueIdx, &VkRenderDevice::queues[this->computeQueueFamily + this->computeQueueIdx]);
	vkGetDeviceQueue(this->dev, this->transferQueueFamily, this->transferQueueIdx, &VkRenderDevice::queues[this->transferQueueFamily + this->transferQueueIdx]);

	// grab queues
	this->drawQueue = VkRenderDevice::queues[this->drawQueueFamily];
	this->computeQueue = VkRenderDevice::queues[this->computeQueueFamily];
	this->transferQueue = VkRenderDevice::queues[this->transferQueueFamily];

	VkPipelineCacheCreateInfo cacheInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
		NULL,
		0,
		0,
		NULL
	};

	// create cache
	res = vkCreatePipelineCache(this->dev, &cacheInfo, NULL, &this->cache);
	n_assert(res == VK_SUCCESS);

	VkDescriptorPoolSize sizes[11];
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
		sizes[i].descriptorCount = descCounts[i];
		sizes[i].type = types[i];
	}
	
	VkDescriptorPoolCreateInfo poolInfo =
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		NULL,
		VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		VkPoolMaxSets,
		11,
		sizes
	};

	res = vkCreateDescriptorPool(this->dev, &poolInfo, NULL, &this->descPool);
	n_assert(res == VK_SUCCESS);

	// create command pool for graphics
	VkCommandPoolCreateInfo cmdPoolInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		NULL,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		this->drawQueueFamily
	};

	res = vkCreateCommandPool(this->dev, &cmdPoolInfo, NULL, &this->mainCmdDrawPool);
	n_assert(res == VK_SUCCESS);

	cmdPoolInfo.queueFamilyIndex = this->computeQueueFamily;
	res = vkCreateCommandPool(this->dev, &cmdPoolInfo, NULL, &this->mainCmdCmpPool);
	n_assert(res == VK_SUCCESS);

	cmdPoolInfo.queueFamilyIndex = this->transferQueueFamily;
	res = vkCreateCommandPool(this->dev, &cmdPoolInfo, NULL, &this->mainCmdTransPool);
	n_assert(res == VK_SUCCESS);

	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	res = vkCreateCommandPool(this->dev, &cmdPoolInfo, NULL, &this->immediateCmdTransPool);
	n_assert(res == VK_SUCCESS);

	cmdPoolInfo.queueFamilyIndex = this->drawQueueFamily;
	res = vkCreateCommandPool(this->dev, &cmdPoolInfo, NULL, &this->immediateCmdDrawPool);
	n_assert(res == VK_SUCCESS);

	for (i = 0; i < NumDrawThreads; i++)
	{
		VkCommandPoolCreateInfo cmdPoolInfo =
		{
			VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			NULL,
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
			this->drawQueueFamily
		};
		res = vkCreateCommandPool(this->dev, &cmdPoolInfo, NULL, &this->dispatchableCmdDrawBufferPool[i]);
	}

	for (i = 0; i < NumTransferThreads; i++)
	{
		VkCommandPoolCreateInfo cmdPoolInfo =
		{
			VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			NULL,
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
			this->transferQueueFamily
		};
		res = vkCreateCommandPool(this->dev, &cmdPoolInfo, NULL, &this->dispatchableCmdTransBufferPool[i]);
	}

	for (i = 0; i < NumComputeThreads; i++)
	{
		VkCommandPoolCreateInfo cmdPoolInfo =
		{
			VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			NULL,
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
			this->computeQueueFamily
		};
		res = vkCreateCommandPool(this->dev, &cmdPoolInfo, NULL, &this->dispatchableCmdCompBufferPool[i]);
	}
	
	// create main command buffer for graphics
	VkCommandBufferAllocateInfo cmdAllocInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		NULL,
		this->mainCmdDrawPool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		1
	};
	res = vkAllocateCommandBuffers(VkRenderDevice::dev, &cmdAllocInfo, &this->mainCmdDrawBuffer);
	n_assert(res == VK_SUCCESS);

	// create main command buffer for computes
	cmdAllocInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		NULL,
		this->mainCmdCmpPool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		1
	};
	res = vkAllocateCommandBuffers(VkRenderDevice::dev, &cmdAllocInfo, &this->mainCmdCmpBuffer);
	n_assert(res == VK_SUCCESS);

	// create main command buffer for transfers
	cmdAllocInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		NULL,
		this->mainCmdTransPool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		1
	};
	res = vkAllocateCommandBuffers(VkRenderDevice::dev, &cmdAllocInfo, &this->mainCmdTransBuffer);
	n_assert(res == VK_SUCCESS);

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

	res = vkCreateFence(this->dev, &fenceInfo, NULL, &this->mainCmdDrawFence);
	n_assert(res == VK_SUCCESS);
	res = vkCreateFence(this->dev, &fenceInfo, NULL, &this->mainCmdCmpFence);
	n_assert(res == VK_SUCCESS);
	res = vkCreateFence(this->dev, &fenceInfo, NULL, &this->mainCmdTransFence);
	n_assert(res == VK_SUCCESS);

	VkEventCreateInfo eventInfo = 
	{
		VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
		NULL,
		0
	};
	res = vkCreateEvent(this->dev, &eventInfo, NULL, &this->mainCmdDrawEvent);
	n_assert(res == VK_SUCCESS);
	res = vkCreateEvent(this->dev, &eventInfo, NULL, &this->mainCmdCmpEvent);
	n_assert(res == VK_SUCCESS);
	res = vkCreateEvent(this->dev, &eventInfo, NULL, &this->mainCmdTransEvent);
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
	this->vertexLayout = NULL;
	this->currentProgram = NULL;
	this->currentPipelineInfo.pVertexInputState = NULL;
	this->currentPipelineInfo.pInputAssemblyState = NULL;

	// create pipeline database
	this->database = VkPipelineDatabase::Create();

	// setup scheduler
	this->scheduler = VkScheduler::Create();

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

	this->database = 0;

	size_t size;
	vkGetPipelineCacheData(this->dev, this->cache, &size, NULL);
	uint8_t* data = (uint8_t*)Memory::Alloc(Memory::ScratchHeap, size);
	vkGetPipelineCacheData(this->dev, this->cache, &size, data);
	Util::String path = Util::String::Sprintf("bin:%s_vkpipelinecache", App::Application::Instance()->GetAppTitle().AsCharPtr());
	Ptr<IO::Stream> cachedData = IO::IoServer::Instance()->CreateStream(path);
	cachedData->SetAccessMode(IO::Stream::WriteAccess);
	if (cachedData->Open())
	{
		cachedData->Write(data, size);
		cachedData->Close();
	}

	// wait for all commands to be done first
	VkResult res = vkQueueWaitIdle(this->drawQueue);	
	n_assert(res == VK_SUCCESS);
	res = vkQueueWaitIdle(this->transferQueue);
	n_assert(res == VK_SUCCESS);
	res = vkQueueWaitIdle(this->computeQueue);
	n_assert(res == VK_SUCCESS);

	IndexT i;
	for (i = 0; i < NumDrawThreads; i++)
	{
		this->drawThreads[i]->Stop();
		this->drawThreads[i] = 0;
		n_delete(this->drawCompletionEvents[i]);

		vkDestroyCommandPool(this->dev, this->dispatchableCmdDrawBufferPool[i], NULL);
	}

	for (i = 0; i < NumTransferThreads; i++)
	{
		this->transThreads[i]->Stop();
		this->transThreads[i] = 0;
		n_delete(this->transCompletionEvents[i]);

		vkDestroyCommandPool(this->dev, this->dispatchableCmdTransBufferPool[i], NULL);
	}

	for (i = 0; i < NumComputeThreads; i++)
	{
		this->compThreads[i]->Stop();
		this->compThreads[i] = 0;
		n_delete(this->compCompletionEvents[i]);

		vkDestroyCommandPool(this->dev, this->dispatchableCmdCompBufferPool[i], NULL);
	}

	// free our main buffers, our secondary buffers should be fine so the pools should be free to destroy
	vkFreeCommandBuffers(this->dev, this->mainCmdDrawPool, 1, &this->mainCmdDrawBuffer);
	vkFreeCommandBuffers(this->dev, this->mainCmdCmpPool, 1, &this->mainCmdCmpBuffer);
	vkFreeCommandBuffers(this->dev, this->mainCmdTransPool, 1, &this->mainCmdTransBuffer);
	vkDestroyCommandPool(this->dev, this->mainCmdDrawPool, NULL);
	vkDestroyCommandPool(this->dev, this->mainCmdTransPool, NULL);
	vkDestroyCommandPool(this->dev, this->mainCmdCmpPool, NULL);
	vkDestroyCommandPool(this->dev, this->immediateCmdTransPool, NULL);
	vkDestroyCommandPool(this->dev, this->immediateCmdDrawPool, NULL);
	vkDestroyFence(this->dev, this->mainCmdDrawFence, NULL);
	vkDestroyFence(this->dev, this->mainCmdCmpFence, NULL);
	vkDestroyFence(this->dev, this->mainCmdTransFence, NULL);


	vkDestroyDevice(this->dev, NULL);
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

	if (gpuCount > 0)
	{
		res = vkEnumeratePhysicalDevices(this->instance, &gpuCount, this->devices);
		n_assert(res == VK_SUCCESS);

		// hmm, this is ugly, perhaps implement a way to get a proper device
		this->physicalDev = devices[0];
	}
	else
	{
		n_error("VkRenderDevice::SetupAdapter(): No GPU available.\n");
	}

	res = vkEnumerateDeviceExtensionProperties(this->physicalDev, NULL, &this->usedPhysicalExtensions, NULL);
	n_assert(res == VK_SUCCESS);

	if (this->usedPhysicalExtensions > 0)
	{
		res = vkEnumerateDeviceExtensionProperties(this->physicalDev, NULL, &this->usedPhysicalExtensions, this->physicalExtensions);

		uint32_t i;
		for (i = 0; i < this->usedPhysicalExtensions; i++)
		{
			this->deviceExtensionStrings[i] = this->physicalExtensions[i].extensionName;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::SetupBufferFormats()
{
	
}

//------------------------------------------------------------------------------
/**
*/
bool
VkRenderDevice::BeginFrame(IndexT frameIndex)
{
	const VkCommandBufferBeginInfo cmdInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		NULL,
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		VK_NULL_HANDLE
	};
	vkBeginCommandBuffer(this->mainCmdDrawBuffer, &cmdInfo);
	vkBeginCommandBuffer(this->mainCmdCmpBuffer, &cmdInfo);
	vkBeginCommandBuffer(this->mainCmdTransBuffer, &cmdInfo);

	// all commands are put on the main command buffer
	this->currentCommandState = MainState;

	this->scheduler->Begin();
	this->scheduler->ExecuteCommandPass(VkScheduler::OnHandleTransferFences);
	this->scheduler->ExecuteCommandPass(VkScheduler::OnHandleDrawFences);
	this->scheduler->ExecuteCommandPass(VkScheduler::OnHandleComputeFences);
	this->scheduler->ExecuteCommandPass(VkScheduler::OnBeginFrame);

	// reset current thread
	this->currentDrawThread = NumDrawThreads - 1;
	this->currentPipelineBits = 0;

	return RenderDeviceBase::BeginFrame(frameIndex);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::SetStreamVertexBuffer(IndexT streamIndex, const Ptr<CoreGraphics::VertexBuffer>& vb, IndexT offsetVertexIndex)
{
	// hmm, build pipeline before we start setting this stuff
	this->BuildRenderPipeline();

	RenderDeviceBase::SetStreamVertexBuffer(streamIndex, vb, offsetVertexIndex);
	VkCmdBufferThread::Command cmd;
	cmd.type = VkCmdBufferThread::InputAssemblyVertex;
	cmd.vbo.buffer = vb->GetVkBuffer();
	cmd.vbo.index = streamIndex;
	cmd.vbo.offset = offsetVertexIndex;
	this->PushToThread(cmd, this->currentDrawThread);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::SetVertexLayout(const Ptr<CoreGraphics::VertexLayout>& vl)
{
	n_assert(this->currentProgram.isvalid());
	RenderDeviceBase::SetVertexLayout(vl);
	this->SetVertexLayoutPipelineInfo(vl->GetDerivative(this->currentProgram));
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::SetIndexBuffer(const Ptr<CoreGraphics::IndexBuffer>& ib)
{
	// hmm, build pipeline before we start setting this stuff
	this->BuildRenderPipeline();

	VkCmdBufferThread::Command cmd;
	cmd.type = VkCmdBufferThread::InputAssemblyIndex;
	cmd.ibo.buffer = ib->GetVkBuffer();
	cmd.ibo.indexType = ib->GetIndexType() == IndexType::Index16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
	cmd.ibo.offset = 0;
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
VkRenderDevice::Compute(int dimX, int dimY, int dimZ, uint flag /*= NoBarrier*/)
{
	RenderDeviceBase::Compute(dimX, dimY, dimZ);
	vkCmdDispatch(this->mainCmdDrawBuffer, dimX, dimY, dimZ);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::BeginPass(const Ptr<CoreGraphics::Pass>& pass)
{
	RenderDeviceBase::BeginPass(pass);
	this->currentCommandState = SharedState;

	// set info
	this->SetFramebufferLayoutInfo(pass->GetVkFramebufferPipelineInfo());
	this->database->SetPass(pass.downcast<VkPass>());
	this->database->SetSubpass(0);

	const Util::FixedArray<VkClearValue>& clearValues = pass->GetVkClearValues();
	VkRenderPassBeginInfo info =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		NULL,
		pass->GetVkRenderPass(),
		pass->GetVkFramebuffer(),
		pass->GetVkRenderArea(),
		(uint32_t)clearValues.Size(),
		clearValues.Size() > 0 ? clearValues.Begin() : NULL
	};
	vkCmdBeginRenderPass(this->mainCmdDrawBuffer, &info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	// run this phase for scheduler
	this->scheduler->ExecuteCommandPass(VkScheduler::OnBeginPass);

	this->passInfo.framebuffer = pass->GetVkFramebuffer();
	this->passInfo.renderPass = pass->GetVkRenderPass();
	this->passInfo.subpass = 0;
	this->passInfo.pipelineStatistics = 0;
	this->passInfo.queryFlags = 0;
	this->passInfo.occlusionQueryEnable = VK_FALSE;

	const Util::FixedArray<VkRect2D>& scissors = pass->GetVkScissorRects(0);
	this->numScissors = scissors.Size();
	this->scissors = scissors.Begin();
	const Util::FixedArray<VkViewport>& viewports = pass->GetVkViewports(0);
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
	this->scheduler->ExecuteCommandPass(VkScheduler::OnNextSubpass);

	// progress to next subpass
	this->passInfo.subpass++;
	this->currentPipelineInfo.subpass++;
	this->currentPipelineBits &= ~PipelineBuilt; // hmm, this is really ugly, is it avoidable?
	this->database->SetSubpass(this->passInfo.subpass);
	vkCmdNextSubpass(this->mainCmdDrawBuffer, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	// get viewports and rectangles for this subpass
	const Util::FixedArray<VkRect2D>& scissors = pass->GetVkScissorRects(this->passInfo.subpass);
	this->numScissors = scissors.Size();
	this->scissors = scissors.Begin();
	const Util::FixedArray<VkViewport>& viewports = pass->GetVkViewports(this->passInfo.subpass);
	this->numViewports = viewports.Size();
	this->viewports = viewports.Begin();
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::BeginFeedback(const Ptr<CoreGraphics::FeedbackBuffer>& fb, CoreGraphics::PrimitiveTopology::Code primType)
{
	RenderDeviceBase::BeginFeedback(fb, primType);
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
VkRenderDevice::DrawFeedback(const Ptr<CoreGraphics::FeedbackBuffer>& fb)
{
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::DrawFeedbackInstanced(const Ptr<CoreGraphics::FeedbackBuffer>& fb, SizeT numInstances)
{
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
	this->scheduler->ExecuteCommandPass(VkScheduler::OnEndPass);

	// end render pass
	vkCmdEndRenderPass(this->mainCmdDrawBuffer);
	RenderDeviceBase::EndPass();
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::EndFeedback()
{
	RenderDeviceBase::EndFeedback();
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

	VkFenceCreateInfo info =
	{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		0,
		0
	};

	// submit transfer stuff
	this->SubmitToQueue(this->transferQueue, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1, &this->mainCmdTransBuffer);
	this->SubmitToQueue(this->transferQueue, this->mainCmdTransFence);
	
	this->scheduler->ExecuteCommandPass(VkScheduler::OnMainTransferSubmitted);
	this->scheduler->EndTransfers();

	// submit compute stuff
	this->SubmitToQueue(this->computeQueue, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1, &this->mainCmdCmpBuffer);
	this->SubmitToQueue(this->computeQueue, this->mainCmdCmpFence);	

	this->scheduler->ExecuteCommandPass(VkScheduler::OnMainComputeSubmitted);
	this->scheduler->EndComputes();
	
	// submit draw stuff
	this->SubmitToQueue(this->drawQueue, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1, &this->mainCmdDrawBuffer);
	this->SubmitToQueue(this->drawQueue, this->mainCmdDrawFence);

	this->scheduler->ExecuteCommandPass(VkScheduler::OnMainDrawSubmitted);
	this->scheduler->EndDraws();

	static VkFence fences[] = { this->mainCmdDrawFence, this->mainCmdTransFence, this->mainCmdCmpFence };
	this->WaitForFences(fences, 3, true);

	VkResult res = vkResetCommandBuffer(this->mainCmdTransBuffer, 0);
	n_assert(res == VK_SUCCESS);
	res = vkResetCommandBuffer(this->mainCmdCmpBuffer, 0);
	n_assert(res == VK_SUCCESS);
	res = vkResetCommandBuffer(this->mainCmdDrawBuffer, 0);
	n_assert(res == VK_SUCCESS);

	// run end-of-frame commands
	this->scheduler->ExecuteCommandPass(VkScheduler::OnEndFrame);

	// reset state
	this->inputInfo.topology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	this->vertexLayout = NULL;
	this->currentProgram = NULL;
	this->currentPipelineInfo.pVertexInputState = NULL;
	this->currentPipelineInfo.pInputAssemblyState = NULL;
	//vkEndCommandBuffer(this->mainCmdGfxBuffer);
	//vkEndCommandBuffer(this->mainCmdCmpBuffer);
	//vkEndCommandBuffer(this->mainCmdTransBuffer);
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
VkRenderDevice::DisplayResized(SizeT width, SizeT height)
{
	// TODO: implement me!
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::BindGraphicsPipelineInfo(const VkGraphicsPipelineCreateInfo& shader, const Ptr<VkShaderProgram>& program)
{
	if (this->currentProgram != program || !(this->currentPipelineBits & ShaderInfoSet))
	{
		this->database->SetShader(program);
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
		this->currentProgram = program;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::SetVertexLayoutPipelineInfo(VkPipelineVertexInputStateCreateInfo* vertexLayout)
{
	if (this->currentPipelineInfo.pVertexInputState != vertexLayout || !(this->currentPipelineBits & VertexLayoutInfoSet))
	{
		this->database->SetVertexLayout(vertexLayout);
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
		this->database->SetInputLayout(inputLayout);
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
			this->scheduler->PushCommand(cmd, VkScheduler::OnBindGraphicsPipeline);
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
		vkCmdBindDescriptorSets(this->mainCmdDrawBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, baseSet, setCount, descriptors, offsetCount, offsets);
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
		this->scheduler->PushCommand(cmd, VkScheduler::OnBindComputePipeline);
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
	vkCmdBindPipeline(this->mainCmdDrawBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

	// run command pass
	this->scheduler->ExecuteCommandPass(VkScheduler::OnBindComputePipeline);
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
	VkPipeline pipeline = this->database->GetCompiledPipeline();
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
	this->scheduler->ExecuteCommandPass(VkScheduler::OnBindGraphicsPipeline);
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
VkRenderDevice::InsertBarrier(const Ptr<CoreGraphics::Barrier>& barrier)
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
		vkCmdPipelineBarrier(this->mainCmdDrawBuffer, 
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
	this->scheduler->ExecuteCommandPass(VkScheduler::OnBeginDrawThread);
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
		this->scheduler->ExecuteCommandPass(VkScheduler::OnDrawThreadsSubmitted);

		// execute commands
		vkCmdExecuteCommands(this->mainCmdDrawBuffer, this->numActiveThreads, this->dispatchableDrawCmdBuffers);

		// destroy command buffers
		for (i = 0; i < this->numActiveThreads; i++)
		{
			VkDeferredCommand cmd;
			cmd.del.type = VkDeferredCommand::FreeCmdBuffers;
			cmd.del.cmdbufferfree.buffers[0] = this->dispatchableDrawCmdBuffers[i];
			cmd.del.cmdbufferfree.numBuffers = 1;
			cmd.del.cmdbufferfree.pool = this->dispatchableCmdDrawBufferPool[i];
			cmd.dev = this->dev;
			this->scheduler->PushCommand(cmd, VkScheduler::OnHandleDrawFences);
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
VkRenderDevice::Copy(const Ptr<CoreGraphics::Texture>& from, Math::rectangle<SizeT> fromRegion, const Ptr<CoreGraphics::Texture>& to, Math::rectangle<SizeT> toRegion)
{
	RenderDeviceBase::Copy(from, fromRegion, to, toRegion);
	n_assert(!this->inBeginPass);
	VkImageCopy region;
	region.dstOffset = { fromRegion.left, fromRegion.top, 0 };
	region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	region.extent = { (uint32_t)toRegion.width(), (uint32_t)toRegion.height(), 1 };
	region.srcOffset = { toRegion.left, toRegion.top, 0 };
	region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	vkCmdCopyImage(this->mainCmdDrawBuffer, from->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, to->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderDevice::Blit(const Ptr<CoreGraphics::RenderTexture>& from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const Ptr<CoreGraphics::RenderTexture>& to, Math::rectangle<SizeT> toRegion, IndexT toMip)
{
	this->Blit(from->GetTexture(), fromRegion, fromMip, to->GetTexture(), toRegion, toMip);
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
	vkCmdBlitImage(this->mainCmdDrawBuffer, from->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, to->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
}

} // namespace Vulkan
