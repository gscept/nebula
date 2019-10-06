#pragma once
//------------------------------------------------------------------------------
/**
	@file random.h

	Contains random-number helper functions

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"

namespace Util
{

/// Produces an xorshift128 pseudo random number.
uint FastRandom();

/// Produces an xorshift128 psuedo based floating point random number in range 0..1
/// Note that this is not a truely random random number generator
float RandomFloat();

/// Produces an xorshift128 psuedo based floating point random number in range -1..1
/// Note that this is not a truely random random number generator
float RandomFloatNTP();

} // namespace Util
