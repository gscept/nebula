#pragma once
//------------------------------------------------------------------------------
/**
	Mono::Mat4

	Contains helper functions for creating and converting
	nebula mat4's to mono objects.

	(C) 2023 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "monoconfig.h"
#include "math/mat4.h"
#include "mono/metadata/image.h"
#include "mono/metadata/object.h"

namespace Mono
{

class Mat4
{
public:
	static void Setup(MonoImage* image);
	static MonoObject* Convert(Math::mat4 const& matrix);
	static Math::mat4 Convert(MonoObject* object);
};

} // namespace Mono
