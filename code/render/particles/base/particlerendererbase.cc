//------------------------------------------------------------------------------
//  particlerendererbase.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "particles/base/particlerendererbase.h"
#include "particles/particlesysteminstance.h"

namespace Base
{
__ImplementAbstractClass(Base::ParticleRendererBase, 'PTRB', Core::RefCounted);

using namespace Particles;

//------------------------------------------------------------------------------
/**
*/
ParticleRendererBase::ParticleRendererBase() :
    isValid(false),
    inAttach(false)
{
}

//------------------------------------------------------------------------------
/**
*/
ParticleRendererBase::~ParticleRendererBase()
{
    n_assert(!this->IsValid());
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleRendererBase::Setup()
{
    n_assert(!this->IsValid());
    this->isValid = true;
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleRendererBase::Discard()
{
    n_assert(this->IsValid());
    this->isValid = false;
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleRendererBase::BeginAttach()
{
    n_assert(this->IsValid());
    n_assert(!this->inAttach);
    this->inAttach = true;    
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleRendererBase::AddVisibleParticleSystem(const Ptr<ParticleSystemInstance>& particleSystemInstance)
{
    n_assert(this->IsValid());
    n_assert(this->inAttach);
    n_assert(particleSystemInstance.isvalid());
    particleSystemInstance->UpdateVertexStreams();
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleRendererBase::EndAttach()
{
    n_assert(this->IsValid());
    n_assert(this->inAttach);
    this->inAttach = false;
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleRendererBase::RenderParticleSystem(const Ptr<Particles::ParticleSystemInstance>& particleSystemInstance)
{
    n_assert(this->IsValid());
    n_assert(!this->inAttach);
    n_assert(particleSystemInstance.isvalid());
    particleSystemInstance->Render();
}

} // namespace Base
