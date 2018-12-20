#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4ParticleSystemInstance
    
    The per-instance object of a ParticleSystem. This is where actual
    particles are created and updated.
    
    (C) 2008 Radon Labs GmbH
	(C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "particles/base/particlesysteminstancebase.h"

//------------------------------------------------------------------------------
namespace OpenGL4
{

class OGL4ParticleSystemInstance : public Particles::ParticleSystemInstanceBase
{
    __DeclareClass(OGL4ParticleSystemInstance);
public:
    /// constructor
    OGL4ParticleSystemInstance();
    /// destructor
    virtual ~OGL4ParticleSystemInstance();

    /// generate vertex streams to render
    virtual void UpdateVertexStreams();
    /// render the particle system
    virtual void Render();
};

} // namespace OpenGL4
//------------------------------------------------------------------------------
    
