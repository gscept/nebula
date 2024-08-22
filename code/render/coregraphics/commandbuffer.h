#pragma once
//------------------------------------------------------------------------------
/**
    CmdBuffer contains general functions related to command buffer management.

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "util/stack.h"
#include "coregraphics/config.h"
#include "coregraphics/primitivetopology.h"
#include "util/fixedarray.h"
#include "coregraphics/config.h"
#include "math/rectangle.h"
#include "coregraphics/indextype.h"
#include "ids/idallocator.h"
#include "coregraphics/memory.h"
#include "math/vec4.h"

namespace CoreGraphics
{

struct BufferId;
struct TextureId;
struct ResourceTableId;
struct VertexLayoutId;
class PrimitiveGroup;
struct ShaderProgramId;
struct BarrierId;
struct EventId;
struct PassId;
struct PipelineId;
struct BlasId;
struct TlasId;

struct TextureBarrierInfo;
struct BufferBarrierInfo;
struct AccelerationStructureBarrierInfo;

enum CmdBufferQueryBits
{
    NoQueries           = 0x0
    , Occlusion         = 0x1
    , Statistics        = 0x2
    , Timestamps        = 0x4
    , NumBits           = 3
};
__ImplementEnumBitOperators(CmdBufferQueryBits);
__ImplementEnumComparisonOperators(CmdBufferQueryBits);

enum class CmdPipelineBuildBits : uint
{
    NoInfoSet = 0,
    ShaderInfoSet = N_BIT(0),
    FramebufferLayoutInfoSet = N_BIT(1),
    InputAssemblyInfoSet = N_BIT(2),

    AllInfoSet = ShaderInfoSet | FramebufferLayoutInfoSet | InputAssemblyInfoSet,

    PipelineBuilt = N_BIT(3)
};

__ImplementEnumBitOperators(CmdPipelineBuildBits);
__ImplementEnumComparisonOperators(CmdPipelineBuildBits);

/// struct for texture copies
struct TextureCopy
{
    Math::rectangle<SizeT> region;
    uint mip;
    uint layer;
    ImageBits bits = ImageBits::ColorBits;
};

struct BufferCopy
{
    uint offset;
    uint rowLength = 0;     // for buffer to image copies
    uint imageHeight = 0;   // for buffer to image copies
};

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

#if NEBULA_GRAPHICS_DEBUG
struct NvidiaAftermathCheckpoint
{
    Util::String name;
    NvidiaAftermathCheckpoint* prev;
    bool push : 1;
};
#endif

//------------------------------------------------------------------------------
/**
*/
ID_24_8_TYPE(CmdBufferPoolId);

struct CmdBufferPoolCreateInfo
{
    CoreGraphics::QueueType queue;
    bool resetable : 1;     // Allow buffers created from this pool to be resetable
    bool shortlived : 1;    // Hint if the buffers allocated from this pool will be short lived
};

/// create new command buffer pool
const CmdBufferPoolId CreateCmdBufferPool(const CmdBufferPoolCreateInfo& info);
/// destroy command buffer pool
void DestroyCmdBufferPool(const CmdBufferPoolId pool);

struct CmdBufferCreateInfo
{
    bool subBuffer : 1;     // create buffer to be executed on another command buffer (subBuffer must be 0 on the other buffer)
    CmdBufferPoolId pool;
    QueueType usage;
    CmdBufferQueryBits queryTypes;

#if NEBULA_GRAPHICS_DEBUG
    const char* name;
#endif

    CmdBufferCreateInfo()
        : subBuffer(false)
        , pool(InvalidCmdBufferPoolId)
        , usage(QueueType::GraphicsQueueType)
        , queryTypes(CmdBufferQueryBits::Timestamps)
#if NEBULA_GRAPHICS_DEBUG
        , name(nullptr)
#endif
    {};
};

struct CmdBufferBeginInfo
{
    bool submitOnce : 1;
    bool submitDuringPass : 1;
    bool resubmittable : 1;
};

struct CmdBufferClearInfo
{
    bool allowRelease : 1;  // release resources when clearing (don't use if buffer converges in size)
};

struct CmdBufferMarkerBundle
{
    Util::Stack<CoreGraphics::FrameProfilingMarker> markerStack;
    Util::Array<CoreGraphics::FrameProfilingMarker> finishedMarkers;
};

