//------------------------------------------------------------------------------
//  @file ddgicontext.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "coregraphics/pipeline.h"
#include "ddgicontext.h"

#include <cstdint>

#include "frame/default.h"
#include "graphics/cameracontext.h"
#include "graphics/view.h"
#include "lighting/lightcontext.h"
#include "raytracing/raytracingcontext.h"
#include "coregraphics/shaperenderer.h"


#include "gi/shaders/probeupdate.h"
#include "gi/shaders/probefinalize.h"

using namespace Graphics;

namespace GI
{
DDGIContext::DDGIVolumeAllocator DDGIContext::ddgiVolumeAllocator;
__ImplementContext(DDGIContext, DDGIContext::ddgiVolumeAllocator)

struct UpdateVolume
{
    uint probeCounts[3];
    uint numRays;
    CoreGraphics::ResourceTableId updateProbesTable, finalizeProbesTable;
};

struct
{
    CoreGraphics::ShaderId probeUpdateShader;
    CoreGraphics::ShaderProgramId probeUpdateProgram;
    CoreGraphics::PipelineRayTracingTable pipeline;

    CoreGraphics::ShaderId probeFinalizeShader;
    CoreGraphics::ShaderProgramId probeFinalizeRadianceProgram, probeFinalizeDistanceProgram;

    CoreGraphics::ResourceTableSet raytracingTable;

    Util::Array<UpdateVolume> volumesToUpdate;

    Math::vec3 probeDirections188[188];
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
#endif

    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    state.probeUpdateShader = CoreGraphics::ShaderGet("shd:gi/shaders/probeupdate.fxb");
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

    state.probeFinalizeShader = CoreGraphics::ShaderGet("shd:gi/shaders/probefinalize.fxb");
    state.probeFinalizeRadianceProgram = CoreGraphics::ShaderGetProgram(state.probeFinalizeShader, CoreGraphics::ShaderFeatureMask("ProbeFinalizeRadiance"));
    state.probeFinalizeDistanceProgram = CoreGraphics::ShaderGetProgram(state.probeFinalizeShader, CoreGraphics::ShaderFeatureMask("ProbeFinalizeDistance"));

    FrameScript_default::RegisterSubgraph_DDGIProbeUpdate_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        if (!state.volumesToUpdate.IsEmpty())
        {
            CoreGraphics::CmdSetRayTracingPipeline(cmdBuf, state.pipeline.pipeline);
            CoreGraphics::CmdSetResourceTable(cmdBuf, Raytracing::RaytracingContext::GetRaytracingTable(bufferIndex), NEBULA_BATCH_GROUP, CoreGraphics::RayTracingPipeline, nullptr);
            CoreGraphics::CmdSetResourceTable(cmdBuf, Raytracing::RaytracingContext::GetLightGridResourceTable(), NEBULA_FRAME_GROUP, CoreGraphics::RayTracingPipeline, nullptr);
            for (const UpdateVolume& volumeToUpdate : state.volumesToUpdate)
            {
                CoreGraphics::CmdSetResourceTable(cmdBuf, volumeToUpdate.updateProbesTable, NEBULA_SYSTEM_GROUP, CoreGraphics::RayTracingPipeline, nullptr);
                CoreGraphics::CmdRaysDispatch(cmdBuf, state.pipeline.table, volumeToUpdate.probeCounts[0] * volumeToUpdate.probeCounts[1] * volumeToUpdate.probeCounts[2], volumeToUpdate.numRays, 1);
            }
        }
    }, {
        { FrameScript_default::BufferIndex::GridLightIndexLists, CoreGraphics::PipelineStage::RayTracingShaderRead }
        , { FrameScript_default::BufferIndex::RayTracingObjectBindings, CoreGraphics::PipelineStage::RayTracingShaderRead }
    } );

    FrameScript_default::RegisterSubgraph_DDGIProbeFinalize_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        if (!state.volumesToUpdate.IsEmpty())
        {
            CoreGraphics::CmdSetShaderProgram(cmdBuf, state.probeFinalizeRadianceProgram);
            for (const UpdateVolume& volumeToUpdate : state.volumesToUpdate)
            {
                CoreGraphics::CmdSetResourceTable(cmdBuf, volumeToUpdate.finalizeProbesTable, NEBULA_SYSTEM_GROUP, CoreGraphics::ComputePipeline, nullptr);
                CoreGraphics::CmdDispatch(cmdBuf, volumeToUpdate.probeCounts[1] * volumeToUpdate.probeCounts[2], volumeToUpdate.probeCounts[0], 1);
            }
        }
    });

    FrameScript_default::RegisterSubgraph_DDGIDebug_Pass([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {

    });


}

