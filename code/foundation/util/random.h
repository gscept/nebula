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

} // namespace Util