ID_24_8_TYPE(CmdBufferId);
_DECL_ACQUIRE_RELEASE(CmdBufferId);

/// create new command buffer
const CmdBufferId CreateCmdBuffer(const CmdBufferCreateInfo& info);
/// destroy command buffer
void DestroyCmdBuffer(const CmdBufferId id);

/// begin recording to command buffer
void CmdBeginRecord(const CmdBufferId id, const CmdBufferBeginInfo& info);
/// end recording command buffer, it may be submitted after this is done
void CmdEndRecord(const CmdBufferId id);

/// clear the command buffer to be empty
void CmdReset(const CmdBufferId id, const CmdBufferClearInfo& info);

/// Set vertex buffer
void CmdSetVertexBuffer(const CmdBufferId id, IndexT streamIndex, const CoreGraphics::BufferId& buffer, SizeT bufferOffset);
/// Set vertex layout
void CmdSetVertexLayout(const CmdBufferId id, const CoreGraphics::VertexLayoutId& vl);
/// Set index buffer
void CmdSetIndexBuffer(const CmdBufferId id, const IndexType::Code indexType, const CoreGraphics::BufferId& buffer, SizeT bufferOffset);
/// Set the type of topology used
void CmdSetPrimitiveTopology(const CmdBufferId id, const CoreGraphics::PrimitiveTopology::Code topo);

/// Set shader program
void CmdSetShaderProgram(const CmdBufferId id, const CoreGraphics::ShaderProgramId pro, bool bindGlobals = true);
/// Set resource table
void CmdSetResourceTable(const CmdBufferId id, const CoreGraphics::ResourceTableId table, const IndexT slot, CoreGraphics::ShaderPipeline pipeline, const Util::FixedArray<uint>& offsets);
/// Set resource table using raw offsets
void CmdSetResourceTable(const CmdBufferId id, const CoreGraphics::ResourceTableId table, const IndexT slot, CoreGraphics::ShaderPipeline pipeline, uint32 numOffsets, uint32* offsets);
/// Set push constants
void CmdPushConstants(const CmdBufferId id, ShaderPipeline pipeline, uint offset, uint size, const void* data);
/// Create (if necessary) and bind pipeline based on state thus far
void CmdSetGraphicsPipeline(const CmdBufferId id);
/// Set graphics pipeline directly
void CmdSetGraphicsPipeline(const CmdBufferId buf, const PipelineId pipeline);
/// Set ray tracing pipeline
void CmdSetRayTracingPipeline(const CmdBufferId buf, const PipelineId pipeline);

/// Insert pipeline barrier
void CmdBarrier(
            const CmdBufferId id,
            CoreGraphics::PipelineStage fromStage,
            CoreGraphics::PipelineStage toStage,
            CoreGraphics::BarrierDomain domain,
            const IndexT fromQueue = InvalidIndex,
            const IndexT toQueue = InvalidIndex,
            const char* name = nullptr
);
/// Insert pipeline barrier
void CmdBarrier(
            const CmdBufferId id,
            CoreGraphics::PipelineStage fromStage,
            CoreGraphics::PipelineStage toStage,
            CoreGraphics::BarrierDomain domain,
            const Util::FixedArray<TextureBarrierInfo>& textures,
            const IndexT fromQueue = InvalidIndex,
            const IndexT toQueue = InvalidIndex,
            const char* name = nullptr
);
/// Insert pipeline barrier
void CmdBarrier(
            const CmdBufferId id,
            CoreGraphics::PipelineStage fromStage,
            CoreGraphics::PipelineStage toStage,
            CoreGraphics::BarrierDomain domain,
            const Util::FixedArray<BufferBarrierInfo>& buffers,
            const IndexT fromQueue = InvalidIndex,
            const IndexT toQueue = InvalidIndex,
            const char* name = nullptr
);
/// Insert pipeline barrier
void CmdBarrier(
            const CmdBufferId id,
            CoreGraphics::PipelineStage fromStage,
            CoreGraphics::PipelineStage toStage,
            CoreGraphics::BarrierDomain domain,
            const Util::FixedArray<AccelerationStructureBarrierInfo>& accelerationStructures,
            const IndexT fromQueue = InvalidIndex,
            const IndexT toQueue = InvalidIndex,
            const char* name = nullptr
);
/// Insert pipeline barrier
void CmdBarrier(
            const CmdBufferId id, 
            CoreGraphics::PipelineStage fromStage,
            CoreGraphics::PipelineStage toStage,
            CoreGraphics::BarrierDomain domain,
            const Util::FixedArray<TextureBarrierInfo>& textures,
            const Util::FixedArray<BufferBarrierInfo>& buffers,
            const Util::FixedArray<AccelerationStructureBarrierInfo>& accelerationStructures,
            const IndexT fromQueue = InvalidIndex,
            const IndexT toQueue = InvalidIndex,
            const char* name = nullptr
);

