//------------------------------------------------------------------------------
//  clustercontext.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "clustercontext.h"
#include "coregraphics/shader.h"
#include "coregraphics/graphicsdevice.h"
#include "frame/framesubgraph.h"
#include "frame/framecode.h"
#include "graphics/graphicsserver.h"

#include "graphics/globalconstants.h"

namespace Clustering
{

static const SizeT ClusterSubdivsX = 64;
static const SizeT ClusterSubdivsY = 64;
static const SizeT ClusterSubdivsZ = 24;

struct
{
    CoreGraphics::ShaderId clusterShader;
    CoreGraphics::ShaderProgramId clusterGenerateProgram;
    CoreGraphics::BufferId clusterBuffer;

    ClusterGenerate::ClusterUniforms uniforms;
    CoreGraphics::BufferId constantBuffer;

    CoreGraphics::WindowId window;

    SizeT clusterDimensions[3];
    float zNear, zFar;
    float zDistribution;
    float zInvScale, zInvBias;
    float xResolution, yResolution;
    float invXResolution, invYResolution;

    Memory::ArenaAllocator<sizeof(Frame::FrameCode)> frameOpAllocator;

    SizeT numThreads;

} state;

__ImplementPluginContext(ClusterContext);

//------------------------------------------------------------------------------
/**
*/
ClusterContext::ClusterContext()
{
}

//------------------------------------------------------------------------------
/**
*/
ClusterContext::~ClusterContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
ClusterContext::Create(float ZNear, float ZFar, const CoreGraphics::WindowId window)
{
#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = ClusterContext::OnRenderDebug;
#endif
    __bundle.OnWindowResized = ClusterContext::WindowResized;

    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    using namespace CoreGraphics;
    state.clusterShader = ShaderGet("shd:cluster_generate.fxb");
    state.clusterGenerateProgram = ShaderGetProgram(state.clusterShader, ShaderFeatureFromString("AABBGenerate"));

    uint numBuffers = CoreGraphics::GetNumBufferedFrames();


    state.window = window;
    state.zNear = ZNear;
    state.zFar = ZFar;
    CoreGraphics::DisplayMode displayMode = CoreGraphics::WindowGetDisplayMode(state.window);

    state.clusterDimensions[0] = Math::divandroundup(displayMode.GetWidth(), ClusterSubdivsX);
    state.clusterDimensions[1] = Math::divandroundup(displayMode.GetHeight(), ClusterSubdivsY);
    state.clusterDimensions[2] = ClusterSubdivsZ;

    state.zDistribution = ZFar / ZNear;
    state.zInvScale = float(state.clusterDimensions[2]) / Math::log2(state.zDistribution);
    state.zInvBias = -(float(state.clusterDimensions[2]) * Math::log2(ZNear) / Math::log2(state.zDistribution));
    state.xResolution = displayMode.GetWidth();
    state.yResolution = displayMode.GetHeight();
    state.invXResolution = 1.0f / displayMode.GetWidth();
    state.invYResolution = 1.0f / displayMode.GetHeight();

    BufferCreateInfo rwb3Info;
    rwb3Info.name = "ClusterAABBBuffer";
    rwb3Info.size = state.clusterDimensions[0] * state.clusterDimensions[1] * state.clusterDimensions[2];
    rwb3Info.elementSize = sizeof(ClusterGenerate::ClusterAABB);
    rwb3Info.mode = BufferAccessMode::DeviceLocal;
    rwb3Info.usageFlags = CoreGraphics::ReadWriteBuffer;
    rwb3Info.queueSupport = CoreGraphics::GraphicsQueueSupport | CoreGraphics::ComputeQueueSupport;
    state.clusterBuffer = CreateBuffer(rwb3Info);
    state.constantBuffer = ShaderCreateConstantBuffer(state.clusterShader, "ClusterUniforms", CoreGraphics::DeviceAndHost);

    for (IndexT i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        CoreGraphics::ResourceTableId frameResourceTable = Graphics::GetFrameResourceTable(i);

        ResourceTableSetRWBuffer(frameResourceTable, { state.clusterBuffer, Shared::Table_Frame::ClusterAABBs::SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetConstantBuffer(frameResourceTable, { state.constantBuffer, Shared::Table_Frame::ClusterUniforms::SLOT, 0, Shared::Table_Frame::ClusterUniforms::SIZE, 0 });
    }

    Frame::FrameCode* op = state.frameOpAllocator.Alloc<Frame::FrameCode>();
    op->domain = CoreGraphics::BarrierDomain::Global;
    op->queue = CoreGraphics::QueueType::ComputeQueueType;
    op->bufferDeps.Add(state.clusterBuffer,
                         {
                             "ClusterBuffer"
                             , CoreGraphics::PipelineStage::ComputeShaderWrite
                             , CoreGraphics::BufferSubresourceInfo()
                         });
    op->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, state.clusterGenerateProgram);

        // Run the job as series of 1024 clusters at a time
        CmdDispatch(cmdBuf, Math::ceil((state.clusterDimensions[0] * state.clusterDimensions[1] * state.clusterDimensions[2]) / 64.0f), 1, 1);
    };
    Frame::AddSubgraph("Cluster AABB Generation", { op });
}

//------------------------------------------------------------------------------
/**
*/
const SizeT 
ClusterContext::GetNumClusters()
{
    return state.clusterDimensions[0] * state.clusterDimensions[1] * state.clusterDimensions[2];
}

//------------------------------------------------------------------------------
/**
*/
const std::array<SizeT, 3> 
ClusterContext::GetClusterDimensions()
{
    return std::array<SizeT, 3> { state.clusterDimensions[0], state.clusterDimensions[1], state.clusterDimensions[2] };
}

//------------------------------------------------------------------------------
/**
*/
void 
ClusterContext::UpdateResources(const Graphics::FrameContext& ctx)
{
    using namespace CoreGraphics;

    state.uniforms.ZDistribution = state.zDistribution;
    state.uniforms.InvZScale = state.zInvScale;
    state.uniforms.InvZBias = state.zInvBias;
    state.uniforms.InvFramebufferDimensions[0] = state.invXResolution;
    state.uniforms.InvFramebufferDimensions[1] = state.invYResolution;
    state.uniforms.FramebufferDimensions[0] = state.xResolution;
    state.uniforms.FramebufferDimensions[1] = state.yResolution;
    state.uniforms.NumCells[0] = state.clusterDimensions[0];
    state.uniforms.NumCells[1] = state.clusterDimensions[1];
    state.uniforms.NumCells[2] = state.clusterDimensions[2];
    state.uniforms.BlockSize[0] = ClusterSubdivsX;
    state.uniforms.BlockSize[1] = ClusterSubdivsY;

    // update constant buffer, probably super unnecessary since these values never change
    BufferUpdate(state.constantBuffer, state.uniforms);
    BufferFlush(state.constantBuffer);
}

//------------------------------------------------------------------------------
/**
*/
void 
ClusterContext::OnRenderDebug(uint32_t flags)
{
}

//------------------------------------------------------------------------------
/**
*/
void
ClusterContext::WindowResized(const CoreGraphics::WindowId id, SizeT width, SizeT height)
{
    if (id == state.window)
    {
        CoreGraphics::DisplayMode displayMode = CoreGraphics::WindowGetDisplayMode(id);

        state.clusterDimensions[0] = Math::divandroundup(displayMode.GetWidth(), ClusterSubdivsX);
        state.clusterDimensions[1] = Math::divandroundup(displayMode.GetHeight(), ClusterSubdivsY);
        state.clusterDimensions[2] = ClusterSubdivsZ;

        state.zDistribution = state.zFar / state.zNear;
        state.zInvScale = float(state.clusterDimensions[2]) / Math::log2(state.zDistribution);
        state.zInvBias = -(float(state.clusterDimensions[2]) * Math::log2(state.zNear) / Math::log2(state.zDistribution));
        state.xResolution = displayMode.GetWidth();
        state.yResolution = displayMode.GetHeight();
        state.invXResolution = 1.0f / displayMode.GetWidth();
        state.invYResolution = 1.0f / displayMode.GetHeight();

        CoreGraphics::DestroyBuffer(state.clusterBuffer);

        CoreGraphics::BufferCreateInfo rwb3Info;
        rwb3Info.name = "ClusterAABBBuffer";
        rwb3Info.size = state.clusterDimensions[0] * state.clusterDimensions[1] * state.clusterDimensions[2];
        rwb3Info.elementSize = sizeof(ClusterGenerate::ClusterAABB);
        rwb3Info.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
        rwb3Info.usageFlags = CoreGraphics::ReadWriteBuffer;
        rwb3Info.queueSupport = CoreGraphics::GraphicsQueueSupport | CoreGraphics::ComputeQueueSupport;
        state.clusterBuffer = CreateBuffer(rwb3Info);

        for (IndexT i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
        {
            CoreGraphics::ResourceTableId frameResourceTable = Graphics::GetFrameResourceTable(i);

            ResourceTableSetRWBuffer(frameResourceTable, { state.clusterBuffer, Shared::Table_Frame::ClusterAABBs::SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
            ResourceTableCommitChanges(frameResourceTable);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::BufferId
ClusterContext::GetClusterBuffer()
{
    return state.clusterBuffer;
}

} // namespace Clustering
