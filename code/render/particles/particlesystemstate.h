#pragma once
//------------------------------------------------------------------------------
/**
    @class Particles::ParticleSystemState
    
    State bits of a particle system instance.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Particles
{
class ParticleSystemState
{
public:
    /// particle system state bits
    enum Bits
    {
        Initial  =		(1<<0),      // has been created but not started yet
        Playing  =		(1<<1),      // is currently emitting particles
        Stopping =		(1<<2),      // is currently stopping (no longer emits, but some particles still alive)
        Stopped  =		(1<<3),      // no longer emitting and no more particles alive
		Restarting =	(1<<4),		 // we are restarting this particle from the start
    };
    typedef ushort Mask;
};

} // namespace Particles
//------------------------------------------------------------------------------
    