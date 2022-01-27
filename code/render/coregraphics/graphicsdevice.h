#pragma once
//------------------------------------------------------------------------------
/**
    @struct CoreGraphics::GraphicsDeviceCreateInfo

    The Graphics Device is the engine which drives the graphics abstraction layer

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "pass.h"
#include "profiling/profiling.h"
#include "frame/framescript.h"
#include "primitivegroup.h"
#include "buffer.h"
#include "imagefileformat.h"
#include "io/stream.h"
#include "fence.h"
#include "debug/debugcounter.h"
#include "coregraphics/rendereventhandler.h"
#include "coregraphics/submissioncontext.h"
#include "coregraphics/resourcetable.h"
#include "timing/timer.h"
#include "util/stack.h"
#include "memory.h"

namespace CoreGraphics
{

enum class PipelineBuildBits : uint
{
    NoInfoSet = 0,
    ShaderInfoSet = N_BIT(0),
    VertexLayoutInfoSet = N_BIT(1),
    FramebufferLayoutInfoSet = N_BIT(2),
    InputLayoutInfoSet = N_BIT(3),

    AllInfoSet = ShaderInfoSet | VertexLayoutInfoSet | FramebufferLayoutInfoSet | InputLayoutInfoSet,

    PipelineBuilt = N_BIT(4)
};

__ImplementEnumBitOperators(PipelineBuildBits);
__ImplementEnumComparisonOperators(PipelineBuildBits);

/// struct for texture copies
struct TextureCopy
{
    Math::rectangle<SizeT> region;
    uint mip;
    uint layer;
};

struct BufferCopy
{
    uint offset;
    uint rowLength = 0;     // for buffer to image copies
    uint imageHeight = 0;   // for buffer to image copies
};

struct GraphicsDeviceCreateInfo
{
    uint64 globalGraphicsConstantBufferMemorySize[NumConstantBufferTypes];
    uint64 globalComputeConstantBufferMemorySize[NumConstantBufferTypes];
    uint64 memoryHeaps[NumMemoryPoolTypes];
    byte numBufferedFrames : 3;
    bool enableValidation : 1;      // enables validation layer and writes output to console
};

/// create graphics device
bool CreateGraphicsDevice(const GraphicsDeviceCreateInfo& info);
/// destroy graphics device
void DestroyGraphicsDevice();

/// get number of buffered frames
SizeT GetNumBufferedFrames();
/// get current frame index, which is between 0 and GetNumBufferedFrames
IndexT GetBufferedFrameIndex();

#ifdef NEBULA_ENABLE_PROFILING
struct FrameProfilingMarker
{
    CoreGraphics::QueueType queue;
    Math::vec4 color;
    Util::String name;
    IndexT gpuBegin;
    IndexT gpuEnd;
    uint64_t start;
    uint64_t duration;
    Util::Array<FrameProfilingMarker> children;
};
#endif

struct Query
{
    CoreGraphics::QueryType type;
    IndexT idx;
    Timing::Time cpuTime;
};

struct DrawThreadResult
{
    CoreGraphics::CommandBufferId buf;
    Threading::Event* event;
};

struct GraphicsDeviceState
{
    Util::Array<CoreGraphics::TextureId> backBuffers;

    CoreGraphics::CommandBufferPoolId submissionGraphicsCmdPool;
    CoreGraphics::CommandBufferPoolId submissionComputeCmdPool;
    CoreGraphics::CommandBufferPoolId submissionTransferCmdPool;
    CoreGraphics::CommandBufferPoolId submissionTransferGraphicsHandoverCmdPool;

    CoreGraphics::SubmissionContextId resourceSubmissionContext;
    CoreGraphics::CommandBufferId resourceSubmissionCmdBuffer;
    Threading::CriticalSection resourceSubmissionCriticalSection;
    bool resourceSubmissionActive;

    CoreGraphics::SubmissionContextId handoverSubmissionContext;
    CoreGraphics::CommandBufferId handoverSubmissionCmdBuffer;
    bool handoverSubmissionActive;

    CoreGraphics::SubmissionContextId setupSubmissionContext;
    CoreGraphics::CommandBufferId setupSubmissionCmdBuffer;
    bool setupSubmissionActive;

    bool sparseSubmitActive;
    bool sparseWaitHandled;

    CoreGraphics::SubmissionContextId queryGraphicsSubmissionContext;
    CoreGraphics::CommandBufferId queryGraphicsSubmissionCmdBuffer;

    CoreGraphics::SubmissionContextId queryComputeSubmissionContext;
    CoreGraphics::CommandBufferId queryComputeSubmissionCmdBuffer;

    CoreGraphics::SubmissionContextId gfxSubmission;
    CoreGraphics::CommandBufferId gfxCmdBuffer;

    CoreGraphics::SubmissionContextId computeSubmission;
    CoreGraphics::CommandBufferId computeCmdBuffer;

    Util::FixedArray<CoreGraphics::FenceId> presentFences;
    Util::FixedArray<CoreGraphics::SemaphoreId> renderingFinishedSemaphores;

    int globalGraphicsConstantBufferMaxValue[NumConstantBufferTypes];
    CoreGraphics::BufferId globalGraphicsConstantStagingBuffer[NumConstantBufferTypes];
    CoreGraphics::BufferId globalGraphicsConstantBuffer[NumConstantBufferTypes];

    int globalComputeConstantBufferMaxValue[NumConstantBufferTypes];
    CoreGraphics::BufferId globalComputeConstantStagingBuffer[NumConstantBufferTypes];
    CoreGraphics::BufferId globalComputeConstantBuffer[NumConstantBufferTypes];

    CoreGraphics::DrawThread* drawThread;
    Util::Stack<CoreGraphics::DrawThread*> drawThreads;

    CoreGraphics::ResourceTableId tickResourceTable;
    CoreGraphics::ResourceTableId frameResourceTable;

    Util::Array<Ptr<CoreGraphics::RenderEventHandler> > eventHandlers;

    PipelineBuildBits currentPipelineBits;
    CoreGraphics::PrimitiveTopology::Code primitiveTopology;
    CoreGraphics::PrimitiveGroup primitiveGroup;
    CoreGraphics::PassId pass;
    bool isOpen : 1;
    bool inNotifyEventHandlers : 1;
    bool inBeginFrame : 1;
    bool inBeginPass : 1;
    bool inBeginBatch : 1;
    bool inBeginCompute : 1;
    bool inBeginAsyncCompute : 1;
    bool inBeginGraphicsSubmission : 1;
    bool inBeginComputeSubmission : 1;
    bool renderWireframe : 1;
    bool visualizeMipMaps : 1;
    bool usePatches : 1;
    bool enableValidation : 1;
    IndexT currentFrameIndex;

    _declare_counter(NumImageBytesAllocated);
    _declare_counter(NumBufferBytesAllocated);
    _declare_counter(NumBytesAllocated);
    _declare_counter(GraphicsDeviceNumComputes);
    _declare_counter(GraphicsDeviceNumPrimitives);
    _declare_counter(GraphicsDeviceNumDrawCalls);

#ifdef NEBULA_ENABLE_PROFILING
    Util::Stack<FrameProfilingMarker> profilingMarkerStack[NumQueueTypes];
    Util::FixedArray<Util::Array<FrameProfilingMarker>> profilingMarkersPerFrame;
    Util::Array<FrameProfilingMarker> frameProfilingMarkers;
#endif NEBULA_ENABLE_PROFILING
};

struct GraphicsDeviceThreadState
{
    CoreGraphics::CommandBufferId graphicsCommandBuffer, computeCommandBuffer;

    PipelineBuildBits currentPipelineBits;
    CoreGraphics::PrimitiveTopology::Code primitiveTopology;
    CoreGraphics::PrimitiveGroup primitiveGroup;
    CoreGraphics::PassId pass;
    bool isOpen : 1;
    bool inNotifyEventHandlers : 1;
    bool inBeginFrame : 1;
    bool inBeginPass : 1;
    bool inBeginBatch : 1;
    bool inBeginCompute : 1;
    bool inBeginAsyncCompute : 1;
    bool inBeginGraphicsSubmission : 1;
    bool inBeginComputeSubmission : 1;
    bool renderWireframe : 1;
    bool visualizeMipMaps : 1;
    bool usePatches : 1;
    bool enableValidation : 1;
};

/// retrieve the current graphics device state
GraphicsDeviceState const* const GetGraphicsDeviceState();

/// attach a render event handler
void AttachEventHandler(const Ptr<CoreGraphics::RenderEventHandler>& h);
/// remove a render event handler
void RemoveEventHandler(const Ptr<CoreGraphics::RenderEventHandler>& h);
/// notify event handlers about an event
bool NotifyEventHandlers(const CoreGraphics::RenderEvent& e);

/// add a render texture used by the back buffer
void AddBackBufferTexture(const CoreGraphics::TextureId tex);
/// remove a render texture
void RemoveBackBufferTexture(const CoreGraphics::TextureId tex);

/// set draw thread to use for subsequent commands
void SetDrawThread(CoreGraphics::DrawThread* thread);

/// begin complete frame
bool BeginFrame(IndexT frameIndex);
/// start a new submission, with an optional argument for waiting for another queue
void BeginSubmission(CoreGraphics::QueueType queue, CoreGraphics::QueueType waitQueue);
/// begin a rendering pass
void BeginPass(const CoreGraphics::PassId pass, PassRecordMode mode);

/// start a new draw thread
void BeginSubpassCommands(const CoreGraphics::CommandBufferId buf);
/// progress to next subpass    
void SetToNextSubpass(PassRecordMode mode);
/// begin rendering a batch
void BeginBatch(Frame::FrameBatchType::Code batchType);

/// reset clip settings to pass
void ResetClipSettings();

/// set a stream vertex buffer
void SetStreamVertexBuffer(IndexT streamIndex, const CoreGraphics::BufferId& buffer, IndexT offsetVertexIndex);
/// set current vertex layout
void SetVertexLayout(const CoreGraphics::VertexLayoutId& vl);
/// set index buffer
void SetIndexBuffer(const CoreGraphics::BufferId& buffer, IndexT offsetIndex);
/// set the type of topology used
void SetPrimitiveTopology(const CoreGraphics::PrimitiveTopology::Code topo);
/// get the type of topology used
const CoreGraphics::PrimitiveTopology::Code& GetPrimitiveTopology();
/// set current primitive group
void SetPrimitiveGroup(const CoreGraphics::PrimitiveGroup& pg);
/// get current primitive group
const CoreGraphics::PrimitiveGroup& GetPrimitiveGroup();
/// set shader program
void SetShaderProgram(const CoreGraphics::ShaderProgramId pro, const CoreGraphics::QueueType queue = GraphicsQueueType, const bool bindSharedResources = true);
/// set resource table
void SetResourceTable(const CoreGraphics::ResourceTableId table, const IndexT slot, ShaderPipeline pipeline, const Util::FixedArray<uint>& offsets, const CoreGraphics::QueueType queue = GraphicsQueueType);
/// set resource table using raw offsets
void SetResourceTable(const CoreGraphics::ResourceTableId table, const IndexT slot, ShaderPipeline pipeline, uint32 numOffsets, uint32* offsets, const CoreGraphics::QueueType queue = GraphicsQueueType);
/// set resoure table layout
void SetResourceTablePipeline(const CoreGraphics::ResourcePipelineId layout);
/// set the resource table to be used for the NEBULA_TICK_GROUP, returns the currently used resource table
CoreGraphics::ResourceTableId SetTickResourceTable(const CoreGraphics::ResourceTableId table);
/// set the resource table to be used for the NEBULA_FRAME_GROUP, returns the currently used resource table
CoreGraphics::ResourceTableId SetFrameResourceTable(const CoreGraphics::ResourceTableId table);

/// push constants
void PushConstants(ShaderPipeline pipeline, uint offset, uint size, byte* data);

/// Unlock constants
void UnlockConstantUpdates();
/// allocate range of graphics memory and set data, return offset
template<class TYPE> uint SetGraphicsConstants(CoreGraphics::GlobalConstantBufferType type, const TYPE& data);
/// allocate range of graphics memory and set data as an array of elements, return offset
template<class TYPE> uint SetGraphicsConstants(CoreGraphics::GlobalConstantBufferType type, const TYPE* data, SizeT elements);
/// set graphics constants based on pre-allocated memory
template<class TYPE> void SetGraphicsConstants(CoreGraphics::GlobalConstantBufferType type, uint offset, const TYPE& data);
/// allocate range of compute memory and set data, return offset
template<class TYPE> uint SetComputeConstants(CoreGraphics::GlobalConstantBufferType type, const TYPE& data);
/// allocate range of graphics memory and set data as an array of elements, return offset
template<class TYPE> uint SetComputeConstants(CoreGraphics::GlobalConstantBufferType type, const TYPE* data, SizeT elements);
/// set graphics constants based on pre-allocated memory
template<class TYPE> void SetComputeConstants(CoreGraphics::GlobalConstantBufferType type, uint offset, const TYPE& data);
/// Lock constant updates
void LockConstantUpdates();

/// allocate range of graphics memory and set data, return offset
int SetGraphicsConstantsInternal(CoreGraphics::GlobalConstantBufferType type, const void* data, SizeT size);
/// use pre-allocated range of memory to update graphics constants
void SetGraphicsConstantsInternal(CoreGraphics::GlobalConstantBufferType type, uint offset, const void* data, SizeT size);
/// allocate range of compute memory and set data, return offset
int SetComputeConstantsInternal(CoreGraphics::GlobalConstantBufferType type, const void* data, SizeT size);
/// use pre-allocated range of memory to update compute constants
void SetComputeConstantsInternal(CoreGraphics::GlobalConstantBufferType type, uint offset, const void* data, SizeT size);

/// reserve range of graphics constant buffer memory and return offset
uint AllocateGraphicsConstantBufferMemory(CoreGraphics::GlobalConstantBufferType type, uint size);
/// return id to global graphics constant buffer
CoreGraphics::BufferId GetGraphicsConstantBuffer(CoreGraphics::GlobalConstantBufferType type);
/// reserve range of compute constant buffer memory and return offset
uint AllocateComputeConstantBufferMemory(CoreGraphics::GlobalConstantBufferType type, uint size);
/// return id to global compute constant buffer
CoreGraphics::BufferId GetComputeConstantBuffer(CoreGraphics::GlobalConstantBufferType type);

/// lock resource updates, blocks if other thread is using it
void LockResourceSubmission();
/// return resource submission context
CoreGraphics::SubmissionContextId GetResourceSubmissionContext();
/// return the handover submission context
CoreGraphics::SubmissionContextId GetHandoverSubmissionContext();
/// unlocks resource updates
void UnlockResourceSubmission();
/// return setup submission context
CoreGraphics::SubmissionContextId GetSetupSubmissionContext();
/// return command buffer for current frame graphics submission
CoreGraphics::CommandBufferId GetGfxCommandBuffer();
/// return command buffer for current frame async compute submission
CoreGraphics::CommandBufferId GetComputeCommandBuffer();

/// set pipeline using current layout, shader and pass
void SetGraphicsPipeline();

/// trigger reloading a shader
void ReloadShaderProgram(const CoreGraphics::ShaderProgramId& pro);

/// insert execution barrier
void InsertBarrier(const CoreGraphics::BarrierId barrier, const CoreGraphics::QueueType queue);
/// signals an event
void SignalEvent(const CoreGraphics::EventId ev, const CoreGraphics::BarrierStage stage, const CoreGraphics::QueueType queue);
/// signals an event
void WaitEvent(
    const EventId id,
    const CoreGraphics::BarrierStage waitStage,
    const CoreGraphics::BarrierStage signalStage,
    const CoreGraphics::QueueType queue
    );
/// signals an event
void ResetEvent(const CoreGraphics::EventId ev, const CoreGraphics::BarrierStage stage, const CoreGraphics::QueueType queue);

/// draw primitives
void Draw();
/// draw primitives instanced
void Draw(SizeT numInstances, IndexT baseInstance);
/// draw indirect, draws is the amount of draws in the buffer, and stride is the byte offset between each call
void DrawIndirect(const CoreGraphics::BufferId buffer, IndexT bufferOffset, SizeT draws, IndexT stride);
/// draw indirect, draws is the amount of draws in the buffer, and stride is the byte offset between each call
void DrawIndirectIndexed(const CoreGraphics::BufferId buffer, IndexT bufferOffset, SizeT draws, IndexT stride);
/// perform computation
void Compute(int dimX, int dimY, int dimZ, const CoreGraphics::QueueType queue = GraphicsQueueType);

/// start a new draw thread
void EndSubpassCommands();
/// execute thread buffer
void ExecuteCommands(const CoreGraphics::CommandBufferId cmds);
/// end current batch
void EndBatch();
/// end current pass
void EndPass(PassRecordMode mode);
/// end the current submission, 
void EndSubmission(CoreGraphics::QueueType queue, CoreGraphics::QueueType waitQueue);
/// end current frame
void EndFrame(IndexT frameIndex);
/// check if inside BeginFrame
bool IsInBeginFrame();
/// wait for an individual queue to finish
void WaitForQueue(CoreGraphics::QueueType queue);
/// wait for all queues to finish
void WaitAndClearPendingCommands();

/// Swap
void Swap(IndexT i);
/// Progress to next frame
void NewFrame();

/// save a screenshot to the provided stream
CoreGraphics::ImageFileFormat::Code SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream);
/// save a region of the screen to the provided stream
CoreGraphics::ImageFileFormat::Code SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream, const Math::rectangle<int>& rect, int x, int y);
/// get visualization of mipmaps flag   
bool GetVisualizeMipMaps();
/// set visualization of mipmaps flag   
void SetVisualizeMipMaps(bool val);
/// get the render as wireframe flag
bool GetRenderWireframe();
/// set the render as wireframe flag
void SetRenderWireframe(bool b);

#if NEBULA_ENABLE_PROFILING
/// insert timestamp, returns handle to timestamp, which can be retreived on the next N'th frame where N is the number of buffered frames
IndexT Timestamp(CoreGraphics::QueueType queue, const CoreGraphics::BarrierStage stage, const char* name);
/// get cpu profiling markers
const Util::Array<FrameProfilingMarker>& GetProfilingMarkers();
/// get number of draw calls this frame
SizeT GetNumDrawCalls();
#endif

/// start query
IndexT BeginQuery(CoreGraphics::QueueType queue, CoreGraphics::QueryType type);
/// end query
void EndQuery(CoreGraphics::QueueType queue, CoreGraphics::QueryType type);
/// copy data between textures
void Copy(
    const CoreGraphics::QueueType queue,
    const CoreGraphics::TextureId fromTexture,
    const Util::Array<CoreGraphics::TextureCopy>& from,
    const CoreGraphics::TextureId toTexture,
    const Util::Array<CoreGraphics::TextureCopy>& to,
    const CoreGraphics::SubmissionContextId sub = CoreGraphics::InvalidSubmissionContextId
);
/// copy data between buffers
void Copy(
    const CoreGraphics::QueueType queue, 
    const CoreGraphics::BufferId fromBuffer,
    const Util::Array<CoreGraphics::BufferCopy>& from,
    const CoreGraphics::BufferId toBuffer,
    const Util::Array<CoreGraphics::BufferCopy>& to,
    const SizeT size,
    const CoreGraphics::SubmissionContextId sub = CoreGraphics::InvalidSubmissionContextId
);
/// copy data from buffer to texture
void Copy(
    const CoreGraphics::QueueType queue, 
    const CoreGraphics::BufferId fromBuffer,
    const Util::Array<CoreGraphics::BufferCopy>& from,
    const CoreGraphics::TextureId toTexture,
    const Util::Array<CoreGraphics::TextureCopy>& to,
    const CoreGraphics::SubmissionContextId sub = CoreGraphics::InvalidSubmissionContextId
);
/// copy data from texture to buffer
void Copy(
    const CoreGraphics::QueueType queue,
    const CoreGraphics::TextureId fromTexture,
    const Util::Array<CoreGraphics::TextureCopy>& from,
    const CoreGraphics::BufferId toBuffer,
    const Util::Array<CoreGraphics::BufferCopy>& to,
    const CoreGraphics::SubmissionContextId sub = CoreGraphics::InvalidSubmissionContextId
);

/// blit between textures
void Blit(const CoreGraphics::TextureId from, const Math::rectangle<SizeT>& fromRegion, IndexT fromMip, IndexT fromLayer, const CoreGraphics::TextureId to, const Math::rectangle<SizeT>& toRegion, IndexT toMip, IndexT toLayer);

/// sets whether or not the render device should tessellate
void SetUsePatches(bool state);
/// gets whether or not the render device should tessellate
bool GetUsePatches();

/// sets a viewport for a certain index
void SetViewport(const Math::rectangle<int>& rect, int index);
/// sets a scissor rect for a certain index
void SetScissorRect(const Math::rectangle<int>& rect, int index);
/// set the stencil reference values
void SetStencilRef(const uint frontRef, const uint backRef);
/// set the stencil read mask (compare mask(
void SetStencilReadMask(const uint readMask);
/// set the stencil write mask
void SetStencilWriteMask(const uint writeMask);

/// update buffer in command buffer
void UpdateBuffer(const CoreGraphics::BufferId buffer, uint offset, uint size, const void* data, CoreGraphics::QueueType queue);

#if NEBULA_GRAPHICS_DEBUG
/// set debug name for object
template<typename OBJECT_ID_TYPE> void ObjectSetName(const OBJECT_ID_TYPE id, const char* name);
/// begin debug marker region
void QueueBeginMarker(const CoreGraphics::QueueType queue, const Math::vec4& color, const char* name);
/// end debug marker region
void QueueEndMarker(const CoreGraphics::QueueType queue);
/// insert marker
void QueueInsertMarker(const CoreGraphics::QueueType queue, const Math::vec4& color, const char* name);
/// begin debug marker region
void CommandBufferBeginMarker(const CoreGraphics::QueueType queue, const Math::vec4& color, const char* name);
/// end debug marker region
void CommandBufferEndMarker(const CoreGraphics::QueueType queue);
/// insert marker
void CommandBufferInsertMarker(const CoreGraphics::QueueType queue, const Math::vec4& color, const char* name);
#endif

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline uint
SetGraphicsConstants(CoreGraphics::GlobalConstantBufferType type, const TYPE& data)
{
    return SetGraphicsConstantsInternal(type, &data, sizeof(TYPE));
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline uint
SetGraphicsConstants(CoreGraphics::GlobalConstantBufferType type, const TYPE* data, SizeT elements)
{
    return SetGraphicsConstantsInternal(type, data, sizeof(TYPE) * elements);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void
SetGraphicsConstants(CoreGraphics::GlobalConstantBufferType type, uint offset, const TYPE& data)
{
    return SetGraphicsConstantsInternal(type, offset, &data, sizeof(TYPE));
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline uint
SetComputeConstants(CoreGraphics::GlobalConstantBufferType type, const TYPE& data)
{
    return SetComputeConstantsInternal(type, &data, sizeof(TYPE));
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> 
inline uint 
SetComputeConstants(CoreGraphics::GlobalConstantBufferType type, const TYPE* data, SizeT elements)
{
    return SetComputeConstantsInternal(type, data, sizeof(TYPE) * elements);
}


//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void 
SetComputeConstants(CoreGraphics::GlobalConstantBufferType type, uint offset, const TYPE& data)
{
    return SetComputeConstantsInternal(type, offset, &data, sizeof(TYPE));
}

} // namespace CoreGraphics
