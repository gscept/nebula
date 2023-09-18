#pragma once
//------------------------------------------------------------------------------
/**
	Mono::Vector4

	Contains helper functions for creating and converting
	nebula vector4's to mono objects.

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "monoconfig.h"
#include "math/vec4.h"
#include "mono/metadata/image.h"
#include "mono/metadata/object.h"

namespace Mono
{

class Vector4
{
public:
	static void Setup(MonoImage* image);
	static MonoObject* Convert(Math::vec4 const& vec);
	static Math::vec4 Convert(MonoObject* object);

};

} // namespace Mono
