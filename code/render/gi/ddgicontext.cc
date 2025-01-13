//------------------------------------------------------------------------------
//  @file ddgicontext.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "coregraphics/pipeline.h"
#include "ddgicontext.h"

#include <cstdint>

#include "clustering/clustercontext.h"
#include "coregraphics/meshloader.h"
#include "coregraphics/meshresource.h"
#include "frame/default.h"
#include "graphics/cameracontext.h"
#include "graphics/view.h"
#include "lighting/lightcontext.h"
#include "raytracing/raytracingcontext.h"
#include "coregraphics/shaperenderer.h"


#include "gi/shaders/probe_update.h"
#include "gi/shaders/probe_finalize.h"
#include "gi/shaders/gi_volume_cull.h"
#include "gi/shaders/probe_classify.h"
#include "gi/shaders/probe_relocate.h"
#include "graphics/globalconstants.h"

#ifndef PUBLIC_BUILD
#include "gi/shaders/probe_debug.h"
#endif
#include "core/cvar.h"

Core::CVar* g_debug_ddgi = Core::CVarCreate(Core::CVar_Int, "g_debug_ddgi", "0", "Draw DDGI probes");

using namespace Graphics;

namespace GI
{
DDGIContext::DDGIVolumeAllocator DDGIContext::ddgiVolumeAllocator;
__ImplementContext(DDGIContext, DDGIContext::ddgiVolumeAllocator)

struct UpdateVolume
{
    uint probeCounts[3];
    uint numRays;
    struct
    {
        CoreGraphics::TextureId radianceDistanceTexture;
    } probeUpdateOutputs;

    struct
    {
        CoreGraphics::TextureId irradianceTexture, distanceTexture, scrollSpaceTexture;
    } volumeBlendOutputs;

    struct
    {
        CoreGraphics::TextureId offsetsTexture, statesTexture;
    } volumeStateOutputs;
    CoreGraphics::ResourceTableId updateProbesTable, blendProbesTable, relocateProbesTable, classifyProbesTable;

#ifndef PUBLIC_BUILD
    CoreGraphics::ResourceTableId debugTable;
#endif
};

struct
{
    CoreGraphics::ShaderId probeUpdateShader;
    CoreGraphics::ShaderProgramId probeUpdateProgram;
    CoreGraphics::PipelineRayTracingTable pipeline;

    CoreGraphics::ShaderId volumeCullShader;
    CoreGraphics::ShaderProgramId volumeCullProgram, volumeClusterDebugProgram;

    CoreGraphics::ShaderId probeFinalizeShader;
    CoreGraphics::ShaderProgramId probeBlendRadianceProgram, probeBlendDistanceProgram, probeBorderRadianceRowsFixup, probeBorderRadianceColumnsFixup, probeBorderDistanceRowsFixup, probeBorderDistanceColumnsFixup;

    CoreGraphics::ShaderId probesRelocateShader, probesClassifyShader;
    CoreGraphics::ShaderProgramId probesRelocateProgram, probesClassifyProgram;

    CoreGraphics::ResourceTableSet raytracingTable;

    CoreGraphics::BufferId clusterGIVolumeIndexLists;
    CoreGraphics::BufferSet stagingClusterGIVolumeList;
    CoreGraphics::BufferId clusterGIVolumeList;

    Util::Array<UpdateVolume> volumesToUpdate;
#ifndef PUBLIC_BUILD
    Util::Array<UpdateVolume> volumesToDraw;
#endif

