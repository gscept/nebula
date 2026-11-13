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
#include "frame/default.h"
#include "gpulang/render/system_shaders/cluster_generate.h"

namespace Clustering
{

static const SizeT ClusterBlockSizeX = 64;
static const SizeT ClusterBlockSizeY = 64;
static const SizeT ClusterBlockSizeZ = 24;

struct
{
    CoreGraphics::ShaderId clusterShader;
    CoreGraphics::ShaderProgramId clusterGenerateProgram;
    CoreGraphics::BufferId clusterBuffer;

    ClusterGenerate::ClusterUniforms::STRUCT uniforms;
    // One per buffered frame, the CPU writes the next frame's grid while the GPU may still read the previous one
    Util::FixedArray<CoreGraphics::BufferId> constantBuffers;
    // Number of clusters in the grid uploaded for each buffered frame
    Util::FixedArray<SizeT> uploadedNumClusters;

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
#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = ClusterContext::OnRenderDebug;
#endif
    __bundle.OnViewportResized = ClusterContext::Resize;

    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    using namespace CoreGraphics;
    state.clusterShader = ShaderGet("shd:system_shaders/cluster_generate.gplb");
    state.clusterGenerateProgram = ShaderGetProgram(state.clusterShader, ShaderFeatureMask("AABBGenerate"));

    state.window = window;
    state.zNear = ZNear;
    state.zFar = ZFar;
    CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(FrameScript_default::Texture_LightBuffer());

    state.clusterDimensions[0] = Math::divandroundup(dims.width, ClusterBlockSizeX);
    state.clusterDimensions[1] = Math::divandroundup(dims.height, ClusterBlockSizeY);
    state.clusterDimensions[2] = ClusterBlockSizeZ;
    n_assert(state.clusterDimensions[0] * state.clusterDimensions[1] * state.clusterDimensions[2] <= ClusterGenerate::NUM_CLUSTER_ENTRIES);

    state.zDistribution = ZFar / ZNear;
    state.zInvScale = float(state.clusterDimensions[2]) / Math::log2(state.zDistribution);
    state.zInvBias = -(float(state.clusterDimensions[2]) * Math::log2(ZNear) / Math::log2(state.zDistribution));
    state.xResolution = dims.width;
    state.yResolution = dims.height;
    state.invXResolution = 1.0f / dims.width;
    state.invYResolution = 1.0f / dims.height;

    // Allocate for the maximum amount of clusters, so the buffer never has to be recreated (and rebound while frames are in flight) on resize
    BufferCreateInfo rwb3Info;
    rwb3Info.name = "ClusterAABBBuffer";
    rwb3Info.size = ClusterGenerate::NUM_CLUSTER_ENTRIES;
    rwb3Info.elementSize = sizeof(ClusterGenerate::ClusterAABBs::STRUCT);
    rwb3Info.mode = BufferAccessMode::DeviceLocal;
    rwb3Info.usageFlags = CoreGraphics::BufferUsage::ReadWrite;
    rwb3Info.queueSupport = CoreGraphics::GraphicsQueueSupport | CoreGraphics::ComputeQueueSupport;
    state.clusterBuffer = CreateBuffer(rwb3Info);
    state.constantBuffers.Resize(CoreGraphics::GetNumBufferedFrames());
    state.uploadedNumClusters.Resize(CoreGraphics::GetNumBufferedFrames());
    state.uploadedNumClusters.Fill(0);
    for (IndexT i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        state.constantBuffers[i] = ShaderCreateConstantBuffer(state.clusterShader, "ClusterUniforms", CoreGraphics::DeviceAndHost);
        auto resourceTables = Graphics::GetFrameResourceTables(i);

        for (auto& table : resourceTables)
        {
            ResourceTableSetRWBuffer(table, { state.clusterBuffer, ClusterGenerate::ClusterAABBs::BINDING, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
            ResourceTableSetConstantBuffer(table, { state.constantBuffers[i], ClusterGenerate::ClusterUniforms::BINDING, 0, sizeof(ClusterGenerate::ClusterUniforms::STRUCT), 0 });
        }
    }

    FrameScript_default::Bind_ClusterBuffer(state.clusterBuffer);
    FrameScript_default::RegisterSubgraph_ClusterAABBGeneration_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, state.clusterGenerateProgram, queue);

        state.clusterDimensions[0] = Math::divandroundup(viewport.width(), ClusterBlockSizeX);
        state.clusterDimensions[1] = Math::divandroundup(viewport.height(), ClusterBlockSizeY);
        state.clusterDimensions[2] = ClusterBlockSizeZ;
        n_assert(state.clusterDimensions[0] * state.clusterDimensions[1] * state.clusterDimensions[2] <= ClusterGenerate::NUM_CLUSTER_ENTRIES);

        state.xResolution = viewport.width();
        state.yResolution = viewport.height();
        state.invXResolution = 1.0f / state.xResolution;
        state.invYResolution = 1.0f / state.yResolution;
        state.zInvScale = float(state.clusterDimensions[2]) / Math::log2(state.zDistribution);
        state.zInvBias = -(float(state.clusterDimensions[2]) * Math::log2(state.zNear) / Math::log2(state.zDistribution));

        // Generate every cluster of the grid this frame's uniforms describe, which is what culling and shading index into.
        // The dimensions computed above are only uploaded for the next frame.
        CmdDispatch(cmdBuf, Math::divandroundup(state.uploadedNumClusters[bufferIndex], 64), 1, 1);
    }, {
        { FrameScript_default::BufferIndex::ClusterBuffer, CoreGraphics::PipelineStage::ComputeShaderWrite }
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
    state.uniforms.BlockSize[0] = ClusterBlockSizeX;
    state.uniforms.BlockSize[1] = ClusterBlockSizeY;

    // The grid changes whenever the viewport is resized, so only write the buffer of the frame being prepared
    BufferUpdate(state.constantBuffers[ctx.bufferIndex], state.uniforms);
    BufferFlush(state.constantBuffers[ctx.bufferIndex]);
    state.uploadedNumClusters[ctx.bufferIndex] = state.clusterDimensions[0] * state.clusterDimensions[1] * state.clusterDimensions[2];
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
ClusterContext::Resize(const uint framescriptHash, SizeT width, SizeT height)
{
    if (framescriptHash == FrameScript_default::ID)
    {
        CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(FrameScript_default::Texture_LightBuffer());

        state.clusterDimensions[0] = Math::divandroundup(dims.width, ClusterBlockSizeX);
        state.clusterDimensions[1] = Math::divandroundup(dims.height, ClusterBlockSizeY);
        state.clusterDimensions[2] = ClusterBlockSizeZ;
        n_assert(state.clusterDimensions[0] * state.clusterDimensions[1] * state.clusterDimensions[2] <= ClusterGenerate::NUM_CLUSTER_ENTRIES);

        state.zDistribution = state.zFar / state.zNear;
        state.zInvScale = float(state.clusterDimensions[2]) / Math::log2(state.zDistribution);
        state.zInvBias = -(float(state.clusterDimensions[2]) * Math::log2(state.zNear) / Math::log2(state.zDistribution));
        state.xResolution = dims.width;
        state.yResolution = dims.height;
        state.invXResolution = 1.0f / dims.width;
        state.invYResolution = 1.0f / dims.height;
    }
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::BufferId*
ClusterContext::GetClusterBuffer()
{
    return &state.clusterBuffer;
}

} // namespace Clustering
