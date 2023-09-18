#pragma once
//------------------------------------------------------------------------------
/**
	Mono::Matrix44

	Contains helper functions for creating and converting
	nebula matrix44's to mono objects.

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "monoconfig.h"
#include "math/matrix44.h"
#include "mono/metadata/image.h"
#include "mono/metadata/object.h"

namespace Mono
{

class Matrix44
{
public:
	static void Setup(MonoImage* image);
	static MonoObject* Convert(Math::matrix44 const& matrix);
};

} // namespace Mono
