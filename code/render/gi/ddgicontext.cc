//------------------------------------------------------------------------------
//  @file ddgicontext.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "coregraphics/pipeline.h"
#include "ddgicontext.h"

#include "gi/shaders/probeupdate.h"

using namespace Graphics;

namespace GI
{
DDGIContext::DDGIVolumeAllocator DDGIContext::ddgiVolumeAllocator;
__ImplementContext(DDGIContext, DDGIContext::ddgiVolumeAllocator)


struct
{
    CoreGraphics::ShaderId probeUpdateShader;
    CoreGraphics::ShaderProgramId probeUpdateProgram;
    CoreGraphics::PipelineRayTracingTable pipeline;
} state;


//------------------------------------------------------------------------------
/**
*/
DDGIContext::DDGIContext()
{
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

    // Create pipeline, the order of hit programs must match RaytracingContext::ObjectType
    state.pipeline = CoreGraphics::CreateRaytracingPipeline({ state.probeUpdateProgram, brdfHitProgram, bsdfHitProgram, gltfHitProgram, particleHitProgram });

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
DDGIContext::SetupVolume(const Graphics::GraphicsEntityId id, const VolumeSetup& setup)
{
    ContextEntityId ctxId = GetContextId(id);
    Volume& volume = ddgiVolumeAllocator.Get<0>(ctxId.id);
    volume.position = setup.position;
    volume.size = setup.size;
    volume.numProbesX = setup.numProbesX;
    volume.numProbesY = setup.numProbesY;
    volume.numProbesZ = setup.numProbesZ;
    volume.numPixelsPerProbe = setup.numPixelsPerProbe;

    CoreGraphics::TextureCreateInfo radianceCreateInfo;
    radianceCreateInfo.width = setup.numProbesY * setup.numProbesZ * Probeupdate::NumColorSamples;
    radianceCreateInfo.height = setup.numProbesX * setup.numPixelsPerProbe;
    radianceCreateInfo.format = CoreGraphics::PixelFormat::R10G10B10A2;
    radianceCreateInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture;
    volume.radiance = CoreGraphics::CreateTexture(radianceCreateInfo);

    CoreGraphics::TextureCreateInfo depthCreateInfo;
    depthCreateInfo.width = setup.numProbesY * setup.numProbesZ * Probeupdate::NumDepthSamples;
    depthCreateInfo.height = setup.numProbesX * setup.numPixelsPerProbe;
    depthCreateInfo.format = CoreGraphics::PixelFormat::R32G32F;
    depthCreateInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture;
    volume.depth = CoreGraphics::CreateTexture(depthCreateInfo);

    CoreGraphics::BufferCreateInfo probeBufferCreateInfo;
    probeBufferCreateInfo.elementSize = sizeof(Probeupdate::Probe);
    probeBufferCreateInfo.size = setup.numProbesX * setup.numProbesY * setup.numProbesZ;
    probeBufferCreateInfo.usageFlags = CoreGraphics::BufferUsageFlag::ReadWriteBuffer;
    probeBufferCreateInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
    volume.probeBuffer = CoreGraphics::BufferWithStaging(probeBufferCreateInfo);

    volume.resourceTable = CoreGraphics::ShaderCreateResourceTable(state.probeUpdateShader, NEBULA_BATCH_GROUP, 1);
    CoreGraphics::ResourceTableSetRWTexture(volume.resourceTable, CoreGraphics::ResourceTableTexture(volume.radiance, Probeupdate::Table_Batch::RadianceOutput_SLOT));
    CoreGraphics::ResourceTableSetRWTexture(volume.resourceTable, CoreGraphics::ResourceTableTexture(volume.depth, Probeupdate::Table_Batch::DepthOutput_SLOT));
    CoreGraphics::ResourceTableSetRWBuffer(volume.resourceTable, CoreGraphics::ResourceTableBuffer(volume.probeBuffer.DeviceBuffer(), Probeupdate::Table_Batch::ProbeBuffer_SLOT));
    CoreGraphics::ResourceTableCommitChanges(volume.resourceTable);
}

//------------------------------------------------------------------------------
/**
*/
void
DDGIContext::SetPosition(const Graphics::GraphicsEntityId id, const Math::vec3& position)
{
    ContextEntityId ctxId = GetContextId(id);
    ddgiVolumeAllocator.Get<0>(ctxId.id).position = position;
}

//------------------------------------------------------------------------------
/**
*/
void
DDGIContext::SetScale(const Graphics::GraphicsEntityId id, const Math::vec3& scale)
{
    ContextEntityId ctxId = GetContextId(id);
    ddgiVolumeAllocator.Get<0>(ctxId.id).size = scale;
}

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