    GiVolumeCull::GIVolume giVolumes[64];

#ifndef PUBLIC_BUILD
    CoreGraphics::ShaderId debugShader;
    CoreGraphics::ShaderProgramId debugProgram;
    CoreGraphics::PipelineId debugPipeline;
    CoreGraphics::MeshResourceId debugMeshResource;
    CoreGraphics::MeshId debugMesh;
#endif
} state;


//------------------------------------------------------------------------------
/**
*/
DDGIContext::DDGIContext()
{
}

//------------------------------------------------------------------------------
/**
*/
DDGIContext::~DDGIContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void
DDGIContext::Create()
{
    __CreateContext();
#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = DDGIContext::OnRenderDebug;

    state.debugMeshResource = Resources::CreateResource("sysmsh:sphere.nvx", "GI", nullptr, nullptr, true, false);
    state.debugMesh = CoreGraphics::MeshResourceGetMesh(state.debugMeshResource, 0);
#endif

    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    state.probeUpdateShader = CoreGraphics::ShaderGet("shd:gi/shaders/probe_update.fxb");
    state.probeUpdateProgram = CoreGraphics::ShaderGetProgram(state.probeUpdateShader, CoreGraphics::ShaderFeatureMask("ProbeRayGen"));

    auto brdfHitShader = CoreGraphics::ShaderGet("shd:raytracing/shaders/brdfhit.fxb");
    auto brdfHitProgram = CoreGraphics::ShaderGetProgram(brdfHitShader, CoreGraphics::ShaderFeatureMask("Hit"));
    auto bsdfHitShader = CoreGraphics::ShaderGet("shd:raytracing/shaders/bsdfhit.fxb");
    auto bsdfHitProgram = CoreGraphics::ShaderGetProgram(bsdfHitShader, CoreGraphics::ShaderFeatureMask("Hit"));
    auto gltfHitShader = CoreGraphics::ShaderGet("shd:raytracing/shaders/gltfhit.fxb");
    auto gltfHitProgram = CoreGraphics::ShaderGetProgram(gltfHitShader, CoreGraphics::ShaderFeatureMask("Hit"));
    auto particleHitShader = CoreGraphics::ShaderGet("shd:raytracing/shaders/particlehit.fxb");
    auto particleHitProgram = CoreGraphics::ShaderGetProgram(particleHitShader, CoreGraphics::ShaderFeatureMask("Hit"));

    state.raytracingTable = CoreGraphics::ShaderCreateResourceTableSet(state.probeUpdateShader, NEBULA_BATCH_GROUP, 1);

    // Create pipeline, the order of hit programs must match RaytracingContext::ObjectType
    state.pipeline = CoreGraphics::CreateRaytracingPipeline({ state.probeUpdateProgram, brdfHitProgram, bsdfHitProgram, gltfHitProgram, particleHitProgram }, CoreGraphics::ComputeQueueType);

    state.probeFinalizeShader = CoreGraphics::ShaderGet("shd:gi/shaders/probe_finalize.fxb");
    state.probeBlendRadianceProgram = CoreGraphics::ShaderGetProgram(state.probeFinalizeShader, CoreGraphics::ShaderFeatureMask("ProbeFinalizeRadiance"));
    state.probeBlendDistanceProgram = CoreGraphics::ShaderGetProgram(state.probeFinalizeShader, CoreGraphics::ShaderFeatureMask("ProbeFinalizeDistance"));
    state.probeBorderRadianceRowsFixup = CoreGraphics::ShaderGetProgram(state.probeFinalizeShader, CoreGraphics::ShaderFeatureMask("ProbeFinalizeBorderRowsRadiance"));
    state.probeBorderRadianceColumnsFixup = CoreGraphics::ShaderGetProgram(state.probeFinalizeShader, CoreGraphics::ShaderFeatureMask("ProbeFinalizeBorderColumnsRadiance"));
    state.probeBorderDistanceRowsFixup = CoreGraphics::ShaderGetProgram(state.probeFinalizeShader, CoreGraphics::ShaderFeatureMask("ProbeFinalizeBorderRowsDistance"));
    state.probeBorderDistanceColumnsFixup = CoreGraphics::ShaderGetProgram(state.probeFinalizeShader, CoreGraphics::ShaderFeatureMask("ProbeFinalizeBorderColumnsDistance"));

    state.volumeCullShader = CoreGraphics::ShaderGet("shd:gi/shaders/gi_volume_cull.fxb");
    state.volumeCullProgram = CoreGraphics::ShaderGetProgram(state.volumeCullShader, CoreGraphics::ShaderFeatureMask("Cull"));
    state.volumeClusterDebugProgram = CoreGraphics::ShaderGetProgram(state.volumeCullShader, CoreGraphics::ShaderFeatureMask("Debug"));

    state.probesRelocateShader = CoreGraphics::ShaderGet("shd:gi/shaders/probe_relocate.fxb");
    state.probesRelocateProgram = CoreGraphics::ShaderGetProgram(state.probesRelocateShader, CoreGraphics::ShaderFeatureMask("ProbeRelocate"));

    state.probesClassifyShader = CoreGraphics::ShaderGet("shd:gi/shaders/probe_classify.fxb");
    state.probesClassifyProgram = CoreGraphics::ShaderGetProgram(state.probesClassifyShader, CoreGraphics::ShaderFeatureMask("ProbeClassify"));

#ifndef PUBLIC_BUILD
    state.debugShader = CoreGraphics::ShaderGet("shd:gi/shaders/probe_debug.fxb");
    state.debugProgram = CoreGraphics::ShaderGetProgram(state.debugShader, CoreGraphics::ShaderFeatureMask("Debug"));
#endif

    CoreGraphics::BufferCreateInfo rwbInfo;
    rwbInfo.name = "GIIndexListsBuffer";
    rwbInfo.size = 1;
    rwbInfo.elementSize = sizeof(GiVolumeCull::GIIndexLists);
    rwbInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
    rwbInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferDestination;
    rwbInfo.queueSupport = CoreGraphics::GraphicsQueueSupport | CoreGraphics::ComputeQueueSupport;
    state.clusterGIVolumeIndexLists = CreateBuffer(rwbInfo);

    rwbInfo.name = "GIVolumeLists";
    rwbInfo.elementSize = sizeof(GiVolumeCull::GIVolumeLists);
    state.clusterGIVolumeList = CreateBuffer(rwbInfo);

    rwbInfo.name = "GIVolumeListsStagingBuffer";
    rwbInfo.mode = CoreGraphics::BufferAccessMode::HostLocal;
    rwbInfo.usageFlags = CoreGraphics::TransferBufferSource;
    state.stagingClusterGIVolumeList = CoreGraphics::BufferSet(rwbInfo);

    for (IndexT i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        CoreGraphics::ResourceTableId frameResourceTable = Graphics::GetFrameResourceTable(i);

        ResourceTableSetRWBuffer(frameResourceTable, { state.clusterGIVolumeIndexLists, Shared::Table_Frame::GIIndexLists_SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetRWBuffer(frameResourceTable, { state.clusterGIVolumeList, Shared::Table_Frame::GIVolumeLists_SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetConstantBuffer(frameResourceTable, { CoreGraphics::GetConstantBuffer(i), Shared::Table_Frame::GIVolumeUniforms_SLOT, 0, sizeof(ProbeFinalize::GIVolumeUniforms), 0 });
        ResourceTableCommitChanges(frameResourceTable);
    }

    FrameScript_default::Bind_ClusterGIList(state.clusterGIVolumeList);
    FrameScript_default::Bind_ClusterGIIndexLists(state.clusterGIVolumeIndexLists);
    FrameScript_default::RegisterSubgraph_GICopy_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CoreGraphics::BufferCopy from, to;
        from.offset = 0;
        to.offset = 0;
        CmdCopy(cmdBuf, state.stagingClusterGIVolumeList.buffers[bufferIndex], { from }, state.clusterGIVolumeList, { to }, sizeof(Shared::GIVolumeLists));
    }, {
        { FrameScript_default::BufferIndex::ClusterGIList, CoreGraphics::PipelineStage::TransferWrite }
    });

    FrameScript_default::RegisterSubgraph_GICull_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, state.volumeCullProgram);
        //CoreGraphics::CmdSetResourceTable(cmdBuf, Raytracing::RaytracingContext::GetLightGridResourceTable(bufferIndex), NEBULA_FRAME_GROUP, CoreGraphics::ComputePipeline, nullptr);

        // Run chunks of 1024 threads at a time
        std::array<SizeT, 3> dimensions = Clustering::ClusterContext::GetClusterDimensions();

        CmdDispatch(cmdBuf, Math::ceil((dimensions[0] * dimensions[1] * dimensions[2]) / 64.0f), 1, 1);
    }, {
        { FrameScript_default::BufferIndex::ClusterGIList, CoreGraphics::PipelineStage::ComputeShaderRead }
        , { FrameScript_default::BufferIndex::ClusterGIIndexLists, CoreGraphics::PipelineStage::ComputeShaderWrite }
        , { FrameScript_default::BufferIndex::ClusterBuffer, CoreGraphics::PipelineStage::ComputeShaderRead }
    });

