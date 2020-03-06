#pragma once
//------------------------------------------------------------------------------
/**
	This thread records commands to a Vulkan Command Buffer in its own thread.
	
	(C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <vulkan/vulkan.h>
#include "coregraphics/drawthread.h"
#include "coregraphics/config.h"
#include "threading/thread.h"
#include "threading/safequeue.h"
#include "coregraphics/primitivegroup.h"
#include "debug/debugtimer.h"
#include "math/rectangle.h"
#include "coregraphics/indextype.h"

namespace Vulkan
{
class VkCommandBufferThread : public CoreGraphics::DrawThread
{
	__DeclareClass(VkCommandBufferThread);

public:

	struct VkCommandBufferBeginCommand
	{
		static const CommandType Type = BeginCommand;
		VkCommandBufferBeginInfo info;
		VkCommandBufferInheritanceInfo inheritInfo;
		VkCommandBuffer buf;
	};

	struct VkCommandBufferResetCommand
	{
		static const CommandType Type = ResetCommand;
	};

	struct VkCommandBufferEndCommand
	{
		static const CommandType Type = EndCommand;
	};

	struct VkGfxPipelineBindCommand
	{
		static const CommandType Type = GraphicsPipeline;
		VkPipeline pipeline;
		VkPipelineLayout layout;
#if NEBULA_GRAPHICS_DEBUG
		const char* name;
#endif
	};

	struct VkComputePipelineBindCommand
	{
		static const CommandType Type = ComputePipeline;
		VkPipeline pipeline;
		VkPipelineLayout layout;
#if NEBULA_GRAPHICS_DEBUG
		const char* name;
#endif
	};

	struct VkVertexBufferCommand
	{
		static const CommandType Type = InputAssemblyVertex;
		VkBuffer buffer;
		IndexT index;
		VkDeviceSize offset;
	};

	struct VkIndexBufferCommand
	{
		static const CommandType Type = InputAssemblyIndex;
		VkBuffer buffer;
		VkDeviceSize offset;
		VkIndexType indexType;
	};

	struct VkDrawCommand
	{
		static const CommandType Type = Draw;
		uint32_t baseIndex;
		uint32_t baseVertex;
		uint32_t numIndices;
		uint32_t numVerts;
		uint32_t baseInstance;
		uint32_t numInstances;
	};

	struct VkDispatchCommand
	{
		static const CommandType Type = Dispatch;
		uint32_t numGroupsX;
		uint32_t numGroupsY;
		uint32_t numGroupsZ;
	};

	struct VkDescriptorsCommand
	{
		static const CommandType Type = BindDescriptors;
		static const byte MaxSets = 8;
		static const byte MaxOffsets = 16;
		VkPipelineBindPoint type;
		uint32_t baseSet;
		uint32_t numSets;
		VkDescriptorSet sets[MaxSets];
		uint32_t numOffsets;
		uint32_t offsets[MaxOffsets];
	};

	struct VkPushConstantsCommand
	{
		static const CommandType Type = PushRange;
		static const short MaxSize = 512;
		VkShaderStageFlags stages;
		VkPipelineLayout layout;
		uint32_t offset;
		uint32_t size;
		char data[MaxSize];
	};

	struct VkViewportCommand
	{
		static const CommandType Type = Viewport;
		VkViewport vp;
		uint32_t index;
	};

	struct VkViewportArrayCommand
	{
		static const CommandType Type = ViewportArray;
		static const byte Max = 8;
		VkViewport vps[Max];
		uint32_t first;
		uint32_t num;
	};

	struct VkScissorRectCommand
	{
		static const CommandType Type = ScissorRect;
		VkRect2D sc;
		uint32_t index;
	};

	struct VkScissorRectArrayCommand
	{
		static const CommandType Type = ScissorRectArray;
		static const byte Max = 8;
		VkRect2D scs[Max];
		uint32_t first;
		uint32_t num;
	};

	struct VkUpdateBufferCommand
	{
		static const CommandType Type = UpdateBuffer;
		bool deleteWhenDone;
		VkBuffer buf;
		VkDeviceSize offset;
		VkDeviceSize size;
		uint32_t* data;
	};

	struct VkSetEventCommand
	{
		static const CommandType Type = SetEvent;
		VkEvent event;
		VkPipelineStageFlags stages;
	};

	struct VkResetEventCommand
	{
		static const CommandType Type = ResetEvent;
		VkEvent event;
		VkPipelineStageFlags stages;
	};

	struct VkWaitForEventCommand
	{
		static const CommandType Type = WaitForEvent;
		static const byte Max = 8;
		VkEvent event;
		uint32_t numEvents;
		VkPipelineStageFlags signalingStage;
		VkPipelineStageFlags waitingStage;
		uint32_t memoryBarrierCount;
		VkMemoryBarrier memoryBarriers[Max];
		uint32_t bufferBarrierCount;
		VkBufferMemoryBarrier bufferBarriers[Max];
		uint32_t imageBarrierCount;
		VkImageMemoryBarrier imageBarriers[Max];
	};

	struct VkBarrierCommand
	{
		static const CommandType Type = Barrier;
		static const byte Max = 8;
		VkPipelineStageFlags srcMask;
		VkPipelineStageFlags dstMask;
		VkDependencyFlags dep;
		uint32_t memoryBarrierCount;
		VkMemoryBarrier memoryBarriers[Max];
		uint32_t bufferBarrierCount;
		VkBufferMemoryBarrier bufferBarriers[Max];
		uint32_t imageBarrierCount;
		VkImageMemoryBarrier imageBarriers[Max];
	};

	struct VkBeginMarkerCommand
	{
		static const CommandType Type = BeginMarker;
		const char* text;
		float values[4];
	};

	struct VkEndMarkerCommand
	{
		static const CommandType Type = EndMarker;
	};

	struct VkInsertMarkerCommand
	{
		static const CommandType Type = InsertMarker;
		const char* text;
		float values[4];
	};

	/// constructor
	VkCommandBufferThread();
	/// destructor
	virtual ~VkCommandBufferThread();

	/// this method runs in the thread context
	void DoWork() override;
private:
	VkCommandBuffer vkCommandBuffer;
	VkPipelineLayout vkPipelineLayout;

#if NEBULA_ENABLE_PROFILING
	_declare_timer(debugTimer);
#endif
};

} // namespace Vulkan