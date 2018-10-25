//------------------------------------------------------------------------------
//  d3d9particlesysteminstance.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"

#ifndef __DX11__
#include "particles/d3d9/d3d9particlesysteminstance.h"
#include "particles/particlerenderer.h"
#include "coregraphics/renderdevice.h"

using namespace Particles;
using namespace CoreGraphics;

namespace Direct3D9
{
__ImplementClass(Direct3D9::D3D9ParticleSystemInstance, 'DPSI', Particles::ParticleSystemInstanceBase);

//------------------------------------------------------------------------------
/**
*/
D3D9ParticleSystemInstance::D3D9ParticleSystemInstance()
{
}

//------------------------------------------------------------------------------
/**
*/
D3D9ParticleSystemInstance::~D3D9ParticleSystemInstance()
{
    n_assert(!this->IsValid());
}

//------------------------------------------------------------------------------
/**
*/
void D3D9ParticleSystemInstance::UpdateVertexStreams()
{
    ParticleSystemInstanceBase::UpdateVertexStreams();

    Ptr<D3D9ParticleRenderer> particleRenderer = D3D9ParticleRenderer::Instance();

    // get the particle ring buffer from
    IndexT baseVertexIndex = particleRenderer->GetCurParticleVertexIndex();

    float* ptr = (float*)particleRenderer->GetCurVertexPtr();
    Math::float4 tmp;
    IndexT i;
    SizeT num = this->particles.Size();
    for (i = 0; (i < num) && (particleRenderer->GetCurParticleVertexIndex() < MaxNumRenderedParticles); i++)
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
D3D9ParticleSystemInstance::Render()
{
    Ptr<D3D9ParticleRenderer> particleRenderer = D3D9ParticleRenderer::Instance();
    n_assert(!particleRenderer->IsInAttach());
    RenderDevice* renderDevice = RenderDevice::Instance();
    SizeT numParticles = renderInfo.GetNumVertices();
    if (numParticles > 0)
    {
        // setup vertex buffers and index buffers for rendering
        IndexT baseVertexIndex = renderInfo.GetBaseVertexIndex();
		
        renderDevice->SetStreamSource(0, particleRenderer->GetCornerVertexBuffer(), 0);
        renderDevice->SetStreamSource(1, particleRenderer->GetParticleVertexBuffer(), baseVertexIndex);
        renderDevice->SetVertexLayout(particleRenderer->GetVertexLayout());
        renderDevice->SetIndexBuffer(particleRenderer->GetCornerIndexBuffer());
        renderDevice->SetPrimitiveGroup(particleRenderer->GetPrimitiveGroup());
        renderDevice->DrawIndexedInstanced(numParticles);
    }
}

} // namespace Direct3D9

#endif