    FrameScript_default::RegisterSubgraph_DDGIProbeUpdate_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        if (!state.volumesToUpdate.IsEmpty())
        {
            CoreGraphics::CmdSetRayTracingPipeline(cmdBuf, state.pipeline.pipeline);
            CoreGraphics::CmdSetResourceTable(cmdBuf, Raytracing::RaytracingContext::GetRaytracingTable(bufferIndex), NEBULA_BATCH_GROUP, CoreGraphics::RayTracingPipeline, nullptr);
            CoreGraphics::CmdSetResourceTable(cmdBuf, Raytracing::RaytracingContext::GetLightGridResourceTable(bufferIndex), NEBULA_FRAME_GROUP, CoreGraphics::RayTracingPipeline, nullptr);
            for (const UpdateVolume& volumeToUpdate : state.volumesToUpdate)
            {
                if (volumeToUpdate.updateProbesTable != CoreGraphics::InvalidResourceTableId)
                {
                    auto bar = CoreGraphics::TextureBarrierInfo{ .tex = volumeToUpdate.probeUpdateOutputs.radianceDistanceTexture, .subres = CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer() };
                    CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::ComputeShaderRead, CoreGraphics::PipelineStage::RayTracingShaderWrite, CoreGraphics::BarrierDomain::Global, {bar});
                    CoreGraphics::CmdSetResourceTable(cmdBuf, volumeToUpdate.updateProbesTable, NEBULA_SYSTEM_GROUP, CoreGraphics::RayTracingPipeline, nullptr);
                    CoreGraphics::CmdRaysDispatch(cmdBuf, state.pipeline.table, volumeToUpdate.numRays, volumeToUpdate.probeCounts[0] * volumeToUpdate.probeCounts[1] * volumeToUpdate.probeCounts[2], 1);
                }
            }
        }
    }, {
        { FrameScript_default::BufferIndex::GridLightIndexLists, CoreGraphics::PipelineStage::RayTracingShaderRead }
        , { FrameScript_default::BufferIndex::GridLightBuffer, CoreGraphics::PipelineStage::RayTracingShaderRead }
        , { FrameScript_default::BufferIndex::RayTracingObjectBindings, CoreGraphics::PipelineStage::RayTracingShaderRead }
    } );

    FrameScript_default::RegisterSubgraph_DDGIProbeFinalize_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        if (!state.volumesToUpdate.IsEmpty())
        {
            Util::FixedArray<CoreGraphics::TextureBarrierInfo, true> radianceDistanceTextures(state.volumesToUpdate.Size());
            Util::FixedArray<CoreGraphics::TextureBarrierInfo, true> volumeBlendTextures(state.volumesToUpdate.Size() * 3);

            uint it = 0;
            for (const UpdateVolume& volumeToUpdate : state.volumesToUpdate)
            {
                radianceDistanceTextures[it] = CoreGraphics::TextureBarrierInfo{ .tex = volumeToUpdate.probeUpdateOutputs.radianceDistanceTexture, .subres = CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer() };
                volumeBlendTextures[it * 3] = CoreGraphics::TextureBarrierInfo{ .tex =  volumeToUpdate.volumeBlendOutputs.irradianceTexture, .subres = CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer()};
                volumeBlendTextures[it * 3 + 1] = CoreGraphics::TextureBarrierInfo{ .tex =  volumeToUpdate.volumeBlendOutputs.distanceTexture, .subres = CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer()};
                volumeBlendTextures[it * 3 + 2] = CoreGraphics::TextureBarrierInfo{ .tex =  volumeToUpdate.volumeBlendOutputs.scrollSpaceTexture, .subres = CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer()};
            }

            CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::RayTracingShaderWrite, CoreGraphics::PipelineStage::ComputeShaderRead, CoreGraphics::BarrierDomain::Global, radianceDistanceTextures);
            CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::ComputeShaderRead, CoreGraphics::PipelineStage::ComputeShaderWrite, CoreGraphics::BarrierDomain::Global, volumeBlendTextures);

            CoreGraphics::CmdSetShaderProgram(cmdBuf, state.probeBlendRadianceProgram);
            for (const UpdateVolume& volumeToUpdate : state.volumesToUpdate)
            {
                CoreGraphics::CmdSetResourceTable(cmdBuf, volumeToUpdate.blendProbesTable, NEBULA_SYSTEM_GROUP, CoreGraphics::ComputePipeline, nullptr);
                CoreGraphics::CmdDispatch(cmdBuf, volumeToUpdate.probeCounts[1] * volumeToUpdate.probeCounts[2], volumeToUpdate.probeCounts[0], 1);
            }

            CoreGraphics::CmdSetShaderProgram(cmdBuf, state.probeBlendDistanceProgram);
            for (const UpdateVolume& volumeToUpdate : state.volumesToUpdate)
            {
                CoreGraphics::CmdSetResourceTable(cmdBuf, volumeToUpdate.blendProbesTable, NEBULA_SYSTEM_GROUP, CoreGraphics::ComputePipeline, nullptr);
                CoreGraphics::CmdDispatch(cmdBuf, volumeToUpdate.probeCounts[1] * volumeToUpdate.probeCounts[2], volumeToUpdate.probeCounts[0], 1);
            }
            CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::ComputeShaderWrite, CoreGraphics::PipelineStage::ComputeShaderRead, CoreGraphics::BarrierDomain::Global, volumeBlendTextures);

            CoreGraphics::CmdSetShaderProgram(cmdBuf, state.probeBorderRadianceRowsFixup);
            for (const UpdateVolume& volumeToUpdate : state.volumesToUpdate)
            {
                uint probeGridWidth = volumeToUpdate.probeCounts[1] * volumeToUpdate.probeCounts[2];
                uint probeGridHeight = volumeToUpdate.probeCounts[0];
                uint irradianceTextureWidth = probeGridWidth * ProbeFinalize::NUM_IRRADIANCE_TEXELS_PER_PROBE;

                CoreGraphics::CmdSetResourceTable(cmdBuf, volumeToUpdate.blendProbesTable, NEBULA_SYSTEM_GROUP, CoreGraphics::ComputePipeline, nullptr);
                CoreGraphics::CmdDispatch(cmdBuf, irradianceTextureWidth, probeGridHeight, 1);
            }

            CoreGraphics::CmdSetShaderProgram(cmdBuf, state.probeBorderDistanceRowsFixup);
            for (const UpdateVolume& volumeToUpdate : state.volumesToUpdate)
            {
                uint probeGridWidth = volumeToUpdate.probeCounts[1] * volumeToUpdate.probeCounts[2];
                uint probeGridHeight = volumeToUpdate.probeCounts[0];
                uint distanceTextureWidth = probeGridWidth * ProbeFinalize::NUM_DISTANCE_TEXELS_PER_PROBE;

                CoreGraphics::CmdSetResourceTable(cmdBuf, volumeToUpdate.blendProbesTable, NEBULA_SYSTEM_GROUP, CoreGraphics::ComputePipeline, nullptr);
                CoreGraphics::CmdDispatch(cmdBuf, distanceTextureWidth, probeGridHeight, 1);
            }

            CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::ComputeShaderRead, CoreGraphics::PipelineStage::ComputeShaderRead, CoreGraphics::BarrierDomain::Global, volumeBlendTextures);

            CoreGraphics::CmdSetShaderProgram(cmdBuf, state.probeBorderRadianceColumnsFixup);
            for (const UpdateVolume& volumeToUpdate : state.volumesToUpdate)
            {
                uint probeGridWidth = volumeToUpdate.probeCounts[1] * volumeToUpdate.probeCounts[2];
                uint probeGridHeight = volumeToUpdate.probeCounts[0];
                uint irradianceTextureHeight = probeGridHeight * ProbeFinalize::NUM_IRRADIANCE_TEXELS_PER_PROBE;

                CoreGraphics::CmdSetResourceTable(cmdBuf, volumeToUpdate.blendProbesTable, NEBULA_SYSTEM_GROUP, CoreGraphics::ComputePipeline, nullptr);
                CoreGraphics::CmdDispatch(cmdBuf, probeGridWidth * 2, irradianceTextureHeight, 1);
            }

            CoreGraphics::CmdSetShaderProgram(cmdBuf, state.probeBorderDistanceColumnsFixup);
            for (const UpdateVolume& volumeToUpdate : state.volumesToUpdate)
            {
                uint probeGridWidth = volumeToUpdate.probeCounts[1] * volumeToUpdate.probeCounts[2];
                uint probeGridHeight = volumeToUpdate.probeCounts[0];
                uint distanceTextureHeight = probeGridHeight * ProbeFinalize::NUM_DISTANCE_TEXELS_PER_PROBE;

                CoreGraphics::CmdSetResourceTable(cmdBuf, volumeToUpdate.blendProbesTable, NEBULA_SYSTEM_GROUP, CoreGraphics::ComputePipeline, nullptr);
                CoreGraphics::CmdDispatch(cmdBuf, probeGridWidth * 2, distanceTextureHeight, 1);
            }

            CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::ComputeShaderRead, CoreGraphics::PipelineStage::ComputeShaderRead, CoreGraphics::BarrierDomain::Global, volumeBlendTextures);

            for (const UpdateVolume& volumeToUpdate : state.volumesToUpdate)
            {
                if (volumeToUpdate.relocateProbesTable != CoreGraphics::InvalidResourceTableId)
                {
                    CoreGraphics::TextureBarrierInfo bar =
                    {
                        .tex = volumeToUpdate.volumeStateOutputs.offsetsTexture,
                        .subres =  CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer()
                    };
                    CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::ComputeShaderRead, CoreGraphics::PipelineStage::ComputeShaderWrite, CoreGraphics::BarrierDomain::Global, { bar });

                    CoreGraphics::CmdSetShaderProgram(cmdBuf, state.probesRelocateProgram);
                    CoreGraphics::CmdSetResourceTable(cmdBuf, volumeToUpdate.relocateProbesTable, NEBULA_SYSTEM_GROUP, CoreGraphics::ComputePipeline, nullptr);
                    CoreGraphics::CmdDispatch(cmdBuf, Math::divandroundup(volumeToUpdate.probeCounts[1] * volumeToUpdate.probeCounts[2], 8), Math::divandroundup(volumeToUpdate.probeCounts[0], 4), 1);
                    CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::ComputeShaderWrite, CoreGraphics::PipelineStage::ComputeShaderRead, CoreGraphics::BarrierDomain::Global, { bar });
                }

                if (volumeToUpdate.classifyProbesTable != CoreGraphics::InvalidResourceTableId)
                {
                    CoreGraphics::TextureBarrierInfo bar =
                    {
                        .tex = volumeToUpdate.volumeStateOutputs.statesTexture,
                        .subres =  CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer()
                    };
                    CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::ComputeShaderRead, CoreGraphics::PipelineStage::ComputeShaderWrite, CoreGraphics::BarrierDomain::Global, { bar });
                    CoreGraphics::CmdSetShaderProgram(cmdBuf, state.probesClassifyProgram);
                    CoreGraphics::CmdSetResourceTable(cmdBuf, volumeToUpdate.classifyProbesTable, NEBULA_SYSTEM_GROUP, CoreGraphics::ComputePipeline, nullptr);
                    CoreGraphics::CmdDispatch(cmdBuf, Math::divandroundup(volumeToUpdate.probeCounts[1] * volumeToUpdate.probeCounts[2], 8), Math::divandroundup(volumeToUpdate.probeCounts[0], 4), 1);
                    CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::ComputeShaderWrite, CoreGraphics::PipelineStage::ComputeShaderRead, CoreGraphics::BarrierDomain::Global, { bar });
                }
            }
        }
    });

