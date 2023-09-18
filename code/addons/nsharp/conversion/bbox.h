#pragma once
//------------------------------------------------------------------------------
/**
	Mono::BoundingBox

	Contains helper functions for creating and converting
	nebula bbox's to mono objects.

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "monoconfig.h"
#include "math/bbox.h"
#include "mono/metadata/image.h"
#include "mono/metadata/object.h"

namespace Mono
{

class BoundingBox
{
public:
	static void Setup(MonoImage* image);
	static MonoObject* Convert(Math::bbox const& matrix);
};

} // namespace Mono
