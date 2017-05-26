#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11ParticleSystemInstance
    
    The per-instance object of a ParticleSystem. This is where actual
    particles are created and updated.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "particles/base/particlesysteminstancebase.h"


namespace Direct3D11
{

class D3D11ParticleSystemInstance : public Particles::ParticleSystemInstanceBase
{
    __DeclareClass(D3D11ParticleSystemInstance);
public:
    /// constructor
    D3D11ParticleSystemInstance();
    /// destructor
    virtual ~D3D11ParticleSystemInstance();

    /// generate vertex streams to render
    virtual void UpdateVertexStreams();
    /// render the particle system
    virtual void Render();
};

} // namespace Direct3D9
//------------------------------------------------------------------------------
    
