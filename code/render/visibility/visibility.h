#pragma once
//------------------------------------------------------------------------------
// visibility.h
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "visibility.h"

namespace Visibility
{

enum ObserverMask : uint8_t
{
	Graphics	= (1 << 0),	// observer sees graphics entities (models, lights, characters, etc)
	Observers	= (1 << 1),	// observer can see other observers (shadow casting lights, for example)

	All = (1 << 2),
	NumObserverMasks = 2
};
__ImplementEnumBitOperators(ObserverMask);

enum ObserverType : uint8_t
{
	Perspective			= 0, // observer uses a 4x4 projection and view matrix to resolve visibility
	Orthographic		= 1, // observer uses a 4x4 orthogonal and view matrix to resolve visibility
	BoundingBox			= 2, // observer uses a non-projective bounding box
	Omnipercipient		= 3, // observer sees everything, without having to do any resolving

	NumObserverTypes	= 4
};
__ImplementEnumBitOperators(ObserverType);

} // namespace Visibility