#pragma once
//------------------------------------------------------------------------------
/**
	Mono::Vector3

	Contains helper functions for creating and converting
	nebula vector3's to mono objects.

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "monoconfig.h"
#include "math/vec3.h"
#include "mono/metadata/image.h"
#include "mono/metadata/object.h"

namespace Mono
{

class Vector3
{
public:
	static void Setup(MonoImage* image);
	static MonoObject* Convert(Math::vec3 const& vec);
	static Math::vec3 Convert(MonoObject* object);
};

} // namespace Mono
