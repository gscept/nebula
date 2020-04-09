#pragma once
//------------------------------------------------------------------------------
/**
    @class Particles::ParticleRenderInfo
    
    ParticleRenderInfo objects are returned by the ParticleRenderer singleton
    when a visible particle system is attached. The caller needs to store
    this object and needs to hand it back to the ParticleRenderer when
    actually rendering of the particle system needs to happen.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/

#include "particles/base/particlerenderinfobase.h"
namespace Particles
{
    typedef Base::ParticleRenderInfoBase ParticleRenderInfo;
}

//------------------------------------------------------------------------------
    