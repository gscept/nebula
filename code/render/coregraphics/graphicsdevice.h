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
#include "coregraphics/resourcetable.h"
#include "memory/rangeallocator.h"
#include "timing/timer.h"
#include "util/stack.h"
#include "memory.h"

namespace CoreGraphics
{

struct GraphicsDeviceCreateInfo
{
    uint64 globalGraphicsConstantBufferMemorySize;
    uint64 globalComputeConstantBufferMemorySize;
    uint64 globalVertexBufferMemorySize;
    uint64 memoryHeaps[NumMemoryPoolTypes];
    uint64 maxOcclusionQueries, maxTimestampQueries, maxStatisticsQueries;
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

struct DrawThreadResult
{
    CoreGraphics::CmdBufferId buf;
    Threading::Event* event;
};

struct SubmissionWaitEvent
{
    // Constructor
    SubmissionWaitEvent();
    // Comparison operator with nullptr
    const bool operator==(const std::nullptr_t) const;
    // Comparison operator with nullptr
    const bool operator!=(const std::nullptr_t) const;
    // Invalidate
    void operator=(const std::nullptr_t);

#if __VULKAN__
    uint64_t timelineIndex;
#endif
};

struct GraphicsDeviceState
{
    Util::Array<CoreGraphics::TextureId> backBuffers;

    CoreGraphics::CmdBufferPoolId setupTransferCommandBufferPool;
    CoreGraphics::CmdBufferId setupTransferCommandBuffer;
    CoreGraphics::CmdBufferPoolId setupGraphicsCommandBufferPool;
    CoreGraphics::CmdBufferId setupGraphicsCommandBuffer;

    Util::FixedArray<CoreGraphics::FenceId> presentFences;
    Util::FixedArray<CoreGraphics::SemaphoreId> renderingFinishedSemaphores;

    int globalGraphicsConstantBufferMaxValue;
    CoreGraphics::BufferId globalGraphicsConstantStagingBuffer;
    CoreGraphics::BufferId globalGraphicsConstantBuffer;

    int globalComputeConstantBufferMaxValue;
    CoreGraphics::BufferId globalComputeConstantStagingBuffer;
    CoreGraphics::BufferId globalComputeConstantBuffer;

    CoreGraphics::ResourceTableId tickResourceTable;
    CoreGraphics::ResourceTableId frameResourceTable;

    Util::Array<Ptr<CoreGraphics::RenderEventHandler> > eventHandlers;

    Memory::RangeAllocator vertexAllocator;
    CoreGraphics::BufferId vertexBuffer;

    bool isOpen : 1;
    bool inNotifyEventHandlers : 1;
    bool renderWireframe : 1;
    bool visualizeMipMaps : 1;
    bool enableValidation : 1;
    IndexT currentFrameIndex;

    _declare_counter(NumImageBytesAllocated);
    _declare_counter(NumBufferBytesAllocated);
    _declare_counter(NumBytesAllocated);
    _declare_counter(GraphicsDeviceNumComputes);
    _declare_counter(GraphicsDeviceNumPrimitives);
    _declare_counter(GraphicsDeviceNumDrawCalls);

#ifdef NEBULA_ENABLE_PROFILING
    Util::Array<FrameProfilingMarker> frameProfilingMarkers;
#endif NEBULA_ENABLE_PROFILING
};

struct GraphicsDeviceThreadState
{
    CoreGraphics::CmdBufferId graphicsCommandBuffer, computeCommandBuffer;

