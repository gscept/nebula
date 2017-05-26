#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D9::D3D9ParticleSystemInstance
    
    The per-instance object of a ParticleSystem. This is where actual
    particles are created and updated.
    
    (C) 2008 Radon Labs GmbH
*/
#include "particles/base/particlesysteminstancebase.h"

//------------------------------------------------------------------------------
namespace Direct3D9
{

class D3D9ParticleSystemInstance : public Particles::ParticleSystemInstanceBase
{
    __DeclareClass(D3D9ParticleSystemInstance);
public:
    /// constructor
    D3D9ParticleSystemInstance();
    /// destructor
    virtual ~D3D9ParticleSystemInstance();

    /// generate vertex streams to render
    virtual void UpdateVertexStreams();
    /// render the particle system
    virtual void Render();
};

} // namespace Direct3D9
//------------------------------------------------------------------------------
    