#ifndef PUBLIC_BUILD
    FrameScript_default::RegisterSubgraphPipelines_DDGIDebug_Pass([](const CoreGraphics::PassId pass, const uint subpass)
    {
        state.debugPipeline = CoreGraphics::CreateGraphicsPipeline(
            {
                .shader = state.debugProgram,
                .pass = pass,
                .subpass = subpass,
                .inputAssembly = CoreGraphics::InputAssemblyKey{ { .topo = CoreGraphics::PrimitiveTopology::TriangleList, .primRestart = false } }
            });
    });

    FrameScript_default::RegisterSubgraph_DDGIDebug_Pass([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        if (Core::CVarReadInt(g_debug_ddgi) == 1)
        {
            CoreGraphics::MeshBind(state.debugMesh, cmdBuf);
            CoreGraphics::CmdSetGraphicsPipeline(cmdBuf, state.debugPipeline);

            for (const UpdateVolume& volumeToDraw : state.volumesToDraw)
            {
                CoreGraphics::CmdSetResourceTable(cmdBuf, volumeToDraw.debugTable, NEBULA_SYSTEM_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
                const CoreGraphics::PrimitiveGroup& prim = CoreGraphics::MeshGetPrimitiveGroup(state.debugMesh, 0);
                CoreGraphics::CmdDraw(cmdBuf, volumeToDraw.probeCounts[0] * volumeToDraw.probeCounts[1] * volumeToDraw.probeCounts[2], prim);
            }
        }
    });
#endif


}

//------------------------------------------------------------------------------
/**
*/
Math::vec4
SphericalFibonacci(float index, float numSamples)
{
    const float b = (sqrt(5.0f) * 0.5f + 0.5f) - 1.0f;
    float phi = 2 * PI * Math::fract(index * b);
    float cosTheta = 1.0f - (2.0f * index + 1.0f) * (1.0f / numSamples);
    float sinTheta = sqrt(Math::clamp(1.0f - (cosTheta * cosTheta), 0.0f, 1.0f));
    return Math::vec4(Math::normalize(Math::vec3((cos(phi) * sinTheta), (sin(phi) * sinTheta), cosTheta)), 0);
}

