//------------------------------------------------------------------------------
//  @file ddgicontext.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "coregraphics/pipeline.h"
#include "ddgicontext.h"

#include <cstdint>

#include "frame/default.h"
#include "gi/shaders/probeupdate.h"
#include "graphics/cameracontext.h"
#include "graphics/view.h"
#include "lighting/lightcontext.h"
#include "raytracing/raytracingcontext.h"
#include "coregraphics/shaperenderer.h"

using namespace Graphics;

namespace GI
{
DDGIContext::DDGIVolumeAllocator DDGIContext::ddgiVolumeAllocator;
__ImplementContext(DDGIContext, DDGIContext::ddgiVolumeAllocator)

struct UpdateVolume
{
    uint numProbes;
    uint numRays;
    CoreGraphics::ResourceTableId table;
};

struct
{
    CoreGraphics::ShaderId probeUpdateShader;
    CoreGraphics::ShaderProgramId probeUpdateProgram;
    CoreGraphics::PipelineRayTracingTable pipeline;

    CoreGraphics::ResourceTableSet raytracingTable;

    Util::Array<UpdateVolume> volumesToUpdate;
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
    state.pipeline = CoreGraphics::CreateRaytracingPipeline({ state.probeUpdateProgram, brdfHitProgram, bsdfHitProgram, gltfHitProgram, particleHitProgram });

    FrameScript_default::RegisterSubgraph_DDGIProbeUpdate_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        if (!state.volumesToUpdate.IsEmpty())
        {
            CoreGraphics::CmdSetRayTracingPipeline(cmdBuf, state.pipeline.pipeline);
            CoreGraphics::CmdSetResourceTable(cmdBuf, state.raytracingTable.tables[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::RayTracingPipeline, nullptr);
            CoreGraphics::CmdSetResourceTable(cmdBuf, Raytracing::RaytracingContext::GetLightGridResourceTable(), NEBULA_FRAME_GROUP, CoreGraphics::RayTracingPipeline, nullptr);
            for (const UpdateVolume& volumeToUpdate : state.volumesToUpdate)
            {
                CoreGraphics::CmdSetResourceTable(cmdBuf, volumeToUpdate.table, NEBULA_SYSTEM_GROUP, CoreGraphics::RayTracingPipeline, nullptr);
                CoreGraphics::CmdRaysDispatch(cmdBuf, state.pipeline.table, volumeToUpdate.numProbes, volumeToUpdate.numRays, 1);
            }
        }
    });
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

    CoreGraphics::TextureCreateInfo radianceCreateInfo;
    radianceCreateInfo.width = setup.numProbesY * setup.numProbesZ * Probeupdate::NumColorSamples;
    radianceCreateInfo.height = setup.numProbesX;
    radianceCreateInfo.format = CoreGraphics::PixelFormat::R10G10B10A2;
    radianceCreateInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture;
    volume.radiance = CoreGraphics::CreateTexture(radianceCreateInfo);

    CoreGraphics::TextureCreateInfo depthCreateInfo;
    depthCreateInfo.width = setup.numProbesY * setup.numProbesZ * Probeupdate::NumDepthSamples;
    depthCreateInfo.height = setup.numProbesX;
    depthCreateInfo.format = CoreGraphics::PixelFormat::R32G32F;
    depthCreateInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture;
    volume.depth = CoreGraphics::CreateTexture(depthCreateInfo);

    CoreGraphics::BufferCreateInfo probeBufferCreateInfo;
    probeBufferCreateInfo.elementSize = sizeof(Probeupdate::Probe);
    probeBufferCreateInfo.size = setup.numProbesX * setup.numProbesY * setup.numProbesZ;
    probeBufferCreateInfo.usageFlags = CoreGraphics::BufferUsageFlag::ReadWriteBuffer;
    probeBufferCreateInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
    volume.probeBuffer = CoreGraphics::BufferWithStaging(probeBufferCreateInfo);

    volume.resourceTable = CoreGraphics::ShaderCreateResourceTable(state.probeUpdateShader, NEBULA_SYSTEM_GROUP, 1);
    CoreGraphics::ResourceTableSetRWTexture(volume.resourceTable, CoreGraphics::ResourceTableTexture(volume.radiance, Probeupdate::Table_System::RadianceOutput_SLOT));
    CoreGraphics::ResourceTableSetRWTexture(volume.resourceTable, CoreGraphics::ResourceTableTexture(volume.depth, Probeupdate::Table_System::DepthOutput_SLOT));
    CoreGraphics::ResourceTableSetRWBuffer(volume.resourceTable, CoreGraphics::ResourceTableBuffer(volume.probeBuffer.DeviceBuffer(), Probeupdate::Table_System::ProbeBuffer_SLOT));
    CoreGraphics::ResourceTableCommitChanges(volume.resourceTable);
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
    for (const Volume& activeVolume : volumes)
    {
        if (activeVolume.boundingBox.contains(xyz(cameraPos)))
        {
            UpdateVolume volumeToUpdate;
            volumeToUpdate.table = activeVolume.resourceTable;
            volumeToUpdate.numProbes = activeVolume.numProbesX * activeVolume.numProbesY * activeVolume.numProbesZ;
            volumeToUpdate.numRays = activeVolume.numRaysPerProbe;
            state.volumesToUpdate.Append(volumeToUpdate);
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
    ddgiVolumeAllocator.Dealloc(id.id);
}

} // namespace GI
