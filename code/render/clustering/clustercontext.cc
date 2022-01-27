//------------------------------------------------------------------------------
//  clustercontext.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "clustercontext.h"
#include "coregraphics/shader.h"
#include "coregraphics/graphicsdevice.h"
#include "frame/frameplugin.h"
#include "graphics/graphicsserver.h"
#include "graphics/cameracontext.h"

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
    IndexT uniformsSlot, clusterAABBSlot;

    CoreGraphics::WindowId window;

    SizeT clusterDimensions[3];
    float zNear, zFar;
    float zDistribution;
    float zInvScale, zInvBias;
    float xResolution, yResolution;
    float invXResolution, invYResolution;

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
    __bundle.OnUpdateResources = ClusterContext::UpdateResources;
    __bundle.StageBits = &ClusterContext::__state.currentStage;
#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = ClusterContext::OnRenderDebug;
#endif
    __bundle.OnWindowResized = ClusterContext::WindowResized;

    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    using namespace CoreGraphics;
    state.clusterShader = ShaderGet("shd:cluster_generate.fxb");
    state.clusterGenerateProgram = ShaderGetProgram(state.clusterShader, ShaderFeatureFromString("AABBGenerate"));

    uint numBuffers = CoreGraphics::GetNumBufferedFrames();

    state.uniformsSlot = ShaderGetResourceSlot(state.clusterShader, "ClusterUniforms");
    state.clusterAABBSlot = ShaderGetResourceSlot(state.clusterShader, "ClusterAABBs");

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
    state.constantBuffer = ShaderCreateConstantBuffer(state.clusterShader, "ClusterUniforms");

    // get per-view resource tables
    const Util::FixedArray<CoreGraphics::ResourceTableId>& viewTables = TransformDevice::Instance()->GetViewResourceTables();

    for (IndexT i = 0; i < viewTables.Size(); i++)
    {
        CoreGraphics::ResourceTableId table = viewTables[i];
        ResourceTableSetRWBuffer(table, { state.clusterBuffer, state.clusterAABBSlot, 0, false, false, -1, 0 });
        ResourceTableSetConstantBuffer(table, { state.constantBuffer, state.uniformsSlot, 0, false, false, sizeof(ClusterGenerate::ClusterUniforms), 0 });
    }

    // called from main script
    Frame::AddCallback("ClusterContext - Update Clusters", [](const IndexT frame, const IndexT bufferIndex) // trigger update
        {
            UpdateClusters();
        });
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
    BufferUpdate(state.constantBuffer, state.uniforms, 0);
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
        CoreGraphics::DestroyBuffer(state.constantBuffer);

        CoreGraphics::BufferCreateInfo rwb3Info;
        rwb3Info.name = "ClusterAABBBuffer";
        rwb3Info.size = state.clusterDimensions[0] * state.clusterDimensions[1] * state.clusterDimensions[2];
        rwb3Info.elementSize = sizeof(ClusterGenerate::ClusterAABB);
        rwb3Info.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
        rwb3Info.usageFlags = CoreGraphics::ReadWriteBuffer;
        rwb3Info.queueSupport = CoreGraphics::GraphicsQueueSupport | CoreGraphics::ComputeQueueSupport;
        state.clusterBuffer = CreateBuffer(rwb3Info);
        state.constantBuffer = ShaderCreateConstantBuffer(state.clusterShader, "ClusterUniforms");

        const Util::FixedArray<CoreGraphics::ResourceTableId>& viewTables = CoreGraphics::TransformDevice::Instance()->GetViewResourceTables();
        for (IndexT i = 0; i < viewTables.Size(); i++)
        {
            CoreGraphics::ResourceTableId table = viewTables[i];
            ResourceTableSetRWBuffer(table, { state.clusterBuffer, state.clusterAABBSlot, 0, false, false, -1, 0 });
            ResourceTableSetConstantBuffer(table, { state.constantBuffer, state.uniformsSlot, 0, false, false, sizeof(ClusterGenerate::ClusterUniforms), 0 });
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ClusterContext::UpdateClusters()
{
    // update constants
    using namespace CoreGraphics;

    const IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();


    // begin command buffer work
    CommandBufferBeginMarker(ComputeQueueType, NEBULA_MARKER_BLUE, "Cluster AABB Generation");

    // make sure to sync so we don't read from data that is being written...
    BarrierInsert(ComputeQueueType,
        BarrierStage::ComputeShader,
        BarrierStage::ComputeShader,
        BarrierDomain::Global,
        nullptr,
        {
            BufferBarrier
            {
                state.clusterBuffer,
                BarrierAccess::ShaderRead,
                BarrierAccess::ShaderWrite,
                0, -1
            }
        }, "AABB begin barrier");

    SetShaderProgram(state.clusterGenerateProgram, ComputeQueueType);

    // run the job as series of 1024 clusters at a time
    Compute(Math::ceil((state.clusterDimensions[0] * state.clusterDimensions[1] * state.clusterDimensions[2]) / 64.0f), 1, 1, ComputeQueueType);

    // make sure to sync so we don't read from data that is being written...
    BarrierInsert(ComputeQueueType,
        BarrierStage::ComputeShader,
        BarrierStage::ComputeShader,
        BarrierDomain::Global,
        nullptr,
        {
            BufferBarrier
            {
                state.clusterBuffer,
                BarrierAccess::ShaderWrite,
                BarrierAccess::ShaderRead,
                0, -1
            }
        }
        , "AABB finish barrier");

    CommandBufferEndMarker(ComputeQueueType);
}
} // namespace Clustering