//------------------------------------------------------------------------------
/**
*/
void
DDGIContext::Discard()
{
}

//------------------------------------------------------------------------------
/**
 *
*/
void
DDGIContext::SetupVolume(const Graphics::GraphicsEntityId id, const VolumeSetup& setup)
{
    n_assert_msg(setup.numRaysPerProbe < 1024, "Maximum allowed number of rays per probe is 1024");

    ContextEntityId ctxId = GetContextId(id);
    Volume& volume = ddgiVolumeAllocator.Get<0>(ctxId.id);
    volume.boundingBox.set(setup.position, setup.size * Math::vec3(0.5f));
    volume.position = setup.position;
    volume.size = setup.size;
    volume.numProbesX = setup.numProbesX;
    volume.numProbesY = setup.numProbesY;
    volume.numProbesZ = setup.numProbesZ;
    volume.numRaysPerProbe = setup.numRaysPerProbe;
    volume.options = setup.options;

    //volume.options.flags.lowPrecisionTextures = true;

    volume.normalBias = setup.normalBias;
    volume.viewBias = setup.viewBias;
    volume.irradianceScale = setup.irradianceScale;
    volume.distanceExponent = setup.distanceExponent;
    volume.encodingGamma = setup.encodingGamma;
    volume.changeThreshold = setup.changeThreshold;
    volume.brightnessThreshold = setup.brightnessThreshold;
    volume.hysteresis = setup.hysteresis;
    volume.blend = setup.blend;
    volume.blendCutoff = setup.blendCutoff;

#if NEBULA_GRAPHICS_DEBUG
    Util::String volumeName = Util::String::Sprintf("DDGIVolume_%d", ctxId.id);
#endif

    CoreGraphics::TextureCreateInfo radianceCreateInfo;
#if NEBULA_GRAPHICS_DEBUG
    radianceCreateInfo.name = Util::String::Sprintf("%s Radiance", volumeName.c_str());
#endif
    radianceCreateInfo.width = setup.numRaysPerProbe;
    radianceCreateInfo.height = setup.numProbesX * setup.numProbesY * setup.numProbesZ;
    radianceCreateInfo.format = CoreGraphics::PixelFormat::R32G32B32A32F;
    radianceCreateInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture;

    volume.radiance = CoreGraphics::CreateTexture(radianceCreateInfo);

    CoreGraphics::TextureCreateInfo irradianceCreateInfo;
#if NEBULA_GRAPHICS_DEBUG
    irradianceCreateInfo.name = Util::String::Sprintf("%s Irradiance", volumeName.c_str());
#endif
    irradianceCreateInfo.width = setup.numProbesY * setup.numProbesZ * (ProbeUpdate::NUM_IRRADIANCE_TEXELS_PER_PROBE + 2);
    irradianceCreateInfo.height = setup.numProbesX * (ProbeUpdate::NUM_IRRADIANCE_TEXELS_PER_PROBE + 2);
    irradianceCreateInfo.format = CoreGraphics::PixelFormat::R32G32B32A32F;
    irradianceCreateInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture;
    volume.irradiance = CoreGraphics::CreateTexture(irradianceCreateInfo);

    CoreGraphics::TextureCreateInfo distanceCreateInfo;
#if NEBULA_GRAPHICS_DEBUG
    distanceCreateInfo.name = Util::String::Sprintf("%s Distance", volumeName.c_str());
#endif
    distanceCreateInfo.width = setup.numProbesY * setup.numProbesZ * (ProbeUpdate::NUM_DISTANCE_TEXELS_PER_PROBE + 2);
    distanceCreateInfo.height = setup.numProbesX * (ProbeUpdate::NUM_DISTANCE_TEXELS_PER_PROBE + 2);
    distanceCreateInfo.format = CoreGraphics::PixelFormat::R16G16F;
    distanceCreateInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture;
    volume.distance = CoreGraphics::CreateTexture(distanceCreateInfo);

    CoreGraphics::TextureCreateInfo offsetCreateInfo;
#if NEBULA_GRAPHICS_DEBUG
    offsetCreateInfo.name = Util::String::Sprintf("%s Offsets", volumeName.c_str());
#endif
    offsetCreateInfo.width = setup.numProbesY * setup.numProbesZ;
    offsetCreateInfo.height = setup.numProbesX;
    offsetCreateInfo.format = CoreGraphics::PixelFormat::R16G16B16A16F;
    offsetCreateInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture;
    volume.offsets = CoreGraphics::CreateTexture(offsetCreateInfo);

    CoreGraphics::TextureCreateInfo statesCreateInfo;
#if NEBULA_GRAPHICS_DEBUG
    statesCreateInfo.name = Util::String::Sprintf("%s States", volumeName.c_str());
#endif
    statesCreateInfo.width = setup.numProbesY * setup.numProbesZ;
    statesCreateInfo.height = setup.numProbesX;
    statesCreateInfo.format = CoreGraphics::PixelFormat::R8;
    statesCreateInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture;
    volume.states = CoreGraphics::CreateTexture(statesCreateInfo);

    CoreGraphics::TextureCreateInfo scrollSpaceCreateInfo;
#if NEBULA_GRAPHICS_DEBUG
    scrollSpaceCreateInfo.name = Util::String::Sprintf("%s ScrollSpace", volumeName.c_str());
#endif
    scrollSpaceCreateInfo.width = setup.numProbesY * setup.numProbesZ;
    scrollSpaceCreateInfo.height = setup.numProbesX;
    scrollSpaceCreateInfo.format = CoreGraphics::PixelFormat::R8;
    scrollSpaceCreateInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture;
    volume.scrollSpace = CoreGraphics::CreateTexture(scrollSpaceCreateInfo);

    for (uint rayIndex = 0; rayIndex < setup.numRaysPerProbe; rayIndex++)
    {
        SphericalFibonacci(rayIndex, setup.numRaysPerProbe).store(volume.volumeConstants.Directions[rayIndex]);
    }

    // Store another set of minimal ray directions for probe activity updates
    for (uint rayIndex = 0; rayIndex < ProbeUpdate::DDGI_NUM_FIXED_RAYS; rayIndex++)
    {
        SphericalFibonacci(rayIndex, ProbeUpdate::DDGI_NUM_FIXED_RAYS).store(volume.volumeConstants.MinimalDirections[rayIndex]);
    }

    // Store another set of minimal ray directions for probe activity updates
    for (uint rayIndex = 0; rayIndex < setup.numRaysPerProbe - ProbeUpdate::DDGI_NUM_FIXED_RAYS; rayIndex++)
    {
        SphericalFibonacci(rayIndex, setup.numRaysPerProbe - ProbeUpdate::DDGI_NUM_FIXED_RAYS).store(volume.volumeConstants.ExtraDirections[rayIndex]);
    }

    volume.volumeConstants.ProbeIrradiance = CoreGraphics::TextureGetBindlessHandle(volume.irradiance);
    volume.volumeConstants.ProbeDistances = CoreGraphics::TextureGetBindlessHandle(volume.distance);
    volume.volumeConstants.ProbeOffsets = CoreGraphics::TextureGetBindlessHandle(volume.offsets);
    volume.volumeConstants.ProbeStates = CoreGraphics::TextureGetBindlessHandle(volume.states);
    volume.volumeConstants.ProbeScrollSpace = CoreGraphics::TextureGetBindlessHandle(volume.scrollSpace);
    volume.volumeConstants.ProbeRadiance = CoreGraphics::TextureGetBindlessHandle(volume.radiance);

    CoreGraphics::BufferCreateInfo probeVolumeUpdateBufferInfo;
#if NEBULA_GRAPHICS_DEBUG
    probeVolumeUpdateBufferInfo.name = Util::String::Sprintf("%s Volume Update Constants", volumeName.c_str());
#endif
    probeVolumeUpdateBufferInfo.byteSize = sizeof(ProbeUpdate::VolumeConstants);
    probeVolumeUpdateBufferInfo.usageFlags = CoreGraphics::BufferUsageFlag::ConstantBuffer;
    probeVolumeUpdateBufferInfo.mode = CoreGraphics::BufferAccessMode::DeviceAndHost;
    volume.volumeConstantBuffer = CoreGraphics::CreateBuffer(probeVolumeUpdateBufferInfo);

    volume.updateProbesTable = CoreGraphics::ShaderCreateResourceTable(state.probeUpdateShader, NEBULA_SYSTEM_GROUP, 1);
    CoreGraphics::ResourceTableSetRWTexture(volume.updateProbesTable, CoreGraphics::ResourceTableTexture(volume.radiance, ProbeUpdate::Table_System::RadianceOutput_SLOT));
    CoreGraphics::ResourceTableSetConstantBuffer(volume.updateProbesTable, CoreGraphics::ResourceTableBuffer(volume.volumeConstantBuffer, ProbeUpdate::Table_System::VolumeConstants_SLOT));
    CoreGraphics::ResourceTableCommitChanges(volume.updateProbesTable);

    volume.blendProbesTable = CoreGraphics::ShaderCreateResourceTable(state.probeFinalizeShader, NEBULA_SYSTEM_GROUP, 1);
    CoreGraphics::ResourceTableSetConstantBuffer(volume.blendProbesTable, CoreGraphics::ResourceTableBuffer(volume.volumeConstantBuffer, ProbeFinalize::Table_System::VolumeConstants_SLOT));
    CoreGraphics::ResourceTableSetRWTexture(volume.blendProbesTable, CoreGraphics::ResourceTableTexture(volume.irradiance, ProbeFinalize::Table_System::IrradianceOutput_SLOT));
    CoreGraphics::ResourceTableSetRWTexture(volume.blendProbesTable, CoreGraphics::ResourceTableTexture(volume.distance, ProbeFinalize::Table_System::DistanceOutput_SLOT));
    CoreGraphics::ResourceTableSetRWTexture(volume.blendProbesTable, CoreGraphics::ResourceTableTexture(volume.scrollSpace, ProbeFinalize::Table_System::ScrollSpaceOutput_SLOT));
    CoreGraphics::ResourceTableCommitChanges(volume.blendProbesTable);

    volume.relocateProbesTable = CoreGraphics::ShaderCreateResourceTable(state.probesRelocateShader, NEBULA_SYSTEM_GROUP, 1);
    CoreGraphics::ResourceTableSetConstantBuffer(volume.relocateProbesTable, CoreGraphics::ResourceTableBuffer(volume.volumeConstantBuffer, ProbeRelocate::Table_System::VolumeConstants_SLOT));
    CoreGraphics::ResourceTableSetRWTexture(volume.relocateProbesTable, CoreGraphics::ResourceTableTexture(volume.offsets, ProbeRelocate::Table_System::ProbeOffsetsOutput_SLOT));
    CoreGraphics::ResourceTableCommitChanges(volume.relocateProbesTable);

    volume.classifyProbesTable = CoreGraphics::ShaderCreateResourceTable(state.probesClassifyShader, NEBULA_SYSTEM_GROUP, 1);
    CoreGraphics::ResourceTableSetConstantBuffer(volume.classifyProbesTable, CoreGraphics::ResourceTableBuffer(volume.volumeConstantBuffer, ProbeClassify::Table_System::VolumeConstants_SLOT));
    CoreGraphics::ResourceTableSetRWTexture(volume.classifyProbesTable, CoreGraphics::ResourceTableTexture(volume.states, ProbeClassify::Table_System::ProbeStateOutput_SLOT));
    CoreGraphics::ResourceTableCommitChanges(volume.classifyProbesTable);

#ifndef PUBLIC_BUILD
    volume.debugResourceTable = CoreGraphics::ShaderCreateResourceTable(state.debugShader, NEBULA_SYSTEM_GROUP, 1);
    CoreGraphics::ResourceTableSetConstantBuffer(volume.debugResourceTable, CoreGraphics::ResourceTableBuffer(volume.volumeConstantBuffer, ProbeDebug::Table_System::VolumeConstants_SLOT));
    CoreGraphics::ResourceTableCommitChanges(volume.debugResourceTable);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
DDGIContext::SetPosition(const Graphics::GraphicsEntityId id, const Math::vec3& position)
{
    ContextEntityId ctxId = GetContextId(id);
    Volume& volume = ddgiVolumeAllocator.Get<0>(ctxId.id);
    volume.position = position;
    volume.boundingBox.set(volume.position, volume.size * 0.5f);
}

//------------------------------------------------------------------------------
/**
*/
void
DDGIContext::SetSize(const Graphics::GraphicsEntityId id, const Math::vec3& size)
{
    ContextEntityId ctxId = GetContextId(id);
    Volume& volume = ddgiVolumeAllocator.Get<0>(ctxId.id);
    volume.size = size;
    volume.boundingBox.set(volume.position, volume.size * 0.5f);
}

//------------------------------------------------------------------------------
/**
*/
void
DDGIContext::UpdateActiveVolumes(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
    const Math::point cameraPos = CameraContext::GetTransform(view->GetCamera()).position;
    const Util::Array<Volume>& volumes = ddgiVolumeAllocator.GetArray<0>();
    Math::mat4 viewTransform = Graphics::CameraContext::GetView(view->GetCamera());

    state.volumesToUpdate.Clear();
    state.volumesToDraw.Clear();
    uint volumeCount = 0;
    for (Volume& activeVolume : volumes)
    {
        UpdateVolume volumeToUpdate;

        volumeToUpdate.probeCounts[0] = activeVolume.numProbesX;
        volumeToUpdate.probeCounts[1] = activeVolume.numProbesY;
        volumeToUpdate.probeCounts[2] = activeVolume.numProbesZ;
        volumeToUpdate.numRays = activeVolume.numRaysPerProbe;
        volumeToUpdate.probeUpdateOutputs.radianceDistanceTexture = activeVolume.radiance;
        volumeToUpdate.volumeBlendOutputs.irradianceTexture = activeVolume.irradiance;
        volumeToUpdate.volumeBlendOutputs.distanceTexture = activeVolume.distance;
        volumeToUpdate.volumeBlendOutputs.scrollSpaceTexture = activeVolume.scrollSpace;
        volumeToUpdate.volumeStateOutputs.offsetsTexture = activeVolume.offsets;
        volumeToUpdate.volumeStateOutputs.statesTexture = activeVolume.states;
        volumeToUpdate.updateProbesTable = CoreGraphics::InvalidResourceTableId;
        volumeToUpdate.blendProbesTable = CoreGraphics::InvalidResourceTableId;
        volumeToUpdate.relocateProbesTable = CoreGraphics::InvalidResourceTableId;
        volumeToUpdate.classifyProbesTable = CoreGraphics::InvalidResourceTableId;

        Math::vec3 size = activeVolume.size;
        Math::mat4 rotation = Math::rotationyawpitchroll(Math::sin(ctx.frameIndex / 5.0f), 0.0f, 0.0f);
        rotation.store(activeVolume.volumeConstants.TemporalRotation);
        size.store(activeVolume.volumeConstants.Scale);
        activeVolume.position.store(activeVolume.volumeConstants.Offset);
        activeVolume.volumeConstants.NumIrradianceTexels = ProbeUpdate::NUM_IRRADIANCE_TEXELS_PER_PROBE;
        activeVolume.volumeConstants.ProbeGridDimensions[0] = activeVolume.numProbesX;
        activeVolume.volumeConstants.ProbeGridDimensions[1] = activeVolume.numProbesY;
        activeVolume.volumeConstants.ProbeGridDimensions[2] = activeVolume.numProbesZ;
        activeVolume.volumeConstants.Options = 0;
        activeVolume.volumeConstants.Options |= activeVolume.options.flags.relocate ? ProbeUpdate::RELOCATION_OPTION : 0x0;
        activeVolume.volumeConstants.Options |= activeVolume.options.flags.scrolling ? ProbeUpdate::SCROLL_OPTION : 0x0;
        activeVolume.volumeConstants.Options |= activeVolume.options.flags.classify ? ProbeUpdate::CLASSIFICATION_OPTION : 0x0;
        activeVolume.volumeConstants.Options |= activeVolume.options.flags.lowPrecisionTextures ? ProbeUpdate::LOW_PRECISION_IRRADIANCE_OPTION : 0x0;
        activeVolume.volumeConstants.Options |= activeVolume.options.flags.partialUpdate ? ProbeUpdate::PARTIAL_UPDATE_OPTION : 0x0;
        activeVolume.volumeConstants.ProbeIndexStart = 0;
        activeVolume.volumeConstants.ProbeIndexCount = volumeToUpdate.probeCounts[0] * volumeToUpdate.probeCounts[1] * volumeToUpdate.probeCounts[2];
        activeVolume.volumeConstants.ProbeScrollOffsets[0] = 0;
        activeVolume.volumeConstants.ProbeScrollOffsets[1] = 0;
        activeVolume.volumeConstants.ProbeScrollOffsets[2] = 0;
        Math::quat().store(activeVolume.volumeConstants.Rotation);
        activeVolume.volumeConstants.ProbeGridSpacing[0] = size[0] / float(activeVolume.numProbesX);
        activeVolume.volumeConstants.ProbeGridSpacing[1] = size[1] / float(activeVolume.numProbesY);
        activeVolume.volumeConstants.ProbeGridSpacing[2] = size[2] / float(activeVolume.numProbesZ);
        activeVolume.volumeConstants.NumDistanceTexels = ProbeUpdate::NUM_DISTANCE_TEXELS_PER_PROBE;
        activeVolume.volumeConstants.IrradianceGamma = activeVolume.encodingGamma;
        activeVolume.volumeConstants.NormalBias = activeVolume.normalBias;
        activeVolume.volumeConstants.ViewBias = activeVolume.viewBias;
        activeVolume.volumeConstants.IrradianceScale = activeVolume.irradianceScale;

        activeVolume.volumeConstants.BackfaceThreshold = activeVolume.backfaceThreshold;
        activeVolume.volumeConstants.MinFrontfaceDistance = activeVolume.minFrontfaceDistance;
        activeVolume.volumeConstants.ProbeDistanceScale = 1.0f;

        activeVolume.volumeConstants.RaysPerProbe = activeVolume.numRaysPerProbe;
        activeVolume.volumeConstants.InverseGammaEncoding = 1.0f / activeVolume.volumeConstants.IrradianceGamma;
        activeVolume.volumeConstants.Hysteresis = activeVolume.hysteresis;
        activeVolume.volumeConstants.NormalBias = activeVolume.normalBias;
        activeVolume.volumeConstants.ViewBias = activeVolume.viewBias;

        activeVolume.volumeConstants.IrradianceScale = activeVolume.irradianceScale;
        activeVolume.volumeConstants.DistanceExponent = activeVolume.distanceExponent;
        activeVolume.volumeConstants.ChangeThreshold = activeVolume.changeThreshold;
        activeVolume.volumeConstants.BrightnessThreshold = activeVolume.brightnessThreshold;

        CoreGraphics::BufferUpdate(activeVolume.volumeConstantBuffer, activeVolume.volumeConstants);


        // Only update the tables if the volume is within update range
        if (activeVolume.boundingBox.contains(xyz(cameraPos)))
        {
            volumeToUpdate.updateProbesTable = activeVolume.updateProbesTable;
            volumeToUpdate.blendProbesTable = activeVolume.blendProbesTable;
            if (activeVolume.options.flags.relocate)
                volumeToUpdate.relocateProbesTable = activeVolume.relocateProbesTable;
            if (activeVolume.options.flags.classify)
                volumeToUpdate.classifyProbesTable = activeVolume.classifyProbesTable;
            state.volumesToUpdate.Append(volumeToUpdate);
        }

#ifndef PUBLIC_BUILD
        volumeToUpdate.debugTable = activeVolume.debugResourceTable;
        state.volumesToDraw.Append(volumeToUpdate);
#endif

        auto& giVolume = state.giVolumes[volumeCount];

        Math::mat4 transform = Math::scaling(activeVolume.size * 1.5f);
        transform.translate(activeVolume.position);
        Math::bbox bbox = viewTransform * transform;
        bbox.pmin.store(giVolume.bboxMin);
        bbox.pmax.store(giVolume.bboxMax);
        Math::quat().store(giVolume.Rotation);
        activeVolume.position.store(giVolume.Offset);
        giVolume.NumDistanceTexels = ProbeUpdate::NUM_DISTANCE_TEXELS_PER_PROBE;
        giVolume.NumIrradianceTexels = ProbeUpdate::NUM_IRRADIANCE_TEXELS_PER_PROBE;
        giVolume.EncodingGamma = activeVolume.volumeConstants.IrradianceGamma;
        giVolume.IrradianceScale = activeVolume.volumeConstants.IrradianceScale;
        memcpy(giVolume.GridCounts, activeVolume.volumeConstants.ProbeGridDimensions, sizeof(activeVolume.volumeConstants.ProbeGridDimensions));
        memcpy(giVolume.ScrollOffsets, activeVolume.volumeConstants.ProbeScrollOffsets, sizeof(activeVolume.volumeConstants.ProbeScrollOffsets));
        memcpy(giVolume.GridSpacing, activeVolume.volumeConstants.ProbeGridSpacing, sizeof(activeVolume.volumeConstants.ProbeGridSpacing));
        activeVolume.size.store(giVolume.Size);

        giVolume.BlendCutoff = activeVolume.blendCutoff;
        giVolume.Blend = activeVolume.blend;
        giVolume.ViewBias = activeVolume.viewBias;
        giVolume.NormalBias = activeVolume.normalBias;
        giVolume.Options = activeVolume.volumeConstants.Options;
        giVolume.Distances = activeVolume.volumeConstants.ProbeDistances;
        giVolume.Irradiance = activeVolume.volumeConstants.ProbeIrradiance;
        giVolume.States = activeVolume.volumeConstants.ProbeStates;
        giVolume.Offsets = activeVolume.volumeConstants.ProbeOffsets;

        volumeCount++;
    }

    // Update shared GI data
    Shared::GIVolumeUniforms giVolumeUniforms;
    giVolumeUniforms.NumGIVolumes = volumeCount;
    giVolumeUniforms.NumGIVolumeClusters = Clustering::ClusterContext::GetNumClusters();

    IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

    CoreGraphics::ResourceTableId frameResourceTable = Graphics::GetFrameResourceTable(bufferIndex);

    uint offset = CoreGraphics::SetConstants(giVolumeUniforms);
    ResourceTableSetConstantBuffer(frameResourceTable, { CoreGraphics::GetConstantBuffer(bufferIndex), Shared::Table_Frame::GIVolumeUniforms_SLOT, 0, sizeof(Shared::GIVolumeUniforms), (SizeT)offset });
    ResourceTableCommitChanges(frameResourceTable);

    if (volumeCount > 0)
    {
        GiVolumeCull::GIVolumeLists volumeList;
        Memory::CopyElements(state.giVolumes, volumeList.GIVolumes, volumeCount);
        CoreGraphics::BufferUpdate(state.stagingClusterGIVolumeList.buffers[bufferIndex], volumeList);
        CoreGraphics::BufferFlush(state.stagingClusterGIVolumeList.buffers[bufferIndex]);
    }

    CoreGraphics::ResourceTableSetRWBuffer(state.raytracingTable.tables[ctx.bufferIndex], CoreGraphics::ResourceTableBuffer(Raytracing::RaytracingContext::GetObjectBindingBuffer(), ProbeUpdate::Table_Batch::ObjectBuffer_SLOT));
    CoreGraphics::ResourceTableSetAccelerationStructure(state.raytracingTable.tables[ctx.bufferIndex], CoreGraphics::ResourceTableTlas(Raytracing::RaytracingContext::GetTLAS(), ProbeUpdate::Table_Batch::TLAS_SLOT));
    CoreGraphics::ResourceTableCommitChanges(state.raytracingTable.tables[ctx.bufferIndex]);
}

#ifndef PUBLIC_BUILD
//------------------------------------------------------------------------------
/**
*/
void
DDGIContext::OnRenderDebug(uint32_t flags)
{
    CoreGraphics::ShapeRenderer* shapeRenderer = CoreGraphics::ShapeRenderer::Instance();

    const Util::Array<Volume>& volumes = ddgiVolumeAllocator.GetArray<0>();
    for (const Volume& volume : volumes)
    {
        CoreGraphics::RenderShape debugShape;
        Math::mat4 transform;
        transform.scale(volume.size);
        transform.translate(volume.position);

        debugShape.SetupSimpleShape(CoreGraphics::RenderShape::Box, CoreGraphics::RenderShape::RenderFlag::CheckDepth, Math::vec4(0, 1, 0, 1), transform);
        shapeRenderer->AddShape(debugShape);
    }
}
#endif

//------------------------------------------------------------------------------
/**
*/
Graphics::ContextEntityId
DDGIContext::Alloc()
{
    return ddgiVolumeAllocator.Alloc();
}

//------------------------------------------------------------------------------
/**
*/
void
DDGIContext::Dealloc(Graphics::ContextEntityId id)
{
    Volume& volume = ddgiVolumeAllocator.Get<0>(id.id);
    CoreGraphics::DestroyTexture(volume.radiance);
    CoreGraphics::DestroyTexture(volume.irradiance);
    CoreGraphics::DestroyTexture(volume.distance);
    CoreGraphics::DestroyTexture(volume.offsets);
    CoreGraphics::DestroyTexture(volume.states);
    CoreGraphics::DestroyTexture(volume.scrollSpace);
    CoreGraphics::DestroyBuffer(volume.volumeConstantBuffer);
    CoreGraphics::DestroyResourceTable(volume.updateProbesTable);
    CoreGraphics::DestroyResourceTable(volume.blendProbesTable);
    ddgiVolumeAllocator.Dealloc(id.id);
}

} // namespace GI
