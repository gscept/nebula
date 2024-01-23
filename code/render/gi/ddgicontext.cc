//------------------------------------------------------------------------------
//  @file ddgicontext.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "coregraphics/pipeline.h"
#include "ddgicontext.h"

using namespace Graphics;

namespace GI
{
DDGIContext::DDGIVolumeAllocator DDGIContext::allocator;
__ImplementContext(DDGIContext, DDGIContext::allocator)


struct
{
    CoreGraphics::PipelineRayTracingTable pipeline;
} state;


//------------------------------------------------------------------------------
/**
*/
DDGIContext::DDGIContext()
{
    auto probeUpdateShader = CoreGraphics::ShaderServer::Instance()->GetShader("shd:gi/shaders/probeupdate.fxb");
    auto probeUpdateProgram = CoreGraphics::ShaderGetProgram(probeUpdateShader, CoreGraphics::ShaderFeatureFromString("ProbeRayGen"));

    auto brdfHitShader = CoreGraphics::ShaderServer::Instance()->GetShader("shd:raytracing/shaders/brdfhit.fxb");
    auto brdfHitProgram = CoreGraphics::ShaderGetProgram(brdfHitShader, CoreGraphics::ShaderFeatureFromString("Hit"));
    auto bsdfHitShader = CoreGraphics::ShaderServer::Instance()->GetShader("shd:raytracing/shaders/bsdfhit.fxb");
    auto bsdfHitProgram = CoreGraphics::ShaderGetProgram(bsdfHitShader, CoreGraphics::ShaderFeatureFromString("Hit"));
    auto gltfHitShader = CoreGraphics::ShaderServer::Instance()->GetShader("shd:raytracing/shaders/gltfhit.fxb");
    auto gltfHitProgram = CoreGraphics::ShaderGetProgram(gltfHitShader, CoreGraphics::ShaderFeatureFromString("Hit"));
    auto particleHitShader = CoreGraphics::ShaderServer::Instance()->GetShader("shd:raytracing/shaders/particlehit.fxb");
    auto particleHitProgram = CoreGraphics::ShaderGetProgram(particleHitShader, CoreGraphics::ShaderFeatureFromString("Hit"));

    // Create pipeline, the order of hit programs must match RaytracingContext::ObjectType
    state.pipeline = CoreGraphics::CreateRaytracingPipeline({ probeUpdateProgram, brdfHitProgram, bsdfHitProgram, gltfHitProgram, particleHitProgram });

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
DDGIContext::SetupVolume(const Graphics::GraphicsEntityId id)
{

}

} // namespace GI