//------------------------------------------------------------------------------
/**
*/
Math::vec3
SphericalFibonacci(float index, float numSamples)
{
    const float b = (sqrt(5.0f) * 0.5f + 0.5f) - 1.0f;
    float phi = 2 * PI * Math::fract(index * b);
    float cosTheta = 1.0f - (2.0f * index + 1.0f) * (1.0f / numSamples);
    float sinTheta = sqrt(Math::clamp(1.0f - (cosTheta * cosTheta), 0.0f, 1.0f));
    return Math::normalize(Math::vec3((cos(phi) * sinTheta), (sin(phi) * sinTheta), cosTheta));
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
*/
void
DDGIContext::SetupVolume(const Graphics::GraphicsEntityId id, const VolumeSetup& setup)
{
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

#if NEBULA_GRAPHICS_DEBUG
    Util::String volumeName = Util::String::Sprintf("DDGIVolume_%d", ctxId.id);
#endif

    CoreGraphics::TextureCreateInfo radianceCreateInfo;
#if NEBULA_GRAPHICS_DEBUG
    radianceCreateInfo.name = Util::String::Sprintf("%s Radiance", volumeName.c_str());
#endif
    radianceCreateInfo.width = setup.numRaysPerProbe;
    radianceCreateInfo.height = setup.numProbesX * setup.numProbesY * setup.numProbesZ;
    radianceCreateInfo.format = CoreGraphics::PixelFormat::R32G32F;
    radianceCreateInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture;
    volume.radiance = CoreGraphics::CreateTexture(radianceCreateInfo);

    CoreGraphics::TextureCreateInfo irradianceCreateInfo;
#if NEBULA_GRAPHICS_DEBUG
    irradianceCreateInfo.name = Util::String::Sprintf("%s Irradiance", volumeName.c_str());
#endif
    irradianceCreateInfo.width = setup.numProbesY * setup.numProbesZ * (Probeupdate::NUM_IRRADIANCE_TEXELS_PER_PROBE + 2);
    irradianceCreateInfo.height = setup.numProbesX * (Probeupdate::NUM_IRRADIANCE_TEXELS_PER_PROBE + 2);
    irradianceCreateInfo.format = CoreGraphics::PixelFormat::R11G11B10F;
    irradianceCreateInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture;
    volume.irradiance = CoreGraphics::CreateTexture(irradianceCreateInfo);

    CoreGraphics::TextureCreateInfo distanceCreateInfo;
#if NEBULA_GRAPHICS_DEBUG
    distanceCreateInfo.name = Util::String::Sprintf("%s Distance", volumeName.c_str());
#endif
    distanceCreateInfo.width = setup.numProbesY * setup.numProbesZ * (Probeupdate::NUM_DISTANCE_TEXELS_PER_PROBE + 2);
    distanceCreateInfo.height = setup.numProbesX * (Probeupdate::NUM_DISTANCE_TEXELS_PER_PROBE + 2);
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
    volume.scrollSpace = CoreGraphics::CreateTexture(statesCreateInfo);

    n_assert(setup.numRaysPerProbe < 1024);
    for (uint rayIndex = 0; rayIndex < setup.numRaysPerProbe; rayIndex++)
    {
        SphericalFibonacci(rayIndex, setup.numRaysPerProbe).store(volume.constants.Directions[rayIndex]);
    }

    // Store another set of minimal ray directions for probe activity updates
    for (uint rayIndex = 0; rayIndex < Probeupdate::NUM_FIXED_RAYS; rayIndex++)
    {
        SphericalFibonacci(rayIndex, Probeupdate::NUM_FIXED_RAYS).store(volume.constants.MinimalDirections[rayIndex]);
    }

    volume.constants.ProbeIrradiance = CoreGraphics::TextureGetBindlessHandle(volume.irradiance);
    volume.constants.ProbeDistances = CoreGraphics::TextureGetBindlessHandle(volume.distance);
    volume.constants.ProbeOffsets = CoreGraphics::TextureGetBindlessHandle(volume.offsets);
    volume.constants.ProbeStates = CoreGraphics::TextureGetBindlessHandle(volume.states);
    volume.constants.ProbeScrollSpace = CoreGraphics::TextureGetBindlessHandle(volume.scrollSpace);

    CoreGraphics::BufferCreateInfo probeVolumeBufferInfo;
#if NEBULA_GRAPHICS_DEBUG
    probeVolumeBufferInfo.name = Util::String::Sprintf("%s Volume", volumeName.c_str());
#endif
    probeVolumeBufferInfo.byteSize = sizeof(Probeupdate::VolumeConstants);
    probeVolumeBufferInfo.usageFlags = CoreGraphics::BufferUsageFlag::ConstantBuffer;
    probeVolumeBufferInfo.mode = CoreGraphics::BufferAccessMode::DeviceAndHost;
    volume.volumeBuffer = CoreGraphics::CreateBuffer(probeVolumeBufferInfo);

    volume.updateProbesTable = CoreGraphics::ShaderCreateResourceTable(state.probeUpdateShader, NEBULA_SYSTEM_GROUP, 1);
    CoreGraphics::ResourceTableSetRWTexture(volume.updateProbesTable, CoreGraphics::ResourceTableTexture(volume.radiance, Probeupdate::Table_System::RadianceOutput_SLOT));
    CoreGraphics::ResourceTableSetConstantBuffer(volume.updateProbesTable, CoreGraphics::ResourceTableBuffer(volume.volumeBuffer, Probeupdate::Table_System::VolumeConstants_SLOT));
    CoreGraphics::ResourceTableCommitChanges(volume.updateProbesTable);

    volume.finalizeProbesTable = CoreGraphics::ShaderCreateResourceTable(state.probeFinalizeShader, NEBULA_SYSTEM_GROUP, 1);
    CoreGraphics::ResourceTableSetRWTexture(volume.finalizeProbesTable, CoreGraphics::ResourceTableTexture(volume.irradiance, Probefinalize::Table_System::IrradianceOutput_SLOT));
    CoreGraphics::ResourceTableSetRWTexture(volume.finalizeProbesTable, CoreGraphics::ResourceTableTexture(volume.distance, Probefinalize::Table_System::DistanceOutput_SLOT));
    CoreGraphics::ResourceTableSetRWTexture(volume.finalizeProbesTable, CoreGraphics::ResourceTableTexture(volume.scrollSpace, Probefinalize::Table_System::ScrollSpaceOutput_SLOT));
    CoreGraphics::ResourceTableCommitChanges(volume.finalizeProbesTable);
    //CoreGraphics::ResourceTableSetConstantBuffer(volume.finalizeProbesTable)
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

    state.volumesToUpdate.Clear();
    for (Volume& activeVolume : volumes)
    {
        if (activeVolume.boundingBox.contains(xyz(cameraPos)))
        {
            UpdateVolume volumeToUpdate;
            volumeToUpdate.updateProbesTable = activeVolume.updateProbesTable;
            volumeToUpdate.probeCounts[0] = activeVolume.numProbesX;
            volumeToUpdate.probeCounts[1] = activeVolume.numProbesY;
            volumeToUpdate.probeCounts[2] = activeVolume.numProbesZ;
            volumeToUpdate.numRays = activeVolume.numRaysPerProbe;
            state.volumesToUpdate.Append(volumeToUpdate);

            Math::mat4 rotation = Math::rotationyawpitchroll(Math::sin(ctx.frameIndex / 10.0f), 0.0f, 0.0f);
            rotation.store(activeVolume.constants.TemporalRotation);
            activeVolume.size.store(activeVolume.constants.Scale);
            activeVolume.position.store(activeVolume.constants.Offset);
            activeVolume.constants.NumIrradianceTexels = Probeupdate::NUM_IRRADIANCE_TEXELS_PER_PROBE;
            activeVolume.constants.ProbeGridDimensions[0] = activeVolume.numProbesX;
            activeVolume.constants.ProbeGridDimensions[1] = activeVolume.numProbesY;
            activeVolume.constants.ProbeGridDimensions[2] = activeVolume.numProbesZ;
            activeVolume.constants.Options |= activeVolume.options.flags.relocate ? Probeupdate::RELOCATION_OPTION : 0x0;
            activeVolume.constants.Options |= activeVolume.options.flags.scrolling ? Probeupdate::SCROLL_OPTION : 0x0;
            activeVolume.constants.Options |= activeVolume.options.flags.classify ? Probeupdate::CLASSIFICATION_OPTION : 0x0;
            activeVolume.constants.Options |= activeVolume.options.flags.lowPrecisionTextures ? Probeupdate::LOW_PRECISION_IRRADIANCE_OPTION : 0x0;
            activeVolume.constants.Options |= activeVolume.options.flags.partialUpdate ? Probeupdate::PARTIAL_UPDATE_OPTION : 0x0;
            activeVolume.constants.ProbeIndexStart = 0;
            activeVolume.constants.ProbeIndexCount = volumeToUpdate.probeCounts[0] * volumeToUpdate.probeCounts[1] * volumeToUpdate.probeCounts[2];
            activeVolume.constants.ProbeScrollOffsets[0] = 0;
            activeVolume.constants.ProbeScrollOffsets[1] = 0;
            activeVolume.constants.ProbeScrollOffsets[2] = 0;
            Math::quat().store(activeVolume.constants.Rotation);
            activeVolume.constants.ProbeGridSpacing[0] = activeVolume.size[0] / activeVolume.numProbesX;
            activeVolume.constants.ProbeGridSpacing[1] = activeVolume.size[1] / activeVolume.numProbesY;
            activeVolume.constants.ProbeGridSpacing[2] = activeVolume.size[2] / activeVolume.numProbesZ;
            activeVolume.constants.NumDistanceTexels = Probeupdate::NUM_DISTANCE_TEXELS_PER_PROBE;
            activeVolume.constants.IrradianceGamma = 1.0f;
            activeVolume.constants.NormalBias = 10.0f;
            activeVolume.constants.ViewBias = 40.0f;
            activeVolume.constants.IrradianceScale = 1.0f;

            CoreGraphics::BufferUpdate(activeVolume.volumeBuffer, activeVolume.constants);
        }
    }

    CoreGraphics::ResourceTableSetRWBuffer(state.raytracingTable.tables[ctx.bufferIndex], CoreGraphics::ResourceTableBuffer(Raytracing::RaytracingContext::GetObjectBindingBuffer(), Probeupdate::Table_Batch::ObjectBuffer_SLOT));
    CoreGraphics::ResourceTableSetAccelerationStructure(state.raytracingTable.tables[ctx.bufferIndex], CoreGraphics::ResourceTableTlas(Raytracing::RaytracingContext::GetTLAS(), Probeupdate::Table_Batch::TLAS_SLOT));
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
    CoreGraphics::DestroyResourceTable(volume.updateProbesTable);
    CoreGraphics::DestroyResourceTable(volume.finalizeProbesTable);
    ddgiVolumeAllocator.Dealloc(id.id);
}

} // namespace GI
