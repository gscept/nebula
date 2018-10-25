//------------------------------------------------------------------------------
//  OGL4particlesysteminstance.cc
//  (C) 2012 Gustav Sterbrant
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "particles/ogl4/ogl4particlesysteminstance.h"
#include "particles/particlerenderer.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/shaderserver.h"

using namespace Particles;
using namespace CoreGraphics;

namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4ParticleSystemInstance, 'DPSI', Particles::ParticleSystemInstanceBase);

//------------------------------------------------------------------------------
/**
*/
OGL4ParticleSystemInstance::OGL4ParticleSystemInstance()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
OGL4ParticleSystemInstance::~OGL4ParticleSystemInstance()
{
    n_assert(!this->IsValid());
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ParticleSystemInstance::UpdateVertexStreams()
{
    ParticleSystemInstanceBase::UpdateVertexStreams();
    Ptr<ParticleRenderer> particleRenderer = ParticleRenderer::Instance();

    // get the particle ring buffer from
    IndexT baseVertexIndex = particleRenderer->GetCurParticleVertexIndex();

    float* ptr = (float*)particleRenderer->GetCurVertexPtr();
    Math::float4 tmp;
    IndexT i;
    SizeT num = this->particles.Size();

	// this is the size of our particles, the maximum is tripled because we use triple buffering
    for (i = 0; (i < num) && (particleRenderer->GetCurParticleIndex() < MaxNumRenderedParticles * 3); i++)
    {
        const Particle& particle = this->particles[i];
		
        if (particle.relAge < 1.0f)
        {
            // NOTE: it's important to write in order here, since the writes
            // go to write-combined memory!
            particle.position.stream(ptr); ptr += 4;
            particle.stretchPosition.stream(ptr); ptr += 4;
            particle.color.stream(ptr); ptr += 4;
            particle.uvMinMax.stream(ptr); ptr += 4;
            tmp.set(particle.rotation, particle.size, particle.particleId, 0.0f);
            tmp.stream(ptr); ptr += 4;
            particleRenderer->AddCurParticleIndex(1);
			particleRenderer->AddCurParticleVertexIndex(1);
        }
    }
    particleRenderer->SetCurVertexPtr(ptr);
    SizeT numVertices = particleRenderer->GetCurParticleVertexIndex() - baseVertexIndex;
    this->renderInfo = ParticleRenderInfo(baseVertexIndex, numVertices);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ParticleSystemInstance::Render()
{
    Ptr<ParticleRenderer> particleRenderer = ParticleRenderer::Instance();
    n_assert(!particleRenderer->IsInAttach());
    RenderDevice* renderDevice = RenderDevice::Instance();
    SizeT numParticles = renderInfo.GetNumVertices();

    if (numParticles > 0)
    {
        // setup vertex buffers and index buffers for rendering
        IndexT baseVertexIndex = renderInfo.GetBaseVertexIndex();
		
        renderDevice->SetStreamVertexBuffer(0, particleRenderer->GetCornerVertexBuffer(), 0);
        renderDevice->SetStreamVertexBuffer(1, particleRenderer->GetParticleVertexBuffer(), 0);
        renderDevice->SetVertexLayout(particleRenderer->GetVertexLayout());
        renderDevice->SetIndexBuffer(particleRenderer->GetCornerIndexBuffer());
        renderDevice->SetPrimitiveGroup(particleRenderer->GetPrimitiveGroup());
        renderDevice->DrawIndexedInstanced(numParticles, baseVertexIndex);
    }
}

} // namespace OpenGL4
