#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::ParticleRendererBase
    
    Base class for platform-specific particle renders.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "particles/particlesysteminstance.h"

//------------------------------------------------------------------------------
namespace Base
{
class ParticleRendererBase : public Core::RefCounted
{
    __DeclareAbstractClass(ParticleRendererBase);
public:
    /// constructor
    ParticleRendererBase();
    /// destructor
    virtual ~ParticleRendererBase();

    /// setup the particle renderer
    virtual void Setup();
    /// discard the particle renderer
    virtual void Discard();
    /// return true if particle renderer has been setup
    bool IsValid() const;

    /// begin adding visible particle systems
    virtual void BeginAttach();
    /// attach a visible particle system instance
    void AddVisibleParticleSystem(const Ptr<Particles::ParticleSystemInstance>& particleSystemInstance);
    /// is renderer in attach?
    bool IsInAttach() const;
    /// finish adding visible particle sytems
    virtual void EndAttach();

    /// render particles of previously attached particle system
    void RenderParticleSystem(const Ptr<Particles::ParticleSystemInstance> &particleSystemInstance);
	/// apply mesh
	void ApplyParticleMesh();
    
protected:
    bool isValid;
    bool inAttach;
};    

//------------------------------------------------------------------------------
/**
*/
inline bool
ParticleRendererBase::IsValid() const
{
    return this->isValid;
}

//------------------------------------------------------------------------------
/**
*/inline bool 
ParticleRendererBase::IsInAttach() const
{
    return this->inAttach;
}

} // namespace Base
//------------------------------------------------------------------------------
 