/// Handover
void CmdHandover(
            const CmdBufferId from,
            const CmdBufferId to,
            CoreGraphics::PipelineStage fromStage,
            CoreGraphics::PipelineStage toStage,
            const Util::FixedArray<TextureBarrierInfo>& textures,
            const Util::FixedArray<BufferBarrierInfo>& buffers,
            const IndexT fromQueue = InvalidIndex,
            const IndexT toQueue = InvalidIndex,
            const char* name = nullptr
);
            

/// Insert execution barrier
void CmdBarrier(const CmdBufferId id, const CoreGraphics::BarrierId barrier);
/// Signals an event
void CmdSignalEvent(const CmdBufferId id, const CoreGraphics::EventId ev, const CoreGraphics::PipelineStage stage);
/// Signals an event
void CmdWaitEvent(
    const CmdBufferId id
    , const EventId ev
    , const CoreGraphics::PipelineStage waitStage
    , const CoreGraphics::PipelineStage signalStage
);
/// Signals an event
void CmdResetEvent(const CmdBufferId id, const CoreGraphics::EventId ev, const CoreGraphics::PipelineStage stage);

/// Begin pass
void CmdBeginPass(const CmdBufferId id, const CoreGraphics::PassId pass);
/// Progress to next subpass
void CmdNextSubpass(const CmdBufferId id);
/// End pass
void CmdEndPass(const CmdBufferId id);
/// Draw primitives
void CmdDraw(const CmdBufferId id, const CoreGraphics::PrimitiveGroup& pg);
/// Draw primitives instanced
void CmdDraw(const CmdBufferId id, SizeT numInstances, const CoreGraphics::PrimitiveGroup& pg);
/// Draw primitives instanced with base instance
void CmdDraw(const CmdBufferId id, SizeT numInstances, IndexT baseInstance, const CoreGraphics::PrimitiveGroup& pg);
/// Draw indirect, draws is the amount of draws in the buffer, and stride is the byte offset between each call
void CmdDrawIndirect(const CmdBufferId id, const CoreGraphics::BufferId buffer, IndexT bufferOffset, SizeT numDraws, SizeT stride);
/// Draw indirect, draws is the amount of draws in the buffer, and stride is the byte offset between each call
void CmdDrawIndirectIndexed(const CmdBufferId id, const CoreGraphics::BufferId buffer, IndexT bufferOffset, SizeT numDraws, SizeT stride);
/// Perform computation
void CmdDispatch(const CmdBufferId id, int dimX, int dimY, int dimZ);
/// Resolve MSAA source to non-MSAA target
void CmdResolve(const CmdBufferId id, const CoreGraphics::TextureId source, const CoreGraphics::TextureCopy sourceCopy, const CoreGraphics::TextureId dest, const CoreGraphics::TextureCopy destCopy);
/// Build BLAS 
void CmdBuildBlas(const CmdBufferId id, const CoreGraphics::BlasId blas);
/// Build TLAS
void CmdBuildTlas(const CmdBufferId id, const CoreGraphics::TlasId tlas);

struct RayDispatchTable
{
    struct Entry
    {
        CoreGraphics::DeviceAddress baseAddress;
        CoreGraphics::DeviceSize entrySize;
        ubyte numEntries;
    };

    Entry genEntry, missEntry, hitEntry, callableEntry;
};
/// Fire rays
void CmdRaysDispatch(const CmdBufferId id, const RayDispatchTable& table, int dimX, int dimY, int dimZ);
/// Draw meshlets
void CmdDrawMeshlets(const CmdBufferId id, int dimX, int dimY, int dimZ);

