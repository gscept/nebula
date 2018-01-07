#pragma once
//------------------------------------------------------------------------------
/**
	Vulkan implementation of the render device.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/renderdevicebase.h"
#include "vkcmdbufferthread.h"
#include "vkdeferredcommand.h"
#include "vkpipelinedatabase.h"
#include "vkscheduler.h"
#include "vksubcontexthandler.h"
#include "vkcmdbuffer.h"
#include "vkshaderprogram.h"
#include "coregraphics/glfw/glfwwindow.h"
#include <array>


namespace Lighting
{
	class VkLightServer;
}
namespace GLFW
{
	class GLFWWindow;
}
namespace Vulkan
{
class VkRenderDevice : public Base::RenderDeviceBase
{
	__DeclareClass(VkRenderDevice);
	__DeclareSingleton(VkRenderDevice);
public:

	/// constructor
	VkRenderDevice();
	/// destructor
	virtual ~VkRenderDevice();

	/// open the device
	bool Open();
	/// close the device
	void Close();

	/// begin complete frame
	bool BeginFrame(IndexT frameIndex);
	/// set the current vertex stream source
	void SetStreamVertexBuffer(IndexT streamIndex, const VkBuffer vb, IndexT offsetVertexIndex);
	/// set current vertex layout
	void SetVertexLayout(const VkPipelineVertexInputStateCreateInfo* vl);
	/// set current index buffer
	void SetIndexBuffer(const VkBuffer ib, IndexT offsetIndex, const CoreGraphics::IndexType::Code type);
	/// set the type of topology to be used
	void SetPrimitiveTopology(const CoreGraphics::PrimitiveTopology::Code& topo);
	/// perform computation
	void Compute(int dimX, int dimY, int dimZ);
	/// begin a rendering pass
	void BeginPass(const Ptr<CoreGraphics::Pass>& pass);
	/// progress to next subpass
	void SetToNextSubpass();
	/// begin batch
	void BeginBatch(CoreGraphics::FrameBatchType::Code batchType);
	/// bake the current state of the render device (only used on DX12 and Vulkan renderers where pipeline creation is required)
	void BuildRenderPipeline();
	/// insert execution barrier
	void InsertBarrier(const CoreGraphics::BarrierId barrier);
	/// draw current primitives
	void Draw();
	/// draw indexed, instanced primitives (see method header for details)
	void DrawIndexedInstanced(SizeT numInstances, IndexT baseInstance);
	/// end batch
	void EndBatch();
	/// end current pass
	void EndPass();
	/// end complete frame
	void EndFrame(IndexT frameIndex);
	/// present the rendered scene
	void Present();
	/// adds a scissor rect
	void SetScissorRect(const Math::rectangle<int>& rect, int index);
	/// sets viewport
	void SetViewport(const Math::rectangle<int>& rect, int index);

	/// copy data between textures
	void Copy(const CoreGraphics::TextureId from, Math::rectangle<SizeT> fromRegion, const CoreGraphics::TextureId to, Math::rectangle<SizeT> toRegion);
	/// blit between render textures
	void Blit(const CoreGraphics::RenderTextureId from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const CoreGraphics::RenderTextureId to, Math::rectangle<SizeT> toRegion, IndexT toMip);
	/// blit between textures
	void Blit(const CoreGraphics::TextureId from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const CoreGraphics::TextureId to, Math::rectangle<SizeT> toRegion, IndexT toMip);

	/// save a screenshot to the provided stream
	CoreGraphics::ImageFileFormat::Code SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream);
	/// save a region of the screen to the provided stream
	CoreGraphics::ImageFileFormat::Code SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream, const Math::rectangle<int>& rect, int x, int y);

	/// get the currently activated device
	VkDevice GetCurrentDevice() { return this->devices[this->currentDevice];  }
	/// get the currently activated physical device
	VkPhysicalDevice GetCurrentPhysicalDevice() { return this->physicalDevices[this->currentDevice]; }
	/// get the current device properties
	VkPhysicalDeviceProperties GetCurrentProperties() { return this->deviceProps[this->currentDevice]; }
	/// get the current device features
	VkPhysicalDeviceFeatures GetCurrentFeatures() { return this->deviceFeatures[this->currentDevice]; }
	/// get queue families
	const std::array<uint32_t, 4> GetQueueFamilies() { return std::array<uint32_t, 4>{ this->drawQueueFamily, this->computeQueueFamily, this->transferQueueFamily, this->sparseQueueFamily }; }
	/// get queue from index and family
	const VkQueue GetQueue(const VkSubContextHandler::SubContextType type, const IndexT index);

	/// call when window gets resized
	void DisplayResized(SizeT width, SizeT height);
	/// returns true if we support parallel transfers
	static bool AsyncTransferSupported();
	/// returns true if we support parallel computes
	static bool AsyncComputeSupported();

	static const short MaxNumRenderTargets = 8;
	static const short MaxNumViewports = 16;

private:
	friend class VkTexture;
	friend class VkShapeRenderer;
	friend class VkTextRenderer;
	friend class VkTransformDevice;
	friend class VkDisplayDevice;
	friend class VkVertexBuffer;
	friend class VkIndexBuffer;
	friend class VkCpuSyncFence;
	friend class VkShader;
	friend class VkShaderProgram;
	friend class VkMemoryVertexBufferPool;
	friend class VkMemoryIndexBufferPool;
	friend class VkMemoryTexturePool;
	friend class VkShaderServer;
	friend class VkRenderTarget;
	friend class VkRenderTargetCube;
	friend class VkMultipleRenderTarget;
	friend class VkDepthStencilTarget;
	friend class VkVertexLayout;
	friend class VkUniformBuffer;
	friend class VkShaderStorageBuffer;
	friend class VkStreamTexturePool;
	friend class VkStreamTextureSaver;
	friend class VkShaderState;
	friend class VkShaderImage;
	friend class VkCmdEvent;
	friend class VkPass;
	friend class VkRenderTexture;
	friend class VkPipelineDatabase;
	friend class Lighting::VkLightServer;
	friend class VkScheduler;
	friend class VkUtilities;
	friend class GLFW::GLFWWindow;
	friend struct VkDeferredCommand;

	friend const CoreGraphics::WindowId GLFW::InternalSetupFunction(const CoreGraphics::WindowCreateInfo& info, const Util::Blob& windowData, bool embed);

	friend VKAPI_ATTR void VKAPI_CALL NebulaVkAllocCallback(void* userData, uint32_t size, VkInternalAllocationType type, VkSystemAllocationScope scope);
	friend VKAPI_ATTR void VKAPI_CALL NebulaVkFreeCallback(void* userData, uint32_t size, VkInternalAllocationType type, VkSystemAllocationScope scope);

	enum PipelineInfoBits
	{
		ShaderInfoSet = 1,
		VertexLayoutInfoSet = 2,
		FramebufferLayoutInfoSet = 4,
		InputLayoutInfoSet = 8,

		AllInfoSet = 15,

		PipelineBuilt = 16
	};

	enum StateMode
	{
		MainState,			// commands go to the main command buffer
		SharedState,		// commands are put on all threads, and are immediately sent when a new thread is started
		LocalState			// commands are entirely local to a currently running thread
	};

	// open Vulkan device context
	bool OpenVulkanContext();
	/// close opengl4 device context
	void CloseVulkanDevice();
	/// setup the requested adapter for the Vulkan device
	void SetupAdapter();
	/// setup the remaining presentation parameters
	void SetupPresentParams();
	/// set the initial Vulkan device state
	void SetInitialDeviceState();
	/// sync with gpu
	void SyncGPU();

	/// sets the current shader pipeline information
	void BindGraphicsPipelineInfo(const VkGraphicsPipelineCreateInfo& shader, const uint32_t programId);
	/// sets the current vertex layout information
	void SetVertexLayoutPipelineInfo(VkPipelineVertexInputStateCreateInfo* vertexLayout);
	/// sets the current framebuffer layout information
	void SetFramebufferLayoutInfo(const VkGraphicsPipelineCreateInfo& framebufferLayout);
	/// sets the current primitive layout information
	void SetInputLayoutInfo(const VkPipelineInputAssemblyStateCreateInfo* inputLayout);
	/// create a new pipeline (or fetch from cache) and bind to command queue
	void CreateAndBindGraphicsPipeline();
	/// bind compute pipeline
	void BindComputePipeline(const VkPipeline& pipeline, const VkPipelineLayout& layout);
	/// bind no pipeline (effectively making all descriptor binds happen on both graphics and compute)
	void UnbindPipeline();

	/// update descriptors
	void BindDescriptorsGraphics(const VkDescriptorSet* descriptors, const VkPipelineLayout& layout, uint32_t baseSet, uint32_t setCount, const uint32_t* offsets, uint32_t offsetCount, bool shared = false);
	/// update descriptors
	void BindDescriptorsCompute(const VkDescriptorSet* descriptors, const VkPipelineLayout& layout, uint32_t baseSet, uint32_t setCount, const uint32_t* offsets, uint32_t offsetCount);
	/// update push ranges
	void UpdatePushRanges(const VkShaderStageFlags& stages, const VkPipelineLayout& layout, uint32_t offset, uint32_t size, void* data);

	/// setup queue from display
	void SetupQueue(uint32_t queueFamily, uint32_t queueIndex);

	/// helper function to submit a command buffer
	void SubmitToQueue(VkQueue queue, VkPipelineStageFlags flags, uint32_t numBuffers, VkCommandBuffer* buffers);
	/// helper function to submit a fence
	void SubmitToQueue(VkQueue queue, VkFence fence);
	/// wait for queue to finish execution using fence, also resets fence
	void WaitForFences(VkFence* fences, uint32_t numFences, bool waitForAll);

	/// wait for deferred delegates to complete
	void UpdateDelegates();
	/// begin using the worker threads to build command buffers
	void BeginGraphicsCmdThreads();
	/// end using the worker threads
	void EndGraphicsCmdThreads();
	/// begin using the worker threads to build command buffers
	void BeginComputeCmdThreads();
	/// end using the worker threads
	void EndComputeCmdThreads();
	/// begin using the worker threads to build command buffers
	void BeginTransferCmdThreads();
	/// end using the worker threads
	void EndTransferCmdThreads();
	/// begin command threads from array of threads and array of command buffers
	void BeginCmdThreads(const Ptr<Vulkan::VkCmdBufferThread>* threads, VkCommandPool* commandBufferPools, IndexT firstThread, SizeT numThreads, VkCommandBuffer* buffers);
	/// end command threads
	void EndCmdThreads(const Ptr<Vulkan::VkCmdBufferThread>* threads, IndexT firstThread, SizeT numThreads, Threading::Event* completionEvents);

	/// start up new draw thread
	void BeginDrawThread();
	/// start up all draw threads
	void BeginDrawThreadCluster();
	/// finish current draw threads
	void EndDrawThreadCluster();
	/// finish current draw threads
	void EndDrawThreads();
	/// continues to next thread
	void NextThread();
	/// add command to thread
	void PushToThread(const VkCmdBufferThread::Command& cmd, const IndexT& index, bool allowStaging = true);
	/// flush remaining staging thread commands
	void FlushToThread(const IndexT& index);

	/// binds common descriptors
	void BindSharedDescriptorSets();

	uint32_t adapter;
	uint32_t frameId;
	VkPhysicalDeviceMemoryProperties memoryProps;

	const uint32_t VkPoolMaxSets = 65536;
	const uint32_t VkPoolSetSize = 65536;

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

	// setup management classes
	VkScheduler scheduler;
	VkSubContextHandler subcontextHandler;
	VkPipelineDatabase database;

	// device handling (multi GPU?!?!)
	Util::FixedArray<VkDevice> devices;
	Util::FixedArray<VkPhysicalDevice> physicalDevices;
	Util::FixedArray<VkPhysicalDeviceProperties> deviceProps;
	Util::FixedArray<VkPhysicalDeviceFeatures> deviceFeatures;
	Util::FixedArray<uint32_t> numCaps;
	Util::FixedArray<Util::FixedArray<VkExtensionProperties>> caps;
	IndexT currentDevice;
	
	static VkInstance instance;

	static VkDescriptorPool descPool;	

	static CoreGraphics::CmdBufferId mainCmdDrawBuffer;
	static CoreGraphics::CmdBufferId mainCmdComputeBuffer;
	static CoreGraphics::CmdBufferId mainCmdTransferBuffer;
	static CoreGraphics::CmdBufferId mainCmdSparseBuffer;

	static VkPipelineCache cache;
	VkFence mainCmdDrawFence;
	VkFence mainCmdCmpFence;
	VkFence mainCmdTransFence;
	VkEvent mainCmdDrawEvent;
	VkEvent mainCmdCmpEvent;
	VkEvent mainCmdTransEvent;
	
	VkShaderProgram::PipelineType currentBindPoint;

	StateMode currentCommandState;


	static const SizeT NumDrawThreads = 8;
	IndexT currentDrawThread;
	VkCommandPool dispatchableCmdDrawBufferPool[NumDrawThreads];
	VkCommandBuffer dispatchableDrawCmdBuffers[NumDrawThreads];
	Ptr<VkCmdBufferThread> drawThreads[NumDrawThreads];
	Threading::Event* drawCompletionEvents[NumDrawThreads];

	static const SizeT NumTransferThreads = 1;
	IndexT currentTransThread;
	VkCommandPool dispatchableCmdTransBufferPool[NumTransferThreads];
	VkCommandBuffer dispatchableTransCmdBuffers[NumTransferThreads];
	Ptr<VkCmdBufferThread> transThreads[NumTransferThreads];
	Threading::Event* transCompletionEvents[NumTransferThreads];

	static const SizeT NumComputeThreads = 1;
	IndexT currentComputeThread;
	VkCommandPool dispatchableCmdCompBufferPool[NumComputeThreads];
	VkCommandBuffer dispatchableCompCmdBuffers[NumComputeThreads];
	Ptr<VkCmdBufferThread> compThreads[NumComputeThreads];
	Threading::Event* compCompletionEvents[NumComputeThreads];

	Util::Array<VkCmdBufferThread::Command> sharedDescriptorSets;
	Util::Array<VkCmdBufferThread::Command> threadCmds[NumDrawThreads];
	SizeT numCallsLastFrame;
	SizeT numActiveThreads;
	SizeT numUsedThreads;

	VkCommandBufferInheritanceInfo passInfo;
	VkPipelineInputAssemblyStateCreateInfo inputInfo;
	VkPipelineColorBlendStateCreateInfo blendInfo;
	VkViewport* passViewports;
	uint32_t numVsInputs;

	static VkAllocationCallbacks alloc;

	VkGraphicsPipelineCreateInfo currentPipelineInfo;
	VkPipelineLayout currentPipelineLayout;
	VkPipeline currentPipeline;
	uint currentPipelineBits;
	uint32_t numViewports;
	VkViewport* viewports;
	uint32_t numScissors;
	VkRect2D* scissors;
	bool viewportsDirty[NumDrawThreads];
	bool scissorsDirty[NumDrawThreads];
	
	uint32_t currentProgram;

	_declare_counter(NumImageBytesAllocated);
	_declare_counter(NumBufferBytesAllocated);
	_declare_counter(NumBytesAllocated);
	_declare_counter(NumPipelinesBuilt);
	_declare_timer(DebugTimer);

#if NEBULAT_VULKAN_DEBUG
	VkDebugReportCallbackEXT debugCallbackHandle;
	PFN_vkCreateDebugReportCallbackEXT debugCallbackPtr;
#endif
};

} // namespace Vulkan