    CmdPipelineBuildBits currentPipelineBits;
    CoreGraphics::PrimitiveTopology::Code primitiveTopology;
    CoreGraphics::PrimitiveGroup primitiveGroup;
    CoreGraphics::PassId pass;
    bool isOpen : 1;
    bool inNotifyEventHandlers : 1;
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

/// Lock resource command buffer
const CoreGraphics::CmdBufferId LockTransferSetupCommandBuffer();
/// Release lock on resource command buffer
void UnlockTransferSetupCommandBuffer();
/// Lock handover command buffer
const CoreGraphics::CmdBufferId LockGraphicsSetupCommandBuffer();
/// Release lock on handover command buffer
void UnlockGraphicsSetupCommandBuffer();

/// Submit a command buffer, but doesn't necessarily execute it immediately
SubmissionWaitEvent SubmitCommandBuffer(const CoreGraphics::CmdBufferId cmds, CoreGraphics::QueueType type);
/// Wait for a submission
void WaitForSubmission(SubmissionWaitEvent index, CoreGraphics::QueueType type, CoreGraphics::QueueType waitType);

/// Set the resource table to be used for the NEBULA_TICK_GROUP, returns the currently used resource table
void SetTickResourceTable(const CoreGraphics::ResourceTableId table);
/// Get tick resoure table
CoreGraphics::ResourceTableId GetTickResourceTable();
/// Set the resource table to be used for the NEBULA_FRAME_GROUP, returns the currently used resource table
void SetFrameResourceTable(const CoreGraphics::ResourceTableId table);
/// Get frame resoure table
CoreGraphics::ResourceTableId GetFrameResourceTable();

/// Unlock constants
void UnlockConstantUpdates();
/// Allocate range of graphics memory and set data, return offset (thread safe)
template<class TYPE> uint SetGraphicsConstants(const TYPE& data);
/// Allocate range of graphics memory and set data as an array of elements, return offset  (thread safe)
template<class TYPE> uint SetGraphicsConstants(const TYPE* data, SizeT elements);
/// Set graphics constants based on pre-allocated memory  (thread safe)
template<class TYPE> void SetGraphicsConstants(ConstantBufferOffset offset, const TYPE& data);
/// Set graphics constants based on pre-allocated memory  (thread safe)
void SetGraphicsConstants(uint offset, const void* data, SizeT bytes);
/// Allocate range of compute memory and set data, return offset  (thread safe)
template<class TYPE> uint SetComputeConstants(const TYPE& data);
/// Allocate range of graphics memory and set data as an array of elements, return offset (thread safe)
template<class TYPE> uint SetComputeConstants(const TYPE* data, SizeT elements);
/// Set graphics constants based on pre-allocated memory (thread safe)
template<class TYPE> void SetComputeConstants(ConstantBufferOffset offset, const TYPE& data);
/// Set graphics constants based on pre-allocated memory  (thread safe)
void SetComputeConstants(uint offset, const void* data, SizeT bytes);
/// Lock constant updates
void LockConstantUpdates();

/// allocate range of graphics memory and set data, return offset
int SetGraphicsConstantsInternal(const void* data, SizeT size);
/// use pre-allocated range of memory to update graphics constants
void SetGraphicsConstantsInternal(ConstantBufferOffset offset, const void* data, SizeT size);
/// allocate range of compute memory and set data, return offset
int SetComputeConstantsInternal(const void* data, SizeT size);
/// use pre-allocated range of memory to update compute constants
void SetComputeConstantsInternal(ConstantBufferOffset offset, const void* data, SizeT size);
/// reserve range of graphics constant buffer memory and return offset
ConstantBufferOffset AllocateGraphicsConstantBufferMemory(uint size);
/// return id to global graphics constant buffer
CoreGraphics::BufferId GetGraphicsConstantBuffer();
/// reserve range of compute constant buffer memory and return offset
ConstantBufferOffset AllocateComputeConstantBufferMemory(uint size);
/// return id to global compute constant buffer
CoreGraphics::BufferId GetComputeConstantBuffer();
/// Flush constants for queue type, do this before recording any commands doing draw or dispatch
void FlushConstants(const CoreGraphics::CmdBufferId cmds, CoreGraphics::QueueType type);

/// trigger reloading a shader
void ReloadShaderProgram(const CoreGraphics::ShaderProgramId& pro);

/// wait for an individual queue to finish
void WaitForQueue(CoreGraphics::QueueType queue);
/// wait for all queues to finish
void WaitAndClearPendingCommands();

/// Add buffer to delete queue
void DelayedDeleteBuffer(const CoreGraphics::BufferId id);
/// Add texture to delete queue
void DelayedDeleteTexture(const CoreGraphics::TextureId id);
/// Add texture view to delete queue
void DelayedDeleteTextureView(const CoreGraphics::TextureViewId id);
/// Add command buffer to late deletion
void DelayedDeleteCommandBuffer(const CoreGraphics::CmdBufferId id);
/// Add memory allocation to delete queue
void DelayedFreeMemory(const CoreGraphics::Alloc alloc);
/// Add a descriptor set to delete queue
void DelayedDeleteDescriptorSet(const CoreGraphics::ResourceTableId id);

/// Allocate a range of queries
uint AllocateQueries(const CoreGraphics::QueryType type, uint numQueries);
/// Copy query results to buffer
void FinishQueries(const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueryType type, IndexT start, SizeT count);

/// Allocate vertices from the global vertex pool
uint AllocateVertices(const SizeT numVertices, const SizeT vertexSize);
/// Deallocate vertices
void DeallocateVertices(uint offset);
/// Get vertex buffer 
const CoreGraphics::BufferId GetVertexBuffer();

/// Swap
void Swap(IndexT i);
/// Finish current frame
void FinishFrame(IndexT frameIndex);
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
IndexT Timestamp(CoreGraphics::QueueType queue, const CoreGraphics::PipelineStage stage, const char* name);
/// get cpu profiling markers
const Util::Array<FrameProfilingMarker>& GetProfilingMarkers();
/// get number of draw calls this frame
SizeT GetNumDrawCalls();
#endif

/// sets whether or not the render device should tessellate
void SetUsePatches(bool state);
/// gets whether or not the render device should tessellate
bool GetUsePatches();

#if NEBULA_GRAPHICS_DEBUG
/// set debug name for object
template<typename OBJECT_ID_TYPE> void ObjectSetName(const OBJECT_ID_TYPE id, const char* name);
/// begin debug marker region
void QueueBeginMarker(const CoreGraphics::QueueType queue, const Math::vec4& color, const char* name);
/// end debug marker region
void QueueEndMarker(const CoreGraphics::QueueType queue);
/// insert marker
void QueueInsertMarker(const CoreGraphics::QueueType queue, const Math::vec4& color, const char* name);
#endif

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline ConstantBufferOffset
SetGraphicsConstants(const TYPE& data)
{
    return SetGraphicsConstantsInternal(&data, sizeof(TYPE));
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline ConstantBufferOffset
SetGraphicsConstants(const TYPE* data, SizeT elements)
{
    return SetGraphicsConstantsInternal(data, sizeof(TYPE) * elements);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void
SetGraphicsConstants(ConstantBufferOffset offset, const TYPE& data)
{
    SetGraphicsConstantsInternal(offset, &data, sizeof(TYPE));
}

//------------------------------------------------------------------------------
/**
*/
inline void
SetGraphicsConstants(ConstantBufferOffset offset, const void* data, SizeT bytes)
{
    SetGraphicsConstantsInternal(offset, data, bytes);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline ConstantBufferOffset
SetComputeConstants(const TYPE& data)
{
    return SetComputeConstantsInternal(&data, sizeof(TYPE));
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> 
inline ConstantBufferOffset
SetComputeConstants(const TYPE* data, SizeT elements)
{
    return SetComputeConstantsInternal(data, sizeof(TYPE) * elements);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void 
SetComputeConstants(ConstantBufferOffset offset, const TYPE& data)
{
    SetComputeConstantsInternal(offset, &data, sizeof(TYPE));
}

//------------------------------------------------------------------------------
/**
*/
inline void
SetComputeConstants(ConstantBufferOffset offset, const void* data, SizeT bytes)
{
    SetComputeConstantsInternal(offset, data, bytes);
}

} // namespace CoreGraphics