/// Copy between textures
void CmdCopy(
    const CmdBufferId id
    , const CoreGraphics::TextureId fromTexture
    , const Util::Array<CoreGraphics::TextureCopy>& from
    , const CoreGraphics::TextureId toTexture
    , const Util::Array<CoreGraphics::TextureCopy>& to
);
/// Copy from texture to buffer
void CmdCopy(
    const CmdBufferId id
    , const CoreGraphics::TextureId fromTexture
    , const Util::Array<CoreGraphics::TextureCopy>& from
    , const CoreGraphics::BufferId toBuffer
    , const Util::Array<CoreGraphics::BufferCopy>& to
);
/// Copy between buffers
void CmdCopy(
    const CmdBufferId id
    , const CoreGraphics::BufferId fromBuffer
    , const Util::Array<CoreGraphics::BufferCopy>& from
    , const CoreGraphics::BufferId toBuffer
    , const Util::Array<CoreGraphics::BufferCopy>& to
    , const SizeT size
);
/// Copy from buffer to texture
void CmdCopy(
    const CmdBufferId id
    , const CoreGraphics::BufferId fromBuffer
    , const Util::Array<CoreGraphics::BufferCopy>& from
    , const CoreGraphics::TextureId toTexture
    , const Util::Array<CoreGraphics::TextureCopy>& to
);
/// Blit textures
void CmdBlit(
    const CmdBufferId id
    , const CoreGraphics::TextureId fromTexture
    , const CoreGraphics::TextureCopy& from
    , const CoreGraphics::TextureId toTexture
    , const CoreGraphics::TextureCopy& to
);

/// Set viewport array
void CmdSetViewports(const CmdBufferId id, Util::FixedArray<Math::rectangle<int>> viewports);
/// Set scissor array
void CmdSetScissors(const CmdBufferId id, Util::FixedArray<Math::rectangle<int>> rects);
/// Sets a viewport for a certain index
void CmdSetViewport(const CmdBufferId id, const Math::rectangle<int>& rect, int index);
/// Sets a scissor rect for a certain index
void CmdSetScissorRect(const CmdBufferId id, const Math::rectangle<int>& rect, int index);
/// Set the stencil reference values
void CmdSetStencilRef(const CmdBufferId id, const uint frontRef, const uint backRef);
/// Set the stencil read mask (compare mask(
void CmdSetStencilReadMask(const CmdBufferId id, const uint readMask);
/// Set the stencil write mask
void CmdSetStencilWriteMask(const CmdBufferId id, const uint writeMask);

/// Update buffer using memory pointer in command buffer
void CmdUpdateBuffer(
    const CmdBufferId id
    , const CoreGraphics::BufferId buffer
    , uint offset
    , uint size
    , const void* data
);

/// Start occlusion queries
void CmdStartOcclusionQueries(const CmdBufferId id);
/// End occlusion queries
void CmdEndOcclusionQueries(const CmdBufferId id);

/// Start pipeline statistics
void CmdStartPipelineQueries(const CmdBufferId id);
/// End pipeline statistics
void CmdEndPipelineQueries(const CmdBufferId id);

#if NEBULA_GRAPHICS_DEBUG
/// Begin command buffer marker
void CmdBeginMarker(const CmdBufferId id, const Math::vec4& color, const char* name);
/// End command buffer marker
void CmdEndMarker(const CmdBufferId id);
/// Insert marker without a beginning and end
void CmdInsertMarker(const CmdBufferId id, const Math::vec4& color, const char* name);
#endif

/// Finish queries
void CmdFinishQueries(const CmdBufferId id);

#if NEBULA_ENABLE_PROFILING
/// This is not a command buffer command, but is used to get the markers in the buffer
Util::Array<CoreGraphics::FrameProfilingMarker> CmdCopyProfilingMarkers(const CmdBufferId id);
/// Get the offset to the first query
uint CmdGetMarkerOffset(const CmdBufferId id);
#endif

#if NEBULA_GRAPHICS_DEBUG
struct CmdMarkerScope
{
    const CmdBufferId id;

    CmdMarkerScope(const CmdBufferId id, const Math::vec4& color, const char* name)
        : id(id)
    {
        CoreGraphics::CmdBeginMarker(id, color, name);
    }
    ~CmdMarkerScope()
    {
        CoreGraphics::CmdEndMarker(this->id);
    }
};

#endif

#if NEBULA_GRAPHICS_DEBUG
    #define N_CMD_SCOPE(buf, color, name) CoreGraphics::CmdMarkerScope __##cmdscope(buf, color, name)
#else
    #define N_CMD_SCOPE(x, y, z)
#endif


} // namespace CoreGraphics

